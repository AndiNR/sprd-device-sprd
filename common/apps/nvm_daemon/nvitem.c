#define LOG_TAG "nvm_daemon"

#include <cutils/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <mtd/mtd-user.h>
#include <time.h>
#include <math.h>
#include <hardware_legacy/power.h>
#include "crc32b.h"

/* #define TEST_MODEM    1 */
#define DATA_BUF_LEN    (2048 * 10)
#define min(A,B)	(((A) < (B)) ? (A) : (B))

typedef uint16_t file_number_t;
typedef struct {
        file_number_t type;	/* 0 : fixnv ; 1 : runtimenv ; 2 : productinfo */
        uint16_t req_type;	/* 0 : data ; 1 : sync command */
        uint32_t offset;
        uint32_t size;
} request_header_t;

#define FIXNV_SIZE              (64 * 1024)
#define RUNTIMENV_SIZE		    (256 * 1024)
#define PRODUCTINFO_SIZE	    (3 * 1024)
#define VBPIPE_BUFFER_MAX	    (RUNTIMENV_SIZE)

#ifdef CONFIG_EMMC
/* see g_sprd_emmc_partition_cfg[] in u-boot/nand_fdl/fdl-2/src/fdl_partition.c */ 
#define PARTITION_MODEM		    "/dev/block/mmcblk0p2"
#define PARTITION_DSP		    "/dev/block/mmcblk0p3"
#define PARTITION_FIX_NV1	    "/dev/block/mmcblk0p4"
#define PARTITION_FIX_NV2	    "/dev/block/mmcblk0p5"
#define PARTITION_RUNTIME_NV1	"/dev/block/mmcblk0p6"
#define PARTITION_RUNTIME_NV2	"/dev/block/mmcblk0p7"
#define PARTITION_PROD_INFO1	"/dev/block/mmcblk0p8"
#define PARTITION_PROD_INFO2	"/dev/block/mmcblk0p9"
#define EMMC_MODEM_SIZE		    (8 * 1024 * 1024)
unsigned char phasecheckdata[PRODUCTINFO_SIZE + 8];
#else
#define PARTITION_MODEM		"modem"
#define PARTITION_DSP		"dsp"
#define F2R1_MODEM_SIZE		(3500 * 1024)
#define F4R2_MODEM_SIZE		(8 * 1024 * 1024)
#endif

#define MODEM_IMAGE_OFFSET  0
#define MODEM_BANK          "guestOS_2_bank"
#define DSP_IMAGE_OFFSET    0
#define DSP_BANK            "dsp_bank"
#define BUFFER_LEN	        (512)
#define REQHEADINFOCOUNT	(3)

unsigned char data[DATA_BUF_LEN], file_flag[4], *vbpipe_buffer = NULL;
static int update_flag = 0, runtime_nvitem_fd_change = -1, runtime_nvitem_fd = -1;
static int runtime_nvitem_w_cnt, runtime_nvitem_res, req_head_info_count = 0;
int mcp_size = 512, modem_image_len = 0;
char gch, modem_mtd[256], dsp_mtd[256];
FILE * gfp;
char gbuffer[BUFFER_LEN];

int check_mocor_reboot(void);
char *loadFile(char *pathName, int *fileSize);

void log2file(void)
{
	if (gfp == NULL)
		return;

	fputs(gbuffer, gfp);
	fflush(gfp);
}

void req_head_info(request_header_t head) 
{
	memset(gbuffer, 0, BUFFER_LEN);
	if (head.req_type == 0)
		sprintf(gbuffer, "\ntype = %d  req_type = %d  offset = %d  size = %d\n", 
            head.type, head.req_type, head.offset, head.size);
	else
		sprintf(gbuffer, "\nreq_type = %d\n", head.req_type);
	log2file();
}

