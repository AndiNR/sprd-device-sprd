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

#define LOG_TAG    "vb_effect"

#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include <sys/mman.h>
#include <eng_audio.h>
#include <stdint.h>
#include <cutils/log.h>
#include "aud_enha.h"

#define VBC_VERSION     "vbc.r0p0"

#define VBC_EQ_FIRMWARE_MAGIC_LEN       (4)
#define VBC_EQ_FIRMWARE_MAGIC_ID        ("VBEQ")
#define VBC_EQ_PROFILE_VERSION          (0x00000001)
#define VBC_EQ_PROFILE_CNT_MAX          (50)
#define VBC_EQ_PROFILE_NAME_MAX         (32)
/* about 61 registers*/
#define VBC_EFFECT_PARAS_LEN            (61)
#define VBC_EFFECT_PROFILE_CNT          (4)

//#define STORED_VBC_EFFECT_PARAS_PATH    "/vendor/firmware/vbc_effect_paras"
#define STORED_VBC_EFFECT_PARAS_PATH    "/data/vbc_effect_parameter"

struct vbc_fw_header {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    uint32_t profile_version;
    uint32_t num_profile;
};

struct vbc_eq_profile {
    char magic[VBC_EQ_FIRMWARE_MAGIC_LEN];
    char name[VBC_EQ_PROFILE_NAME_MAX];
    /* FIXME */
    uint32_t effect_paras[VBC_EFFECT_PARAS_LEN];
};

static const uint32_t vbc_reg_default[VBC_EFFECT_PARAS_LEN] = {	
    0x0,       /*DAPATCHCTL*/
    0x1818,    /*DADGCTL   */
    0x7F,      /*DAHPCTL   */
    0x0,       /*DAALCCTL0 */
    0x0,       /*DAALCCTL1 */
    0x0,       /*DAALCCTL2 */
    0x0,       /*DAALCCTL3 */
    0x0,       /*DAALCCTL4 */
    0x0,       /*DAALCCTL5 */
    0x0,       /*DAALCCTL6 */
    0x0,       /*DAALCCTL7 */
    0x0,       /*DAALCCTL8 */
    0x0,       /*DAALCCTL9 */
    0x0,       /*DAALCCTL10*/
    0x183,     /*STCTL0    */
    0x183,     /*STCTL1    */
    0x0,       /*ADPATCHCTL*/
    0x1818,    /*ADDGCTL   */
    0x0,       /*HPCOEF0   */
    0x0,       /*HPCOEF1   */
    0x0,       /*HPCOEF2   */
    0x0,       /*HPCOEF3   */
    0x0,       /*HPCOEF4   */
    0x0,       /*HPCOEF5   */
    0x0,       /*HPCOEF6   */
    0x0,       /*HPCOEF7   */
    0x0,       /*HPCOEF8   */
    0x0,       /*HPCOEF9   */
    0x0,       /*HPCOEF10  */
    0x0,       /*HPCOEF11  */
    0x0,       /*HPCOEF12  */
    0x0,       /*HPCOEF13  */
    0x0,       /*HPCOEF14  */
    0x0,       /*HPCOEF15  */
    0x0,       /*HPCOEF16  */
    0x0,       /*HPCOEF17  */
    0x0,       /*HPCOEF18  */
    0x0,       /*HPCOEF19  */
    0x0,       /*HPCOEF20  */
    0x0,       /*HPCOEF21  */
    0x0,       /*HPCOEF22  */
    0x0,       /*HPCOEF23  */
    0x0,       /*HPCOEF24  */
    0x0,       /*HPCOEF25  */
    0x0,       /*HPCOEF26  */
    0x0,       /*HPCOEF27  */
    0x0,       /*HPCOEF28  */
    0x0,       /*HPCOEF29  */
    0x0,       /*HPCOEF30  */
    0x0,       /*HPCOEF31  */
    0x0,       /*HPCOEF32  */
    0x0,       /*HPCOEF33  */
    0x0,       /*HPCOEF34  */
    0x0,       /*HPCOEF35  */
    0x0,       /*HPCOEF36  */
    0x0,       /*HPCOEF37  */
    0x0,       /*HPCOEF38  */
    0x0,       /*HPCOEF39  */
    0x0,       /*HPCOEF40  */
    0x0,       /*HPCOEF41  */
    0x0,       /*HPCOEF42  */
};

