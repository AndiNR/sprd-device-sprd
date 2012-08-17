/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "audio_hw"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <hardware/audio.h>
#include <system/audio.h>

#include <tinyalsa/asoundlib.h>

#define CARD_DUMMY			0
#define CARD_VBC			1

#define PORT_PCM			0
#define PORT_VOICE			1

#define OUT_PERIOD_SIZE			1600
#define OUT_PERIOD_COUNT		4
#define OUT_SAMPLING_RATE		44100

/* used in screen off mode */
#define OUT_PERIOD_SIZE_LONG		6400
#define OUT_PERIOD_COUNT_LONG		8

#define IN_PERIOD_SIZE			1600
#define IN_PERIOD_COUNT			4
#define IN_SAMPLING_RATE		44100

static struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = OUT_SAMPLING_RATE,
    .period_size = OUT_PERIOD_SIZE,
    .period_count = OUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = OUT_PERIOD_SIZE * OUT_PERIOD_COUNT,
};

static struct pcm_config pcm_config_in = {
    .channels = 1,
    .rate = IN_SAMPLING_RATE,
    .period_size = IN_PERIOD_SIZE,
    .period_count = IN_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = 1,
    .stop_threshold = (IN_PERIOD_SIZE * IN_PERIOD_COUNT),
};

struct sprd_audio_device {
    struct audio_hw_device	device;

    pthread_mutex_t		lock;
    struct mixer		*mixer;
    audio_devices_t		devices;
    audio_mode_t		mode;
    bool			mic_mute;
    bool			screen_off;
};

struct sprd_stream_out {
    struct audio_stream_out	stream;

    struct sprd_audio_device	*dev;
    pthread_mutex_t		lock;
    struct pcm			*pcm;
    struct pcm_config		*config;
    bool			standby;
};

struct sprd_stream_in {
    struct audio_stream_in	stream;

    struct sprd_audio_device	*dev;
    pthread_mutex_t		lock;
    struct pcm			*pcm;
    struct pcm_config		*config;
    bool			standby;

    unsigned int		requested_rate;
};

#define MIXER_MASTER_PLAY_VOL		"Master Playback Volume"
#define MIXER_LINE_PLAY_VOL		"Line Playback Volume"
#define MIXER_CAPTURE_VOL		"Capture Volume"
#define MIXER_MIC_BOOST_VOL		"Mic Boost Volume"
#define MIXER_EAR_BOOST_VOL		"Ear Boost Volume"
#define MIXER_HEADPHONE_PLAY_VOL	"HeadPhone Playback Volume"
#define MIXER_SPEAKER_FUNC		"Speaker Function"
#define MIXER_EARPIECE_FUNC		"Earpiece Function"
#define MIXER_HEADPHONE_FUNC		"HeadPhone Function"
#define MIXER_LINE_FUNC			"Line Function"
#define MIXER_MIC_FUNC			"Mic Function"
#define MIXER_HP_MIC_FUNC		"HP Mic Function"
#define MIXER_LINEIN_CAP_FUNC		"Linein Rec Function"
#define MIXER_INNER_PA_PLAY_VOL		"Inter PA Playback Volume"
#define MIXER_LINEIN_CAP_VOL		"LineinRec Capture Volume"
#define MIXER_VBC_SWITCH		"VBC Switch"
#define MIXER_VBC_EQ_SWITCH		"VBC EQ Switch"
#define MIXER_VBC_EQ_UPDATE		"VBC EQ Update"

#define	MIXER_ON			"on"
#define	MIXER_OFF			"off"

#define	MIXER_TO_ARM			"arm"
#define	MIXER_TO_DSP			"dsp"

struct route_setting
{
    char *ctl_name;
    int intval;
    char *strval;
};

static struct route_setting route_defaults[] = {
    {
        .ctl_name = MIXER_SPEAKER_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_EARPIECE_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_HEADPHONE_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_LINE_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_MIC_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_HP_MIC_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_LINEIN_CAP_FUNC,
        .strval = MIXER_OFF,
    }, {
        .ctl_name = MIXER_VBC_SWITCH,
        .strval = MIXER_TO_ARM,
    }, {
        .ctl_name = NULL,
    },
};

