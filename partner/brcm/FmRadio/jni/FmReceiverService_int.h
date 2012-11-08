/* Copyright 2009 - 2011 Broadcom Corporation
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


#define UINT8 uint8_t
#define UINT16 uint16_t  
#define UINT32 uint32_t  
#define INT32 int32_t  

#define BOOLEAN bool


/* Max AF number */
#ifndef BTA_FM_AF_MAX_NUM
#define BTA_FM_AF_MAX_NUM   25
#endif

/* FM band region, bit 0, 1,2 of func_mask */
#ifndef BTA_MAX_REGION_SETTING
#define BTA_MAX_REGION_SETTING  3   /* max region code defined */
#endif

typedef UINT8   tBTA_FM_STATUS;
/* FM function mask */
#define     BTA_FM_REGION_NA    0x00        /* bit0/bit1/bit2: north america */
#define     BTA_FM_REGION_EUR   0x01        /* bit0/bit1/bit2: Europe           */
#define     BTA_FM_REGION_JP    0x02        /* bit0/bit1/bit2: Japan            */
#define     BTA_FM_REGION_JP_II   0x03        /* bit0/bit1/bit2: Japan II         */
#define     BTA_FM_RDS_BIT      1<<4        /* bit4: RDS functionality */
#define     BTA_FM_RBDS_BIT     1<<5        /* bit5: RBDS functionality, exclusive with RDS bit */
#define     BTA_FM_AF_BIT       1<<6        /* bit6: AF functionality */
#define     BTA_FM_SOFTMUTE_BIT       1<<8        /* bit8: SOFTMUTE functionality */
typedef UINT16   tBTA_FM_FUNC_MASK;

#define     BTA_FM_REGION_MASK  0x07             /* low 3 bits (bit0, 1, 2)of FUNC mask is region code */
typedef UINT8  tBTA_FM_REGION_CODE;


#define     BTA_FM_DEEMPHA_50U      0       /* 6th bit in FM_AUDIO_CTRL0 set to 0, Europe default */
#define     BTA_FM_DEEMPHA_75U      1<<6    /* 6th bit in FM_AUDIO_CTRL0 set to 1, US  default */
typedef UINT8 tBTA_FM_DEEMPHA_TIME;

typedef UINT8 tBTA_FM_SCH_RDS_TYPE;



typedef UINT8  tBTA_FM_AUDIO_MODE;

/* FM audio output mode */
enum
{
    BTA_FM_AUDIO_PATH_NONE,             /* None */
    BTA_FM_AUDIO_PATH_SPEAKER,          /* speaker */
    BTA_FM_AUDIO_PATH_WIRE_HEADSET,     /* wire headset */
    BTA_FM_AUDIO_PATH_I2S               /* digital */
};
typedef UINT8  tBTA_FM_AUDIO_PATH;

/* FM audio output quality */
#define BTA_FM_STEREO_ACTIVE    0x01     /* audio stereo detected */
#define BTA_FM_MONO_ACTIVE      0x02     /* audio mono */
#define BTA_FM_BLEND_ACTIVE     0x04     /* stereo blend active */

typedef UINT8  tBTA_FM_AUDIO_QUALITY;

/* FM audio routing configuration */
#define BTA_FM_SCO_ON       0x01        /* routing FM over SCO off, voice on SCO on */
#define BTA_FM_SCO_OFF      0x02        /* routing FM over SCO on, voice on SCO off */
#define BTA_FM_A2D_ON       0x10        /* FM analog output active */
#define BTA_FM_A2D_OFF      0x40        /* FM analog outout off */
#define BTA_FM_I2S_ON       0x20        /* FM digital (I2S) output on */
#define BTA_FM_I2S_OFF      0x80        /* FM digital output off */

typedef UINT8 tBTA_FM_AUDIO_PATH;

/* scan mode */
//#define BTA_FM_PRESET_SCAN      I2C_FM_SEARCH_PRESET        /* preset scan : bit0 = 1 */
//#define BTA_FM_NORMAL_SCAN      I2C_FM_SEARCH_NORMAL        /* normal scan : bit0 = 0 */
typedef UINT8 tBTA_FM_SCAN_METHOD;

/* frequency scanning direction */
#define BTA_FM_SCAN_DOWN        0x00        /* bit7 = 0 scanning toward lower frequency */
#define BTA_FM_SCAN_UP          0x80        /* bit7 = 1 scanning toward higher frequency */
typedef UINT8 tBTA_FM_SCAN_DIR;

#define BTA_FM_SCAN_FULL        (BTA_FM_SCAN_UP | BTA_FM_NORMAL_SCAN|0x02)       /* full band scan */
#define BTA_FM_FAST_SCAN        (BTA_FM_SCAN_UP | BTA_FM_PRESET_SCAN)       /* use preset scan */  
#define BTA_FM_SCAN_NONE        0xff

typedef UINT8 tBTA_FM_SCAN_MODE;

#define  BTA_FM_STEP_100KHZ     0x00
#define  BTA_FM_STEP_50KHZ      0x10
typedef UINT8   tBTA_FM_STEP_TYPE;

typedef UINT8 tBTA_FM_RDS_B;

typedef UINT8 tBTA_FM_AF_TYPE;

typedef UINT8 tBTA_FM_NFE_LEVL;



