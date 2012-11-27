/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/statfs.h>
#include <cutils/properties.h>

#include "slog.h"

int slog_enable = 1;
int screenshot_enable = 1;
int slog_initialized = 0;
int slog_start_step = 0;
int stream_log_handler_started = 0;
int snapshot_log_handler_started = 0;
int notify_log_handler_started = 0;

int internal_log_size = 10; /*M*/

char *config_log_path = INTERNAL_LOG_PATH;
char *current_log_path;
char top_logdir[MAX_NAME_LEN];
char external_storage[MAX_NAME_LEN];
char external_path[MAX_NAME_LEN];

struct slog_info *stream_log_head, *snapshot_log_head;
struct slog_info *notify_log_head, *misc_log;

pthread_t stream_tid, snapshot_tid, notify_tid, sdcard_tid, command_tid;


void exec_or_dump_content(struct slog_info *info, char *filepath)
{
	struct stat st;
	FILE *fcmd, *fp;
	int ret;
	time_t t;
	struct tm tm;
	char buffer[4096];

	if(!strncmp("bugreport", info->content, 9)) {
		sprintf(buffer, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
		ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
		if(-1 == ret && (errno != EEXIST)){
			err_log("mkdir %s failed.", buffer);
			exit(0);
		}
		sprintf(buffer, "bugreport >> %s/%s/%s/%s.log",
			current_log_path, top_logdir, info->log_path, info->log_basename);
		system(buffer);
		return;
	}

	/* setup log file first */
	if( filepath == NULL ) {
		sprintf(buffer, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
		ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
		if(-1 == ret && (errno != EEXIST)){
			err_log("mkdir %s failed.", buffer);
			exit(0);
		}
		sprintf(buffer, "%s/%s/%s/%s.log",
			current_log_path, top_logdir, info->log_path, info->log_basename);
	} else {
		strcpy(buffer, filepath);
	}

	fp = fopen(buffer, "a+");
	if(fp == NULL) {
		err_log("open file %s failed!", buffer);
		return;
	}

	if(!strncmp(info->opt, "file", 4)) {
		fcmd = fopen(info->content, "r");
	} else {
		fcmd = popen(info->content, "r");
	}

	if(fcmd == NULL) {
		err_log("open target %s failed!", info->content);
		fclose(fp);
		return;
	}

        /* add timestamp */
        t = time(NULL);
        localtime_r(&t, &tm);
        fprintf(fp, "\n============ %s  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
				info->log_basename,
				tm.tm_year % 100,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

	/* recording... */
	while( (ret = fread(buffer, 1, 4096, fcmd)) > 0)
		fwrite(buffer, 1, ret, fp);

	if(!strncmp(info->opt, "file", 4))
		fclose(fcmd);
	else
		pclose(fcmd);

	fclose(fp);

	if(!strncmp("apanic_console", info->name, 14) || !strncmp("apanic_threads", info->name, 14)){
		sprintf(buffer, "rm -r %s", info->content);
		system(buffer);
	}

	return;
}

static int capture_by_name(struct slog_info *head, const char *name)
{
	struct slog_info *info = head;
	while(info) {
		if(!strncmp(info->name, name, strlen(name))) {
			exec_or_dump_content(info, NULL);
			return 0;
		}
		info = info->next;
	}
	return -1;
}

static int capture_last_log(struct slog_info *head)
{
        struct slog_info *info = head;
        while(info) {
		if(info->level == 7)
			exec_or_dump_content(info, NULL);
                info = info->next;
        }
        return 0;
}


static int capture_all(struct slog_info *head)
{
	char filepath[MAX_NAME_LEN];
	int ret;
	time_t t;
        struct tm tm;
	struct slog_info *info = head;

	sprintf(filepath, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
        ret = mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
        if(-1 == ret && (errno != EEXIST)){
                err_log("mkdir %s failed.", filepath);
                exit(0);
        }

        t = time(NULL);
        localtime_r(&t, &tm);
	sprintf(filepath, "%s/%s/%s/snapshot.log_%02d%02d%02d",
				current_log_path,
				top_logdir,
				info->log_path,
                                tm.tm_hour,
                                tm.tm_min,
                                tm.tm_sec
	);

	while(info) {
		if(info->level <= 6)
			exec_or_dump_content(info, filepath);
		info = info->next;
	}
	return 0;
}

static void handler_last_dir()
{
        DIR *p_dir;
        struct dirent *p_dirent;
        int log_num = 0, last_flag =0, ret;
        char buffer[MAX_NAME_LEN], ybuffer[MAX_NAME_LEN], mbuffer[MAX_NAME_LEN];

        if(( p_dir = opendir(current_log_path)) == NULL)
                {
                        err_log("can not open %s.", current_log_path);
                        return;
                }

        while((p_dirent = readdir(p_dir)))
                {
                        if( !strncmp(p_dirent->d_name, "20", 2) ) {
                                log_num += 1;
                        }else if( !strncmp(p_dirent->d_name, "last_log", 3) ){
                                last_flag = 1;
                        }
                }

        if(log_num < LOG_DIR_NUM){
                closedir(p_dir);
                return;
        }

        sprintf(buffer, "%s/%s/", current_log_path, LAST_LOG);
        sprintf(ybuffer, "%s %s", "rm -r", buffer);

        if(last_flag == 1){
                system(ybuffer);
        }
        debug_log("%s\n", ybuffer);

	ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
	if(-1 == ret && (errno != EEXIST)){
		err_log("mkdir %s failed.", buffer);
		exit(0);
	}

        sprintf(mbuffer, "%s %s/%s %s", "mv", current_log_path, "20*", buffer);
        debug_log("%s\n", mbuffer);
        system(mbuffer);

        closedir(p_dir);
        return;
}

/*
 *
 */
static int cp_internal_to_external()
{
        DIR *p_dir;
        struct dirent *p_dirent;
	char buffer[MAX_NAME_LEN], cmd_buffer[MAX_NAME_LEN];

        if(( p_dir = opendir(INTERNAL_LOG_PATH)) == NULL)
                {
                        err_log("can not open %s.", current_log_path);
                        return 0;
                }

        while((p_dirent = readdir(p_dir)))
                {
                        if( !strncmp(p_dirent->d_name, "20", 2) ) {
				strcpy(top_logdir, p_dirent->d_name);
				sprintf(buffer, "busybox cp -r %s/%s %s/", INTERNAL_LOG_PATH, top_logdir, external_storage);
				system(buffer);
				debug_log("%s", buffer);
				return 1;
                        }
                }

	return 0;
}

static void create_log_dir()
{
	time_t when;
	struct tm start_tm;
	struct stat st;
	char path[MAX_NAME_LEN];
	int ret = 0;

	handler_last_dir();

	if(slog_start_step == 1){
		if (cp_internal_to_external()){
			sprintf(path, "%s/modem_memory.log", external_path);
			if(!stat(path, &st)){
				sprintf(path, "mv %s/modem_memory.log %s/%s/misc/", external_path, current_log_path, top_logdir);
				system(path);
			}
			return;
		}
	}

	/* generate log dir */
	when = time(NULL);
	localtime_r(&when, &start_tm);
	sprintf(top_logdir, "20%02d%02d%02d%02d%02d%02d",
						start_tm.tm_year % 100,
						start_tm.tm_mon + 1,
						start_tm.tm_mday,
						start_tm.tm_hour,
						start_tm.tm_min,
						start_tm.tm_sec);


	sprintf(path, "%s/%s", current_log_path, top_logdir);

	ret = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)){
		err_log("mkdir %s failed.", path);
		exit(0);
	}

	debug_log("%s\n",top_logdir);
	return;
}

static void use_ori_log_dir()
{
        DIR *p_dir;
        struct dirent *p_dirent;
        int log_num = 0;

        if(( p_dir = opendir(current_log_path)) == NULL)
                {
                        err_log("can not open %s.", current_log_path);
                        return;
                }

        while((p_dirent = readdir(p_dir)))
                {
                        if( !strncmp(p_dirent->d_name, "20", 2) ) {
				strcpy(top_logdir, p_dirent->d_name);
				log_num = 1;
				debug_log("%s\n",top_logdir);
				break;
                        }
                }

	if(log_num == 0)
		create_log_dir();

	return;
}

static int start_sub_threads()
{
	if(!stream_log_handler_started)
		pthread_create(&stream_tid, NULL, stream_log_handler, NULL);
	if(!snapshot_log_handler_started)
		pthread_create(&snapshot_tid, NULL, snapshot_log_handler, NULL);
	if(!notify_log_handler_started)
		pthread_create(&notify_tid, NULL, notify_log_handler, NULL);
	return 0;
}

static int stop_sub_threads()
{
	return 0;
}

static int reload()
{
	kill(getpid(), SIGTERM);
	return 0;
}

static void init_external_storage()
{
	char *p;

	p = getenv("SECONDARY_STORAGE");
	if(p == NULL)
		p = getenv("EXTERNAL_STORAGE");
	if(p == NULL){
		debug_log("cannot find the external storage environment");
		exit(0);
	}
	strcpy(external_path, p);
	sprintf(external_storage, "%s/slog", p);
	debug_log("the external storage : %s", external_storage);
	return;
}

static int sdcard_mounted()
{
	FILE *str;
	char buffer[MAX_LINE_LEN];

	str = fopen("/proc/mounts", "r+");
	if(str == NULL) {
		err_log("can't open /proc/mounts!");
		return 0;
	}

	while(fgets(buffer, MAX_LINE_LEN, str) != NULL){
		if(strstr(buffer, external_path)){
			fclose(str);
			return 1;
		}
	}

	fclose(str);
	return 0;
}

static int recv_socket(int sockfd, void* buffer, int size)
{
        int received = 0, result;
        while(buffer && (received < size))
        {
                result = recv(sockfd, (char *)buffer + received, size - received, MSG_NOSIGNAL);
                if (result > 0) {
                        received += result;
                } else {
                        received = result;
                        break;
                }
        }
        return received;
}

static int send_socket(int sockfd, void* buffer, int size)
{
        int result = -1;
        int ioffset = 0;
        while(sockfd > 0 && ioffset < size) {
                result = send(sockfd, (char *)buffer + ioffset, size - ioffset, MSG_NOSIGNAL);
                if (result > 0) {
                        ioffset += result;
                } else {
                        break;
                }
        }
        return result;
}

int clear_all_log()
{
	char cmd[MAX_NAME_LEN];

	slog_enable = 0;
	stop_sub_threads();
	sleep(3);
	sprintf(cmd, "rm -r %s/%s/", current_log_path, top_logdir);
	system(cmd);
	sprintf(cmd, "rm -r %s/last_log", current_log_path);
	system(cmd);
	reload();
	return 0;
}

int dump_all_log(const char *name)
{
	char cmd[MAX_NAME_LEN];
	capture_all(snapshot_log_head);
	capture_by_name(snapshot_log_head, "bugreport");
	capture_by_name(snapshot_log_head, "getprop");
	if(!strncmp(current_log_path ,INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)))
		return -1;
	sprintf(cmd, "tar czf %s/../%s /%s %s", current_log_path, name, current_log_path, INTERNAL_LOG_PATH);
	return system(cmd);
}

void *command_handler(void *arg)
{
	struct slog_cmd cmd;
	struct sockaddr_un serv_addr;
        int ret, server_sock, client_sock;
	char filename[MAX_NAME_LEN];
	time_t t;
	struct tm tm;

	/* init unix domain socket */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family=AF_UNIX; 
	strcpy(serv_addr.sun_path, SLOG_SOCKET_FILE);
	unlink(serv_addr.sun_path);

 	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_sock < 0) {
		err_log("create socket failed!");
		return NULL;
	}

	if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		err_log("bind socket failed!");
		return NULL;
	}
	
	if (listen(server_sock, 5) < 0) {
		err_log("listen socket failed!");
		return NULL;
	}

	while(1) {
		client_sock = accept(server_sock, NULL, NULL);
		if (client_sock < 0) {
			err_log("accept failed!");
			sleep(1);
			continue;
		}
                ret = recv_socket(client_sock, (void *)&cmd, sizeof(cmd));
		if(ret <  0) {
			err_log("recv data failed!");
			close(client_sock);
			continue;
		}
		if(cmd.type == CTRL_CMD_TYPE_RELOAD) {
			cmd.type = CTRL_CMD_TYPE_RSP;
			sprintf(cmd.content, "OK");
			send_socket(client_sock, (void *)&cmd, sizeof(cmd));
			close(client_sock);
			reload();
			continue;
		}

		while(slog_initialized == 0) sleep(1);

		switch(cmd.type) {
		case CTRL_CMD_TYPE_SNAP:
			ret = capture_by_name(snapshot_log_head, cmd.content);
			break;
		case CTRL_CMD_TYPE_SNAP_ALL:
			ret = capture_all(snapshot_log_head);
			break;
		case CTRL_CMD_TYPE_EXEC:
			/* not implement */
			ret = -1;
			break;
		case CTRL_CMD_TYPE_ON:
			slog_enable = 1;
			ret = start_sub_threads();
			break;
		case CTRL_CMD_TYPE_OFF:
			slog_enable = 0;
			ret = stop_sub_threads();
			sleep(3);
			break;
		case CTRL_CMD_TYPE_QUERY:
			ret = gen_config_string(cmd.content);
			break;
		case CTRL_CMD_TYPE_CLEAR:
			ret = clear_all_log();
			break;
		case CTRL_CMD_TYPE_DUMP:
			ret = dump_all_log(cmd.content);
			break;
		case CTRL_CMD_TYPE_SCREEN:
			if(cmd.content[0])
				ret = screen_shot(cmd.content);
			else {
				t = time(NULL);
				localtime_r(&t, &tm);
				sprintf(filename, "%s/%s/misc/screenshot_%02d%02d%02d.jpg",
						current_log_path, top_logdir,
						tm.tm_hour, tm.tm_min, tm.tm_sec); 
				ret = screen_shot(filename);
			}
			break;
		default:
			err_log("wrong cmd cmd: %d %s.", cmd.type, cmd.content);
			break;
		}
		cmd.type = CTRL_CMD_TYPE_RSP;
		if(ret && cmd.content[0] == 0)
			sprintf(cmd.content, "FAIL");
		else if(!ret && cmd.content[0] == 0)
			sprintf(cmd.content, "OK");
		send_socket(client_sock, (void *)&cmd, sizeof(cmd));
		close(client_sock);
	}
}

