#include <utils/Log.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>

//#ifdef __cplusplus
//extern "c"
//{
//#endif


//#define MY_DEBUG

#ifdef MY_DEBUG
#define MY_TRACE    ALOGW
#else
#define MY_TRACE
#endif

#define VBC_CMD_TAG   "VBC"

#define READ_PARAS(type, exp)    if (s_vbpipe_fd > 0 && paras_ptr != NULL) { \
        exp = read(s_vbpipe_fd, paras_ptr, sizeof(type)); \
        }

#define ENG_AUDIO_PGA       "/sys/class/vbc_param_config/vbc_pga_store"

static uint32_t android_cur_device = 0x0;		// devices value the same as audiosystem.h
static pthread_t s_vbc_ctrl_thread = 0;
static int s_vbpipe_fd = -1;
static int s_is_exit = 0;
static int s_is_active = 0;


/* vbc control parameters struct here.*/
typedef struct Paras_Mode_Gain
{
    unsigned short  is_mode;
    unsigned short  is_volume;

    unsigned short  mode_index;
    unsigned short  volume_index;

    unsigned short  dac_set;
    unsigned short  adc_set;

    unsigned short  dac_gain;
    unsigned short  adc_gain;

    unsigned short  path_set;
    unsigned short  pa_setting;

    unsigned short  reserved[16];
}paras_mode_gain_t;

typedef struct Switch_ctrl
{
    unsigned int  is_switch; /* switch vbc contrl to dsp.*/
}switch_ctrl_t;

typedef struct Set_Mute
{
    unsigned int  is_mute;
}set_mute_t;

typedef struct Device_ctrl
{
    unsigned short  	is_open; /* if is_open is true, open device; else close device.*/
    unsigned short  	is_headphone;
	unsigned int 		is_downlink_mute;
	unsigned int 		is_uplink_mute;
	paras_mode_gain_t 	paras_mode;
}device_ctrl_t;



typedef struct {
	unsigned short adc_pga_gain_l;
	unsigned short adc_pga_gain_r;
	unsigned short dac_pga_gain_l;
	unsigned short dac_pga_gain_r;
	unsigned short pa_setting;
	uint32_t devices;
	uint32_t mode;
}pga_gain_nv_t;

typedef struct{
	unsigned short pa_setting;
	uint32_t dac_pga_l;
	uint32_t dac_pga_r;
	uint32_t hp_pga_l;
	uint32_t hp_pga_r;
}dac_output_pga_t;

/* list vbc cmds */
enum VBC_CMD_E
{
    VBC_CMD_NONE = 0,
/* current mode and volume gain parameters.*/
    VBC_CMD_SET_MODE = 1,
    VBC_CMD_RSP_MODE = 2,
    VBC_CMD_SET_GAIN = 3,
    VBC_CMD_RSP_GAIN = 4,
/* whether switch vb control to dsp parameters.*/
    VBC_CMD_SWITCH_CTRL = 5,
    VBC_CMD_RSP_SWITCH = 6,
/* whether mute or not.*/
    VBC_CMD_SET_MUTE = 7,
    VBC_CMD_RSP_MUTE = 8,
/* open/close device parameters.*/
    VBC_CMD_DEVICE_CTRL = 9,
    VBC_CMD_RSP_DEVICE = 10,

	VBC_CMD_HAL_OPEN = 11,
	VBC_CMD_RSP_OPEN  =12,

	VBC_CMD_HAL_CLOSE = 13,
	VBC_CMD_RSP_CLOSE = 14,

    VBC_CMD_MAX
};

typedef struct
{
    char        tag[4];   /* "VBC" */
    unsigned int    cmd_type;
    unsigned int    paras_size; /* the size of Parameters Data, unit: bytes*/
}parameters_head_t;

/* Transfer packet by vbpipe, packet format as follows.*/
/************************************************
----------------------------
|Paras Head |  Paras Data  |
----------------------------
************************************************/

/*
 * local functions declaration.
 */
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr);