int copyfile(const char *src, const char *dst)
{
    int ret, length, srcfd, dstfd;
	struct stat statbuf;

	if ((src == NULL) || (dst == NULL))
		return -1;

	if (strcmp(src, dst) == 0)
		return -1;

	if (strcmp(src, "/proc/mtd") == 0) {
		ret = 800;
		length = ret;
	} else {
		ret = stat(src, &statbuf);
		if (ret == ENOENT) {
			ALOGE("src file is not exist\n");
			return -1;
		}
		length = statbuf.st_size;
	}

	if (length == 0) {
		ALOGE("src file length is 0\n");
		return -1;
	}

	memset(vbpipe_buffer, 0, VBPIPE_BUFFER_MAX);	
	srcfd = open(src, O_RDONLY);
	if (srcfd < 0) {
		ALOGE("open %s error\n", src);
		return -1;
	}

	ret = read(srcfd, vbpipe_buffer, length);
	close(srcfd);
	if (ret != length) {
		ALOGE("read error length = %d  ret = %d\n", length, ret);
		return -1;
	}

	if (access(dst, 0) == 0)
		remove(dst);

	dstfd = open(dst, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	if (dstfd < 0) {
		ALOGE("create %s error\n", dst);
		return -1;
	}

	ret = write(dstfd, vbpipe_buffer, length);
	close(dstfd);
	if (ret != length) {
		ALOGE("write error length = %d  ret = %d\n", length, ret);
		return -1;
	}

	return length;
}

char select_log_file(void) 
{
	int ret;
	FILE *fp;
	char ch;

	ret = opendir("/data/local");
	if (ret <= 0)
		ret = mkdir("/data/local", 771);

	ret = opendir("/data/local/tmp");
	if (ret <= 0)
		ret = mkdir("/data/local/tmp", 771);

	ret = access("/data/local/tmp/index.txt", 0);
	if (ret != 0) {
		ALOGE("/data/local/tmp/index.txt is not exist, create it\n");
		fp = fopen("/data/local/tmp/index.txt", "w+");
		if (fp == NULL) {
			ALOGE("can not creat /data/local/tmp/index.txt\n");
			return 0;
		} else {
			fputc('1', fp);
			fclose(fp);
			return '1';
		}
	}
	ALOGE("/data/local/tmp/index.txt is exist\n");
	fp = fopen("/data/local/tmp/index.txt", "r+");
	fseek(fp, 0, SEEK_SET);
	ch = fgetc(fp);
	ch++;
	if (ch == '6')
		ch = '1';
	fseek(fp, 0, SEEK_SET);
	fputc(ch, fp);
	fclose(fp);
	
    return ch;
}

/* cat /proc/mtd */ 
char *get_proc_mtd(void) 
{
	int fd, fileSize;
	char *buf;

    copyfile("/proc/mtd", "/data/local/tmp/mtd.txt");
	fd = open("/data/local/tmp/mtd.txt", O_RDONLY);
    if (fd < 0) {
		ALOGE("can not open /data/local/tmp/mtd.txt\n");
		return 0;
	}
	
    fileSize = lseek(fd, 0, SEEK_END);
	if (fileSize < 0) {
		ALOGE("fileSize is error\n");
		return 0;
	}
	buf = (char *)malloc((fileSize) + 1);
	if (buf == 0) {
		ALOGE("Malloc buffer failed\n");
		return 0;
	}
	if (lseek(fd, 0, SEEK_SET) < 0) {
		ALOGE("lseek header error\n");
		return 0;
	}
	if (read(fd, buf, fileSize) != fileSize) {
		free(buf);
		buf = 0;
	} else
		buf[fileSize] = 0;
	
    return buf;
}

int get_mcp_size(char *buf) 
{
	char *pos;
	int pos_len;
	char number[8];
	int nandsize;

	if (buf == 0)
		return 0;

    pos = strstr(buf, "MiB");
	if (pos == 0) {
		ALOGE("failed to find MiB\n");
		return 0;
	}
	pos_len = 0;
	while (*pos != ' ') {
		pos--;
		pos_len++;
	}
	pos++;
	pos_len--;
	memset(number, 0, 8);
	strncpy(number, pos, pos_len);
	nandsize = atoi(number);
	
    return nandsize;
}

void allocat_mtd_partition(char *name, char *mtdpath) 
{
	strcpy(mtdpath, "/dev/mtd/");
	strncat(mtdpath, name, strlen(name));
} 

void get_mtd_partition(char *buf, char *name, char *mtdpath)
{
	char *pos;
	int pos_len;
	
    pos = strstr(buf, name);
	if (pos == 0)
		ALOGE("failed to find %s\n", name);
	while (*pos != ':') {
		pos--;
	}
	pos_len = 0;
	while (*pos != 'm') {
		pos--;
		pos_len++;
	}
	strcpy(mtdpath, "/dev/mtd/");
	strncat(mtdpath, pos, pos_len);
}

/* update fixnv to backupfixnv */ 
void update_fixnv(void) 
{
#ifndef CONFIG_EMMC
    copyfile("/backupfixnv/fixnv.bin", "/fixnv/fixnvchange.bin");
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "copy /backupfixnv/fixnv.bin /fixnv/fixnvchange.bin\n");
	log2file();
	copyfile("/fixnv/fixnvchange.bin", "/fixnv/fixnv.bin");
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "copy /fixnv/fixnvchange.bin /fixnv/fixnv.bin\n");
	log2file();