/* monitoring sdcard status thread */
static void *monitor_sdcard_fun()
{
	char *last = current_log_path;
	while( !strncmp (config_log_path, external_storage, strlen(external_storage))) {
		if(sdcard_mounted()) {
			current_log_path = external_storage;
			if(last != current_log_path)
				reload();
			last = current_log_path;
		} else {
			current_log_path = INTERNAL_LOG_PATH;
			if(last != current_log_path)
				reload();
			last = current_log_path;
		}
		sleep(TIMEOUT_FOR_SD_MOUNT);
	}
	return 0;
}

/*
 * handle top_logdir
 *
 */
static void handle_top_logdir()
{
	int ret;
	char value[PROPERTY_VALUE_MAX];

	property_get("slog.step", value, "");
	slog_start_step = atoi(value);

	ret = mkdir(current_log_path, S_IRWXU | S_IRWXG | S_IRWXO);
	if(-1 == ret && (errno != EEXIST)){
		err_log("mkdir %s failed.", current_log_path);
                exit(0);
	}

	if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH))) {
		debug_log("slog use internal storage");
		switch(slog_start_step){
		case 0:
			create_log_dir();
			capture_last_log(snapshot_log_head);
			property_set("slog.step", "1");
			break;
		case 2:
			create_log_dir();
			property_set("slog.step", "3");
			break;
		default:
			use_ori_log_dir();
			break;
		}
	} else {
		debug_log("slog use external storage");
		switch(slog_start_step){
                case 0:
			create_log_dir();
			capture_last_log(snapshot_log_head);
			property_set("slog.step", "2");
                        break;
                case 1:
			create_log_dir();
			property_set("slog.step", "3");
                        break;
                default:
			use_ori_log_dir();
                        break;
		}
	}

	capture_all(snapshot_log_head);
}