static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr);

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr);

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr);

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr);

void *vbc_ctrl_thread_routine(void *args);

/*
 * local functions definition.
 */
static int  ReadParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && head_ptr != NULL) {
        ret = read(fd_pipe, head_ptr, sizeof(parameters_head_t));
    }
    return ret;
}

static int  WriteParas_Head(int fd_pipe,  parameters_head_t *head_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && head_ptr != NULL) {
        ret = write(fd_pipe, head_ptr, sizeof(parameters_head_t));
    }
    return ret;
}

static int  ReadParas_DeviceCtrl(int fd_pipe, device_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(device_ctrl_t));
    }
    return ret;
}

static int  ReadParas_ModeGain(int fd_pipe,  paras_mode_gain_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(paras_mode_gain_t));
    }
    return ret;
}

static int  ReadParas_SwitchCtrl(int fd_pipe,  switch_ctrl_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(switch_ctrl_t));
    }
    return ret;
}

static int  ReadParas_Mute(int fd_pipe,  set_mute_t *paras_ptr)
{
    int ret = 0;
    if (fd_pipe > 0 && paras_ptr != NULL) {
        ret = read(fd_pipe, paras_ptr, sizeof(set_mute_t));
    }
    return ret;
}

int Write_Rsp2cp(int fd_pipe, unsigned int cmd)
{
	int ret = 0;
	int count = 0;
	parameters_head_t write_common_head;
    memset(&write_common_head, 0, sizeof(parameters_head_t));
	memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = cmd+1;
    write_common_head.paras_size = 0;
	if(fd_pipe < 0){
		ALOGE("%s vbpipe has not open...",__func__);
		return -1;
	}
	WriteParas_Head(fd_pipe, &write_common_head);
	MY_TRACE("%s: send  cmd(%d) to cp .",__func__,write_common_head.cmd_type);
	return 0;
}

unsigned short GetCall_Cur_Device()
{
	return android_cur_device;
}

static void SetCall_ModePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
	unsigned short switch_earpice = 0;
	unsigned short switch_headset = 0;
	unsigned short switch_speaker = 0;
	unsigned short switch_mic0 = 0;
	unsigned short switch_mic1 = 0;
	unsigned short switch_hp_mic = 0;

	MY_TRACE("%s path_set:0x%x .",__func__,mode_gain_paras->path_set);
	switch_earpice = (mode_gain_paras->path_set & 0x0040)>>6;
	switch_headset = mode_gain_paras->path_set & 0x0001;
	switch_speaker = (mode_gain_paras->path_set & 0x0008)>>3;
	switch_mic0 = (mode_gain_paras->path_set & 0x0400)>>10;
	switch_mic1 = (mode_gain_paras->path_set & 0x0800)>>11;
	switch_hp_mic = (mode_gain_paras->path_set & 0x1000)>>12;

//At present, switch of pa cannot handle mulit-device
	android_cur_device = 0;

	if(switch_earpice){
		android_cur_device |= 0x1;
	}
	if(switch_speaker){
		android_cur_device |= 0x2;
	}
	if(switch_headset){
		android_cur_device |= 0x4;
	}
	if(switch_mic0 | switch_mic1){
		android_cur_device |= 0x40000;
	}
	if(switch_hp_mic){
		android_cur_device |= 0x100000;
	}

	set_call_route(adev, AUDIO_DEVICE_OUT_EARPIECE, switch_earpice);
	set_call_route(adev, AUDIO_DEVICE_OUT_SPEAKER, switch_speaker);
	set_call_route(adev, AUDIO_DEVICE_IN_BUILTIN_MIC, switch_mic0);
	set_call_route(adev, AUDIO_DEVICE_IN_WIRED_HEADSET, switch_hp_mic);

	ALOGW("%s successfully, device: earpice(%s), headphone(%s), speaker(%s), Mic(%s), hp_mic(%s) android_cur_device(0x%x)"
				,__func__,switch_earpice ? "Open":"Close",switch_headset ? "Open":"Close",switch_speaker ? "Open":"Close",
				switch_mic0 ? "Open":"Close",switch_hp_mic ? "Open":"Close",android_cur_device);
}

