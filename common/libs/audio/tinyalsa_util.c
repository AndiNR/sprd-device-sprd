/*
 * Copyright (C) 2012 The Android Open Source Project
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

//#define ALOG_NDEBUG 0
#define LOG_TAG "Alsa_Util"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <limits.h>

#include <linux/ioctl.h>
#include <sound/asound.h>
#include <tinyalsa/asoundlib.h>
#include <errno.h>


#define ALSA_DEVICE_DIRECTORY "/dev/snd/"

#define SND_FILE_CONTROL	ALSA_DEVICE_DIRECTORY "controlC%i"

struct snd_ctl_card_info_t {
	int card;			/* card number */
	int pad;			/* reserved for future (was type) */
	unsigned char id[16];		/* ID of card (user selectable) */
	unsigned char driver[16];	/* Driver name */
	unsigned char name[32];		/* Short name of soundcard */
	unsigned char longname[80];	/* name + info text about soundcard */
	unsigned char reserved_[16];	/* reserved for future (was ID of mixer) */
	unsigned char mixername[80];	/* visual mixer identification */
	unsigned char components[128];	/* card components / fine identification, delimited with one space (AC97 etc..) */
};

static int get_snd_card_name(int card, char *name);

int set_snd_card_samplerate(int pcm_fd, unsigned int flags, struct pcm_config *config, unsigned short samplerate)
{
    struct snd_pcm_hw_params params;

    if(pcm_fd < 0){
        ALOGE("%s, error pcm_fd (%d) ",__func__,pcm_fd);
        return -1;
    }
    if(config == NULL){
        ALOGE("%s, error pcm config ",__func__);
        return -1;
    }

    param_init(&params);
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_FORMAT,
                   pcm_format_to_alsa(config->format));
    param_set_mask(&params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                   SNDRV_PCM_SUBFORMAT_STD);
    param_set_min(&params, SNDRV_PCM_HW_PARAM_PERIOD_SIZE, config->period_size);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS,
                  pcm_format_to_bits(config->format));
    param_set_int(&params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                  pcm_format_to_bits(config->format) * config->channels);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_CHANNELS,
                  config->channels);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_PERIODS, config->period_count);
    param_set_int(&params, SNDRV_PCM_HW_PARAM_RATE, samplerate);

    if (flags & PCM_NOIRQ) {
        if (!(flags & PCM_MMAP)) {
            ALOGE("%s, noirq only currently supported with mmap(). ", __func__);
            return -1;
        }
        params.flags |= SNDRV_PCM_HW_PARAMS_NO_PERIOD_WAKEUP;
    }
    if (flags & PCM_MMAP)
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                   SNDRV_PCM_ACCESS_MMAP_INTERLEAVED);
    else
        param_set_mask(&params, SNDRV_PCM_HW_PARAM_ACCESS,
                   SNDRV_PCM_ACCESS_RW_INTERLEAVED);

    if (ioctl(pcm_fd, SNDRV_PCM_IOCTL_HW_PARAMS, &params)) {
        ALOGE("%s, SNDRV_PCM_IOCTL_HW_PARAMS failed (%s) ", __func__,strerror(errno));
        return -1;
    }
    ALOGW("%s, out,samplerate (%d) ",__func__,samplerate);
    return 0;
}

int get_snd_card_number(const char *card_name)
{
    int i = 0;
    char cur_name[64] = {0};

    //loop search card number
    for (i = 0; i < 32; i++) {
        get_snd_card_name(i, &cur_name[0]);
        if (strcmp(cur_name, card_name) == 0) {
            ALOGI("Search Completed, cur_name is %s, card_num=%d", cur_name, i);
            return i;
        }
    }
    ALOGE("There is no one found.");
    return -1;
}

static int get_snd_card_name(int card, char *name)
{
    int fd;
    struct snd_ctl_card_info_t info;
    char control[sizeof(SND_FILE_CONTROL) + 10] = {0};
    sprintf(control, SND_FILE_CONTROL, card);

    fd = open(control, O_RDONLY);
    if (fd < 0) {
        ALOGE("open snd control failed.");
        return -1;
    }
    if (ioctl(fd, SNDRV_CTL_IOCTL_CARD_INFO, &info) < 0) {
        ALOGE("SNDRV_CTL_IOCTL_CARD_INFO failed.");
        close(fd);
        return -1;
    }
    close(fd);
    ALOGI("card name is %s, query card=%d", info.name, card);
    //get card name
    if (name) strcpy(name, (char *)info.name);
    return 0;
}

