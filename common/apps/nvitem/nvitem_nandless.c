#define LOG_TAG 	"NVITEMD"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <utils/Log.h>

#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <mtd/mtd-user.h>
#include <time.h>
#include <math.h>
#include <hardware_legacy/power.h>

#include "log.h"
#include "package.h"

#define MUX0_NVITEMD_PATH	"/dev/ts0710mux0"
#define CMUX_NVITEMD_PATH	"/dev/ts0710mux20"

extern int fixnv_dirty;
extern int runnv_dirty;

int nvitem_receive_fd;
//int at_fd;
int fd0;

int open_port()
{	
  int fd;

  NVITEM_DEBUG("[Nvitemd] Opening MUX PATH...\n");
  fd = open( CMUX_NVITEMD_PATH, O_RDWR);
  
  if (-1 == fd){
    NVITEM_DEBUG("[Nvitemd] Open MUX PATH ERR !!!\n");
    NVITEM_LOG("[Nvitemd] Open MUX PATH ERR !!!\n");
    return -1;
  }

  return fd;
}

int main(int argc, char *argv[])
{
	pthread_t t1;
	int r_cnt,ret_data_size;
	nvrequest_header_t header;
	int begin_at;
	int timeout=0;

	init_nvitem_log();
	
	NVITEM_DEBUG("[Nvitemd] Service Starting...\n");
/**/
	while(1){
		sleep(1);
		nvitem_receive_fd = open_port();
		if (nvitem_receive_fd < 0)
		{
			//timeout++;
			//if(timeout > 30){
				NVITEM_DEBUG("[Nvitemd] Service Start ERR !!!\n");
				NVITEM_LOG("[Nvitemd] Service Start ERR !!!\n");
			//	exit(1);
			//}
		} else { break;}
	}
	NVITEM_DEBUG("[Nvitemd] Service Start OK!!!\n");
	NVITEM_DEBUG("[Nvitemd] sizeof(nvrequest_header_t) is %d !!!\n",sizeof(nvrequest_header_t));	

	init_save();

	extern int nandless();
	nandless();

	for(;;)
	{
		r_cnt = read(nvitem_receive_fd,data,DATA_BUF_LEN);
		if(r_cnt>DATA_BUF_LEN)
		{
			NVITEM_DEBUG("[Nvitemd] DATA_BUF_LEN is too small, must bigger than %d !!!\n",r_cnt);
			break;
		}
		memcpy(&header,data,sizeof(nvrequest_header_t));
		
		//NVITEM_DEBUG("[Nvitemd] Data lens : %d \n Data : %s \n",r_cnt,data);
		//NVITEM_DEBUG("[Nvitemd] Parsing !\n");
		parser_header(&header);
		gen_ret_header(&header,1,&ret_data_size);
		//NVITEM_DEBUG("[Nvitemd] ret_data_size is %d !!!\n",ret_data_size);
		write(nvitem_receive_fd,ret_data,ret_data_size);

		if(fixnv_dirty == 1 || runnv_dirty == 1)
			run_save();
		//sleep(10);

	}
	
	close(nvitem_receive_fd);
	//close(fd0);
	return 0;
}

