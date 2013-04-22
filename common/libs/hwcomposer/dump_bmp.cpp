#include "dump_bmp.h"

//namespace android {
int g_rand_set_Num = 0;
bool g_Reset_hwcset_DumpIndexFlag = false;
int g_rand_prepare_Num = 0;
bool g_Reset_hwcprepare_DumpIndexFlag = false;
int dump_bmp(const char* filename, void* buffer_addr, unsigned int buffer_format, unsigned int buffer_width, unsigned int buffer_height)
{
    FILE* fp;
    WORD bfType;
    BITMAPINFO bmInfo;
    RGBQUAD quad;
    int ret = 0;
    fp = fopen(filename, "wb");
    if(!fp) 
    {
        ret = -1;
        goto fail_open;
    }
    bfType = 0x4D42;

    memset(&bmInfo, 0, sizeof(BITMAPINFO));

    bmInfo.bmfHeader.bfOffBits = sizeof(WORD) + sizeof(BITMAPINFO);
    bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmInfo.bmiHeader.biWidth = buffer_width;
    bmInfo.bmiHeader.biHeight = -buffer_height;
    bmInfo.bmiHeader.biPlanes = 1;

    switch (buffer_format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x001F;
        quad.rgbGreenMask    = 0x07E0;
        quad.rgbBlueMask     = 0xF800;
        quad.rgbReservedMask = 0;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U16);
        break;

    case HAL_PIXEL_FORMAT_RGBA_8888:
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x00FF0000;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x000000FF;
        quad.rgbReservedMask = 0xFF000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case HAL_PIXEL_FORMAT_RGBX_8888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x00FF0000;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x000000FF;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case 	HAL_PIXEL_FORMAT_BGRA_8888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 32;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0xFF000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U32);
        break;
    case HAL_PIXEL_FORMAT_RGB_888:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 24;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 3;
        break;
    case HAL_PIXEL_FORMAT_RGBA_5551: /*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 2;
        break;
    case HAL_PIXEL_FORMAT_RGBA_4444:/*not sure need investigation*/
        bmInfo.bmfHeader.bfOffBits += 4*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 16;
        bmInfo.bmiHeader.biCompression = BI_BITFIELDS;
        quad.rgbRedMask      = 0x000000FF;
        quad.rgbGreenMask    = 0x0000FF00;
        quad.rgbBlueMask     = 0x00FF0000;
        quad.rgbReservedMask = 0x00000000;
        bmInfo.bmiHeader.biSizeImage = buffer_width * buffer_height * sizeof(U8) * 2;
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YV12:
        bmInfo.bmfHeader.bfOffBits += 256*sizeof(U32);
        bmInfo.bmiHeader.biBitCount = 8;
        bmInfo.bmiHeader.biCompression = BI_RGB;
        {
            for(int i=0; i<256; i++)
            {
                quad.table[i].rgbRed      = i;
                quad.table[i].rgbGreen    = i;
                quad.table[i].rgbBlue     = i;
                quad.table[i].rgbReserved = 0;
            }
        }
        bmInfo.bmiHeader.biSizeImage = (buffer_width * buffer_height * sizeof(U8) * 3)>>1;
        break;

    default:
        assert(false);
    }

    bmInfo.bmfHeader.bfSize = bmInfo.bmfHeader.bfOffBits + bmInfo.bmiHeader.biSizeImage;

    fwrite(&bfType, sizeof(WORD), 1, fp);
    fwrite(&bmInfo, sizeof(BITMAPINFO), 1, fp);
    switch (buffer_format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
    case HAL_PIXEL_FORMAT_RGBA_8888:
        fwrite(&quad, 4*sizeof(U32), 1, fp);
        break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
    case HAL_PIXEL_FORMAT_YV12:
        //fwrite(&quad, 256*sizeof(U32), 1, fp);
        break;
    }
    fwrite(buffer_addr, bmInfo.bmiHeader.biSizeImage, 1, fp);
    fclose(fp);
    return ret;
fail_open:
    ALOGE("dump layer failed to open path is:%s" , filename);
    return ret;
}