/*
 *handler log size according to internal available space
 *
 */
static void handler_internal_log_size()
{
	struct statfs diskInfo;

	if(!strncmp(current_log_path, external_storage, strlen(external_storage)))
		return;
	statfs(current_log_path, &diskInfo);
	unsigned int blocksize = diskInfo.f_bsize;
	unsigned int availabledisk = diskInfo.f_bavail * blocksize;
	debug_log("internal available %dM", availabledisk >> 20);

	/* default setting internal log size, half of available */
	internal_log_size = (availabledisk >> 20) /10;
	debug_log("set internal log size %dM", internal_log_size);
}

/*
 * 1.start running slog system(stream,snapshot,inotify)
 * 2.monitoring sdcard status
 */
static int do_init()
{
	/* handle sdcard issue */
	if(!strncmp(config_log_path, external_storage, strlen(external_storage))) {
		if(!sdcard_mounted())
			current_log_path = INTERNAL_LOG_PATH;
		else {
			/*avoid can't unload SD card*/
			sleep(TIMEOUT_FOR_SD_MOUNT);
			current_log_path = external_storage;
		}
		/* create a sdcard monitor thread */
		pthread_create(&sdcard_tid, NULL, monitor_sdcard_fun, NULL);
	} else
		current_log_path = INTERNAL_LOG_PATH;

	handle_top_logdir();

	handler_internal_log_size();

	/* all backend log capture handled by follows threads */
	start_sub_threads();

	slog_initialized = 1;

	return 0;
}

