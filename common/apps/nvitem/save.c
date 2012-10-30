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

#include "package.h"
#include "save.h"
#include "log.h"
#define FIXNV_FILENAME	"/dev/block/mmcblk0p4"
#define RUNNINGNV_FILENAME "/dev/block/mmcblk0p6"

#define BACKUP_FIXNV_FILENAME	"/system/bin/fixnv.bin"
#define BACKUP_RUNNINGNV_FILENAME "/system/bin/runtimenv.bin"

extern unsigned char fixnv_buf[];
extern unsigned char runnv_buf[];
extern int fixnv_dirty;
extern int runnv_dirty;
extern int cursor_fixnv;
extern int cursor_runnv;

pthread_t t1;
int fixnv_fd;
int runningnv_fd;

static void *proc_route(void *par)
{
	unsigned char end_flag[4];
	end_flag[0]=end_flag[1]=end_flag[2]=end_flag[3]=0x5a;
	NVITEM_DEBUG("[Nvitemd] save proc_route running !!!\n");
	if(fixnv_dirty == 2)
	{
		NVITEM_DEBUG("[Nvitemd] save proc_route write fixnv.bin !!!\n");
		//backup
		fixnv_fd = open(BACKUP_FIXNV_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		write(fixnv_fd,fixnv_buf,FIXNV_REALSIZE*BLOCK_SIZE);
		write(fixnv_fd,end_flag,4);
		close(fixnv_fd);
		//fixnv
		fixnv_fd = open(FIXNV_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		write(fixnv_fd,fixnv_buf,FIXNV_REALSIZE*BLOCK_SIZE);
		write(fixnv_fd,end_flag,4);
		close(fixnv_fd);
		fixnv_dirty = 0;
	}

	if(runnv_dirty == 2)
	{
		NVITEM_DEBUG("[Nvitemd] save proc_route write runtimenv.bin !!!\n");
		//backup
		runningnv_fd = open(BACKUP_RUNNINGNV_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		write(runningnv_fd,runnv_buf,RUNNV_BLOCK_SIZE*BLOCK_SIZE);
		close(runningnv_fd);
		//runtimenv
		runningnv_fd = open(RUNNINGNV_FILENAME, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
		write(runningnv_fd,runnv_buf,RUNNV_BLOCK_SIZE*BLOCK_SIZE);
		close(runningnv_fd);
		runnv_dirty = 0;
	}

	pthread_exit(0);

	return 0;
}

void init_save()
{
	cursor_fixnv = 0;
	cursor_runnv = 0;
	fixnv_dirty = 0;
	runnv_dirty = 0;
}

void run_save()
{
	NVITEM_DEBUG("[Nvitemd] save init_save running !!!\n");

	if(fixnv_dirty == 1 )
		fixnv_dirty = 2;
	if(runnv_dirty == 1 )
		runnv_dirty = 2;

	pthread_create(&t1, NULL, (void*)proc_route, NULL);
}