static int set_route_by_array(struct mixer *mixer, struct route_setting *route)
{
    struct mixer_ctl *ctl;
    unsigned int i, j;

    /* Go through the route array and set each value */
    i = 0;
    while (route[i].ctl_name) {
        ctl = mixer_get_ctl_by_name(mixer, route[i].ctl_name);
        if (!ctl)
            return -EINVAL;

        if (route[i].strval) {
            mixer_ctl_set_enum_by_string(ctl, route[i].strval);
        } else {
            /* This ensures multiple (i.e. stereo) values are set jointly */
            for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
                mixer_ctl_set_value(ctl, j, route[i].intval);
            }
        }
        i++;
    }

    return 0;
}

static int set_route_direct(struct mixer *mixer, char *ctl_name, char *strval, int intval)
{
    struct mixer_ctl *ctl;
    int j;

    ctl = mixer_get_ctl_by_name(mixer, ctl_name);
    if (!ctl)
        return -EINVAL;

    if (strval) {
        mixer_ctl_set_enum_by_string(ctl, strval);
    } else {
        for (j = 0; j < mixer_ctl_get_num_values(ctl); j++) {
            mixer_ctl_set_value(ctl, j, intval);
        }
    }

    return 0;
}

static void select_out_devices(struct sprd_audio_device *adev)
{
    int headphone_on;
    int speaker_on;
    int earpiece_on;

    ALOGV("select out devices %08x\n", adev->devices);

    headphone_on = adev->devices & (AUDIO_DEVICE_OUT_WIRED_HEADSET |
                                    AUDIO_DEVICE_OUT_WIRED_HEADPHONE);
    speaker_on = adev->devices & AUDIO_DEVICE_OUT_SPEAKER;
    earpiece_on = adev->devices & AUDIO_DEVICE_OUT_EARPIECE;

    if (speaker_on)
        set_route_direct(adev->mixer, MIXER_SPEAKER_FUNC, MIXER_ON, -1);
    else
        set_route_direct(adev->mixer, MIXER_SPEAKER_FUNC, MIXER_OFF, -1);

    if (headphone_on)
        set_route_direct(adev->mixer, MIXER_HEADPHONE_FUNC, MIXER_ON, -1);
    else
        set_route_direct(adev->mixer, MIXER_HEADPHONE_FUNC, MIXER_OFF, -1);

    if (earpiece_on)
        set_route_direct(adev->mixer, MIXER_EARPIECE_FUNC, MIXER_ON, -1);
    else
        set_route_direct(adev->mixer, MIXER_EARPIECE_FUNC, MIXER_OFF, -1);

    ALOGV("headphone=%c speaker=%c earpiece=%c", headphone_on ? 'y' : 'n',
          speaker_on ? 'y' : 'n', earpiece_on ? 'y' : 'n');
}

static void select_in_devices(struct sprd_audio_device *adev)
{
    int main_mic_on;
    int headset_mic_on;

    ALOGV("select in devices %08x\n", adev->devices);

    main_mic_on = adev->devices & AUDIO_DEVICE_IN_BUILTIN_MIC;
    headset_mic_on = adev->devices & AUDIO_DEVICE_IN_WIRED_HEADSET;

    if (main_mic_on) {
        set_route_direct(adev->mixer, MIXER_MIC_FUNC, MIXER_ON, -1);
    } else {
        set_route_direct(adev->mixer, MIXER_MIC_FUNC, MIXER_OFF, -1);
    }

    if (headset_mic_on) {
        set_route_direct(adev->mixer, MIXER_HP_MIC_FUNC, MIXER_ON, -1);
    } else {
        set_route_direct(adev->mixer, MIXER_HP_MIC_FUNC, MIXER_OFF, -1);
    }

    ALOGV("main_mic=%c headset_mic=%c", main_mic_on ? 'y' : 'n',
          headset_mic_on ? 'y' : 'n');
}

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return pcm_config_out.rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return pcm_config_out.period_size *
               audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return AUDIO_CHANNEL_OUT_STEREO;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