#endif
} 

void close_runtime_nvitem_fd(void)
{
	if (runtime_nvitem_fd_change != -1)
		close(runtime_nvitem_fd_change);
	
    if (runtime_nvitem_fd != -1)
		close(runtime_nvitem_fd);
	
    runtime_nvitem_fd_change = -1;
	runtime_nvitem_fd = -1;
}

int main(int argc, char **argv) 
{
	int pipe_fd, fix_nvitem_fd, product_info_fd, fd, ret = 0;
	int r_cnt, w_cnt, res, total, length, rectotal;
	request_header_t req_header;
	char *mtdbuf = NULL;
	unsigned long crc;

	ret = nice(-16);
	if ((-1 == ret) && (errno == EACCES))
		ALOGE("set priority fail\n");

    gch = select_log_file();
	gfp = NULL;
	if (gch == '1')
		gfp = fopen("/data/local/tmp/1.txt", "w+");
	else if (gch == '2')
		gfp = fopen("/data/local/tmp/2.txt", "w+");
	else if (gch == '3')
		gfp = fopen("/data/local/tmp/3.txt", "w+");
	else if (gch == '4')
		gfp = fopen("/data/local/tmp/4.txt", "w+");
	else if (gch == '5')
		gfp = fopen("/data/local/tmp/5.txt", "w+");
	else
		ALOGE("nothing is selected\n");
	if (gfp == NULL)
		ALOGE("\nno log file\n");

	vbpipe_buffer = (unsigned char *)malloc(VBPIPE_BUFFER_MAX);
	if (vbpipe_buffer == NULL) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Malloc vbpipe buffer failed, so exit!\n");
		log2file();
		exit(1);
	} else {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Malloc vbpipe buffer success : %d\n", VBPIPE_BUFFER_MAX);
		log2file();
	}
	
#ifdef CONFIG_EMMC
	modem_image_len = EMMC_MODEM_SIZE;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem length : %d\n", modem_image_len);
	log2file();

	memset(modem_mtd, 0, 256);
	strcpy(modem_mtd, PARTITION_MODEM);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem emmc dev : %s\n", modem_mtd);
	log2file();
	
    memset(dsp_mtd, 0, 256);
	strcpy(dsp_mtd, PARTITION_DSP);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "dsp emmc dev : %s\n", dsp_mtd);
	log2file();