static void SetCall_VolumePara(struct tiny_audio_device *adev,paras_mode_gain_t *mode_gain_paras)
{
	int ret = 0;
	pga_gain_nv_t pga_gain_nv;
	memset(&pga_gain_nv,0,sizeof(pga_gain_nv_t));
	if(NULL == mode_gain_paras){
		ret = -1;
		ALOGE("%s mode paras is NULL!!",__func__);
		return;
	}
	MY_TRACE("%s devices:0x%x mode:%d ,adc_gain:0x%x ,dac_gain:0x%x ,pa_setting:0x%x .",
		__func__,adev->devices,adev->mode,mode_gain_paras->adc_gain,mode_gain_paras->dac_gain,mode_gain_paras->pa_setting);

	pga_gain_nv.devices = adev->devices;
	pga_gain_nv.mode = adev->mode;
	pga_gain_nv.adc_pga_gain_l= mode_gain_paras->adc_gain & 0x00ff;
	pga_gain_nv.adc_pga_gain_r= (mode_gain_paras->adc_gain & 0xff00) >> 8;
	pga_gain_nv.dac_pga_gain_l= mode_gain_paras->dac_gain & 0x00ff;
	pga_gain_nv.dac_pga_gain_r= (mode_gain_paras->dac_gain & 0xff00) >> 8;
	pga_gain_nv.pa_setting = (mode_gain_paras->pa_setting) & 0x00ff;
	//To do...
	
//	ALOGW("%s parse pga successfully ,dac_pga_l:0x%x ,dac_pga_r:0x%x,hp_pga_l:0x%x ,hp_pga_r:0x%x .pa_setting:0x%x ",
//		__func__,pga_out_gain.dac_pga_l,pga_out_gain.dac_pga_r,pga_out_gain.hp_pga_l,pga_out_gain.hp_pga_r,pga_out_gain.pa_setting);
}

int GetParas_DeviceCtrl_Incall(int fd_pipe,device_ctrl_t *device_ctrl_param)	//open,close
{
	int ret = 0;
	MY_TRACE("%s in... ",__func__);
	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_DEVICE_CTRL);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
	ret = ReadParas_DeviceCtrl(fd_pipe,device_ctrl_param);
	if (ret <= 0) {
		ALOGE("Error, read %s failed(%d).",__func__,ret);
	}
	if((!device_ctrl_param->paras_mode.is_mode) || (!device_ctrl_param->paras_mode.is_volume)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s,ReadParas_DeviceCtrl wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully ,is_open(%d) is_headphone(%d) is_downlink_mute(%d) is_uplink_mute(%d) volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x) ",__func__,device_ctrl_param->is_open,device_ctrl_param->is_headphone, \
		device_ctrl_param->is_downlink_mute,device_ctrl_param->is_uplink_mute,device_ctrl_param->paras_mode.volume_index,device_ctrl_param->paras_mode.adc_gain,device_ctrl_param->paras_mode.path_set,device_ctrl_param->paras_mode.dac_gain,device_ctrl_param->paras_mode.pa_setting);
	return ret;
}

int GetParas_Route_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SET_MODE);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s failed(%d).",__func__,ret);
    }
	if((!mode_gain_paras->is_mode)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
		mode_gain_paras->path_set, mode_gain_paras->dac_gain,mode_gain_paras->pa_setting);
	return ret;
}

