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
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <sys/inotify.h>

#include "slog.h"

#include <cutils/logger.h>
#include <cutils/logd.h>
#include <cutils/sockets.h>
#include <cutils/logprint.h>
#include <cutils/event_tag_map.h>

static void gen_logfile(char *filename, struct slog_info *info)
{
	int ret;

	sprintf(filename, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
	ret = mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)){
                err_log("mkdir %s failed.", filename);
                exit(0);
        }

	sprintf(filename, "%s/%s/%s/%s.log", current_log_path, top_logdir, info->log_path, info->log_basename);
}

void record_snap(struct slog_info *info)
{
	struct stat st;
	FILE *fcmd, *fp;
	char buffer[4096];
	time_t t;
	struct tm tm;
	int ret;

	/* setup log file first */
	gen_logfile(buffer, info);
	fp = fopen(buffer, "a+");
	if(fp == NULL) {
		err_log("open file %s failed!", buffer);
		exit(0);
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
	fprintf(fp, "\n============  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
				tm.tm_year % 100,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

	/* recording... */
	while( (ret = fread(buffer, 1, 4096, fcmd)) > 0)
		fwrite(buffer, 1, ret, fp);

	fclose(fp);

        if(!strncmp(info->opt, "file", 4))
                fclose(fcmd);
        else
                pclose(fcmd);

	return;
}

void *snapshot_log_handler(void *arg)
{
	struct slog_info *info;
	int tmp, sleep_secs = 0, next_sleep_secs = 0;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return NULL;

	if(!snapshot_log_head)
		return NULL;

	snapshot_log_handler_started = 1;

	while(slog_enable) {
		info = snapshot_log_head;
		while(info) {
			/* misc_log level decided which log will be restored */
			if(info->level > misc_log->level){
				info = info->next;
				continue;
			}

			/* internal equal zero means this log will trigger by user command */
			if(info->interval == 0){
				info = info->next;
				continue;
			}

			/* "state" field unused, so we store pass time at here */
			info->state += sleep_secs;
			if(info->state >= info->interval) {
				info->state = 0;
				record_snap(info);
			}
			tmp = info->interval - info->state;
			if(tmp < next_sleep_secs || !next_sleep_secs)
				next_sleep_secs = tmp;
			info = info-> next;
		}
		/* means no snapshot will be triggered in thread */
		if(!next_sleep_secs)
			break;

		/* update sleep times */
		sleep_secs = next_sleep_secs;
		while(next_sleep_secs-- > 0 && slog_enable)
			sleep(1);
		next_sleep_secs = 0;
	}
	snapshot_log_handler_started = 0;
	return NULL;
}

void cp_file(char *path, char *new_path)
{
	FILE *fp_src, *fp_dest;
	char buffer[4096];
	int ret;

	fp_src = fopen(path, "r");
	if(fp_src == NULL) {
		err_log("open notify src file failed!");
		return;
	}
	fp_dest = fopen(new_path, "w");
	if(fp_dest == NULL) {
		err_log("open notify dest file failed!");
		return;
	}

	while( (ret = fread(buffer, 1, 4096, fp_src)) > 0)
		fwrite(buffer, 1, ret, fp_dest);

	fclose(fp_src);
	fclose(fp_dest);

	return;
}

static int capture_snap_for_notify(struct slog_info *head, char *filepath)
{
        struct slog_info *info = head;

        while(info) {
                if(info->level <= 6)
                        exec_or_dump_content(info, filepath);
                info = info->next;
        }
        return 0;
}

static void handle_notify_file(int wd, const char *name)
{
	struct slog_info *info;
	char src_file[MAX_NAME_LEN], dest_file[MAX_NAME_LEN];
	time_t t;
        struct tm tm;
	int ret;

        t = time(NULL);
        localtime_r(&t, &tm);

	info = notify_log_head;
	while(info) {
		if(wd != info->interval){
			info = info->next;
			continue;
		}

		gettimeofday(&info->current, NULL);
		if(info->current.tv_sec > info->last.tv_sec + 10) {
			info->last.tv_sec = info->current.tv_sec;
		} else {
			return;
		}

		sprintf(dest_file, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
		ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)){
			err_log("mkdir %s failed.", dest_file);
			exit(0);
		}
		sprintf(dest_file, "%s/%s/%s/%s", current_log_path, top_logdir, info->log_path, info->name);
		ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)){
                        err_log("mkdir %s failed.", dest_file);
                        exit(0);
                }

		sprintf(dest_file, "%s/%s/%s/%s/snapshot_%02d%02d%02d.log",
				current_log_path,
				top_logdir,
				info->log_path,
				info->name,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

		capture_snap_for_notify(snapshot_log_head, dest_file);

		sprintf(dest_file, "%s/%s/%s/%s/screenshot_%02d%02d%02d.jpg",
				current_log_path,
				top_logdir,
				info->log_path,
				info->name,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

		screen_shot(dest_file);

		/* for collect log */
		sleep(2);

		sprintf(src_file, "%s/%s", info->content, name);
		sprintf(dest_file, "%s/%s/%s/%s/%s_%02d%02d%02d.log",
				current_log_path,
				top_logdir,
				info->log_path,
				info->name,
				name,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec
		);
		debug_log("%s, %s", src_file, dest_file);
		cp_file(src_file, dest_file);
		return;
	}
	return;
}

void *notify_log_handler(void *arg)
{
	ssize_t size;
	char buffer[MAX_LINE_LEN];
	int notify_fd, wd, len;
	struct slog_info *info;
	struct inotify_event *event;
	int ret;
	fd_set readset;
	int result;
	struct timeval timeout;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return NULL;

	if(!notify_log_head)
		return NULL;

	notify_log_handler_started = 1;

	/* init notify list */
	notify_fd = inotify_init();
	if(notify_fd < 0) {
		err_log("inotify_init() failed!");
		notify_log_handler_started = 0;
		return NULL;
	}

	/* add all watch dir to list */
	info = notify_log_head;
	while(info) {
		/* misc_log level decided which log will be restored */
		if(info->level > misc_log->level){
			info = info->next;
                        continue;
                }
		ret = mkdir(info->content, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)){
                        err_log("mkdir %s failed.", info->content);
                        exit(0);
                }
		debug_log("notify add watch %s\n", info->content);
		wd = inotify_add_watch(notify_fd, info->content, IN_MODIFY);
		if(wd == -1) {
			err_log("inotify_add_watch failed!");
			notify_log_handler_started = 0;
			return NULL;
		}
		info->interval = wd;
		info->current.tv_sec = 0;
		info->last.tv_sec = 0;
		info = info->next;
	}

	while(slog_enable) {

		FD_ZERO(&readset);
		FD_SET(notify_fd, &readset);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		result = select(notify_fd + 1, &readset, NULL, NULL, &timeout);

		if(result == 0)
			continue;

		if(result < 0) {
			sleep(1);
			continue;
		}

		if(FD_ISSET(notify_fd, &readset) <= 0){
			continue;
                }

		/* when watched dir changed, event will be read */
		size = read(notify_fd, buffer, MAX_LINE_LEN);
		if(size <= 0) {
			debug_log("read inotify fd failed, %d.", (int)size);
			sleep(1);
			continue;
		}
		event = (struct inotify_event *)buffer;
		while(size > 0) {
			len = sizeof(struct inotify_event) + event->len;
			debug_log("notify event: wd: %d, mask %d, ret %d, len %d, file %s\n",
					event->wd, event->mask, (int)size, len, event->name);
			handle_notify_file(event->wd, event->name);
			event = (struct inotify_event *)((char *)event + len);
			size -= len;
		}
	}
	close(notify_fd);
	notify_log_handler_started = 0;
	return NULL;
}