#else //#ifdef CONFIG_EMMC
	mtdbuf = get_proc_mtd();
	mcp_size = get_mcp_size(mtdbuf);
	if (mcp_size >= 512)
		modem_image_len = F4R2_MODEM_SIZE;
	else
		modem_image_len = F2R1_MODEM_SIZE;
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "mcp size : %d  modem length : %d\n", mcp_size, modem_image_len);
	log2file();
	
    memset(modem_mtd, 0, 256);
	get_mtd_partition(mtdbuf, PARTITION_MODEM, modem_mtd);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "modem mtd dev : %s\n", modem_mtd);
	log2file();
	
    memset(dsp_mtd, 0, 256);
	get_mtd_partition(mtdbuf, PARTITION_DSP, dsp_mtd);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "dsp mtd dev : %s\n", dsp_mtd);
	log2file();

	if (mtdbuf != 0)
		free(mtdbuf);
	if (access("/productinfo/productinfo.bin", 0) == 0)
		chmod("/productinfo/productinfo.bin", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#endif //#ifdef CONFIG_EMMC

reopen_vbpipe:
    req_head_info_count = 0;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "reopen vbpipe\n");
	log2file();
	
    pipe_fd = open("/dev/vbpipe1", O_RDWR);
	if (pipe_fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "cannot open vbpipe1\n");
		log2file();
		exit(1);
	}
	
    runtime_nvitem_fd_change = -1;
	runtime_nvitem_fd = -1;
	
    while (1) {
		r_cnt = read(pipe_fd, &req_header, sizeof(request_header_t));
		acquire_wake_lock(PARTIAL_WAKE_LOCK, "NvItemdLock");
		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "1 r_cnt = %d\n", r_cnt);
			log2file();
		}
		/* write_proc_file("/proc/nk/stop", 0,  "2");     --->   echo 2 > /proc/nk/stop */ 
		/* write_proc_file("/proc/nk/restart", 0,  "2");  --->   echo 2 > /proc/nk/restart */
        if (r_cnt == 0) {
		    close_runtime_nvitem_fd();
            /* check if the system is shutdown , if yes just reload the binary image */
            check_mocor_reboot();
			/* 
             * when read returns 0 : it means vbpipe has received
			 * sysconf so we should  close the file descriptor
			 * and re-open the pipe.
			 */
            close(pipe_fd);
            memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "goto reopen vbpipe\n");
			log2file();
			goto reopen_vbpipe;
		}

		if (r_cnt < 0) {
			close_runtime_nvitem_fd();
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "1 no nv data : %d\n", r_cnt);
			log2file();
			release_wake_lock("NvItemdLock");
			continue;
		}

		if (req_head_info_count < REQHEADINFOCOUNT) {
			req_head_info_count++;
			req_head_info(req_header);
		}

		if (req_header.req_type == 1) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "update_flag = %d\n", update_flag);
			log2file();
			if (update_flag) {
				update_flag = 0;
				update_fixnv();
			}
			
	        /* 1 : sync command */
            memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "sync start\n");
		    log2file();

            close_runtime_nvitem_fd();
		    sync();

            memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "sync end\n");
		    log2file();

            req_header.type = 0;
		    memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "write back sync start\n");
		    log2file();
		    
            w_cnt = write(pipe_fd, &req_header, sizeof(request_header_t));
		    memset(gbuffer, 0, BUFFER_LEN);
		    sprintf(gbuffer, "write back sync end w_cnt = %d\n", w_cnt);
		    log2file();
            if (w_cnt < 0) {
			    memset(gbuffer, 0, BUFFER_LEN);
			    sprintf(gbuffer, "failed to write sync command\n");
			    log2file();
		    }
            release_wake_lock("NvItemdLock");
		    continue;
		}

		if (req_header.type == 0) {
#ifdef CONFIG_EMMC
	        fix_nvitem_fd = open(PARTITION_FIX_NV2, O_RDWR, 0);
#else
			copyfile("/backupfixnv/fixnv.bin", "/backupfixnv/fixnvchange.bin");
			fix_nvitem_fd = open("/backupfixnv/fixnvchange.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
            if (fix_nvitem_fd < 0) {
				close_runtime_nvitem_fd();
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open fixnv mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

			if ((req_header.offset + req_header.size) > FIXNV_SIZE) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "Fixnv is too big\n");
				log2file();
			}

			fd = fix_nvitem_fd;
			length = FIXNV_SIZE;
		} else if (req_header.type == 1) {
			if (runtime_nvitem_fd_change == -1)
#ifdef CONFIG_EMMC
		        runtime_nvitem_fd_change = open(PARTITION_RUNTIME_NV2, O_RDWR, 0);
#else
				runtime_nvitem_fd_change = open("/runtimenv/runtimenvchange.bin",
                O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);		
#endif
            if (runtime_nvitem_fd_change < 0) {
			    runtime_nvitem_fd_change = -1;
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open runtimenvchange mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

			if (runtime_nvitem_fd == -1)
#ifdef CONFIG_EMMC
		        runtime_nvitem_fd = open(PARTITION_RUNTIME_NV1, O_RDWR, 0);
#else
                runtime_nvitem_fd = open("/runtimenv/runtimenv.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
			if (runtime_nvitem_fd < 0) {
			    runtime_nvitem_fd = -1;
			    close(runtime_nvitem_fd_change);
			    runtime_nvitem_fd_change = -1;
 
                memset(gbuffer, 0, BUFFER_LEN);
			    sprintf(gbuffer, "cannot open runtimenv mtd device\n");
			    log2file();
			    close(pipe_fd);
			    release_wake_lock("NvItemdLock");
			    exit(1);
			}

			if ((req_header.offset + req_header.size) > RUNTIMENV_SIZE) {
			    memset(gbuffer, 0, BUFFER_LEN);
			    sprintf(gbuffer, "Runtimenv is too big\n");
			    log2file();
			}

            fd = runtime_nvitem_fd_change;
			length = RUNTIMENV_SIZE;
		} else if (req_header.type == 2) {
			/* phase check */
#ifdef CONFIG_EMMC
			product_info_fd = open(PARTITION_PROD_INFO2, O_RDWR, 0);
#else
			if (access("/productinfo/productinfo.bin", 0) == 0)
                    copyfile("/productinfo/productinfo.bin", "/productinfo/productinfochange.bin");
			product_info_fd = open("/productinfo/productinfochange.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
			if (product_info_fd < 0) {
				close_runtime_nvitem_fd();
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "cannot open productinfo mtd device\n");
				log2file();
				close(pipe_fd);
				release_wake_lock("NvItemdLock");
				exit(1);
			}

			if ((req_header.offset + req_header.size) > PRODUCTINFO_SIZE) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "Productinfo is too big\n");
				log2file();
			}

			fd = product_info_fd;
			length = PRODUCTINFO_SIZE;
		} else {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "file type error\n");
			log2file();
			release_wake_lock("NvItemdLock");
			continue;
		}

		total = req_header.size;
		rectotal = 0;
		while (total > 0) {
			if (total > DATA_BUF_LEN)
				r_cnt = read(pipe_fd, vbpipe_buffer + rectotal, DATA_BUF_LEN);
			else
				r_cnt = read(pipe_fd, vbpipe_buffer + rectotal, total);
			
            if (req_head_info_count < REQHEADINFOCOUNT) {
				req_head_info_count++;
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "2 r_cnt = %d  size = %d\n", r_cnt, total);
				log2file();
			}

			if (r_cnt == 0) {
				close_runtime_nvitem_fd();
				close(fd);
				check_mocor_reboot();
				close(pipe_fd);
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "goto reopen vbpipe\n");
				log2file();
				release_wake_lock("NvItemdLock");
				goto reopen_vbpipe;
			}

			if (r_cnt < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "2 no nv data : %d\n", r_cnt);
				log2file();
				continue;
			}

			rectotal += r_cnt;
			total -= r_cnt;
		} /* while (total > 0) */
		
		/* clear flag */
        res = lseek(fd, length, SEEK_SET);
		if (res < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "lseek errno 1\n");
			log2file();
		}

		file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0xff;
		res = write(fd, file_flag, 4);
		if (res < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "\nfailed to write file_flag 1\n");
			log2file();
			continue;
		}

		fsync(fd);
		res = lseek(fd, req_header.offset, SEEK_SET);
		if (res < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "lseek errno\n");
			log2file();
		}
		
        w_cnt = write(fd, vbpipe_buffer, rectotal);
		if (w_cnt < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "\nfailed to write write\n");
			log2file();
			continue;
		}
		
        /* set flag */
        res = lseek(fd, length, SEEK_SET);
		if (res < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "lseek errno 2\n");
			log2file();
		}
		
        file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0x5a;
		res = write(fd, file_flag, 4);
		if (res < 0) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "\nfailed to write file_flag 2\n");
			log2file();
			continue;
		}

		fsync(fd);
		
        if (length != RUNTIMENV_SIZE)
			close(fd);
		if (length == RUNTIMENV_SIZE) {
			/* clear flag */
            lseek(runtime_nvitem_fd, length, SEEK_SET);
			file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0xff;
			write(runtime_nvitem_fd, file_flag, 4);
			fsync(runtime_nvitem_fd);
			
			/* write data */
            runtime_nvitem_res = lseek(runtime_nvitem_fd, req_header.offset, SEEK_SET);
			if (runtime_nvitem_res < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "runtime nv lseek errno\n");
				log2file();
			}

			runtime_nvitem_w_cnt = write(runtime_nvitem_fd, vbpipe_buffer, rectotal);
			if (runtime_nvitem_w_cnt < 0) {
				memset(gbuffer, 0, BUFFER_LEN);
				sprintf(gbuffer, "\nfailed to write nv write\n");
				log2file();
				continue;
			}
			
			/* set flag */
            lseek(runtime_nvitem_fd, length, SEEK_SET);
			file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0x5a;
			write(runtime_nvitem_fd, file_flag, 4);
			fsync(runtime_nvitem_fd);
		} else if (length == PRODUCTINFO_SIZE) {
#ifdef CONFIG_EMMC
			/* crc32 */
            memset(phasecheckdata, 0xff, PRODUCTINFO_SIZE + 8);
			product_info_fd = open(PARTITION_PROD_INFO2, O_RDWR, 0);
			read(product_info_fd, phasecheckdata, PRODUCTINFO_SIZE + 4);
			crc = crc32b(0xffffffff, phasecheckdata, PRODUCTINFO_SIZE + 4);
			phasecheckdata[PRODUCTINFO_SIZE + 4] = (crc & (0xff << 24)) >> 24;
			phasecheckdata[PRODUCTINFO_SIZE + 5] = (crc & (0xff << 16)) >> 16;
			phasecheckdata[PRODUCTINFO_SIZE + 6] = (crc & (0xff << 8)) >> 8;
			phasecheckdata[PRODUCTINFO_SIZE + 7] = crc & 0xff;
			write(product_info_fd, phasecheckdata, PRODUCTINFO_SIZE + 8);
			close(product_info_fd);

			product_info_fd = open(PARTITION_PROD_INFO1, O_RDWR, 0);
			write(product_info_fd, phasecheckdata, PRODUCTINFO_SIZE + 8);
			close(product_info_fd);
#else
			copyfile("/productinfo/productinfochange.bin", "/productinfo/productinfo.bin");
#endif
		} else if (length == FIXNV_SIZE) {
#ifdef CONFIG_EMMC
	        fix_nvitem_fd = open(PARTITION_FIX_NV1, O_RDWR, 0);
			/* clear flag */
            file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0xff;
			lseek(fix_nvitem_fd, length, SEEK_SET);
			write(fix_nvitem_fd, file_flag, 4);
			fsync(fix_nvitem_fd);
			/* write data */
            lseek(fix_nvitem_fd, req_header.offset, SEEK_SET);
			write(fix_nvitem_fd, vbpipe_buffer, rectotal);
			/* set flag */
            file_flag[0] = file_flag[1] = file_flag[2] = file_flag[3] = 0x5a;
			lseek(fix_nvitem_fd, length, SEEK_SET);
			write(fix_nvitem_fd, file_flag, 4);
			fsync(fix_nvitem_fd);
			close(fix_nvitem_fd);
#else
			copyfile("/backupfixnv/fixnvchange.bin", "/backupfixnv/fixnv.bin");
#endif
			update_flag++;
		}
		release_wake_lock("NvItemdLock");
	} /* while(1) */

	close_runtime_nvitem_fd();
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "close pipe_fd fixnv runtimenv\n");
	log2file();
	
    fclose(gfp);
	close(pipe_fd);
	
    return 0;
}