int GetParas_Volume_Incall(int fd_pipe,paras_mode_gain_t *mode_gain_paras)	//set_volume & set_route
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SET_GAIN);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d).",__func__,ret);
	}
    ret = ReadParas_ModeGain(fd_pipe,mode_gain_paras);
    if (ret <= 0) {
        ALOGE("Error, read %s failed(%d).",__func__,ret);
    }
	if((!mode_gain_paras->is_volume)){	//check whether is setDevMode
		ret =-1;
		ALOGE("Error: %s ReadParas_ModeGain wrong cmd_type.",__func__);
		return ret;
	}
	MY_TRACE("%s successfully,volume_index(%d) adc_gain(0x%x), path_set(0x%x), dac_gain(0x%x), pa_setting(0x%x)",__func__,mode_gain_paras->volume_index,mode_gain_paras->adc_gain, \
		mode_gain_paras->path_set, mode_gain_paras->dac_gain, mode_gain_paras->pa_setting);
	return ret;
}

int GetParas_Switch_Incall(int fd_pipe,switch_ctrl_t *swtich_ctrl_paras)	/* switch vbc contrl to dsp.*/
{
	int ret = 0;
	parameters_head_t read_common_head;
	memset(&read_common_head, 0, sizeof(parameters_head_t));
	MY_TRACE("%s in...",__func__);

	ret = Write_Rsp2cp(fd_pipe,VBC_CMD_SWITCH_CTRL);
	if(ret < 0){
		ALOGE("Error, %s Write_Rsp2cp failed(%d)",__func__,ret);
	}
	ret = ReadParas_SwitchCtrl(fd_pipe,swtich_ctrl_paras);
	if (ret <= 0) {
	    ALOGE("Error, read ReadParas_SwitchCtrl failed(%d)",ret);
	}
	MY_TRACE("%s successfully ,is_switch(%d) ",__func__,swtich_ctrl_paras->is_switch);
	return ret;
}

int SetParas_Route_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	unsigned short switch_earpice = 0;
	unsigned short switch_headset = 0;
	unsigned short switch_speaker = 0;
	unsigned short switch_mic0 = 0;
	unsigned short switch_mic1 = 0;
	unsigned short switch_hp_mic = 0;
	paras_mode_gain_t mode_gain_paras;
	memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
	MY_TRACE("%s in.....",__func__);
	ret = GetParas_Route_Incall(fd_pipe,&mode_gain_paras);
	if(ret < 0){
		return ret;
	}
	SetCall_ModePara(adev,&mode_gain_paras);
	SetCall_VolumePara(adev,&mode_gain_paras);
	Write_Rsp2cp(fd_pipe,VBC_CMD_SET_MODE);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_Volume_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	paras_mode_gain_t mode_gain_paras;
	memset(&mode_gain_paras,0,sizeof(paras_mode_gain_t));
	MY_TRACE("%s in.....",__func__);
	ret = GetParas_Volume_Incall(fd_pipe,&mode_gain_paras);
	if(ret < 0){
		return ret;
	}
	SetCall_VolumePara(adev,&mode_gain_paras);
	Write_Rsp2cp(fd_pipe,VBC_CMD_SET_GAIN);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_Switch_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	parameters_head_t write_common_head;
	switch_ctrl_t swtich_ctrl_paras;
	memset(&swtich_ctrl_paras,0,sizeof(swtich_ctrl_paras));
	MY_TRACE("%s in...",__func__);
	ret = GetParas_Switch_Incall(fd_pipe,&swtich_ctrl_paras);
	if(ret < 0){
		return ret;
	}
    mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, !swtich_ctrl_paras.is_switch);
	ALOGW("%s, VBC %s dsp...",__func__,(swtich_ctrl_paras.is_switch)?"Switch control to":"Get control back from");
	Write_Rsp2cp(fd_pipe,VBC_CMD_SWITCH_CTRL);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int SetParas_DeviceCtrl_Incall(int fd_pipe,struct tiny_audio_device *adev)
{
	int ret = 0;
	device_ctrl_t device_ctrl_paras;
	memset(&device_ctrl_paras,0,sizeof(device_ctrl_t));
	MY_TRACE("%s in.....",__func__);

	//because of codec,we should set headphone on first...
	set_call_route(adev, AUDIO_DEVICE_OUT_WIRED_HEADSET, 1);

	ret =GetParas_DeviceCtrl_Incall(fd_pipe,&device_ctrl_paras);
	if(ret < 0){
		return ret;
	}

	//set arm mode paras
	if(device_ctrl_paras.is_open){
		SetCall_ModePara(adev,&device_ctrl_paras.paras_mode);
		SetCall_VolumePara(adev,&device_ctrl_paras.paras_mode);
	}else{
		ALOGW("%s close device...",__func__);
	}
	Write_Rsp2cp(fd_pipe,VBC_CMD_DEVICE_CTRL);
	MY_TRACE("%s send rsp to cp...",__func__);
	return ret;
}

