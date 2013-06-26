/*
* hardware/sprd/hsdroid/libcamera/sprdcamerahardwareinterface.h
 * Dcam HAL based on sc8800g2
 *
 * Copyright (C) 2011 Spreadtrum 
 * 
 * Author: Xiaozhe wang <xiaozhe.wang@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H

//#include <camera/CameraHardwareInterface.h>
//#include <camera/CameraParameters.h>
//#include <binder/MemoryBase.h>
//#include <binder/MemoryHeapBase.h>
#include <binder/MemoryHeapIon.h>
#include <utils/threads.h>
extern "C" {
    #include <linux/android_pmem.h>
}
#include <sys/types.h>

#include "SprdOEMCamera.h"
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/CameraParameters.h>


namespace android {

typedef void (*preview_callback)(sp<MemoryBase>, void * );
typedef void (*recording_callback)(sp<MemoryBase>, void *);
typedef void (*shutter_callback)(void *);
typedef void (*raw_callback)(sp<MemoryBase> , void *);
typedef void (*jpeg_callback)(sp<MemoryBase>, void *);
typedef void (*autofocus_callback)(bool, void *);

typedef struct sprd_camera_memory {
	camera_memory_t *camera_memory;
	MemoryHeapIon *ion_heap;
	uint32_t phys_addr, phys_size;
	void *handle;
	void *data;
}sprd_camera_memory_t;

typedef enum
{
	CAMERA_FLUSH_RAW_HEAP,
	CAMERA_FLUSH_RAW_HEAP_ALL,
	CAMERA_FLUSH_MAX
}CameraFlushMemTypeEnum;

class SprdCameraHardware : public virtual RefBase {
public:
    //virtual sp<IMemoryHeap> getPreviewHeap() const;
    //virtual sp<IMemoryHeap> getRawHeap() const;

    //virtual status_t    dump(int fd, const Vector<String16>& args) const;
    //virtual status_t    startPreview(preview_callback cb, void* user);
    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();
    //virtual status_t    startRecording(recording_callback cb, void* user);
    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled();
    //virtual void        releaseRecordingFrame(const sp<IMemory>& mem);
    //virtual status_t    autoFocus(autofocus_callback, void *user);
    virtual status_t    autoFocus();    
    /*virtual status_t    takePicture(shutter_callback,
                                    raw_callback,
                                    jpeg_callback,
                                    void* user);*/
   virtual status_t    takePicture();                                  
    /*virtual status_t    cancelPicture(bool cancel_shutter,
                                      bool cancel_raw, bool cancel_jpeg);*/
    virtual status_t    cancelPicture();                                      
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;

    virtual void release();

    //static sp<SprdCameraHardware> createInstance();   
    //static sp<SprdCameraHardware> getInstance();

	void* get_preview_mem(uint32_t size, uint32_t *phy_addr, uint32_t index);
	void* get_raw_mem(uint32_t size, uint32_t *phy_addr, uint32_t index,uint32_t *return_size);
	void* get_jpeg_encoder_mem_by_HW(uint32_t *phy_addr);
	void* get_temp_mem_by_HW(uint32_t size, uint32_t num, uint32_t *phy_addr);
	void* get_temp_mem_by_jpegslice(uint32_t size, uint32_t num, uint32_t *phy_addr);
	void* get_fd_mem(uint32_t size, uint32_t *phy_addr);
	void free_preview_mem(uint32_t *phy_addr, uint32_t size, uint32_t index);
	void free_raw_mem(uint32_t *phy_addr, uint32_t size, uint32_t index);
	void free_jpeg_encoder_mem_by_HW(void);
	void free_temp_mem_by_HW(void);
	void free_temp_mem_by_jpegslice(void);
	void FreeCameraMem(void);
	void free_fd_mem(void);
     //virtual void setCallbacks(notify_callback notify_cb,data_callback data_cb,data_callback_timestamp data_cb_timestamp, void* user);
    virtual void        setCallbacks(camera_notify_callback notify_cb,
                                     camera_data_callback data_cb,
                                     camera_data_timestamp_callback data_cb_timestamp,
                                     camera_request_memory get_memory,
                                     void *user);
	//for compiler	
   // virtual void setCallbacks(notify_callback notify_cb,data_callback data_cb,data_callback_timestamp data_cb_timestamp, void* user){}
    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);	
   // virtual status_t    startPreview(){return 0;}
   // virtual status_t    startRecording(){return 0;}
   // virtual status_t    autoFocus() {return 0;}
    virtual status_t    cancelAutoFocus();//{return 0;}
    //virtual status_t    takePicture(){return 0;}
    //virtual status_t    cancelPicture(){return 0;}
    virtual status_t sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);// {return 0;}
     void getPreviewFormat(int *format);
     void getPictureFormat(int *format);

    //
    virtual void        releaseRecordingFrame(const void *opaque);
    virtual status_t    dump(int fd) const;
    virtual status_t    setPreviewWindow(preview_stream_ops *w);
    virtual status_t    storeMetaDataInBuffers(bool enable);
    SprdCameraHardware(int cameraId);
    virtual             ~SprdCameraHardware();	
    inline  int         getCameraId() const;

	int flush_buffer(CameraFlushMemTypeEnum  type ,  void *v_addr, void *p_addr, int size);

