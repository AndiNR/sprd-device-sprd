/*
 * Copyright (C) 2009 The Android Open Source Project
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

#define LOG_TAG "AudioPolicyManagerSPRD"
//#define LOG_NDEBUG 0
#include <utils/Log.h>
#include "AudioPolicyManagerSPRD.h"
#include <media/mediarecorder.h>

namespace android_audio_legacy {

using namespace android;

int AudioPolicyManagerSPRD::CurveEditor::getSize() const
{
    return sizeof(AudioPolicyManagerBase::VolumeCurvePoint) * AudioPolicyManagerBase::VOLCNT;
}

int AudioPolicyManagerSPRD::CurveEditor::readDataFromTree(XListNode *node, void *dataBuf)
{
    //ALOGE("dump pointer tree");
    //node->dump();
    VolumeCurvePoint *curve = (VolumeCurvePoint *)dataBuf;
    if (!node || node->type() != XLIST_NODE_LIST) {
        ALOGE("bad data");
        return 0;
    }
    XListValueList *curveValue = (XListValueList *)node->value();
    XListTree *curveTree = curveValue->list();
    int index = -1;
    for (int i = 0; i < VOLCNT; i++) {
        XListNode *point = curveTree->find("point", &index, index + 1);
        if (!point || point->type() != XLIST_NODE_LIST) {
            curveTree->dump();
            ALOGE("can't find point %d %p", i, point);
            return 0;
        }
        XListValueList *pointValue = (XListValueList *)point->value();
        XListTree *pointTree = pointValue->list();
        XListNode *volIndex = pointTree->find("index");
        if (!volIndex || volIndex->type() != XLIST_NODE_INTEGER) {
            pointTree->dump();
            ALOGE("can't find index in %d %p", i, volIndex);
            return 0;
        }
        XListNode *volDb = pointTree->find("db");
        if (!volDb || volDb->type() != XLIST_NODE_FLOAT) {
            pointTree->dump();
            ALOGE("can't find db in %d %p", i, volDb);
            return 0;
        }
        XListValueInt *indexValue = (XListValueInt *)volIndex->value();
        XListValueFloat *dbValue = (XListValueFloat *)volDb->value();
        curve->mIndex = indexValue->value();
        curve->mDBAttenuation = dbValue->value();
        curve++;
    }
    return 1;
}

int AudioPolicyManagerSPRD::CurveEditor::writeDataToTree(XListNode *node, void *dataBuf)
{
    ALOGE("dump pointer tree for write");
    node->dump();

    return 1;
}

// ----------------------------------------------------------------------------
// AudioPolicyManagerSPRD
// ----------------------------------------------------------------------------

// ---  class factory

extern "C" AudioPolicyInterface* createAudioPolicyManager(AudioPolicyClientInterface *clientInterface)
{
    return new AudioPolicyManagerSPRD(clientInterface);
}

extern "C" void destroyAudioPolicyManager(AudioPolicyInterface *interface)
{
    delete interface;
}

// Nothing currently different between the Base implementation.

AudioPolicyManagerSPRD::AudioPolicyManagerSPRD(AudioPolicyClientInterface *clientInterface)
    : AudioPolicyManagerBase(clientInterface)
{
    loadVolumeProfiles();
}

AudioPolicyManagerSPRD::~AudioPolicyManagerSPRD()
{
    freeVolumeProfiles();
}

int AudioPolicyManagerSPRD::loadVolumeProfiles()
{
    int rowCount;
    int columeCount;
    int ret = true;

    memset(mVolumeProfiles, 0, sizeof(mVolumeProfiles));

    // load device volume profiles
    CurveEditor curveEditor;
    MapDataFile editor(&curveEditor);

    // parse file
    ret = editor.open("/data/devicevolume.xml", "streams", "devices", "profiles", MapDataFile::open_ro);
    if (!ret)
        ret = editor.open("/system/etc/devicevolume.xml", "streams", "devices", "profiles", MapDataFile::open_ro);
    if (!ret)
        return 0;

    // check brief info
    if (ret) {
        rowCount = editor.getRowSize();
        columeCount = editor.getColumeSize();
        if (rowCount != AudioSystem::NUM_STREAM_TYPES || columeCount != DEVICE_CATEGORY_CNT) {
            ALOGW("corrupt profile data: %d rows should be %d, %d columes should be %d",
                rowCount, AudioSystem::NUM_STREAM_TYPES,
                columeCount, DEVICE_CATEGORY_CNT);
            ret = 0;
        }
    }

    // alloc mem, read all data
    if (ret) {
        for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
            for (int j = 0; j < DEVICE_CATEGORY_CNT; j++) {
                mVolumeProfiles[i][j] = new VolumeCurvePoint[VOLCNT];
                if (mVolumeProfiles[i][j]) {
                    ret = editor.readElement(i, j, mVolumeProfiles[i][j]);
                    if (!ret) {
                        ALOGE("wrong data @%d, %d", i, j);
                        goto read_failed;
                    }
                }
            }
        }
    }

    // using new profiles
    if (ret) {
        for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
            for (int j = 0; j < DEVICE_CATEGORY_CNT; j++) {
                mStreams[i].mVolumeCurve[j] = mVolumeProfiles[i][j];
            }
        }
    }

read_failed:
    editor.close();

    if (!ret) {
        // cleanup
        for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
            for (int j = 0; j < DEVICE_CATEGORY_CNT; j++) {
                if (mVolumeProfiles[i][j]) {
                    delete [] mVolumeProfiles[i][j];
                    mVolumeProfiles[i][j] = 0;
                }
            }
        }
        ALOGW("can't load device volume profiles, using default");
    }

    return ret;
}

void AudioPolicyManagerSPRD::freeVolumeProfiles()
{
    for (int i = 0; i < AudioSystem::NUM_STREAM_TYPES; i++) {
        for (int j = 0; j < DEVICE_CATEGORY_CNT; j++) {
            if (mVolumeProfiles[i][j]) {
                delete [] mVolumeProfiles[i][j];
                mVolumeProfiles[i][j] = 0;
            }
        }
    }
}

}; // namespace android_audio_legacy
