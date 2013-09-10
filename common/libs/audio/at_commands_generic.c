#include <sys/uio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cutils/properties.h>
#include "AtChannel.h"

static int at_cmd_fd = -1;
//static int at_cmd_prefix_len;
static char at_cmd_prefix[16];
static pthread_mutex_t  ATlock = PTHREAD_MUTEX_INITIALIZER;         //eng cannot handle many at commands once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


static int cur_call_sim = 0;
static int cur_cp_type = 0;

int do_cmd_dual(int modemId, int simId, const  char* at_cmd) 
{
    pthread_mutex_lock(&ATlock);
    if (at_cmd)  { 
        ALOGE("do_cmd_dual Switch incall AT command [%s][%s] ", at_cmd, sendAt(modemId, simId, at_cmd)); \
    } 
    pthread_mutex_unlock(&ATlock);
    return 0;
}


int at_cmd_init(void)
{
#ifndef _VOICE_CALL_VIA_LINEIN
    if ( cur_call_sim != android_sim_num 
        ||cur_cp_type!= st_vbc_ctrl_thread_para->adev->cp_type) {
        cur_call_sim = android_sim_num;
	 cur_cp_type =  st_vbc_ctrl_thread_para->adev->cp_type;
    }
#endif

    return 0;
}

static int at_cmd_route(struct tiny_audio_device *adev)
{
    const char *at_cmd = NULL;
    if (adev->mode != AUDIO_MODE_IN_CALL) {
        ALOGE("Error: NOT mode_in_call, current mode(%d)", adev->mode);
        return -1;
    }

    if (adev->out_devices & (AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) {
        at_cmd = "AT+SSAM=2";
    } else if (adev->out_devices & (AUDIO_DEVICE_OUT_BLUETOOTH_SCO
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) {
        at_cmd = "AT+SSAM=5";
    } else if (adev->out_devices & AUDIO_DEVICE_OUT_SPEAKER) {
        at_cmd = "AT+SSAM=1";
    } else {
        at_cmd = "AT+SSAM=0";
    }
    do_cmd_dual(adev->cp_type, cur_call_sim, at_cmd);

    return 0;
}

#define AT_CMD_INCALL_FREQ1 "AT+STONE=1,400,100"
#define AT_CMD_INCALL_FREQ2 "AT+STONE=1,1200,50"
#define AT_CMD_INCALL_FREQ3 "AT+STONE=1,425,100"
#define AT_CMD_INCALL_FREQ4 "AT+STONE=1,0,400"
#define AT_CMD_INCALL_STOP  "AT+STONE=0"
int at_cmd_incall_tone(int type)
{
    char r_buf[32];
    do_cmd_dual(cur_cp_type, cur_call_sim, AT_CMD_INCALL_FREQ2);
    usleep(100*1000);
    do_cmd_dual(cur_cp_type, cur_call_sim, AT_CMD_INCALL_FREQ1);
    usleep(100*1000);
    do_cmd_dual(cur_cp_type, cur_call_sim, AT_CMD_INCALL_STOP);

    return 0;
}

#if AT_INCALL_TONE

#define check_incalltone_stop(freq, time) \
{ const char *at_cmd = freq; \
    FD_ZERO(&rfds); \
    FD_SET(incalltone_stop, &rfds); \
    {\
        tv.tv_sec = time / 1000;\
        tv.tv_usec = (time % 1000) * 1000;\
        do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd); \
        retval = select(incalltone_stop + 1, &rfds, NULL, NULL, &tv);\
        if (retval) {\
            read(incalltone_stop, r_buf, sizeof(r_buf) - 1);\
            at_cmd = AT_CMD_INCALL_STOP; \
            do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd); \
            break;\
        }\
    }\
}

void stop_at_incall_tone_thread(void)
{
    int incalltone_stop;
    incalltone_stop  = open("/dev/pipe/ipc.0.1", O_RDWR);
    if (incalltone_stop >= 0) {
        write(incalltone_stop, "stop", 4);
        close(incalltone_stop);
        ALOGW("stop_at_incall_tone_thread");
    }
}