char *loadOpenFile(int fd, int *fileSize) 
{
	char *buf;
	
    *fileSize = lseek(fd, 0, SEEK_END);
	if (*fileSize < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "loadOpenFile error\n");
		log2file();
		return 0;
	}
	
    buf = (char *)malloc((*fileSize) + 1);
	if (buf == 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Malloc failed\n");
		log2file();
		return 0;
	}
	
    if (lseek(fd, 0, SEEK_SET) < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "lseek error\n");
		log2file();
		return 0;
	}

	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "file size %d \n", *fileSize);
	log2file();
	
    if (read(fd, buf, *fileSize) != *fileSize) {
		free(buf);
		buf = 0;
	} else
		buf[*fileSize] = 0;
	
    return buf;
}

char *loadFile(char *pathName, int *fileSize) 
{
	int fd;
	char *buf;
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "pathName = %s\n", pathName);
	log2file();
	fd = open(pathName, O_RDONLY);
	if (fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Unknown file %s\n", pathName);
		log2file();
		return 0;
	}
	
    buf = loadOpenFile(fd, fileSize);
	close(fd);
	
    return buf;
}

int check_proc_file(char *file, char *string) 
{
	int filesize;
	char *buf;
	
    buf = loadFile(file, &filesize);
	if (!buf) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to load %s \n", file);
		log2file();
		return -1;
	}
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "buf = %s\n", buf);
	log2file();
	if (strstr(buf, string)) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "String found <%s> in <%s>\n", string, file);
		log2file();
		return 0;
	}
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "failed to find %s in %s\n", string, file);
	log2file();
	
    return 1;
}

