#include <engat.h>
#include <sys/uio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cutils/properties.h>

static int at_cmd_fd = -1;
static int at_cmd_prefix_len;
static char at_cmd_prefix[16];
static char pwrprop[PROPERTY_VALUE_MAX];
static  char cmd[30];

#ifdef __cplusplus
extern "C" {
#endif
    int engapi_read(int, void*, size_t);
    int engapi_write(int, const void*, size_t);
    int engapi_open(int);
    void engapi_close(int);
#ifdef __cplusplus
}
#endif

#define do_cmd(at_cmd) \
    if (at_cmd && at_cmd_send_recv((void*)at_cmd, strlen(at_cmd), r_buf, sizeof r_buf)) { \
        ALOGE("--------------------------------------\n" \
             "Switch incall AT command [%s][%s] failed", at_cmd, r_buf); \
    } else if (at_cmd) { \
        ALOGW("--------------------------------------\n" \
             "Switch incall AT command [%s][%s] good", at_cmd, r_buf); \
    }

int at_cmd_deinit(void)
{
    if (at_cmd_fd > 0) {
        engapi_close(at_cmd_fd);
        at_cmd_fd = -1;
    }
    return 0;
}

// external/sprd/engineeringmodel/engcs/eng_appclient.c
static int android_sim;
static int android_sim_num;
int at_cmd_init(void)
{
    char buf[8];
    int sim = 0;
//    if (android_sim <= 0 && !access("/sys/class/modem/sim", W_OK)) {
//        android_sim = open("/sys/class/modem/sim", O_RDWR);
//    }
//    if (android_sim > 0) {
//        lseek(android_sim, 0, SEEK_SET);
//        read(android_sim, buf, sizeof(buf) - 1);
//        sim = strtol(buf, NULL, NULL);
//    }
    if (at_cmd_fd <= 0 /*|| sim != android_sim_num*/) {
        at_cmd_deinit();
        at_cmd_fd = engapi_open(sim);
//        android_sim_num = sim;
        at_cmd_prefix_len = sprintf(at_cmd_prefix, "%d,%d,", ENG_AT_NOHANDLE_CMD, 1);
    }
    if (at_cmd_fd > 0)
        ALOGW("at cmd using sim%d", sim);
    else ALOGE("at cmd using sim%d failed", sim);
    return 0;
}

int at_cmd_send_recv(void *s_buf, size_t s_len, void *r_buf, size_t r_len)
{
//  struct iovec iov[2];
    char at_cmd[64];

    at_cmd_init();

    if (at_cmd_fd > 0) {
//      iov[0].iov_base = at_cmd_prefix;
//      iov[0].iov_len = at_cmd_prefix_len;
//      iov[1].iov_base = s_buf;
//      iov[1].iov_len = s_len;
        if (r_buf) memset(r_buf, 0, r_len); // ((char*)r_buf)[0] = 0;
//      if (writev(at_cmd_fd, iov, 2) < 0) {
        ALOGW("write incall AT command [%s]", (char*)s_buf);
        if (engapi_write(at_cmd_fd, at_cmd, sprintf(at_cmd, "%s%s", at_cmd_prefix, (char*)s_buf)) < 0) {
            at_cmd_fd = -1;
            ALOGE("--------------------------------------\n"
                 "Switch incall AT command [%s][%s] failed", (char*)s_buf, strerror(errno));
        } else {
            ALOGW("read incall AT command [%s]", (char*)s_buf);
            engapi_read(at_cmd_fd, r_buf, r_len);
        }

        //at_cmd_deinit();

        return strstr((char *)r_buf, "OK") == NULL; // strncmp((char*)r_buf, "OK", 2);
    } else {
        snprintf((char*)r_buf, r_len, "%s", "at_cmd_fd = -1");
    }

    return -1;
}

void at_cmd_route_done(struct tiny_audio_device *adev)
{
    property_get("ro.gpio.pcm_device_switch",pwrprop,NULL);
    sprintf(cmd,"echo %d 0 1 0 > /dev/gpio",atoi(pwrprop));
    system(cmd);
    ALOGI("Switch PCM control back to AP, adev->devices=0x%08x", adev->devices);
}