/* must be called with hw device and output stream mutexes locked */
static void do_out_standby(struct sprd_stream_out *out)
{
    struct sprd_audio_device *adev = out->dev;

    if (!out->standby) {
        pcm_close(out->pcm);
        out->pcm = NULL;
        out->standby = true;
    }
}

static int out_standby(struct audio_stream *stream)
{
    struct sprd_stream_out *out = (struct sprd_stream_out *)stream;

    ALOGV("%s()\n", __func__);
    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);
    do_out_standby(out);
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct sprd_stream_out *out = (struct sprd_stream_out *)stream;
    struct sprd_audio_device *adev = out->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    unsigned int val;

    ALOGV("%s()\n", __func__);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        if (((adev->devices & AUDIO_DEVICE_OUT_ALL) != val) && (val != 0)) {
            adev->devices &= ~AUDIO_DEVICE_OUT_ALL;
            adev->devices |= val;
            select_out_devices(adev);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    str_parms_destroy(parms);

    return ret;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGV("%s()\n", __func__);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    ALOGV("%s()\n", __func__);
    return pcm_config_out.period_size *
		    pcm_config_out.period_count * 1000 / pcm_config_out.rate;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

static int start_output_stream(struct sprd_stream_out *out)
{
    struct sprd_audio_device *adev = out->dev;
    unsigned int flags = PCM_OUT | PCM_MMAP;
    int i;

    if (adev->mode != AUDIO_MODE_IN_CALL) {
        select_out_devices(adev);
    }

    out->config = &pcm_config_out;
    out->pcm = pcm_open(CARD_VBC, PORT_PCM, flags, out->config);
    if (!out->pcm || !pcm_is_ready(out->pcm)) {
        ALOGE("pcm_open(out) failed: %s", pcm_get_error(out->pcm));
        return -ENOMEM;
    }

    return 0;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t bytes)
{
    struct sprd_stream_out *out = (struct sprd_stream_out *)stream;
    struct sprd_audio_device *adev = out->dev;
    int ret;
    int i;

    ALOGV("%s() %d\n", __func__, bytes);
    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
            goto exit;
        }
        out->standby = 0;
    }

    ret = pcm_mmap_write(out->pcm, (void *)buffer, bytes);

exit:
    pthread_mutex_unlock(&out->lock);

    if (ret != 0) {
        usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
               out_get_sample_rate(&stream->common));
    }

    return bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    ALOGV("%s()\n", __func__);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    ALOGV("%s()\n", __func__);
    return -EINVAL;
}

/** audio_stream_in implementation **/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct sprd_stream_in *in = (struct sprd_stream_in *)stream;

    ALOGV("%s()\n", __func__);
    return in->requested_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct sprd_stream_in *in = (struct sprd_stream_in *)stream;
    size_t size;

    ALOGV("%s()\n", __func__);
    /*
     * take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size = (in->config->period_size * in_get_sample_rate(stream)) /
            in->config->rate;
    size = ((size + 15) / 16) * 16;

    return size * audio_stream_frame_size((struct audio_stream *)stream);
}

static uint32_t in_get_channels(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return AUDIO_CHANNEL_IN_MONO;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    ALOGV("%s()\n", __func__);
    return AUDIO_FORMAT_PCM_16_BIT;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

/* must be called with hw device and input stream mutexes locked */
static void do_in_standby(struct sprd_stream_in *in)
{
    struct sprd_audio_device *adev = in->dev;

    if (!in->standby) {
        pcm_close(in->pcm);
        in->pcm = NULL;
        in->standby = true;
    }
}