int write_proc_file(char *file, int offset, char *string) 
{
	int fd, stringsize, res = -1;
	
    fd = open(file, O_RDWR);
	if (fd < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Unknown file %s \n", file);
		log2file();
		return 0;
	}

	if (lseek(fd, offset, SEEK_SET) != offset) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Cant lseek file %s \n", file);
		log2file();
		goto leave;
	}

	stringsize = strlen(string);
	if (write(fd, string, stringsize) != stringsize) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "Could not write %s in %s \n", string, file);
		log2file();
		goto leave;
	}

	res = 0;
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Wrote %s in file %s \n", string, file);
	log2file();

leave:
    close(fd);
	
    return res;
}

int loadimage(char *fin, int offsetin, char *fout, int offsetout, int size)  
{
	int res = -1, fdin, fdout, bufsize, i, rsize, rrsize, wsize;
	char buf[8192];
#ifdef TEST_MODEM
	int fd_modem_dsp;
#endif

	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Loading %s in bank %s:%d %d\n", fin, fout, offsetout, size);
	log2file();

	fdin = open(fin, O_RDONLY, 0);
	fdout = open(fout, O_RDWR, 0);
	if (fdin < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to open %s \n", fin);
		log2file();
		return -1;
	}
	if (fdout < 0) {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to open %s \n", fout);
		log2file();
		return -1;
	}
	
#ifdef TEST_MODEM
	if (size == modem_image_len)
	    fd_modem_dsp = open("/data/local/tmp/modem.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
	else
	    fd_modem_dsp = open("/data/local/tmp/dsp.bin", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
	if (lseek(fdin, offsetin, SEEK_SET) != offsetin) {
	    memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "failed to lseek %d in %s \n", offsetin, fin);
		log2file();
		goto leave;
	}

	if (lseek(fdout, offsetout, SEEK_SET) != offsetout) {
	    memset(gbuffer, 0, BUFFER_LEN);
	    sprintf(gbuffer, "failed to lseek %d in %s \n", offsetout, fout);
	    log2file();
	    goto leave;
	}

	for (i = 0; size > 0; i += min(size, sizeof(buf))) {
		rsize = min(size, sizeof(buf));
		rrsize = read(fdin, buf, rsize);
		if (rrsize != rsize) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "failed to read %s \n", fin);
			log2file();
			goto leave;
		}
		wsize = write(fdout, buf, rsize);
#ifdef TEST_MODEM
		wsize = write(fd_modem_dsp, buf, rsize);
#endif
		if (wsize != rsize) {
			memset(gbuffer, 0, BUFFER_LEN);
			sprintf(gbuffer, "failed to write %s [wsize = %d  rsize = %d  remain = %d]\n",
                fout, wsize, rsize, size);
			log2file();
			goto leave;
		}
		size -= rsize;
	}

	res = 0;