void *at_incall_tone_thread(void *args)
{
    int retval, i;
    fd_set rfds;
    struct timeval tv;
    const char *at_cmd;
    char r_buf[128];
    int hold_ms_time;
    int max_repeat_count = 200;
    int incalltone_start, incalltone_stop, max_fd;
    incalltone_start = open("/dev/pipe/ipc.0.0", O_RDWR);
    incalltone_stop  = open("/dev/pipe/ipc.0.1", O_RDWR);
    if (incalltone_stop > incalltone_start) max_fd = incalltone_stop;
    else max_fd = incalltone_start;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(incalltone_start, &rfds);
        FD_SET(incalltone_stop, &rfds);
        ALOGW("at_incall_tone_thread waiting for incall tone command");
        retval = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (retval == -1) {
           ALOGW("================== [ BUG sync2ril_modem pipe error ] ==================");
           usleep(200*1000);
           continue;
        }

        if (FD_ISSET(incalltone_start, &rfds)) {
            read(incalltone_start, r_buf, sizeof(r_buf) - 1);
            if (FD_ISSET(incalltone_stop, &rfds)) read(incalltone_stop, r_buf, sizeof(r_buf) - 1);
            ALOGW("start incalltone");
            for (i = 0; i < max_repeat_count; i++) {
                check_incalltone_stop(AT_CMD_INCALL_FREQ2, 50);
                check_incalltone_stop(AT_CMD_INCALL_FREQ1, 100);
                at_cmd = AT_CMD_INCALL_STOP; 
                do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
                check_incalltone_stop(NULL, 1500);
            }
            if (retval == 0) { at_cmd = AT_CMD_INCALL_STOP; 
                do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
            }
            ALOGW("stop incalltone");

            FD_ZERO(&rfds);
            FD_SET(incalltone_start, &rfds);
            FD_SET(incalltone_stop, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1;
            retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
            if (retval) {
                if (FD_ISSET(incalltone_start, &rfds)) read(incalltone_start, r_buf, sizeof(r_buf) - 1);
            } else continue;
        }

        if (FD_ISSET(incalltone_stop, &rfds)) {
            read(incalltone_stop, r_buf, sizeof(r_buf) - 1);
        }
    }
    return NULL;
}
#endif

int at_cmd_headset_volume_max(void)
{
    const char *at_cmd = "AT+CLVL=7";
    ALOGW("audio at_cmd_headset_volume_max");
    do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
    return 0;
}

#include <cutils/properties.h>
#define VOICECALL_VOLUME_MAX_UI	6

int at_cmd_volume(float vol, int mode)
{
    char buf[16];
    char *at_cmd = buf;
    int ret = 0;
    unsigned short cur_device;

    int volume = vol * VOICECALL_VOLUME_MAX_UI + 1;
    if (volume >= VOICECALL_VOLUME_MAX_UI) volume = VOICECALL_VOLUME_MAX_UI;
    ALOGI("%s mode=%d ,volume=%d, android vol:%f ", __func__,mode,volume,vol);
    snprintf(at_cmd, sizeof buf, "AT+VGR=%d", volume);
    do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
    return 0;
}

int at_cmd_mic_mute(bool mute)
{
    const char *at_cmd;
    ALOGW("audio at_cmd_mic_mute %d", mute);
    if (mute) at_cmd = "AT+CMUT=1";
    else at_cmd = "AT+CMUT=0";
    do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
    return 0;
}

int at_cmd_audio_loop(int enable, int mode, int volume,int loopbacktype,int voiceformat,int delaytime)
{
    char buf[89];
    char *at_cmd = buf;
	if(volume >9) {
		volume = 9;
	}
    ALOGW("audio at_cmd_audio_loop enable:%d,mode:%d,voluem:%d,loopbacktype:%d,voiceformat:%d,delaytime:%d",enable,mode,volume,loopbacktype,voiceformat,delaytime);

    snprintf(at_cmd, sizeof buf, "AT+SPVLOOP=%d,%d,%d,%d,%d,%d", enable,mode,volume,loopbacktype,voiceformat,delaytime);
    do_cmd_dual(cur_cp_type, cur_call_sim, at_cmd);
    return 0;
}