int vbc_ctrl_open(struct tiny_audio_device *adev)
{
    if (s_is_active) return (-1);
    int rc;
    MY_TRACE("%s IN.",__func__);
    s_is_exit = 0;
    s_is_active = 1;
    rc = pthread_create(&s_vbc_ctrl_thread, NULL, vbc_ctrl_thread_routine, (void *)adev);
    if (rc) {
        ALOGE("error, pthread_create failed, rc=%d", rc);
        return (-1);
    }

    return (0);
}

int vbc_ctrl_close()
{
    if (!s_is_active) return (-1);
    MY_TRACE("%s IN.",__func__);
   
    s_is_exit = 1;
    s_is_active = 0;
    /* close vbpipe.*/
    close(s_vbpipe_fd);
    s_vbpipe_fd = -1;

    /* terminate thread.*/
    //pthread_cancel (s_vbc_ctrl_thread);    
    return (0);
}

void *vbc_ctrl_thread_routine(void *arg)
{
    int ret = 0;
    struct tiny_audio_device *adev;
    parameters_head_t read_common_head;
    parameters_head_t write_common_head;
    adev = (struct tiny_audio_device *)arg;

    memset(&read_common_head, 0, sizeof(parameters_head_t));
    memset(&write_common_head, 0, sizeof(parameters_head_t));
    
    memcpy(&write_common_head.tag[0], VBC_CMD_TAG, 3);
    write_common_head.cmd_type = VBC_CMD_NONE;
    write_common_head.paras_size = 0;
    MY_TRACE("vbc_ctrl_thread_routine in.");
    
RESTART:
    if (s_is_exit) goto EXIT;
    /* open vbpipe to build connection.*/
    if (s_vbpipe_fd == -1) {
        s_vbpipe_fd = open("/dev/vbpipe6", O_RDWR);
        if (s_vbpipe_fd <= 0) {
            ALOGE("Error: s_vbpipe_fd(%d) open failed.", s_vbpipe_fd);
            return 0;
        } else {
            ALOGW("s_vbpipe_fd(%d) open successfully.", s_vbpipe_fd);
        }
    } else {
        ALOGW("warning: s_vbpipe_fd(%d) NOT closed.", s_vbpipe_fd);
    }

    /* loop to read parameters from vbpipe.*/
    while(!s_is_exit)
    {
    	ALOGW("looping now...");
        /* read parameters common head of the packet.*/
        ret = ReadParas_Head(s_vbpipe_fd, &read_common_head);
        if (ret <= 0) {
            ALOGE("Error, %s read head failed(%d).",__func__,ret);
            goto RESTART;
        }
        ALOGW("%s In Call, Get CMD(%d) from cp, paras_size:%d devices:0x%x mode:%d", adev->in_call ? "":"NOT",read_common_head.cmd_type,read_common_head.paras_size,adev->devices,adev->mode);
        if (!memcmp(&read_common_head.tag[0], VBC_CMD_TAG, 3)) {
            switch (read_common_head.cmd_type)
            {
            case VBC_CMD_HAL_OPEN:
            {
                MY_TRACE("VBC_CMD_HAL_OPEN IN.");
                force_all_standby(adev);
                pthread_mutex_lock(&adev->lock);
                adev->pcm_modem_dl= pcm_open(s_tinycard, PORT_MODEM, PCM_OUT | PCM_MMAP, &pcm_config_vx);
                if (!pcm_is_ready(adev->pcm_modem_dl)) {
                    ALOGE("cannot open pcm_out driver: %s", pcm_get_error(adev->pcm_modem_dl));
                    pcm_close(adev->pcm_modem_dl);
                    s_is_exit = 1;
                }
                adev->pcm_modem_ul= pcm_open(s_tinycard, PORT_MODEM, PCM_IN, &pcm_config_vrec_vx);
                if (!pcm_is_ready(adev->pcm_modem_ul)) {
					ALOGE("cannot open pcm modem in");
					pcm_close(adev->pcm_modem_ul);
					s_is_exit = 1;
				}
                ALOGW("START CALL,open pcm device...");
                adev->in_call = 1;
                pthread_mutex_unlock(&adev->lock);
                write_common_head.cmd_type = VBC_CMD_RSP_OPEN;
                WriteParas_Head(s_vbpipe_fd, &write_common_head);
                MY_TRACE("VBC_CMD_HAL_OPEN OUT.");
            }
            break;
            case VBC_CMD_HAL_CLOSE:
            {
                MY_TRACE("VBC_CMD_HAL_CLOSE IN.");
                mixer_ctl_set_value(adev->private_ctl.vbc_switch, 0, 1);	//switch to arm
                pthread_mutex_lock(&adev->lock);
                pcm_close(adev->pcm_modem_dl);
                pcm_close(adev->pcm_modem_ul);
                adev->in_call = 0;
                ALOGW("END CALL,close pcm device & switch to arm...");
                force_all_standby(adev);
                pthread_mutex_unlock(&adev->lock);
                write_common_head.cmd_type = VBC_CMD_RSP_CLOSE;
                WriteParas_Head(s_vbpipe_fd, &write_common_head);
                MY_TRACE("VBC_CMD_HAL_CLOSE OUT.");
            }
            break;
            case VBC_CMD_SET_MODE:
            {
                MY_TRACE("VBC_CMD_SET_MODE IN.");
                //paras_mode_gain_t *mode_gain_paras_ptr = malloc(sizeof(mode_gain_paras_ptr));
                ret = SetParas_Route_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SET_MODE SetParas_Route_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_SET_MODE OUT.");
            }
            break;
            case VBC_CMD_SET_GAIN:
            {
                MY_TRACE("VBC_CMD_SET_GAIN IN.");
                //paras_mode_gain_t *mode_gain_paras_ptr = malloc(sizeof(mode_gain_paras_ptr));
                ret = SetParas_Volume_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SET_GAIN SetParas_Route_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_SET_GAIN OUT.");
            }
            break; 
            case VBC_CMD_SWITCH_CTRL:
            {
                MY_TRACE("VBC_CMD_SWITCH_CTRL IN.");
                ret = SetParas_Switch_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_SWITCH_CTRL SetParas_Switch_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_SWITCH_CTRL OUT.");
            }
            break;
            case VBC_CMD_SET_MUTE:
            {
                MY_TRACE("VBC_CMD_SET_MUTE IN.");

                MY_TRACE("VBC_CMD_SET_MUTE OUT.");
            }
            break;
            case VBC_CMD_DEVICE_CTRL:
            {
                MY_TRACE("VBC_CMD_DEVICE_CTRL IN.");

                ret = SetParas_DeviceCtrl_Incall(s_vbpipe_fd,adev);
                if(ret < 0){
                    MY_TRACE("VBC_CMD_DEVICE_CTRL SetParas_DeviceCtrl_Incall error.s_is_exit:%d ",s_is_exit);
                    s_is_exit = 1;
                }
                MY_TRACE("VBC_CMD_DEVICE_CTRL OUT.");
            }
            break;
            default:
                ALOGE("Error: %s wrong cmd_type(%d)",__func__,read_common_head.cmd_type);
            break;
            }
        } else {
            ALOGE("Error, (0x%x)NOT match VBC_CMD_TAG, wrong packet.", *((int*)read_common_head.tag));
        }
    }
EXIT:
    ALOGW("vbc_ctrl_thread exit!!!");
    return 0;
}