static int dump_layer(hwc_layer_t const* l , char* path ,int randNum ,  int index) {
    struct private_handle_t *private_h = (struct private_handle_t *)l->handle;
    char fileName[MAX_DUMP_PATH_LENGTH + MAX_DUMP_FILENAME_LENGTH];
    ALOGI("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%d,%d,%d,%d}, {%d,%d,%d,%d}",
            l->compositionType, l->flags, l->handle, l->transform, l->blending,
            l->sourceCrop.left,
            l->sourceCrop.top,
            l->sourceCrop.right,
            l->sourceCrop.bottom,
            l->displayFrame.left,
            l->displayFrame.top,
            l->displayFrame.right,
            l->displayFrame.bottom);
    if(private_h == NULL)
    {
        ALOGI("dump_layer private handle is null");
        return -1;
    }
    switch(private_h->format)
    {
    case HAL_PIXEL_FORMAT_RGBA_8888:
		sprintf(fileName , "%sLayer%x_%x_rgba_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case HAL_PIXEL_FORMAT_RGBX_8888:
		sprintf(fileName , "%sLayer%x_%x_rgbx_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case 	HAL_PIXEL_FORMAT_BGRA_8888:
		sprintf(fileName , "%sLayer%x_%x_bgra_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case HAL_PIXEL_FORMAT_RGB_888:
		sprintf(fileName , "%sLayer%x_%x_rgb888_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case HAL_PIXEL_FORMAT_RGBA_5551:
		sprintf(fileName , "%sLayer%x_%x_rgba5551_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case HAL_PIXEL_FORMAT_RGBA_4444:
		sprintf(fileName , "%sLayer%x_%x_rgba4444_%d.bmp" ,path, (unsigned int)l , randNum , index);
		break;
    case HAL_PIXEL_FORMAT_RGB_565:
		sprintf(fileName , "%sLayer%x_%x_rgb565_%d.bmp" ,path,(unsigned int)l, randNum , index);
		break;
    case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		sprintf(fileName , "%sLayer%x_%x_ybrsp_%dx%d_%d.yuv" ,path, (unsigned int)l , randNum , private_h->width , private_h->height , index);
		break;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		sprintf(fileName , "%sLayer%x_%x_yrbsp_%dx%d_%d.yuv" ,path, (unsigned int)l , randNum  , private_h->width , private_h->height , index);
		break;
    case HAL_PIXEL_FORMAT_YV12:
		sprintf(fileName , "%sLayer%x_%x_yv12_%dx%d_%d.yuv" , path, (unsigned int)l , randNum , private_h->width , private_h->height  , index);
		break;
    case HAL_PIXEL_FORMAT_YCbCr_420_P:
		sprintf(fileName , "%sLayer%x_%x_ybrp_%dx%d_%d.yuv" , path, (unsigned int)l , randNum , private_h->width , private_h->height  , index);
		break;
    default:
		ALOGE("dump layer failed because of error format %d" , private_h->format);
    		return -2;
    }
    	
    return dump_bmp(fileName , (void*)private_h->base, private_h->format,private_h->width,private_h->height);
}

int  dump_layers(hwc_layer_list_t *list , int flag)
{
    char value[PROPERTY_VALUE_MAX];
    char dumpPath[MAX_DUMP_PATH_LENGTH];
    static int64_t index_set  = 0;
    static int64_t index_prepare = 0;
    switch(flag)
    {
    case DUMP_AT_HWCOMPOSER_HWC_SET:
    property_get("dump.hwcomposer.set.path" , value , "0");
    if(strchr(value , '/') == NULL)
    {
	  return -1;
    }
    else
    {
        /*'/' must be the last character.*/
        sprintf(dumpPath, "%s" , value);
    }
    property_get("dump.hwcomposer.set.flag" , value , "0");
    if(g_Reset_hwcset_DumpIndexFlag == true)
    {
        index_set = 0;
    }
    if(atoi(value) == 1)
    {
        if(list != NULL)
        {
            ALOGD("dump layer number is:%d" , list->numHwLayers);
            for (size_t i=0 ; i<list->numHwLayers ; i++) {
                dump_layer(&list->hwLayers[i] , dumpPath , g_rand_set_Num , index_set);
            }
	      index_set ++;
        }
    }
    break;
    case DUMP_AT_HWCOMPOSER_HWC_PREPARE:
    property_get("dump.hwcomposer.prepare.path" , value , "0");
    if(strchr(value , '/') == NULL)
    {
	  return -1;
    }
    else
    {
        /*'/' must be the last character.*/
        sprintf(dumpPath, "%s" , value);
    }
    property_get("dump.hwcomposer.prepare.flag" , value , "0");
    if(g_Reset_hwcprepare_DumpIndexFlag == true)
    {
        index_prepare = 0;
    }
    if(atoi(value) == 1)
    {
        if(list != NULL)
        {
            ALOGD("dump layer number is:%d" , list->numHwLayers);
            for (size_t i=0 ; i<list->numHwLayers ; i++) {
                dump_layer(&list->hwLayers[i] , dumpPath , g_rand_prepare_Num ,  index_prepare);
            }
	      index_prepare ++;
        }
    }
    break;
    default:
	ALOGD("undefined flag to dump");
	return -1;
}
    return 0;
}

//}