void create_pidfile()
{
	pid_t pid = 0;
	FILE *fp;
	struct stat st;
	char buffer[MAX_NAME_LEN];
	int ret;

	if(stat(PID_FILE, &st))
		goto write_pid;

	fp = fopen(PID_FILE, "r");
	if(fp == NULL) {
		err_log("can't open pid file: %s.", PID_FILE);
		exit(0);
	}
	memset(buffer, 0, MAX_NAME_LEN);
	fgets(buffer, MAX_NAME_LEN, fp);
	fclose(fp);

	pid = atoi(buffer);
	if(pid != 0) {
		sprintf(buffer, "/proc/%d/cmdline", pid);
		fp = fopen(buffer, "r");
		if (fp != NULL) {
			fgets(buffer, MAX_NAME_LEN, fp);
			if(!strncmp(buffer, "/system/bin/slog", 16)) {
				/* means daemon has started, just exit */
				err_log("Slog is running already, the quit immediately.");
				fclose(fp);
				exit(0);
			}
			fclose(fp);
		}
	}

write_pid:
	ret = mkdir(TMP_FILE_PATH, S_IRWXU | S_IRWXG | S_IRWXO);
        if (-1 == ret && (errno != EEXIST)){
                err_log("mkdir %s failed.", TMP_FILE_PATH);
                exit(0);
        }

	fp = fopen(PID_FILE, "w");
	if(fp == NULL) {
		err_log("can't open pid file: %s.", PID_FILE);
		exit(0);
	}

	fprintf(fp, "%d", getpid());
	fclose(fp);
	return;
}