static int fd_src_paras;
static FILE *  fd_dest_paras;

//get audio nv struct
AUDIO_TOTAL_T *get_aud_paras(uint32_t aud_dev_mode);
void free_aud_paras();

int load_vb_effect(void)
{
    AUDIO_TOTAL_T * aud_params_ptr = NULL;
    AUDIO_TOTAL_T * cur_params_ptr = NULL;
    struct vbc_fw_header  fw_header;
    struct vbc_eq_profile  effect_profile;
    uint32_t i = 0;

    ALOGI("Load_vb_effect....start");
    //audio para nv file--> fd_src
    //vb effect paras file-->fd_dest
    fd_dest_paras = fopen(STORED_VBC_EFFECT_PARAS_PATH, "wb");
    if (NULL  == fd_dest_paras) {
        ALOGE("file %s open failed:%s", STORED_VBC_EFFECT_PARAS_PATH, strerror(errno));
        return -1;
    }
    //init temp buffer for paras calculated.
    memset(&fw_header, 0, sizeof(struct vbc_fw_header));

    memcpy(fw_header.magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
    fw_header.profile_version = VBC_EQ_PROFILE_VERSION;
    fw_header.num_profile = VBC_EFFECT_PROFILE_CNT; //TODO

    ALOGI("fd_dest_paras(0x%x), header_len(%d), profile_len(%d)", (unsigned int)fd_dest_paras,
         sizeof(struct vbc_fw_header), sizeof(struct vbc_eq_profile));
    //write dest file header
    fwrite(&fw_header, sizeof(struct vbc_fw_header), 1, fd_dest_paras);
    //read audio params from source file.
    aud_params_ptr = get_aud_paras(0);

    //loop profile
    for (i=0; i<fw_header.num_profile; i++) {
        cur_params_ptr = aud_params_ptr + i;
        //reset the paras buffer and copy default register value.
        memset(&effect_profile, 0, sizeof(struct vbc_eq_profile));
        memcpy(effect_profile.effect_paras, &vbc_reg_default[0], sizeof(vbc_reg_default));
        //set paras to buffer.
        AUDENHA_SetPara(cur_params_ptr, effect_profile.effect_paras);
        //write buffer to stored file.
        memcpy(effect_profile.magic, VBC_EQ_FIRMWARE_MAGIC_ID, VBC_EQ_FIRMWARE_MAGIC_LEN);
        memcpy(effect_profile.name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName, VBC_EQ_PROFILE_NAME_MAX);
	//strcpy(effect_profile.name, cur_params_ptr->audio_nv_arm_mode_info.ucModeName);
        ALOGI("effect_profile.name is %s", effect_profile.name);
        fwrite(&effect_profile, sizeof(struct vbc_eq_profile), 1, fd_dest_paras);
    }

    //close fd
    munmap((void *)aud_params_ptr, 4*sizeof(AUDIO_TOTAL_T));
    close(fd_src_paras);

    fclose(fd_dest_paras);
    ALOGI("Load_vb_effect....done");
    return 0;
}

AUDIO_TOTAL_T *get_aud_paras(uint32_t aud_dev_mode)
{
    AUDIO_TOTAL_T * aud_params_ptr = NULL;

    ALOGI("aud_dev_mode=%d", aud_dev_mode);
    if (aud_dev_mode > VBC_EFFECT_PROFILE_CNT) {
        ALOGE("aud_dev_mode(%d) overflow.", aud_dev_mode);
        return NULL;
    }

    fd_src_paras = open(ENG_AUDIO_PARA_DEBUG, O_RDONLY);
    if (-1 == fd_src_paras) {
        ALOGW("file %s open failed:%s\n",ENG_AUDIO_PARA_DEBUG,strerror(errno));
        fd_src_paras = open(ENG_AUDIO_PARA,O_RDONLY);
        if(-1 == fd_src_paras){
            ALOGE("file %s open error:%s\n",ENG_AUDIO_PARA,strerror(errno));
            return NULL;
        }
    }

    aud_params_ptr = (AUDIO_TOTAL_T *)mmap(0, 4*sizeof(AUDIO_TOTAL_T),PROT_READ,MAP_SHARED,fd_src_paras,0);
    if ( NULL == aud_params_ptr ) {
        ALOGE("mmap failed %s",strerror(errno));
        return NULL;
    }

    return (aud_params_ptr + aud_dev_mode);
}