leave:
    close(fdin);
	close(fdout);
#ifdef TEST_MODEM
	fsync(fd_modem_dsp);
	close(fd_modem_dsp);
#endif
	
    return res;
}

int check_reboot(int guestID) 
{
	char guest_dir[256];
	char buf[256];
	
    memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "Check morco reboot \n");
	log2file();
	
    memset(guest_dir, 0, 256);
	memset(buf, 0, 256);
	sprintf(guest_dir, "/proc/nk/guest-%02d", guestID);
	sprintf(buf, "%s/status", guest_dir);
	memset(gbuffer, 0, BUFFER_LEN);
	sprintf(gbuffer, "buf = %s\n", buf);
	log2file();
	
    if (check_proc_file(buf, "not started") == 0) {
		memset(buf, 0, 256);
		sprintf(buf, "%s/%s", guest_dir, MODEM_BANK);
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "buf = %s\n", buf);
		log2file();
		/* gillies set modem_image_len 0x600000 */
        loadimage(modem_mtd, MODEM_IMAGE_OFFSET, buf, 0x1000, modem_image_len);
		memset(buf, 0, 256);
		sprintf(buf, "%s/%s", guest_dir, DSP_BANK);
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "buf = %s\n", buf);
		log2file();

		/* dsp is only 3968KB, 0x420000 is too big */
        loadimage(dsp_mtd, DSP_IMAGE_OFFSET, buf, 0x20000, (3968 * 1024));
		system("busybox killall -SIGUSR1 sprd_monitor");
		
        memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "kill 1 sprd_monitor\n");
		log2file();
		
        write_proc_file("/proc/nk/restart", 0, "2");
		write_proc_file("/proc/nk/resume", 0, "2");
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "wait for 20s\n");
		log2file();
		
        sleep(20);
		
        system("busybox killall -SIGUSR2 sprd_monitor");
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "kill 2 sprd_monitor\n");
		log2file();
		
        return 1;
	} else {
		memset(gbuffer, 0, BUFFER_LEN);
		sprintf(gbuffer, "guest seems started \n");
		log2file();
	}

	return 0;
}

int check_mocor_reboot(void) 
{
	return check_reboot(2);
}

