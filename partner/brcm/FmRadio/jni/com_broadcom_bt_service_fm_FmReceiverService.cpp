/* Copyright 2009-2011 Broadcom Corporation
**
** This program is the proprietary software of Broadcom Corporation and/or its
** licensors, and may only be used, duplicated, modified or distributed 
** pursuant to the terms and conditions of a separate, written license 
** agreement executed between you and Broadcom (an "Authorized License").  
** Except as set forth in an Authorized License, Broadcom grants no license 
** (express or implied), right to use, or waiver of any kind with respect to 
** the Software, and Broadcom expressly reserves all rights in and to the 
** Software and all intellectual property rights therein.  
** IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS 
** SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE 
** ALL USE OF THE SOFTWARE.  
**
** Except as expressly set forth in the Authorized License,
** 
** 1.     This program, including its structure, sequence and organization, 
**        constitutes the valuable trade secrets of Broadcom, and you shall 
**        use all reasonable efforts to protect the confidentiality thereof, 
**        and to use this information only in connection with your use of 
**        Broadcom integrated circuit products.
** 
** 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED 
**        "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, 
**        REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, 
**        OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY 
**        DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, 
**        NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, 
**        ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR 
**        CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
**        OF USE OR PERFORMANCE OF THE SOFTWARE.
**
** 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
**        ITS LICENSORS BE LIABLE FOR 
**        (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY 
**              DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO 
**              YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM 
**              HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR 
**        (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
**              SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE 
**              LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF 
**              ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
*/

#define LOG_TAG "com_broadcom_bt_service_fm_FmReceiverService.cpp"
#define LOG_NDEBUG 0

#include "fm_cfg.h"

#include "jni.h"
#if IS_STANDALONE_FM != TRUE
//Remove includes used in framework
#include "JNIHelp.h"
#include "utils/Log.h"
#include "utils/misc.h"
#include "android_runtime/AndroidRuntime.h"
#else
#include "pthread.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <ctype.h>

// for FM-BT_ON-OFF
#ifdef HAVE_BLUETOOTH
#include <dbus/dbus.h>
#include <bluedroid/bluetooth.h>
#endif

//#include <btl_ifc_wrapper.h>

#ifdef BRCM_BT_USE_BTL_IF
extern "C"
{
#include <btl_ifc.h>
};
#endif

#ifdef BRCM_USE_BTAPP
extern "C"
{
#include <btapp_fm.h>
};

#endif

#include "com_broadcom_bt_service_fm_FmReceiverService_int.h"