private:

	sprd_camera_memory_t* GetCachePmem(const char *device_name, int buf_size, int num_bufs);

	sprd_camera_memory_t* GetPmem(const char *device_name, int buf_size, int num_bufs);
	void FreePmem(sprd_camera_memory_t* camera_memory);
	void* get_redisplay_mem(uint32_t size, uint32_t count, uint32_t *phy_addr);
       
    //SprdCameraHardware();
    //virtual ~SprdCameraHardware();
/*    status_t startPreviewInternal(preview_callback pcb, void *puser,
                                  recording_callback rcb, void *ruser);*/
    status_t startPreviewInternal();                              
    void stopPreviewInternal();
    static wp<SprdCameraHardware> singleton;

    /* These constants reflect the number of buffers that libqcamera requires
       for preview and raw, and need to be updated when libqcamera
       changes.
    */
    static const int kPreviewBufferCount = 8;
    static const int kRawBufferCount = 1;
    static const int kJpegBufferCount = 1;
    static const int kRawFrameHeaderSize = 0x0;

    //TODO: put the picture dimensions in the CameraParameters object;
    CameraParameters mParameters;
    int mPreviewHeight;
    int mPreviewWidth;
    int mRawHeight;
    int mRawWidth;
    int mPreviewFormat;//0:YUV422;1:YUV420;2:RGB
    int mPictureFormat;//0:YUV422;1:YUV420;2:RGB;3:JPEG  
    int interpoation_flag;
    int zoom_flag;
    int zoom_level;
    int mOrientation_parm;

    void receivePreviewFrame(camera_frame_type *frame);
    void receivePreviewFDFrame(camera_frame_type *frame);
    void HandleErrorState(void);

    static void stop_camera_cb(camera_cb_type cb,
            const void *client_data,
            camera_func_type func,
            int32_t parm4);

    static void camera_cb(camera_cb_type cb,
            const void *client_data,
            camera_func_type func,
            int32_t parm4);

    // This class represents a heap which maintains several contiguous
    // buffers.  The heap may be backed by pmem (when pmem_pool contains
    // the name of a /dev/pmem* file), or by ashmem (when pmem_pool == NULL).

    struct MemPool : public RefBase {
        MemPool(int buffer_size, int num_buffers,
                int frame_size,
                int frame_offset,
                const char *name);

        virtual ~MemPool() = 0;

        void completeInitialization();
        bool initialized() const { 
            //return mHeap != NULL && mHeap->base() != MAP_FAILED;
            if(mHeap != NULL)
	    {
		if(MAP_FAILED != mHeap->base())
			return true;
	    	else
			return false;	
            }
	    else
		return false;
        }

        virtual status_t dump(int fd, const Vector<String16>& args) const;

        int mBufferSize;
        int mNumBuffers;
        int mFrameSize;
        int mFrameOffset;
        sp<MemoryHeapBase> mHeap;
        sp<MemoryBase> *mBuffers;

        const char *mName;
    };

    struct AshmemPool : public MemPool {
        AshmemPool(int buffer_size, int num_buffers,
                   int frame_size,
                   int frame_offset,
                   const char *name);
    };

    struct PmemPool : public MemPool {
        PmemPool(const char *pmem_pool,
                int buffer_size, int num_buffers,
                int frame_size,
                int frame_offset,
                const char *name);
        virtual ~PmemPool() { }
        int mFd;
        uint32_t mAlignedSize;
        struct pmem_region mSize;
    };

    struct PreviewPmemPool : public PmemPool {
        virtual ~PreviewPmemPool();
        PreviewPmemPool(int buffer_size, int num_buffers,
                        int frame_size,
                        int frame_offset,
                        const char *name);
    };

    struct RawPmemPool : public PmemPool {
        virtual ~RawPmemPool();
        RawPmemPool(const char *pmem_pool,
                    int buffer_size, int num_buffers,
                    int frame_size,
                    int frame_offset,
                    const char *name);
    };
  /*
    sp<PreviewPmemPool> mPreviewHeap;	
    sp<RawPmemPool> mRawHeap;
    sp<RawPmemPool> mJpegencHWHeap;	
    sp<RawPmemPool> mTempHWHeap;	
    sp<AshmemPool> mJpegHeap;
sp<RawPmemPool> mJpegencZoomHeap; //for capture zoom.	
sp<RawPmemPool> mJpegencSwapHeap; //for capture zoom scale.
*/

    sprd_camera_memory_t *mPreviewHeap;
    sprd_camera_memory_t *mRawHeap;
    sprd_camera_memory_t *mJpegencHWHeap;
    sprd_camera_memory_t *mTempHWHeap;
    sprd_camera_memory_t *mTempJpegSliceHeap;
	sprd_camera_memory_t *mFDHeap;
    sp<AshmemPool> mJpegHeap;
    sprd_camera_memory_t *mJpegencZoomHeap; //for capture zoom.
    sprd_camera_memory_t *mJpegencSwapHeap; //for capture zoom scale.
    sprd_camera_memory_t *mReDisplayHeap;
    camera_memory_t *mMetadataHeap;

    bool startCameraIfNecessary();
    bool initPreview();
    void deinitPreview();
    bool initRaw(bool initJpegHeap);

    status_t initDefaultParameters();
    status_t initCameraParameters();
    void setCameraDimensions();

    // The states described by Sprd_camera_state are very similar to the
    // CAMERA_FUNC_xxx notifications reported by libqcamera.  The differences
    // are that they reflect not only the response from libqcamera, but also
    // the requests made by the clients of this object.  For example,
    // QCS_PREVIEW_REQUESTED is a state that we enter when we call
    // SprdCameraHardware::startPreview(), and stay in until libqcamera
    // confirms that it has received the start-preview command (but not
    // actually initiated preview yet).
    //
    // NOTE: keep those values small; they are used internally as indices
    //       into a array of strings.
    // NOTE: if you add to this enumeration, make sure you update
    //       getCameraStateStr().

    enum Sprd_camera_state {
        QCS_INIT,
        QCS_IDLE,
        QCS_ERROR,
        QCS_PREVIEW_IN_PROGRESS,
        QCS_WAITING_RAW,
        QCS_WAITING_JPEG,
        // internal states 
        QCS_INTERNAL_PREVIEW_STOPPING,
        QCS_INTERNAL_CAPTURE_STOPPING,
        QCS_INTERNAL_PREVIEW_REQUESTED,
        QCS_INTERNAL_RAW_REQUESTED,
        QCS_INTERNAL_STOPPING,
    };

    volatile Sprd_camera_state mCameraState;
    const char* getCameraStateStr(Sprd_camera_state s);
    //static const char* getCameraStateStr(Sprd_camera_state s);
    Sprd_camera_state change_state(Sprd_camera_state new_state, 
                                       bool lock = true);

    void notifyShutter();
    void receiveJpegPictureFragment(JPEGENC_CBrtnType *encInfo);
    void receiveJpegPosPicture(void);
    void receivePostLpmRawPicture(camera_frame_type *frame);
    void receiveRawPicture(camera_frame_type *frame);
    void receiveJpegPicture(void);
     bool allocZoomBufferForCap(void);
   bool  allocSwapBufferForCap(uint32_t swap_size);

    Mutex mLock; // API lock -- all public methods
    Mutex mCallbackLock;
    Mutex mStateLock;
    Condition mStateWait;

    /* mJpegSize keeps track of the size of the accumulated JPEG.  We clear it
       when we are about to take a picture, so at any time it contains either
       zero, or the size of the last JPEG picture taken.
    */
    uint32_t mJpegSize;
    camera_handle_type camera_handle;
    camera_encode_properties_type encode_properties;
    camera_position_type pt;