static int in_standby(struct audio_stream *stream)
{
    struct sprd_stream_in *in = (struct sprd_stream_in *)stream;

    ALOGV("%s()\n", __func__);
    pthread_mutex_lock(&in->dev->lock);
    pthread_mutex_lock(&in->lock);
    do_in_standby(in);
    pthread_mutex_unlock(&in->lock);
    pthread_mutex_unlock(&in->dev->lock);

    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct sprd_stream_in *in = (struct sprd_stream_in *)stream;
    struct sprd_audio_device *adev = in->dev;
    struct str_parms *parms;
    char value[32];
    int ret;
    unsigned int val;

    ALOGV("%s()\n", __func__);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        val = atoi(value);
        pthread_mutex_lock(&adev->lock);
        if (((adev->devices & AUDIO_DEVICE_IN_ALL) != val) && (val != 0)) {
            adev->devices &= ~AUDIO_DEVICE_IN_ALL;
            adev->devices |= val;
            select_in_devices(adev);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    str_parms_destroy(parms);
    return ret;
}

static char * in_get_parameters(const struct audio_stream *stream,
                                const char *keys)
{
    ALOGV("%s()\n", __func__);
    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int start_input_stream(struct sprd_stream_in *in)
{
    struct sprd_audio_device *adev = in->dev;
    unsigned int flags = PCM_IN;

    if (adev->mode != AUDIO_MODE_IN_CALL) {
        select_in_devices(adev);
    }

    in->pcm = pcm_open(CARD_VBC, PORT_PCM, flags, in->config);
    if (!in->pcm || !pcm_is_ready(in->pcm)) {
        ALOGE("pcm_open(in) failed: %s", pcm_get_error(in->pcm));
        return -ENOMEM;
    }

    return 0;
}

static ssize_t in_read(struct audio_stream_in *stream, void* buffer,
                       size_t bytes)
{
    struct sprd_stream_in *in = (struct sprd_stream_in *)stream;
    struct sprd_audio_device *adev = in->dev;
    int ret = 0;

    ALOGV("%s() %d\n", __func__, bytes);

    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        ret = start_input_stream(in);
        if (ret == 0)
            goto exit;
    }

    ret = pcm_read(in->pcm, buffer, bytes);

exit:
    if (ret < 0)
        usleep(bytes * 1000000 / audio_stream_frame_size(&stream->common) /
               in_get_sample_rate(&stream->common));

    pthread_mutex_unlock(&in->lock);

    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int adev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;
    struct sprd_stream_out *out;
    int ret;

    ALOGV("%s()\n", __func__);
    out = (struct sprd_stream_out *)calloc(1, sizeof(struct sprd_stream_out));
    if (!out)
        return -ENOMEM;

    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;

    out->dev = adev;
    out->standby = true;

    config->format = out_get_format(&out->stream.common);
    config->channel_mask = out_get_channels(&out->stream.common);
    config->sample_rate = out_get_sample_rate(&out->stream.common);

    *stream_out = &out->stream;

    return 0;

err_open:
    free(out);
    *stream_out = NULL;
    return ret;
}

static void adev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    ALOGV("%s()\n", __func__);
    out_standby(&stream->common);
    free(stream);
}

static int adev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;
    struct str_parms *parms;
    char value[32];
    int ret;

    ALOGV("%s()\n", __func__);
    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        pthread_mutex_lock(&adev->lock);
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0) {
            ALOGV("screen on: low latency audio\n");
            adev->screen_off = false;
            pcm_config_out.period_size = OUT_PERIOD_SIZE;
            pcm_config_out.period_count = OUT_PERIOD_COUNT;
	} else {
            ALOGV("screen off: low power audio\n");
            adev->screen_off = true;
            pcm_config_out.period_size = OUT_PERIOD_SIZE_LONG;
            pcm_config_out.period_count = OUT_PERIOD_COUNT_LONG;
	}

        pthread_mutex_unlock(&adev->lock);
    }

    str_parms_destroy(parms);
    return ret;
}

static char * adev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGV("%s()\n", __func__);
    return strdup("");
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

static int adev_get_master_volume(struct audio_hw_device *dev,
                                  float *volume)
{
    ALOGV("%s()\n", __func__);
    return -ENOSYS;
}

static void select_mode(struct sprd_audio_device *adev)
{
    /* TODO */
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;

    ALOGV("%s()\n", __func__);

    pthread_mutex_lock(&adev->lock);
    if (adev->mode != mode) {
        adev->mode = mode;
        select_mode(adev);
    }
    pthread_mutex_unlock(&adev->lock);

    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;

    ALOGV("%s()\n", __func__);
    adev->mic_mute = state;

    return 0;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;

    ALOGV("%s()\n", __func__);
    *state = adev->mic_mute;

    return 0;
}