#if IS_STANDALONE_FM != TRUE
namespace android {
#endif

typedef struct {
    int event;
    
} tANDROID_EVENT_FM_RX;

#define DTUN_LOCAL_SERVER_ADDR "brcm.bt.dtun"


#define USER_CLASSPATH "."

static JavaVM *jvm;

/* Local references to java callback event functions. */
static jmethodID method_onRadioOnEvent;
static jmethodID method_onRadioOffEvent;
static jmethodID method_onRadioMuteEvent;
static jmethodID method_onRadioTuneEvent;
static jmethodID method_onRadioSearchEvent;
static jmethodID method_onRadioSearchCompleteEvent;
static jmethodID method_onRadioAfJumpEvent;
static jmethodID method_onRadioAudioModeEvent;
static jmethodID method_onRadioAudioPathEvent;
static jmethodID method_onRadioAudioDataEvent;
static jmethodID method_onRadioRdsModeEvent;
static jmethodID method_onRadioRdsTypeEvent;
static jmethodID method_onRadioRdsUpdateEvent;
static jmethodID method_onRadioDeemphEvent;
static jmethodID method_onRadioScanStepEvent;
static jmethodID method_onRadioRegionEvent;
static jmethodID method_onRadioNflEstimationEvent;
static jmethodID method_onRadioVolumeEvent;

static jboolean systemActive;

#ifdef BRCM_BT_USE_BTL_IF
static tCTRL_HANDLE btl_if_fm_handle;
#endif

/* Native callback data. */
typedef struct {
    pthread_mutex_t mutex;
    jobject object;       // JNI global ref to the Java object
} fm_native_data_t;

/* Local static reference to callback java object. */
static JNIEnv *local_env;
static fm_native_data_t fm_nat;

static tBTA_FM_RDS_UPDATE      previous_rds_update;
static jint rds_type_save;
static jboolean af_on_save;
static jboolean rds_on_save;

/* Declarations. */
#ifdef BRCM_BT_USE_BTL_IF
static void decodePendingEvent(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params);
#endif

static void fmReceiverServiceCallbackNative (tBTA_FM_EVT event, tBTA_FM* data);

static void enqueuePendingEvent(tBTA_FM_EVT new_event, tBTA_FM* new_event_data);
static jboolean setRdsRdbsNative(jint rdsType);


/* Static native initializer. */
static void classFmInitNative(JNIEnv* env, jclass clazz) {
    LOGV("%s", __FUNCTION__);
    
    if (env->GetJavaVM(&jvm) < 0) {
            LOGE("%s: Java VM not found!", __FUNCTION__);
    }

    method_onRadioOnEvent = env->GetMethodID(clazz, "onRadioOnEvent", "(I)V");
    method_onRadioOffEvent = env->GetMethodID(clazz, "onRadioOffEvent", "(I)V");
    method_onRadioMuteEvent = env->GetMethodID(clazz, "onRadioMuteEvent", "(IZ)V");
    method_onRadioTuneEvent = env->GetMethodID(clazz, "onRadioTuneEvent", "(IIII)V");
    method_onRadioSearchEvent = env->GetMethodID(clazz, "onRadioSearchEvent", "(III)V");
    method_onRadioSearchCompleteEvent = env->GetMethodID(clazz, "onRadioSearchCompleteEvent", "(IIII)V");
    method_onRadioAfJumpEvent = env->GetMethodID(clazz, "onRadioAfJumpEvent", "(III)V");
    method_onRadioAudioPathEvent = env->GetMethodID(clazz, "onRadioAudioPathEvent", "(II)V");
    method_onRadioAudioModeEvent = env->GetMethodID(clazz, "onRadioAudioModeEvent", "(II)V"); 
    method_onRadioAudioDataEvent = env->GetMethodID(clazz, "onRadioAudioDataEvent", "(IIII)V");
    method_onRadioRdsModeEvent = env->GetMethodID(clazz, "onRadioRdsModeEvent", "(IZZI)V"); 
    method_onRadioRdsTypeEvent = env->GetMethodID(clazz, "onRadioRdsTypeEvent", "(II)V"); 
    method_onRadioRdsUpdateEvent = env->GetMethodID(clazz, "onRadioRdsUpdateEvent", "(IIILjava/lang/String;)V"); 
    method_onRadioDeemphEvent = env->GetMethodID(clazz, "onRadioDeemphEvent", "(II)V"); 
    method_onRadioScanStepEvent = env->GetMethodID(clazz, "onRadioScanStepEvent", "(I)V"); 
    method_onRadioRegionEvent = env->GetMethodID(clazz, "onRadioRegionEvent", "(II)V"); 
    method_onRadioNflEstimationEvent = env->GetMethodID(clazz, "onRadioNflEstimationEvent", "(I)V"); 
    method_onRadioVolumeEvent = env->GetMethodID(clazz, "onRadioVolumeEvent", "(II)V"); 

    /* Initialize the callback object references. */
    pthread_mutex_init(&fm_nat.mutex, NULL);     
    fm_nat.object=NULL;
    
    /* Indicate that events are to be listened to. */
    systemActive = true;
}

/* Dynamic native initializer. */
static void initNativeDataNative(JNIEnv* env, jobject obj)
{     
    LOGV("%s",__FUNCTION__);
} 


static void initLoopNative(JNIEnv* env, jobject obj)
{     
    LOGV("%s",__FUNCTION__);
    fm_nat.object = env->NewGlobalRef(obj);

} 

static void cleanupLoopNative(JNIEnv* env, jobject object) {
    LOGV("%s",__FUNCTION__);
    env->DeleteGlobalRef(fm_nat.object);
}

#ifdef BRCM_BT_USE_BTL_IF
/* Callback handler for stack originated events. */
static void decodePendingEvent(tCTRL_HANDLE handle, tBTLIF_CTRL_MSG_ID id, tBTL_PARAMS *params) {

    tBTA_FM *fm_params;
    LOGI("%s: Event ID: %d",__FUNCTION__,id);
    
    /* Deecode the events coming from the BT-APP and enqueue them for further upwards transport. */
    switch (id) {
        case BTLIF_FM_ENABLE: 
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->status = (tBTA_FM_STATUS)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_ENABLE_EVT, fm_params);
            break;
        case BTLIF_FM_DISABLE:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->status = (tBTA_FM_STATUS)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_DISABLE_EVT, fm_params);
            break;
        case BTLIF_FM_TUNE:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->chnl_info.freq = (UINT16)(params->fm_IIII_param.i3);
            fm_params->chnl_info.snr = (INT8)(params->fm_IIII_param.i4);
            enqueuePendingEvent(BTA_FM_TUNE_EVT, fm_params);
            break;
        case BTLIF_FM_MUTE:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->mute_stat.status = (tBTA_FM_STATUS)(params->fm_IZ_param.i1);
            fm_params->mute_stat.is_mute = (BOOLEAN)(params->fm_IZ_param.z1);
            enqueuePendingEvent(BTA_FM_MUTE_AUD_EVT, fm_params);
            break;
        case BTLIF_FM_SEARCH:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->scan_data.rssi = (UINT8)(params->fm_III_param.i1);
            fm_params->scan_data.freq = (UINT16)(params->fm_III_param.i2);
            fm_params->scan_data.snr = (INT8)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_SEARCH_EVT, fm_params);
            break;
        case BTLIF_FM_SEARCH_CMPL_EVT:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->chnl_info.freq = (UINT16)(params->fm_IIII_param.i3);
            fm_params->chnl_info.snr = (INT8)(params->fm_IIII_param.i4);
            enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, fm_params);
            break;
        case BTLIF_FM_SEARCH_ABORT:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_III_param.i1);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_III_param.i2);
            fm_params->chnl_info.freq = (UINT16)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, fm_params);
            break;
        case BTLIF_FM_SET_RDS_MODE:
            LOGI("%s: RDS MODE EVENT",__FUNCTION__);
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_mode.status = (tBTA_FM_STATUS)(params->fm_IZZ_param.i1);
            fm_params->rds_mode.rds_on = (BOOLEAN)(params->fm_IZZ_param.z1);
            fm_params->rds_mode.af_on = (BOOLEAN)(params->fm_IZZ_param.z2);
            enqueuePendingEvent(BTA_FM_RDS_MODE_EVT, fm_params);
            break;
        case BTLIF_FM_SET_RDS_RBDS:
            LOGI("%s: RDS TYPE EVENT",__FUNCTION__);
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_type.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->rds_type.type = (tBTA_FM_RDS_B)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_RDS_TYPE_EVT, fm_params);
            break;
        case BTLIF_FM_RDS_UPDATE:
            LOGI("%s: RDS UPDATE EVENT",__FUNCTION__);
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->rds_update.status = (tBTA_FM_STATUS)(params->fm_IIIS_param.i1);
            fm_params->rds_update.data = (UINT8)(params->fm_IIIS_param.i2);
            fm_params->rds_update.index = (UINT8)(params->fm_IIIS_param.i3);
            memcpy(fm_params->rds_update.text, params->fm_IIIS_param.s1, 65);                    
            enqueuePendingEvent(BTA_FM_RDS_UPD_EVT, fm_params);
            break;
        case BTLIF_FM_AUDIO_MODE:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->mode_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->mode_info.audio_mode = (tBTA_FM_AUDIO_MODE)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_AUD_MODE_EVT, fm_params);
            break;
        case BTLIF_FM_AUDIO_PATH:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->path_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->path_info.audio_path = (tBTA_FM_AUDIO_PATH)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_AUD_PATH_EVT, fm_params);
            break;
        case BTLIF_FM_SCAN_STEP:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->scan_step = (tBTA_FM_STEP_TYPE)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_SCAN_STEP_EVT, fm_params);
            break;
        case BTLIF_FM_SET_REGION:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->region_info.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->region_info.region = (tBTA_FM_REGION_CODE)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_SET_REGION_EVT, fm_params);
            break;
        case BTLIF_FM_CONFIGURE_DEEMPHASIS:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->deemphasis.status = (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->deemphasis.time_const = (tBTA_FM_DEEMPHA_TIME)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_SET_DEEMPH_EVT, fm_params);
            break;
        case BTLIF_FM_ESTIMATE_NFL:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->nfloor = (UINT8)(params->fm_I_param.i1);
            enqueuePendingEvent(BTA_FM_NFL_EVT, fm_params);
            break;
        case BTLIF_FM_GET_AUDIO_QUALITY:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->audio_data.status = (tBTA_FM_STATUS)(params->fm_IIII_param.i1);
            fm_params->audio_data.rssi = (UINT8)(params->fm_IIII_param.i2);
            fm_params->audio_data.audio_mode = (tBTA_FM_AUDIO_QUALITY)(params->fm_IIII_param.i3);
            fm_params->audio_data.snr = (INT8)(params->fm_IIII_param.i4);
            enqueuePendingEvent(BTA_FM_AUD_DATA_EVT, fm_params);
            break;
        case BTLIF_FM_AF_JMP_EVT:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
            fm_params->chnl_info.status = (tBTA_FM_STATUS)(params->fm_III_param.i1);
            fm_params->chnl_info.freq = (UINT16)(params->fm_III_param.i2);
            fm_params->chnl_info.rssi = (UINT8)(params->fm_III_param.i3);
            enqueuePendingEvent(BTA_FM_AF_JMP_EVT, fm_params);
        	break;
        case BTLIF_FM_SET_VOLUME_EVT:
            fm_params = (tBTA_FM*)malloc(sizeof(tBTA_FM));
	        fm_params->volume.status= (tBTA_FM_STATUS)(params->fm_II_param.i1);
            fm_params->volume.volume= (UINT16)(params->fm_II_param.i2);
            enqueuePendingEvent(BTA_FM_VOLUME_EVT, fm_params);
            break;    
                                                            
        default: break;
    }

}

