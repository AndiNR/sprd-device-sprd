/* Copyright 2010 - 2011 Broadcom Corporation
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
package com.broadcom.bt.service.fm;

import android.content.Context;


/**
 * Utility class for managing the configuration of the Services
 * 
 * This class is used by the Fm ServiceManagement architecture and should not be
 * exposed to Android application writers.
 * 
 * @hide
 */
public class FmServiceConfig {
    public static final boolean D = true;
    public static final boolean V = true;

    /**
     * List of services to manage
     * 
     * @hide
     */
    static final String[] SERVICES = { FmReceiver.SERVICE_NAME };

    /**
     * Flags to indicate which services will be available in the system
     * SERVICES_AVAILABLE[i] = true means SERVICES[i] is available
     * 
     * @hide
     */
    static final boolean[] SERVICES_AVAILABLE = { true };

    /**
     * Flags to indicate if the service (if available) should be enabled the
     * first time the system starts up SERVICES_STARTUP_ENABLED[i] = true means
     * that if SERVICES_AVAILABLE[i] is true, the SERVICES[i] will startup the
     * first time enabled.
     * 
     * @hide
     */
    static final boolean[] SERVICES_STARTUP_ENABLED = { true };


    /**
     * The prefix for the preferences settings key
     * 
     * @hide
     */
    public static final String SETTINGS_PREFIX = "fm_svcst_";

    /**
     * @hide
     */
    public static final int STATE_NOT_AVAILABLE = -1;
    /**
     * @hide
     */
    public static final int STATE_STOPPED = 1;
    /**
     * @hide
     */
    public static final int STATE_STARTED = 2;

    /**
     * Returns true if the service is supported
     * 
     * @param svcName
     * @return
     * @hide
     */

    public static final String FM_SVC_STATE_CHANGE_ACTION = "broadcom.bt.intent.action.FM_SVC_STATE_CHANGE";

    /**
     * Unique name of the Fm service
     */
    public static final String EXTRA_FM_SVC_NAME = "fm_svc_name";

    /**
     * Current state of the Fm service
     */

    public static final String EXTRA_FM_SVC_STATE = "fm_svc_state";

    /**
     * Indicates service is stopped
     */
    public static final int SVC_STATE_STOPPED = 1;

    /**
     * Indicates service is started
     */
    public static final int SVC_STATE_STARTED = 2;

    public static boolean isServiceSupported(String svcName) {
        for (int i = 0; i < SERVICES.length; i++) {
            if (SERVICES[i].equals(svcName)) {
                return SERVICES_AVAILABLE[i];
            }
        }
        return false;
    }

    /**
     * Returns true if the Fm service is supported
     */
    public static boolean isServiceEnabled(Context ctx, String svcName) {
        if (ctx != null && svcName != null && isServiceSupported(svcName)) {
            return true;
        }
        return false;
    }

}