/*
 * open log devices
 *
 */
static void open_device(struct slog_info *info, char *path)
{
	info->fd_device = open(path, O_RDONLY);
	if(info->fd_device < 0){
		err_log("Unable to open log device '%s', close '%s' log.", path, info->name);
		info->state = SLOG_STATE_OFF;
	}

	return;
}

/*
 * open output file
 *
 */
static int gen_outfd(struct slog_info *info)
{
	int fd;
	char buffer[MAX_NAME_LEN];

	gen_logfile(buffer, info);
	fd = open(buffer, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
	if(fd < 0){
		err_log("Unable to open file %s.",buffer);
		exit(0);
	}

	return fd;
}

/*
 *
 *
 */
static char* mstrncpy(char* dest, const char* src, size_t pos)
{
	strncpy(dest, src, pos);
	dest += pos;
	return dest;
}

/*
 * get current time and return strings
 *
 */
static void obtime(char *src, int len)
{
	time_t when;
	struct tm* tm;
	struct tm tmBuf;
	struct timeval tv;
	int ret;

	/* add timestamp */
	when = time(NULL);
	gettimeofday(&tv, NULL);
	tm = localtime_r(&when, &tmBuf);
	ret = strftime(src, len, "%m-%d %H:%M:%S", tm);
	sprintf(src + ret, ".%03d ", tv.tv_usec / 1000);
	return;
}

/*
 * search character '\n' in dest, insert "src" before all of '\n' into "dest"
 *
 */
static void strinst(char* dest, char* src)
{
	char *pos1, *pos2, *p;
	char time[MAX_NAME_LEN];

	p = dest;
	pos2 = src;

	obtime(time, sizeof(time));
	p = mstrncpy(p, time, strlen(time));
	pos1 = strchr(pos2, '\n');

	while(pos1 != NULL){
		p = mstrncpy(p, pos2, pos1 - pos2 + 1);
		if(strchr(pos1 + 1, '\n') == NULL)
			break;
		p = mstrncpy(p, time, strlen(time));
		pos2 = pos1 + 1;
		pos1 = strchr(pos2, '\n');
	}

	return;
}
/*
 *  File volume
 *
 *  When the file is written full, rename file to file.1
 *  and rename file.1 to file.2, and so on.
 */
static void rotatelogs(struct slog_info *info)
{
	int err, i;
	char buffer[MAX_NAME_LEN];

	gen_logfile(buffer, info);

	close(info->fd_out);

	for (i = MAXROLLLOGS ; i > 0 ; i--) {
		char *file0, *file1;

		asprintf(&file1, "%s.%d", buffer, i);

		if (i - 1 == 0) {
			asprintf(&file0, "%s", buffer);
		} else {
			asprintf(&file0, "%s.%d", buffer, i - 1);
		}

		err = rename (file0, file1);

		if (err < 0 && errno != ENOENT) {
			perror("while rotating log files");
		}

		free(file1);
		free(file0);
	}

	info->fd_out = gen_outfd(info);

	info->outbytecount = 0;

}

/*
 * Handler log file size according to the configuration file.
 *
 *
 */
static void log_size_handler(struct slog_info *info)
{
	if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)) ) {
		if(info->outbytecount >= internal_log_size * 1024 * 1024) {
                        lseek(info->fd_out, 0, SEEK_SET);
                        info->outbytecount = 0;
                }
		return;
	}
	if(info->max_size == 0){
		if(info->outbytecount >= DEFAULT_MAX_LOG_SIZE * 1024 * 1024)
			rotatelogs(info);
	} else {
		if(info->outbytecount >= info->max_size * 1024 * 1024) {
			lseek(info->fd_out, 0, SEEK_SET);
			info->outbytecount = 0;
		}
	}
}