#endif

/* Assumes data exists. Screens the text field for illegal (non ASCII) characters. */
static void screenData(tBTA_FM* data) {
    int i = 0;
    char c = data->rds_update.text[i];

    while ((i < 65) && (0 != c)) { 
        /* Check legality of all characters. */
        if ((c < 32) || (c > 126)) {
            data->rds_update.text[i] = '*';
        }
        /* Look at next character. */
        c = data->rds_update.text[++i];
    }
    /* Cap for safety at end of 64 bytes. */
    data->rds_update.text[64] = 0;
}


/* Forwards the event to the application. */
static void enqueuePendingEvent(tBTA_FM_EVT new_event, tBTA_FM* new_event_data) {

    jint r;
    jclass cls;
    
    jmethodID mid;
    jint event = new_event;
    tBTA_FM* data = new_event_data;

#ifdef BRCM_BT_USE_BTL_IF

    pthread_mutex_lock(&fm_nat.mutex);

    /* Attach callback thread to JVM. */
    LOGI("%s: event ID: %d, ATTACHING THREAD",__FUNCTION__,new_event);

    if (jvm->AttachCurrentThread(&local_env, NULL) != JNI_OK) {            
        LOGI("%s: THREAD NOT ATTACHED",__FUNCTION__);
    } else {
        LOGI("%s: THREAD ATTACHED OK",__FUNCTION__);
    }

    LOGI("[JNI] - TRANSMITTING EVENT UP :    event = %i", event);
       
#endif

    switch (event)
    {
    case BTA_FM_ENABLE_EVT:
//	    fm_enable_complete();
	    memset(&previous_rds_update, 0, sizeof(tBTA_FM_RDS_UPDATE));	
        local_env->CallVoidMethod(fm_nat.object, method_onRadioOnEvent, (jint)(data->status)); 
        break;        
    case BTA_FM_DISABLE_EVT:
        #if IS_STANDALONE_FM != TRUE
        fm_disable();
        #endif
        local_env->CallVoidMethod(fm_nat.object, method_onRadioOffEvent, (jint)(data->status));
//        BTL_IFC_UnregisterSubSystem(&btl_if_fm_handle, SUB_FM);
        break;
    case BTA_FM_TUNE_EVT: 
	    memset(&previous_rds_update, 0, sizeof(tBTA_FM_RDS_UPDATE));	
        local_env->CallVoidMethod(fm_nat.object, method_onRadioTuneEvent, 
            (jint)(data->chnl_info.status), (jint)(data->chnl_info.rssi),
            (jint)(data->chnl_info.snr), (jint)(data->chnl_info.freq));
        break;
    case BTA_FM_MUTE_AUD_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioMuteEvent, 
            (jint)(data->mute_stat.status), (jboolean)(data->mute_stat.is_mute)); 
        break;
    case BTA_FM_SEARCH_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioSearchEvent, 
            (jint)(data->scan_data.rssi), (jint)(data->scan_data.snr), (jint)(data->scan_data.freq)); 
        break;
    case BTA_FM_SEARCH_CMPL_EVT:
	    memset(&previous_rds_update, 0, sizeof(tBTA_FM_RDS_UPDATE));	
        local_env->CallVoidMethod(fm_nat.object, method_onRadioSearchCompleteEvent, 
            (jint)(data->chnl_info.status), (jint)(data->chnl_info.rssi),
            (jint)(data->chnl_info.snr), (jint)(data->chnl_info.freq));
        LOGI("[JNI] - TRANSMITTING EVENT BTA_FM_SEARCH_CMPL_EVT : rssi = %i freq = %i snr = %i", data->chnl_info.rssi, data->chnl_info.freq, data->chnl_info.snr);
        break;
    case BTA_FM_AF_JMP_EVT:
   	    memset(&previous_rds_update, 0, sizeof(tBTA_FM_RDS_UPDATE));	
        local_env->CallVoidMethod(fm_nat.object, method_onRadioAfJumpEvent,
            (jint)(data->chnl_info.status), (jint)(data->chnl_info.rssi),
            (jint)(data->chnl_info.freq));
             LOGI("[JNI] - TRANSMITTING EVENT BTA_FM_AF_JMP_EVT :    status = %i rssi = %i freq = %i", data->chnl_info.status,data->chnl_info.rssi,data->chnl_info.freq);
        break;
    case BTA_FM_AUD_MODE_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioAudioModeEvent, 
            (jint)(data->mode_info.status), (jint)(data->mode_info.audio_mode));
        break;
    case BTA_FM_AUD_PATH_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioAudioPathEvent,
            (jint)(data->path_info.status), (jint)(data->path_info.audio_path));
        break;
    case BTA_FM_AUD_DATA_EVT:
	LOGI("BTA_FM_AUD_DATA_EVT rssi:%d snr:%d", (jint)(data->audio_data.rssi), (jint)(data->audio_data.snr));
        local_env->CallVoidMethod(fm_nat.object, method_onRadioAudioDataEvent, 
            (jint)(data->audio_data.status), (jint)(data->audio_data.rssi),
            (jint)(data->audio_data.audio_mode), (jint)(data->audio_data.snr));
        break;
    case BTA_FM_RDS_MODE_EVT:
        LOGI("%s: BTA_FM_RDS_MODE_EVT",__FUNCTION__);
        if(data->rds_mode.status == BTA_FM_OK)
        	{ 
        	   af_on_save = (jboolean)(data->rds_mode.af_on);
	          rds_on_save = (jboolean)(data->rds_mode.rds_on);
                 setRdsRdbsNative(rds_type_save);
        	}
	else{	
              local_env->CallVoidMethod(fm_nat.object, method_onRadioRdsModeEvent, 
            (jint)(data->rds_mode.status), (jboolean)(data->rds_mode.rds_on),
            (jboolean)(data->rds_mode.af_on),((jint)rds_type_save));
		}
        break;
    case BTA_FM_RDS_TYPE_EVT:
        LOGI("%s: BTA_FM_RDS_TYPE_EVT",__FUNCTION__);
