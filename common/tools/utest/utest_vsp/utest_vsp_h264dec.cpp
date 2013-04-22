#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#include <binder/MemoryHeapIon.h>
using namespace android;


#include "h264dec.h"


#include "util.h"



#define ERR(x...)	fprintf(stderr, ##x)
#define INFO(x...)	fprintf(stdout, ##x)




#define ONEFRAME_BITSTREAM_BFR_SIZE	(1500*1024)  //for bitstream size of one encoded frame.
#define DEC_YUV_BUFFER_NUM   17


#define	STARTCODE_H263_PSC	0x00008000
#define STARTCODE_MP4V_VO	0x000001b0
#define STARTCODE_MP4V_VOP	0x000001b6
#define	STARTCODE_MJPG_SOI	0xffd80000
#define	STARTCODE_FLV1_PSC	0x00008400
#define STARTCODE_H264_NAL1	0x00000100
#define STARTCODE_H264_NAL2	0x00000101


static const unsigned int table_startcode1[] = 
{
	STARTCODE_H263_PSC, 
	STARTCODE_MP4V_VO, 
	STARTCODE_MJPG_SOI, 
	STARTCODE_FLV1_PSC, 
	STARTCODE_H264_NAL1
};

static const unsigned int table_maskcode1[] = 
{
		0x000073ff, 
		0x00000005, 
		0x0000ffff, 
		0x000073ff, 
		0x0000007f
};

static const unsigned int table_startcode2[] = 
{
	STARTCODE_H263_PSC, 
	STARTCODE_MP4V_VOP, 
	STARTCODE_MJPG_SOI, 
	STARTCODE_FLV1_PSC, 
	STARTCODE_H264_NAL2
};

static const unsigned int table_maskcode2[] = 
{
		0x000073ff, 
		0x00000005, 
		0x0000ffff, 
		0x000073ff, 
		0x00000074
};





static void usage()
{
	INFO("usage:\n");
	INFO("utest_vsp_dec -i filename_bitstream -o filename_yuv [OPTIONS]\n");
	INFO("-i       string: input bitstream filename\n");
	INFO("[OPTIONS]:\n");
        INFO("-o       string: output yuv filename\n");
	INFO("-format integer: video format(0:H263 / 1:MPEG4 / 2:MJPG / 3:FLVH263 / 4:H264), auto detection if default\n");
	INFO("-frames integer: number of frames to decode, default is 0 means all frames\n");
	INFO("-help          : show this help message\n");
	INFO("Built on %s %s, Written by JamesXiong(james.xiong@spreadtrum.com)\n", __DATE__, __TIME__);
}


static int dec_init(int format, unsigned char* pheader_buffer, unsigned int header_size, unsigned char* pbuf_inter, unsigned int size_inter, MMCodecBuffer extra_mem[])
{
	MMCodecBuffer codec_buf;
	MMDecVideoFormat video_format;
	int ret;

	codec_buf.int_buffer_ptr = pbuf_inter;
    	codec_buf.int_size = size_inter;

	video_format.video_std = format;
	video_format.i_extra = header_size;
	video_format.p_extra = pheader_buffer;
	video_format.frame_width = 0;
    	video_format.frame_height = 0;

	ret = H264DecInit(&codec_buf,&video_format);
	if (ret == 0)
	{
		H264DecMemInit(extra_mem);
	}

	return ret;
}

static int dec_decode_frame(unsigned char* pframe, unsigned int size, int* frame_effective, unsigned int* width, unsigned int* height, int* type)
{
	MMDecInput dec_in;
    	MMDecOutput dec_out;
	int ret;

	dec_in.pStream = pframe;
    	dec_in.dataLen = size;
    	dec_in.beLastFrm = 0;
    	dec_in.expected_IVOP = 0;
    	dec_in.beDisplayed = 1;
    	dec_in.err_pkt_num = 0;

    	dec_out.frameEffective = 0;

	ret = H264DecDecode(&dec_in, &dec_out);
	if (ret == 0)
	{
		*width = dec_out.frame_width;
		*height = dec_out.frame_height;
		*frame_effective = dec_out.frameEffective;
	}

	return ret;
}