static AndroidLogFormat * g_logformat;

void *stream_log_handler(void *arg)
{
	struct slog_info *info;
	int max = 0, ret, result;
	fd_set readset_tmp, readset;
	char buf[LOGGER_ENTRY_MAX_LEN+1], buf_kmsg[LOGGER_ENTRY_MAX_LEN], wbuf_kmsg[LOGGER_ENTRY_MAX_LEN *2];
	struct logger_entry *entry;
	AndroidLogEntry entry_write;
	static AndroidLogPrintFormat format;
	char devname[MAX_NAME_LEN];
	struct timeval timeout;

	stream_log_handler_started = 1;

	info = stream_log_head;
	/*open all of the stream devices*/
	while(info){
		if(info->state != SLOG_STATE_ON){
			info = info->next;
			continue;
		}
		if(!strncmp(info->name, "kernel", 6)) {
			open_device(info, KERNEL_LOG_SOURCE);
			info->fd_out = gen_outfd(info);
		} else if(!strncmp(info->name, "modem", 5)) {
			if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)) ) {
				info->state = SLOG_STATE_OFF;
				info = info->next;
				continue;
			}
			open_device(info, MODEM_LOG_SOURCE);
			info->fd_out = gen_outfd(info);
		} else {
			sprintf(devname, "%s/%s", "/dev/log", info->name);
			open_device(info, devname);
			info->fd_out = gen_outfd(info);
		}

		FD_SET(info->fd_device, &readset_tmp);

		/*find the max fd*/
		if(info->fd_device > max)
			max = info->fd_device;
		info = info->next;
	}

	/*Initialize android log format*/
	g_logformat = android_log_format_new();
	format = android_log_formatFromString("threadtime");
	android_log_setPrintFormat(g_logformat, format);

	while(slog_enable) {
		FD_ZERO(&readset);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		memcpy(&readset, &readset_tmp, sizeof(readset_tmp));
		result = select(max + 1, &readset, NULL, NULL, &timeout);

		/* timeout */
		if (result == 0)
			continue;

		/* error */
		if (result < 0) {
			sleep(1);
			continue;
		}

		info = stream_log_head;
		while(info) {

			if(info->state != SLOG_STATE_ON){
				info = info->next;
				continue;
			}

			if(FD_ISSET(info->fd_device, &readset) <= 0){
				info = info->next;
				continue;
			}

			if(!strncmp(info->name, "kernel", 6)){
				memset(buf_kmsg, 0, LOGGER_ENTRY_MAX_LEN);
				memset(wbuf_kmsg, 0, LOGGER_ENTRY_MAX_LEN *2);
				ret = read(info->fd_device, buf_kmsg, LOGGER_ENTRY_MAX_LEN);
				if(ret <= 0) {
					err_log("read %s log failed!", info->name);
					info = info->next;
					continue;
				}
				strinst(wbuf_kmsg, buf_kmsg);

				do {
					ret = write(info->fd_out, wbuf_kmsg, strlen(wbuf_kmsg));
				} while (ret < 0 && errno == EINTR);

				if((size_t)ret < strlen(wbuf_kmsg)) {
					err_log("write %s log partial (%d of %d)", info->name, ret, strlen(wbuf_kmsg));
					info = info->next;
					continue;
				}

				info->outbytecount += ret;
				log_size_handler(info);
			} else if(!strncmp(info->name, "modem", 5)) {
				memset(buf_kmsg, 0, LOGGER_ENTRY_MAX_LEN);
				ret = read(info->fd_device, buf_kmsg, LOGGER_ENTRY_MAX_LEN);
				if (ret == 0) {
					close(info->fd_device);
					open_device(info, MODEM_LOG_SOURCE);
					info = info->next;
					continue;
				} else if (ret < 0) {
					err_log("read %s log failed!", info->name);
					info = info->next;
					continue;
				}

				do {
					ret = write(info->fd_out, buf_kmsg, strlen(buf_kmsg));
				} while (ret < 0 && errno == EINTR);

				info->outbytecount += ret;
				log_size_handler(info);
			} else {
				ret = read(info->fd_device, buf, LOGGER_ENTRY_MAX_LEN);
				if(ret <= 0) {
					err_log("read %s log failed!", info->name);
					info = info->next;
					continue;
                                }

				entry = (struct logger_entry *)buf;
				entry->msg[entry->len] = '\0';
				/*add android log 'tag' and other format*/
				android_log_processLogBuffer(entry, &entry_write);
				ret = android_log_printLogLine(g_logformat, info->fd_out, &entry_write);

				info->outbytecount += ret;
				log_size_handler(info);
			}

			info = info->next;
		}
	}

	/* close all open fds */
	info = stream_log_head;
	while(info) {
		if(info->fd_device)
			close(info->fd_device);
		if(info->fd_out)
			close(info->fd_out);
		info = info->next;
	}
	stream_log_handler_started = 0;
	return NULL;
}