//        local_env->CallVoidMethod(fm_nat.object, method_onRadioRdsTypeEvent, 
//            (jint)(data->rds_type.status), (jint)(data->rds_type.type)); 
              local_env->CallVoidMethod(fm_nat.object, method_onRadioRdsModeEvent, 
            (jint)(data->rds_mode.status), rds_on_save,
           af_on_save,(jint)(data->rds_type.type));


        break;
    case BTA_FM_RDS_UPD_EVT:
        LOGI("%s: BTA_FM_RDS_UPD_EVT, 0x%8x",__FUNCTION__,data);
        if (NULL != data) {
            /* Pre-process data. */
            screenData(data);
	     //not to overload system with useless events check if data has changed
        LOGI("%s: BTA_FM_RDS_UPD_EVT, previous_rds%s",__FUNCTION__,previous_rds_update.text);
        LOGI("%s: BTA_FM_RDS_UPD_EVT, new_rds%s",__FUNCTION__,data->rds_update.text);
        LOGI("%s: BTA_FM_RDS_UPD_EVT, memcmp 0x%8x",__FUNCTION__,memcmp(&previous_rds_update.text, &data->rds_update.text, 65));
//        LOGI("%s: BTA_FM_RDS_UPD_EVT, 0x%8x",__FUNCTION__,data);
	     if(memcmp(&previous_rds_update, &data->rds_update, sizeof(tBTA_FM_RDS_UPDATE)))
	     	{
	     	memcpy(&previous_rds_update, &data->rds_update, sizeof(tBTA_FM_RDS_UPDATE));
            /* Transmit upwards. */
            local_env->CallVoidMethod(fm_nat.object, method_onRadioRdsUpdateEvent,
                (jint)(data->rds_update.status), (jint)(data->rds_update.data),
                (jint)(data->rds_update.index),
                local_env->NewStringUTF(data->rds_update.text)); 
	     	}
        }
        break;
    case BTA_FM_SET_DEEMPH_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioDeemphEvent, 
            (jint)(data->deemphasis.status), (jint)(data->deemphasis.time_const)); 
        break;
    case BTA_FM_SCAN_STEP_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioScanStepEvent, 
            (jint)(data->scan_step)); 
        break;
    case BTA_FM_SET_REGION_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioRegionEvent, 
            (jint)(data->region_info.status), (jint)(data->region_info.region)); 
        break;
    case BTA_FM_NFL_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioNflEstimationEvent, 
            (jint)(data->nfloor)); 
        break;        
    case BTA_FM_VOLUME_EVT:
        local_env->CallVoidMethod(fm_nat.object, method_onRadioVolumeEvent, 
            (jint)(data->volume.status), (jint)(data->volume.volume));
        break;	    
    default:
        LOGI("[JNI_CALLBACK] - androidFmRxCback :    unknown event received");
        break;
    }
    if (NULL != data) {
        free(data);
    }