static void dec_release()
{
	H264Dec_ReleaseRefBuffers();

	H264DecRelease();
}



static int sniff_format(unsigned char* pbuffer, unsigned int size, unsigned int startcode, unsigned int maskcode)
{
	int confidence = 0;

	unsigned int code = (pbuffer[0] << 24) | (pbuffer[1] << 16) | (pbuffer[2] << 8) | pbuffer[3];
	if ((code & (~maskcode)) == (startcode & (~maskcode)))
	{
		confidence = size;
	}

	for (unsigned int i=1; i<=size-4; i++)
	{
		unsigned int code = (pbuffer[i] << 24) | (pbuffer[i+1] << 16) | (pbuffer[i+2] << 8) | pbuffer[i+3];
		if ((code & (~maskcode)) == (startcode & (~maskcode)))
		{
			confidence ++;
		}
	}

	return confidence;
}

static int detect_format(unsigned char* pbuffer, unsigned int size)
{
	int format = 0;

	int confidence[5];
	int i;
	for (i=0; i<5; i++)
	{
		confidence[i] = sniff_format(pbuffer, size, table_startcode1[i], table_maskcode1[i]);
	}

	int max_confidence = 0;
	for (i=0; i<5; i++)
	{
		if (max_confidence < confidence[i])
		{
			max_confidence = confidence[i];
			format = i;
		}
	}

	return format;
}





static const char* format2str(int format)
{
	if (format == 0)
	{
		return "H263";
	}
	else if (format == 1)
	{
		return "MPEG4";
	}
	else if (format == 2)
	{
		return "MJPG";
	}
	else if (format == 3)
	{
		return "FLVH263";
	}
	else if (format == 4)
	{
		return "H264";
	}
	else
	{
		return "UnKnown";
	}
}

static const char* type2str(int type)
{
	if (type == 2)
	{
		return "I";
	}
	else if (type == 0)
	{
		return "P";
	}
	else if (type == 1)
	{
		return "B";
	}
	else
	{
		return "N";
	}
}


sp<MemoryHeapIon> pmem_extra = NULL;
sp<MemoryHeapIon> pmem_extra_cachable = NULL;

static int FlushCache(void* aUserData,int* vaddr,int* paddr,int size)
{
    return pmem_extra_cachable->flush_ion_buffer((void *)vaddr,(void* )paddr,size);
}


static int ActivateSPS(void* aUserData, uint width,uint height, uint aNumBuffers)
{
	return 1;
}