/*    shutter_callback    mShutterCallback;
    raw_callback        mRawPictureCallback;
    jpeg_callback       mJpegPictureCallback;
    void                *mPictureCallbackCookie;

    autofocus_callback  mAutoFocusCallback;
    void                *mAutoFocusCallbackCookie;

    preview_callback    mPreviewCallback;
    void                *mPreviewCallbackCookie;
    recording_callback  mRecordingCallback;
    void                *mRecordingCallbackCookie;
    bool setCallbackFuns(preview_callback pcb, void *pu,
                      recording_callback rcb, void *ru);*/

    int                 mPreviewFrameSize;
    int                 mRawSize;
    int                 mJpegMaxSize;

    // hack to prevent black frame on first preview
    int                 mPreviewCount;
    int                 mRecordingMode;

   camera_notify_callback mNotify_cb;
   camera_data_callback mData_cb;
   camera_data_timestamp_callback mData_cb_timestamp;
   camera_request_memory      mGetMemory_cb;
   void *mUser;

   preview_stream_ops *mPreviewWindow;
   mutable Mutex       mPreviewLock;
   
   static gralloc_module_t const* mGrallocHal;

   int32_t             mMsgEnabled;
   //sp<CameraHardwareInterface> mHardware;
   bool mIsStoreMetaData;
   enum PreviewWindowState{
        PREVIEW_WINDOW_SET_IDLE,
        PREVIEW_WINDOW_SETTIN,
        PREVIEW_WINDOW_SET_OK,
        PREVIEW_WINDOW_SET_FAILED
    };
    PreviewWindowState mSettingPreviewWindowState;
};

}; // namespace android

#endif //ANDROID_HARDWARE_SPRD_CAMERA_HARDWARE_H