int at_cmd_route(struct tiny_audio_device *adev, int on)
{
    char r_buf[32];
    const char *at_cmd = NULL;
    if (adev->mode != AUDIO_MODE_IN_CALL) {
        ALOGE("Error: NOT mode_in_call, current mode(%d)", adev->mode);
        return -1;
    }

    if (adev->devices & (AUDIO_DEVICE_OUT_WIRED_HEADSET | AUDIO_DEVICE_OUT_WIRED_HEADPHONE)) {
        at_cmd = "AT+SSAM=2";
    } else if (adev->devices & (AUDIO_DEVICE_OUT_BLUETOOTH_SCO
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET
                                | AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT)) {

        property_get("ro.gpio.pcm_device_switch",pwrprop,NULL);
        sprintf(cmd,"echo %d 1 1 1 > /dev/gpio",atoi(pwrprop));
        system(cmd);
        ALOGI("Switch PCM control to CP for BT call.");

        at_cmd = "AT+SSAM=5";
    } else if (adev->devices & AUDIO_DEVICE_OUT_SPEAKER) {
        at_cmd = "AT+SSAM=1";
    } else {
        at_cmd = "AT+SSAM=0";
    }
    ALOGI("at_cmd_route, at_cmd:%s ",at_cmd);
    at_cmd_init();
    do_cmd(at_cmd);
    at_cmd_deinit();
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
    do_cmd(AT_CMD_INCALL_FREQ2);
    usleep(100*1000);
    do_cmd(AT_CMD_INCALL_FREQ1);
    usleep(100*1000);
    do_cmd(AT_CMD_INCALL_STOP);

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
        do_cmd(at_cmd); \
        retval = select(incalltone_stop + 1, &rfds, NULL, NULL, &tv);\
        if (retval) {\
            read(incalltone_stop, r_buf, sizeof(r_buf) - 1);\
            at_cmd = AT_CMD_INCALL_STOP; \
            do_cmd(at_cmd);\
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
                #if 1
                check_incalltone_stop(AT_CMD_INCALL_FREQ2, 50);
                check_incalltone_stop(AT_CMD_INCALL_FREQ1, 100);
                at_cmd = AT_CMD_INCALL_STOP; do_cmd(at_cmd);
                check_incalltone_stop(NULL, 1500);
                #else
                check_incalltone_stop(AT_CMD_INCALL_FREQ3, 1000);
                check_incalltone_stop(AT_CMD_INCALL_FREQ4, 4000);
                #endif
            }
            if (retval == 0) { at_cmd = AT_CMD_INCALL_STOP; do_cmd(at_cmd); }
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
    char r_buf[32];
    const char *at_cmd = "AT+CLVL=7";
    ALOGW("audio at_cmd_headset_volume_max");
    do_cmd(at_cmd);
    return 0;
}

int at_cmd_volume(float vol, int mode)
{
    char r_buf[256];
    char buf[16];
    char *at_cmd = buf;
    static float offset_vol = -1;
    static int at_vgr_modem_max;
    float vol1;
    if (offset_vol < 0) {
        /*********** sp6810a ***********
        这个问题是这样的，打电话的时候，如果你将音量调到最小（UI上还会剩下一格），
        如果不加0.125会导致，对方听不见声音。
        这个在我们其他好几个产品上提过bug：
        也就是说调到最小也不能让对方听见声音，而且也得和UI保持一致。
        AudioFlinger::setVoiceVolume ==> vol + 0.125
        ********************************/
        property_get("ro.hardware", r_buf, "");
        if (strcmp(r_buf, "sp6810a") == 0 ||
            strcmp(r_buf, "sp6820a") == 0 ) {
            offset_vol = 0.125;
            at_vgr_modem_max = 6;
        } else {
            offset_vol = 0;
            at_vgr_modem_max = 6;
        }
    }
    vol1 = vol - offset_vol;
    int volume = vol1 * at_vgr_modem_max + 1;
    if (volume >= at_vgr_modem_max) volume = at_vgr_modem_max;
    ALOGW("audio mode=%d %s ==> %d-%f, offset_vol=%f", mode, __func__, volume, vol, offset_vol);
    snprintf(at_cmd, sizeof buf, "AT+VGR=%d", volume);
    do_cmd(at_cmd);
    return 0;
}

int at_cmd_mic_mute(bool mute)
{
    char r_buf[32];
    const char *at_cmd;
    ALOGW("audio at_cmd_mic_mute %d", mute);
    if (mute) at_cmd = "AT+CMUT=1";
    else at_cmd = "AT+CMUT=0";
    do_cmd(at_cmd);
    return 0;
}