int main(int argc, char **argv)
{
	char* filename_bs = NULL;
	FILE* fp_bs = NULL;
	char* filename_yuv = NULL;
	FILE* fp_yuv = NULL;
	int format = -1;
	unsigned int frames = 0;
	unsigned int width = 1280;
	unsigned int height = 720;

	unsigned int startcode = 0;
	unsigned int maskcode = 0;
	int i;


	
	// bitstream buffer, read from bs file
	unsigned char buffer_data[ONEFRAME_BITSTREAM_BFR_SIZE];
	int buffer_size = 0;
	sp<MemoryHeapIon> pmem_bs = NULL;
	unsigned char* pframe_buffer = NULL;
	unsigned char* pframe_buffer_phy = NULL;

	// yuv420sp buffer, decode from bs buffer
	sp<MemoryHeapIon> pmem_yuv420sp = NULL;
	unsigned char* pyuv[DEC_YUV_BUFFER_NUM] = {NULL};
	unsigned char* pyuv_phy[DEC_YUV_BUFFER_NUM] = {NULL};

	// yuv420p buffer, transform from yuv420sp and write to yuv file
	unsigned char* py = NULL;
	unsigned char* pu = NULL;
	unsigned char* pv = NULL;
    
	unsigned int framenum_bs = 0;
	unsigned int framenum_err = 0;
	unsigned int framenum_yuv = 0;
	unsigned int time_total_ms = 0;


	// VSP buffer
	unsigned char* pbuf_inter = NULL;
	unsigned int size_inter = 0;
	
        MMCodecBuffer extra_mem[MAX_MEM_TYPE];
        memset(extra_mem, 0, sizeof(extra_mem));
	int phy_addr = 0;
	int size = 0;



	/* parse argument */

	if (argc < 3) 
	{
		usage();
		return -1;
	}

	for (i=1; i<argc; i++)
	{
		if (strcmp(argv[i], "-i") == 0 && (i < argc-1))
		{
			filename_bs = argv[++i];
		}
		else if (strcmp(argv[i], "-o") == 0 && (i < argc-1))
		{
			filename_yuv = argv[++i];
		}
		else if (strcmp(argv[i], "-format") == 0 && (i < argc-1))
		{
			format = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-frames") == 0 && (i < argc-1))
		{
			frames = atoi(argv[++i]);
		}
		else if (strcmp(argv[i], "-help") == 0)
		{
			usage();
			return 0;
		}
		else
		{
			usage();
			return -1;
		}
	}


	/* check argument */
	if (filename_bs == NULL)
	{
		usage();
		return -1;
	}
	


	fp_bs = fopen(filename_bs, "rb");
	if (fp_bs == NULL)
	{
		ERR("Failed to open file %s\n", filename_bs);
		goto err;
	}
    if (filename_yuv != NULL)
    {
        fp_yuv = fopen(filename_yuv, "wb");
        if (fp_yuv == NULL)
        {
            ERR("Failed to open file %s\n", filename_yuv);
            goto err;
        }
    }

	
	if ((format < 0) || (format > 4))
	{
		// detect bistream format
		unsigned char header_data[64];
		int header_size = fread(header_data, 1, 64, fp_bs);
		if (header_size <= 0)
		{
			ERR("Failed to read file %s\n", filename_bs);
			goto err;
		}
		rewind(fp_bs);
		format = detect_format(header_data, header_size);
	}



	
	/* bs buffer */
	pmem_bs = new MemoryHeapIon("/dev/ion", ONEFRAME_BITSTREAM_BFR_SIZE, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
        if (pmem_bs->getHeapID() < 0)
        {
                  ERR("Failed to alloc bitstream pmem buffer\n");
		goto err;
        }
	pmem_bs->get_phy_addr_from_ion(&phy_addr, &size);
	pframe_buffer = (unsigned char*)pmem_bs->base();
	pframe_buffer_phy = (unsigned char*)phy_addr;
	if (pframe_buffer == NULL)
	{
		ERR("Failed to alloc bitstream pmem buffer\n");
		goto err;
	}

	/* yuv420sp buffer */
	pmem_yuv420sp = new MemoryHeapIon("/dev/ion", width*height*3/2 * DEC_YUV_BUFFER_NUM, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
        if (pmem_yuv420sp->getHeapID() < 0)
        {
                ERR("Failed to alloc yuv pmem buffer\n");
		goto err;
        }
	pmem_yuv420sp->get_phy_addr_from_ion(&phy_addr, &size);
	for (i=0; i<DEC_YUV_BUFFER_NUM; i++)
	{
		pyuv[i] = ((unsigned char*)pmem_yuv420sp->base()) + width*height*3/2 * i;
		pyuv_phy[i] = ((unsigned char*)phy_addr) + width*height*3/2 * i;
	}
	if (pyuv[0] == NULL)
	{
		ERR("Failed to alloc yuv pmem buffer\n");
		goto err;
	}

	/* yuv420p buffer */
	py = (unsigned char*)vsp_malloc(width * height * sizeof(unsigned char), 4);
	if (py == NULL)
	{
		ERR("Failed to alloc yuv buffer\n");
		goto err;
	}
	pu = (unsigned char*)vsp_malloc(width/2 * height/2 * sizeof(unsigned char), 4);
	if (pu == NULL)
	{
		ERR("Failed to alloc yuv buffer\n");
		goto err;
	}
	pv = (unsigned char*)vsp_malloc(width/2 * height/2 * sizeof(unsigned char), 4);
	if (pv == NULL)
	{
		ERR("Failed to alloc yuv buffer\n");
		goto err;
	}




	INFO("Try to decode %s to %s, format = %s\n", filename_bs, filename_yuv, format2str(format));

	



	/* step 1 - init vsp */
	size_inter = 500 * 1024;
	pbuf_inter = (unsigned char*)vsp_malloc(size_inter * sizeof(unsigned char), 4);
	if (pbuf_inter == NULL)
	{
		ERR("Failed to alloc inter memory\n");
		goto err;
	}
        
	extra_mem[SW_CACHABLE].size= 4500 * 1024;
	extra_mem[SW_CACHABLE].common_buffer_ptr = (unsigned char*)vsp_malloc(extra_mem[SW_CACHABLE].size * sizeof(unsigned char), 4);
	if (extra_mem[SW_CACHABLE].common_buffer_ptr == NULL)
	{
		ERR("Failed to alloc cache memory\n");
		goto err;
	}
    
	extra_mem[HW_NO_CACHABLE].size = 8000 * 1024;
	pmem_extra = new MemoryHeapIon("/dev/ion", extra_mem[HW_NO_CACHABLE].size, MemoryHeapBase::NO_CACHING, ION_HEAP_CARVEOUT_MASK);
        if (pmem_extra->getHeapID() < 0)
        {
                    ERR("Failed to alloc extra memory\n");
		goto err;
        }
	pmem_extra->get_phy_addr_from_ion(&phy_addr, &size);
	extra_mem[HW_NO_CACHABLE].common_buffer_ptr= (unsigned char*)pmem_extra->base();
	extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy= (unsigned char*)phy_addr;
	if (extra_mem[HW_NO_CACHABLE].common_buffer_ptr == NULL)
	{
		ERR("Failed to alloc extra memory\n");
		goto err;
	}

        extra_mem[HW_CACHABLE].size = 8000 * 1024;
	pmem_extra_cachable = new MemoryHeapIon("/dev/ion", extra_mem[HW_CACHABLE].size, 0, (1<<31) |ION_HEAP_CARVEOUT_MASK);
        if (pmem_extra_cachable->getHeapID() < 0)
            {
                ERR("Failed to alloc extra memory\n");
		goto err;
            }
	pmem_extra_cachable->get_phy_addr_from_ion(&phy_addr, &size);
	extra_mem[HW_CACHABLE].common_buffer_ptr= (unsigned char*)pmem_extra_cachable->base();
	extra_mem[HW_CACHABLE].common_buffer_ptr_phy= (unsigned char*)phy_addr;
	if (extra_mem[HW_CACHABLE].common_buffer_ptr == NULL)
	{
		ERR("Failed to alloc extra memory\n");
		goto err;
	}
    
	H264Dec_RegSPSCB(ActivateSPS);
	H264Dec_RegFlushCacheCB(FlushCache);
	if (dec_init(format, NULL, 0, pbuf_inter, size_inter, extra_mem) != 0)
	{
		ERR("Failed to init VSP\n");
		goto err;
	}



	/* step 2 - decode with vsp */
	startcode = table_startcode2[format];
	maskcode = table_maskcode2[format];
	while (!feof(fp_bs))
	{
		int read_size = fread(buffer_data+buffer_size, 1, ONEFRAME_BITSTREAM_BFR_SIZE-buffer_size, fp_bs);
		if (read_size <= 0)
		{
			break;
		}
		buffer_size += read_size;


		unsigned char* ptmp = buffer_data;
		unsigned int frame_size = 0;
		while (buffer_size > 0)
		{
			// search a frame
			frame_size = find_frame(ptmp, buffer_size, startcode, maskcode);
			if (frame_size == 0)
			{
				if ((ptmp == buffer_data) || feof(fp_bs))
				{
					frame_size = buffer_size;
				}
				else
				{
					break;
				}
			}


			// read a bitstream frame
			memcpy(pframe_buffer, ptmp, frame_size);

			ptmp += frame_size;
			buffer_size -= frame_size;


			// decode bitstream to yuv420sp
			int frame_effective = 0;
			int type = 0;
			unsigned int width_new = 0;
			unsigned int height_new = 0;
			unsigned char* pyuv420sp = pyuv[framenum_bs % DEC_YUV_BUFFER_NUM];
			unsigned char* pyuv420sp_phy = pyuv_phy[framenum_bs % DEC_YUV_BUFFER_NUM];
			H264Dec_SetCurRecPic(pyuv420sp, pyuv420sp_phy, NULL);
			framenum_bs ++;
			int64_t start = systemTime();
			int ret = dec_decode_frame(pframe_buffer, frame_size, &frame_effective, &width_new, &height_new, &type);
			int64_t end = systemTime();
			unsigned int duration = (unsigned int)((end-start) / 1000000L);
			time_total_ms += duration;

            if (duration < 40)
                usleep((40 - duration)*1000);
            
			if (ret != 0)
			{
				framenum_err ++;
				ERR("frame %d: time = %dms, size = %d, type = %s, failed(%d)\n", framenum_bs, duration, frame_size, type2str(type), ret);
				continue;
			}



			if ((width_new != width) || (height_new != height))
			{
				width = width_new;
				height = height_new;
			}
			INFO("frame %d[%dx%d]: time = %dms, size = %d, type = %s, effective(%d)\n", framenum_bs, width, height, duration, frame_size, type2str(type), frame_effective);
			

			if ((frame_effective) && (fp_yuv != NULL))
			{
				// yuv420sp to yuv420p
				yuv420sp_to_yuv420p(pyuv420sp, pyuv420sp+width*height, py, pu, pv, width, height);

				// write yuv420p
				if (write_yuv_frame(py, pu, pv, width, height, fp_yuv)!= 0)
				{
					break;
				}

				framenum_yuv ++;
			}

			if (frames != 0)
			{
				if (framenum_yuv >= frames)
				{
					break;
				}
			}
		}

		if (buffer_size > 0)
		{
			memmove(buffer_data, ptmp, buffer_size);
		}
	}


	/* step 3 - release vsp */
	dec_release();


	INFO("Finish decoding %s(%s, %d frames) to %s(%d frames)", filename_bs, format2str(format), framenum_bs, filename_yuv, framenum_yuv);
	if (framenum_err > 0)
	{
		INFO(", %d frames failed", framenum_err);
	}
	if (framenum_bs > 0)
	{
		INFO(", average time = %dms", time_total_ms/framenum_bs);
	}
	INFO("\n");



err:
	if (extra_mem[HW_NO_CACHABLE].common_buffer_ptr != NULL)
	{
    	        pmem_extra.clear();
		extra_mem[HW_NO_CACHABLE].common_buffer_ptr = NULL;
	}

        if (extra_mem[HW_CACHABLE].common_buffer_ptr != NULL)
	{
    	        pmem_extra_cachable.clear();
		extra_mem[HW_CACHABLE].common_buffer_ptr = NULL;
	}
    
	if (extra_mem[SW_CACHABLE].common_buffer_ptr != NULL)
	{
		vsp_free(extra_mem[SW_CACHABLE].common_buffer_ptr);
		extra_mem[SW_CACHABLE].common_buffer_ptr = NULL;
	}
	if (pbuf_inter != NULL)
	{
		vsp_free(pbuf_inter);
		pbuf_inter = NULL;
	}

	if (pframe_buffer != NULL)
	{
		pmem_bs.clear();
		pframe_buffer = NULL;
	}
	if (pyuv[0] != NULL)
	{
		pmem_yuv420sp.clear();
		pyuv[0] = NULL;
	}
	if (py != NULL)
	{
		vsp_free(py);
		py = NULL;
	}
	if (pu != NULL)
	{
		vsp_free(pu);
		pu = NULL;
	}
	if (pv != NULL)
	{
		vsp_free(pv);
		pv = NULL;
	}

	if (fp_yuv != NULL)
	{
		fclose(fp_yuv);
		fp_yuv = NULL;
	}
	if (fp_bs != NULL)
	{
		fclose(fp_bs);
		fp_bs = NULL;
	}

	return 0;
}