#ifdef BRCM_BT_USE_BTL_IF
    
    if (jvm->DetachCurrentThread() != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __FUNCTION__);  
    }

    /* Detach thread from JVM. */
    pthread_mutex_unlock(&fm_nat.mutex);

#endif

}



static jboolean enableNative(JNIEnv *env, jobject obj, jint functionalityMask)
{
    LOGI("[JNI] - enableNative :    functionalityMask = %i", functionalityMask);

#ifdef BRCM_BT_USE_BTL_IF 
    tBTLIF_FM_REQ_I_PARAM params;
    tBTL_IF_Result res;
    // Call fm_enable to load btld if needed
    #if IS_STANDALONE_FM != TRUE
    //if (fm_enable() != -1)
    //{
    #endif
        /* Initialize datapath client  */
	    res = BTL_IFC_ClientInit();

	    LOGI("[JNI] - enableNative :    INIT = %i", (int)res);

	    /* Register with BTL-IF as subsystem client. */
	    res = BTL_IFC_RegisterSubSystem(&btl_if_fm_handle, SUB_FM, NULL, decodePendingEvent);

    LOGI("[JNI] - enableNative :    REGISTER = %i", (int)res);

    /* Set up bt-app parameters. */
    params.i1 = (INT32) functionalityMask;

    res = BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ENABLE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

	    LOGI("[JNI] - enableNative :    ENABLE = %i", (int)res);
	if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;
    #if IS_STANDALONE_FM != TRUE
   // }
    #endif

#else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");  
    
        /* Simulate a matched thread environment. */
        local_env = env;
        fm_nat.object = obj;        

    if (NULL != data) {
        data->status = BTA_FM_OK;
        enqueuePendingEvent(BTA_FM_ENABLE_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean disableNative(JNIEnv *env, jobject obj, jboolean bForcing)
{
    LOGI("[JNI] - disableNative :");
    
#ifdef BRCM_BT_USE_BTL_IF    
    tBTL_IF_Result res,res2;

    #if IS_STANDALONE_FM != TRUE

    if (bForcing == true)
    {
	    fm_disable();
    } else {
    #endif
        pthread_mutex_lock(&fm_nat.mutex);

        res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_DISABLE, NULL, 0);
        pthread_mutex_unlock(&fm_nat.mutex);

	    if((res!= BTL_IF_SUCCESS))
	        return JNI_FALSE;
    #if IS_STANDALONE_FM != TRUE
    }
    #endif
#else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {
        data->status = BTA_FM_OK;
        enqueuePendingEvent(BTA_FM_DISABLE_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean muteNative(JNIEnv *env, jobject obj, jboolean toggle)
{
    LOGI("[JNI] - muteNative :    toggle = %i", toggle);
    
#ifdef BRCM_BT_USE_BTL_IF
    tBTL_IF_Result res;
    tBTLIF_FM_REQ_Z_PARAM params;

    /* Set up bt-app parameters. */
    params.z1 = (BOOLEAN) toggle;

    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_MUTE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_Z_PARAM));

    if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;

    #else
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->mute_stat.status = BTA_FM_OK;
        data->mute_stat.is_mute = toggle;
        enqueuePendingEvent(BTA_FM_MUTE_AUD_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean tuneNative(JNIEnv *env, jobject obj, jint freq)
{
    LOGI("[JNI] - tuneNative :    freq = %i", freq);

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_I_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (int) freq;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_TUNE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;
#else

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->chnl_info.status = BTA_FM_OK;
        data->chnl_info.rssi = 120;
        data->chnl_info.freq = (UINT16) freq;
        enqueuePendingEvent(BTA_FM_TUNE_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean searchNative(JNIEnv *env, jobject obj, jint scanMode, jint rssiThreshold, jint condVal, jint condType)
{
    LOGI("[JNI] - searchNative :    scanMode = %i  rssiThreshold = %i  condVal = %i  condType = %i", scanMode, rssiThreshold, condVal, condType);
    
#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_IIII_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (int) scanMode;    
    params.i2 = (int) rssiThreshold;
    params.i3 = (int) condVal;    
    params.i4 = (int) condType;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_IIII_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {                
        data->scan_data.rssi = 120;
        data->scan_data.freq = 10000;
        data->scan_data.snr = 20;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean comboSearchNative(JNIEnv *env, jobject obj, jint startFreq,  jint endFreq, jint rssiThreshold, jint direction, jint scanMethod, jboolean multiChannel, jint rdsType, jint rdsTypeValue)
{
    LOGI("[JNI] - comboSearchNative");

#ifdef BRCM_BT_USE_BTL_IF
    tBTLIF_FM_REQ_IIIIIZII_PARAM params;
    tBTL_IF_Result res;

    /* Set up bt-app parameters. */
    params.i1 = (UINT16) startFreq;
    params.i2 = (UINT16) endFreq;
    params.i3 = (int) rssiThreshold;
    params.i4 = (int) direction;
    params.i5 = (int) scanMethod;
    params.z1 = (BOOLEAN) multiChannel;
    params.i6 = (int) rdsType;
    params.i7 = (int) rdsTypeValue;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_COMBO_SEARCH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_IIIIIZII_PARAM));
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {
        data->scan_data.rssi = 120;
        data->scan_data.freq = 10000;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean searchAbortNative(JNIEnv *env, jobject obj)
{
    LOGI("[JNI] - searchAbortNative :");

#ifdef BRCM_BT_USE_BTL_IF  
    tBTL_IF_Result res;

    pthread_mutex_lock(&fm_nat.mutex);
    res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SEARCH_ABORT, NULL, 0);
    pthread_mutex_unlock(&fm_nat.mutex);

    if(res!= BTL_IF_SUCCESS)
	    return JNI_FALSE;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {        
        data->chnl_info.status = BTA_FM_SCAN_ABORT;
        data->chnl_info.rssi = 30;
        data->chnl_info.freq = 10000;
        enqueuePendingEvent(BTA_FM_SEARCH_CMPL_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean setRdsModeNative(JNIEnv *env, jobject obj, jboolean rdsOn, jboolean afOn, jint rdsType)
{
    LOGI("[JNI] - setRdsModeNative :");
    
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_ZZ_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.z1 = (BOOLEAN) rdsOn;    
     params.z2 = (BOOLEAN) afOn;    
     rds_type_save = (int) rdsType;   
	 
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_MODE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_ZZ_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    if (NULL != data) {        
        data->rds_mode.status = BTA_FM_OK;
        data->rds_mode.rds_on = rdsOn;
        data->rds_mode.af_on = afOn;
        enqueuePendingEvent(BTA_FM_RDS_MODE_EVT, data);
    }
#endif
    return JNI_TRUE;
}

//static jboolean setRdsRdbsNative(JNIEnv *env, jobject obj, jint rdsType)
static jboolean setRdsRdbsNative(jint rdsType)
{
    LOGI("[JNI] - setRdsRdbsNative :");

#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) rdsType;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_RDS_RBDS, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
   // local_env = env;
    //fm_nat.object = obj;
    
    if (NULL != data) {        
        data->rds_type.status = BTA_FM_OK;
        data->rds_type.type = (tBTA_FM_RDS_B) rdsType;       
        enqueuePendingEvent(BTA_FM_RDS_TYPE_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean setAudioModeNative(JNIEnv *env, jobject obj, jint audioMode)
{
    LOGI("[JNI] - setAudioModeNative :    audioMode = %i", audioMode);

#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) audioMode;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_MODE, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {        
        data->mode_info.status = BTA_FM_OK;
        data->mode_info.audio_mode = (tBTA_FM_AUDIO_MODE)audioMode;       
        enqueuePendingEvent(BTA_FM_AUD_MODE_EVT, data);
    }
#endif

    return JNI_TRUE;
}

static jboolean setAudioPathNative(JNIEnv *env, jobject obj, jint audioPath)
{
    LOGI("[JNI] - setAudioPathNative :    audioPath = %i", audioPath);

#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) audioPath;

     res = BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_AUDIO_PATH, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;
#else

    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));

    LOGI("[JNI] - SIMULATE COMMAND");

    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;

    if (NULL != data) {
        data->path_info.status = BTA_FM_OK;
        data->path_info.audio_path = (tBTA_FM_AUDIO_PATH)audioPath;
        enqueuePendingEvent(BTA_FM_AUD_PATH_EVT, data);
    }
#endif

    return JNI_TRUE;
}




static jboolean setScanStepNative(JNIEnv *env, jobject obj, jint stepSize)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) stepSize;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SCAN_STEP, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {        
        data->scan_step = (tBTA_FM_STEP_TYPE)stepSize;       
        enqueuePendingEvent(BTA_FM_SCAN_STEP_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean setRegionNative(JNIEnv *env, jobject obj, jint region)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) region;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_REGION, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {
        data->region_info.status = BTA_FM_OK;
        data->region_info.region = (tBTA_FM_REGION_CODE) region;
        enqueuePendingEvent(BTA_FM_SET_REGION_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean configureDeemphasisNative(JNIEnv *env, jobject obj, jint time)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) time;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_CONFIGURE_DEEMPHASIS,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;


#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {
        data->deemphasis.status = BTA_FM_OK;
        data->deemphasis.time_const = (tBTA_FM_DEEMPHA_TIME) time;
        enqueuePendingEvent(BTA_FM_SET_DEEMPH_EVT, data);
    }
#endif
    return JNI_TRUE;
}

static jboolean estimateNoiseFloorNative(JNIEnv *env, jobject obj, jint level)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) level;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_ESTIMATE_NFL,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;


#else
    
    /* Only for testing. */
    tBTA_FM *data = (tBTA_FM*)malloc(sizeof(tBTA_FM));
    
    LOGI("[JNI] - SIMULATE COMMAND");
    
    /* Simulate a matched thread environment. */
    local_env = env;
    fm_nat.object = obj;
    
    if (NULL != data) {        
        data->nfloor = (tBTA_FM_NFE_LEVL) level;
        enqueuePendingEvent(BTA_FM_NFL_EVT, data);
    }
#endif   
    return JNI_TRUE;
}

static jboolean getAudioQualityNative(JNIEnv *env, jobject obj, jboolean enable)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_Z_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.z1 = (BOOLEAN) enable;         
    
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_GET_AUDIO_QUALITY,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_Z_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    
#endif   
    return JNI_TRUE;
}

static jboolean configureSignalNotificationNative(JNIEnv *env, jobject obj, jint pollInterval)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) pollInterval;         
            
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_CONFIGURE_SIGNAL_NOTIFICATION,
        (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
        
#endif  
    return JNI_TRUE;
}

static jboolean setFMVolumeNative(JNIEnv *env, jobject obj, jint volume)
{
#ifdef BRCM_BT_USE_BTL_IF  
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;
    
     /* Set up bt-app parameters. */
     params.i1 = (int) volume;         
    LOGI("[JNI] - setFMVolumeNative volume =%d", volume);
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_VOLUME, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
 	    return JNI_FALSE;

#else
    

#endif
    return JNI_TRUE;
}

static jboolean setSNRThresholdNative(JNIEnv *env, jobject obj, jint snr_thres)
{
#ifdef BRCM_BT_USE_BTL_IF
     tBTLIF_FM_REQ_I_PARAM params;
     tBTL_IF_Result res;

     /* Set up bt-app parameters. */
     params.i1 = (int) snr_thres;
    LOGI("[JNI] - setSNRThresholdNative snr =%d", snr_thres);
     res=BTL_IFC_CtrlSend(btl_if_fm_handle, SUB_FM, BTLIF_FM_SET_SNR_THRES, (tBTL_PARAMS*)(&params), sizeof(tBTLIF_FM_REQ_I_PARAM));

     if(res!= BTL_IF_SUCCESS)
        return JNI_FALSE;

#else

#endif
    return JNI_TRUE;
}

/* List of native functions that will be registered with Android startup. */

/* Note: All functions must be "called" by Java or they will be removed by 
   the compiler, resulting in mysterious JNI registration errors. */

/* These functions are used for initialization and calls to the FM stack.*/
JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    {"initNativeDataNative", "()V", (void *)initNativeDataNative},
    {"enableNative", "(I)Z", (void *)enableNative},
    {"disableNative", "(Z)Z", (void *)disableNative},
    {"tuneNative", "(I)Z", (void *)tuneNative},
    {"muteNative", "(Z)Z", (void *)muteNative},
    {"searchNative", "(IIII)Z", (void *)searchNative},
    {"comboSearchNative", "(IIIIIZII)Z", (void *)comboSearchNative},
    {"searchAbortNative", "()Z", (void *)searchAbortNative},
    {"setRdsModeNative", "(ZZI)Z", (void *) setRdsModeNative},
//    {"setRdsRdbsNative", "(I)Z", (void *) setRdsRdbsNative},
    {"setAudioModeNative", "(I)Z", (void *)setAudioModeNative},
    {"setAudioPathNative", "(I)Z", (void *)setAudioPathNative},
    {"setScanStepNative", "(I)Z", (void *) setScanStepNative},
    {"setRegionNative", "(I)Z", (void *) setRegionNative},
    {"configureDeemphasisNative", "(I)Z", (void *) configureDeemphasisNative},
    {"estimateNoiseFloorNative", "(I)Z", (void *) estimateNoiseFloorNative},
    {"getAudioQualityNative", "(Z)Z", (void *) getAudioQualityNative},
    {"configureSignalNotificationNative", "(I)Z", (void *) configureSignalNotificationNative},
    {"setFMVolumeNative", "(I)Z", (void *) setFMVolumeNative},
    {"setSNRThresholdNative", "(I)Z", (void *) setSNRThresholdNative},
    {"classFmInitNative", "()V", (void *)classFmInitNative},
    {"initLoopNative", "()V", (void *)initLoopNative},
    {"cleanupLoopNative", "()V", (void *)cleanupLoopNative},
};



#if IS_STANDALONE_FM != TRUE
/* Register stack calling functions with Android JNI. */
int register_com_broadcom_bt_service_fm_FmReceiverService(JNIEnv *env) {
    return AndroidRuntime::registerNativeMethods(env,
            "com/broadcom/bt/service/fm/FmReceiverService", sMethods, NELEM(sMethods));
}
} /* namespace android */
#else


/**
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

/*
 * Register native methods for all classes we know about.
 *
 * returns JNI_TRUE on success.
 */
int register_com_broadcom_bt_service_fm_FmReceiverService(JNIEnv *env)
{
    if (!registerNativeMethods(env, "com/broadcom/bt/service/fm/FmReceiverService",
            sMethods, sizeof(sMethods) / sizeof(sMethods[0]))) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

#endif
