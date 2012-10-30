#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#include <utils/Log.h>

#include "log.h"
#include "package.h"

unsigned char fixnv_buf[FIXNV_BLOCK_SIZE*BLOCK_SIZE];
unsigned char runnv_buf[RUNNV_BLOCK_SIZE*BLOCK_SIZE];
int cursor_fixnv = 0;
int cursor_runnv = 0;

int fixnv_dirty = 0;
int runnv_dirty = 0;

extern unsigned char ret_data[DATA_BUF_LEN];

void parser_header(nvrequest_header_t *nv_header)
{
	
	if( NV_PACKAGE_HEADFLAG != nv_header->flag)
		NVITEM_LOG("[Nvitemd] Nvitem flag error\n");
	
	//check partition
	if((nv_header->attr & ATTR_PARTITION_MASK)>>ATTR_PARTITION_OFFSET == 0)
		;
	else if((nv_header->attr & ATTR_PARTITION_MASK)>>ATTR_PARTITION_OFFSET == 2)
		;
	else
		;

	//check write or read
	if((nv_header->attr & ATTR_RW_MASK)>>ATTR_RW_OFFSET == 0)
	{
		;	//writer
		if( nv_header->offset < 120*1024)	//fixnv
		{
			// save directly		
			//lseek(fixnv_fd,nv_header->offset,SEEK_SET);
			//write(fixnv_fd,data+sizeof(nvrequest_header_t),nv_header->size);
			
			//save to ram then wrire to file
			
			if(cursor_fixnv >= FIXNV_BLOCK_SIZE-1)
			{

				NVITEM_DEBUG("[Nvitemd] cursor_fixnv >= FIXNV_BLOCK_SIZE-1 !!!\n");
				memcpy(&fixnv_buf[cursor_fixnv*BLOCK_SIZE],data+sizeof(nvrequest_header_t),nv_header->size);
				cursor_fixnv = 0;
				fixnv_dirty = 1;
				//save to file and cpy to partition
			}			
			else
			{
				NVITEM_DEBUG("[Nvitemd] cursor_fixnv: %d!!!\n",cursor_fixnv);
				memcpy(&fixnv_buf[cursor_fixnv*BLOCK_SIZE],data+sizeof(nvrequest_header_t),nv_header->size);
				cursor_fixnv++;
			}
			
		}		
		else	//running nv
		{
			// save directly
			//lseek(runningnv_fd,(nv_header->offset-128*1024),SEEK_SET);
			//write(runningnv_fd,data+sizeof(nvrequest_header_t),nv_header->size);

			//save to ram then wrire to file
			
			if(cursor_runnv >= RUNNV_BLOCK_SIZE-1)
			{
				NVITEM_DEBUG("[Nvitemd] cursor_runnv >= RUNNV_BLOCK_SIZE-1 !!!\n");
				memcpy(&runnv_buf[cursor_runnv*BLOCK_SIZE],data+sizeof(nvrequest_header_t),nv_header->size);
				cursor_runnv = 0;
				runnv_dirty = 1;
			}			
			else
			{
				NVITEM_DEBUG("[Nvitemd] cursor_runnv :%d !!!\n",cursor_runnv);
				memcpy(&runnv_buf[cursor_runnv*BLOCK_SIZE],data+sizeof(nvrequest_header_t),nv_header->size);
				cursor_runnv++;
			}
			
		}
	}	
	else
		;	//read

	//check sync or non-sync transfer mode
	if((nv_header->attr & ATTR_SYNC_MASK)>>ATTR_SYNC_OFFSET == 0)
		;	//none sync
	else
		;	//sync transfer

	//check data or info
	if((nv_header->attr & ATTR_INFO_MASK)>>ATTR_INFO_OFFSET == 0)
		;	//nvdata
	else
		;	//some info


}

void gen_ret_header(nvrequest_header_t *nv_header,int ok,int *ret_size)
{
	//clear ret_date
	memset(ret_data,0,DATA_BUF_LEN);
	//check write or read
	if((nv_header->attr & ATTR_RW_MASK)>>ATTR_RW_OFFSET == 0)
	{
		;//writer nv ret info
		
		//make info 
		nv_header->attr = nv_header->attr | (unsigned int)0x1<<ATTR_INFO_OFFSET;

		if(ok = 1)	//success
		{
			memcpy(ret_data,nv_header,sizeof(nvrequest_header_t));
			ret_data[sizeof(nvrequest_header_t)] = 0x80;
			*ret_size = sizeof(nvrequest_header_t)+4;
		}
		else		//unsuccess
		{
			memcpy(ret_data,nv_header,sizeof(nvrequest_header_t));
			ret_data[sizeof(nvrequest_header_t)] = 0x81;
			*ret_size = sizeof(nvrequest_header_t)+4;
		}
	}
	else
	{
		;//read nv ret nvdata

		if(ok = 1)	//success
		{
			memcpy(ret_data,nv_header,sizeof(nvrequest_header_t));
			ret_size = sizeof(nvrequest_header_t)+nv_header->size;
		}
		else		//unsuccess
		{
			//make info 
			nv_header->attr = nv_header->attr | (unsigned int)0x1<<ATTR_INFO_OFFSET;
			memcpy(ret_data,nv_header,sizeof(nvrequest_header_t));
			ret_data[sizeof(nvrequest_header_t)] = 0x82;
			ret_size = sizeof(nvrequest_header_t)+4;
		}	
	}

}
