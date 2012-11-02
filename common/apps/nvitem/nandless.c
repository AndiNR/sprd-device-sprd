//#define LOG_TAG "NANDLESS"

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

#include "engapi.h"
#include "engat.h"

#define NANDLESS_DBG(fmt, arg...) ALOGD("NANDLESS:%s "fmt, __FUNCTION__ , ## arg)
#define NANDLESS_LOG(fmt, arg...) ALOGD(fmt, ## arg)
#define NVSYN_CMD		"AT+NVSYNLINK=21,1000,10000,512"

int nandless()
{
	int fd;
	char cmd[64];
	int ret_write;
	int timeout=0;

/**/
	while(1){
		fd = engapi_open(0);
		if(timeout>30){
			if(fd < 0){
				NANDLESS_LOG("nandless engapi_open err !!!\n");
				//exit(-1);
			} else { break;}
		}
		sleep(1);
		timeout++;
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%d,%d,%s",ENG_AT_NOHANDLE_CMD,1,NVSYN_CMD);
	NANDLESS_DBG("nandless engapi_write: %s !!!\n",cmd);
	ret_write = engapi_write(fd, cmd, strlen(cmd));
	NANDLESS_DBG("nandless engapi_write: lens %d !!!\n",ret_write);
	if(strlen(cmd) != ret_write)
		NANDLESS_DBG("nandless engapi_write : wirte fail !!!\n");

	memset(cmd, 0, sizeof(cmd));
	engapi_read(fd, cmd, sizeof(cmd));
	NANDLESS_DBG("nandless engapi_read: %s !!!\n",cmd);

	if(strstr(cmd,"ERR") != NULL)
	{
		engapi_close(fd);
		return 0;
	}
	engapi_close(fd);
	return 0;
}