static void sig_handler1(int sig)
{
	err_log("get a signal %d.", sig);
	unlink(PID_FILE);
	exit(0);
}

static void sig_handler2(int sig)
{
	err_log("get a signal %d.", sig);
	return;
}

/*
 * setup_signals - initialize signal handling.
 */
static void setup_signals()
{
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

#define SIGNAL(s, handler)      do { \
		act.sa_handler = handler; \
		if (sigaction(s, &act, NULL) < 0) \
			err_log("Couldn't establish signal handler (%d): %m", s); \
	} while (0)

        SIGNAL(SIGTERM, sig_handler1);
	SIGNAL(SIGBUS, sig_handler1);
	SIGNAL(SIGSEGV, sig_handler1);
	SIGNAL(SIGHUP, sig_handler1);
	SIGNAL(SIGQUIT, sig_handler1);
	SIGNAL(SIGABRT, sig_handler1);
	SIGNAL(SIGILL, sig_handler1);

	SIGNAL(SIGFPE, sig_handler2);
	SIGNAL(SIGPIPE, sig_handler2);
	SIGNAL(SIGALRM, sig_handler2);

	return;
}

/*
 * the main function
 */
int main(int argc, char *argv[])
{
	/* become a background service */
/*	if(daemon(0, 0)){
		err_log("Can't start Slog daemon.");
		exit(0);
	}
*/
	err_log("Slog begin to work.");

	/* pid file */
	create_pidfile();

	/* handle signal */
	setup_signals();

	/*find the external storage environment*/
	init_external_storage();

	/* read and parse config file */
	parse_config();

	/* even backend capture threads disabled, we also accept user command */
	pthread_create(&command_tid, NULL, command_handler, NULL);

	/* backend capture threads started here */
	do_init();

	while(1) sleep(10);
	return 0;
}