static size_t adev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    size_t size;

    ALOGV("%s()\n", __func__);
    /*
     * take resampling into account and return the closest majoring
     * multiple of 16 frames, as audioflinger expects audio buffers to
     * be a multiple of 16 frames
     */
    size = (pcm_config_in.period_size * config->sample_rate) / pcm_config_in.rate;
    size = ((size + 15) / 16) * 16;

    return (size * popcount(config->channel_mask) *
                audio_bytes_per_sample(config->format));
}

static int adev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in)
{
    struct sprd_audio_device *adev = (struct sprd_audio_device *)dev;
    struct sprd_stream_in *in;
    int ret;

    ALOGV("%s()\n", __func__);
    in = (struct sprd_stream_in *)calloc(1, sizeof(struct sprd_stream_in));
    if (!in)
        return -ENOMEM;

    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->dev = adev;
    in->standby = true;
    in->requested_rate = config->sample_rate;
    in->config = &pcm_config_in; 

    *stream_in = &in->stream;
    return 0;

err_open:
    free(in);
    *stream_in = NULL;
    return ret;
}

static void adev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *in)
{
    ALOGV("%s()\n", __func__);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGV("%s()\n", __func__);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    ALOGV("%s()\n", __func__);
    free(device);
    return 0;
}

static uint32_t adev_get_supported_devices(const struct audio_hw_device *dev)
{
    ALOGV("%s()\n", __func__);
    return (/* OUT */
            AUDIO_DEVICE_OUT_EARPIECE |
            AUDIO_DEVICE_OUT_SPEAKER |
            AUDIO_DEVICE_OUT_WIRED_HEADSET |
            AUDIO_DEVICE_OUT_WIRED_HEADPHONE |
            AUDIO_DEVICE_OUT_AUX_DIGITAL |
            AUDIO_DEVICE_OUT_ANLG_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_DGTL_DOCK_HEADSET |
            AUDIO_DEVICE_OUT_ALL_SCO |
            AUDIO_DEVICE_OUT_DEFAULT |
            /* IN */
            AUDIO_DEVICE_IN_COMMUNICATION |
            AUDIO_DEVICE_IN_AMBIENT |
            AUDIO_DEVICE_IN_BUILTIN_MIC |
            AUDIO_DEVICE_IN_WIRED_HEADSET |
            AUDIO_DEVICE_IN_AUX_DIGITAL |
            AUDIO_DEVICE_IN_BACK_MIC |
            AUDIO_DEVICE_IN_ALL_SCO |
            AUDIO_DEVICE_IN_DEFAULT);
}

static int adev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct sprd_audio_device *adev;
    int ret;

    ALOGV("%s()\n", __func__);
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    adev = calloc(1, sizeof(struct sprd_audio_device));
    if (!adev)
        return -ENOMEM;

    adev->device.common.tag = HARDWARE_DEVICE_TAG;
    adev->device.common.version = AUDIO_DEVICE_API_VERSION_1_0;
    adev->device.common.module = (struct hw_module_t *) module;
    adev->device.common.close = adev_close;

    adev->device.get_supported_devices = adev_get_supported_devices;
    adev->device.init_check = adev_init_check;
    adev->device.set_voice_volume = adev_set_voice_volume;
    adev->device.set_master_volume = adev_set_master_volume;
    adev->device.get_master_volume = adev_get_master_volume;
    adev->device.set_mode = adev_set_mode;
    adev->device.set_mic_mute = adev_set_mic_mute;
    adev->device.get_mic_mute = adev_get_mic_mute;
    adev->device.set_parameters = adev_set_parameters;
    adev->device.get_parameters = adev_get_parameters;
    adev->device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->device.open_output_stream = adev_open_output_stream;
    adev->device.close_output_stream = adev_close_output_stream;
    adev->device.open_input_stream = adev_open_input_stream;
    adev->device.close_input_stream = adev_close_input_stream;
    adev->device.dump = adev_dump;

    adev->mixer = mixer_open(CARD_VBC);
    if (!adev->mixer) {
        free(adev);
        ALOGE("Unable to open the mixer, aborting.");
        return -EINVAL;
    }
    set_route_by_array(adev->mixer, route_defaults);

    *device = &adev->device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "SC8810 audio HW HAL",
        .author = "The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
