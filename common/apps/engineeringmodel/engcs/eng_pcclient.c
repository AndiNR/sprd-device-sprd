
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/resource.h>
#include "engopt.h"
#include "engat.h"
#include "engclient.h"
#include "eng_appclient.h"
#include "eng_pcclient.h"
#include "eng_sqlite.h"
#include "eng_diag.h"
#include "eng_cmd4linuxhdlr.h"
#include <termios.h>
#include <sys/ioctl.h>
#include "cutils/sockets.h"
#include "cutils/properties.h"



static int client_server_fd =-1;
static int pc_client_fd = -1;
static char eng_rspbuf[ENG_BUFFER_SIZE];
static char eng_atautobuf[ENG_BUFFER_SIZE];
static void *eng_atauto_thread(void *par);


static int eng_pcclient_init(void)
{
	struct termios ser_settings;

	pc_client_fd = open(PC_GSER_DEV, O_RDWR); 
	
	if(pc_client_fd < 0){
		ENG_LOG("%s: open %s fail [%s]\n",__FUNCTION__, PC_GSER_DEV, strerror(errno));
		return -1;
	}
	
 	tcgetattr(pc_client_fd, &ser_settings);
	cfmakeraw(&ser_settings);

	//tcsetattr(pc_client_fd, TCSANOW, &ser_settings);
	
	while((client_server_fd = eng_at_open(0)) < 0){
		ENG_LOG("%s: open server socket failed!, error[%d][%s]\n",\
			__FUNCTION__, errno, strerror(errno));
		usleep(500*1000);
	}
	return 0;
}

static int eng_atreq(int fd, char *buf, int length)
{
	int ret=0;
	int index=0;
	char cmd[ENG_BUFFER_SIZE];
 
	ENG_LOG("Call %s\n",__FUNCTION__);

	memset(cmd, 0, ENG_BUFFER_SIZE);

	if((index=eng_at2linux(buf)) >= 0) {
		ENG_LOG("%s: Handle %s at Linux\n",__FUNCTION__, buf);
		memcpy(cmd, buf, length);
		memset(buf, 0, length);
		eng_linuxcmd_hdlr(index, cmd, buf);
		ret = ENG_CMD4LINUX;
	} else {
		
		sprintf(cmd, "%d,%d,%s",ENG_AT_NOHANDLE_CMD, 1, buf);

		ENG_LOG("%s: cmd=%s\n",__FUNCTION__, cmd);
		ret = eng_at_write(fd, cmd, strlen(cmd));

		if(ret < 0) {
			ENG_LOG("%s: write cmd[%s] to server fail [%s]\n",__FUNCTION__, buf, strerror(errno));
			ret = -1; 
		} else {
			ENG_LOG("%s: write cmd[%s] to server success\n",__FUNCTION__, buf);
		}

		ret = ENG_CMD4MODEM;
	}
	
	return ret;
}

static int restart_gser(void)
{
	struct termios ser_settings;
	
	ENG_LOG("%s ERROR : %s\n", __FUNCTION__, strerror(errno));

	close(pc_client_fd);

	ENG_LOG("reopen serial\n");
	pc_client_fd = open(PC_GSER_DEV,O_RDWR);
	if(pc_client_fd < 0) {
		ENG_LOG("cannot open vendor serial\n");
		return -1;
	}
	
 	tcgetattr(pc_client_fd, &ser_settings);
	cfmakeraw(&ser_settings);

	//tcsetattr(pc_client_fd, TCSANOW, &ser_settings);
	
	return 0;
}

static int eng_pc2client(int fd, char* databuf)
{
	char engbuf[ENG_BUFFER_SIZE];
	int i, length, ret;
	int is_continue = 1;
	int buf_len = 0;

	ENG_LOG("%s: Waitting cmd from PC fd=%d\n", __FUNCTION__, fd);
	if(fd < 0) {
		ENG_LOG("%s: Bad fd",__FUNCTION__);
		return -1;
	}

	do{
		memset(engbuf, 0, ENG_BUFFER_SIZE);
		length = read(fd, engbuf, ENG_BUFFER_SIZE);
		if (length <= 0) {
                        ENG_LOG("%s: read length error\n",__FUNCTION__);
			return -1;
		}
		else{
                        ENG_LOG("%s: length = %d\n",__FUNCTION__, length);
			for(i=0;i<length;i++){
				ENG_LOG("%s: %x %c\n",__FUNCTION__, engbuf[i],engbuf[i]);
				if ( engbuf[i] == 0xd ){ //\r
					continue;
				}
                                else if (engbuf[i] == 0xa && length ==1) { // only \n
                                        ENG_LOG("%s: only \\n,do nothing\n",__FUNCTION__);
					continue;
				}
				else if ( engbuf[i] == 0xa || engbuf[i]==0x1a ||buf_len >= ENG_BUFFER_SIZE){ //\n ^z
					is_continue = 0;
					break;
				}
				else{
					databuf[buf_len]=engbuf[i];
					buf_len ++;
				}
			}
		}
	}while(is_continue);

#if 1
	ENG_LOG("%s: buf[%d]=%s\n",__FUNCTION__, length, databuf);
	for(i=0; i<length; i++) {
		ENG_LOG("0x%x, ",databuf[i]);
	}
	ENG_LOG("\n");
#endif	

	return 0;
}


static int eng_modem2client(int fd, char * databuf, int length)
{
	int counter=0;

	ENG_LOG("%s",__FUNCTION__);
	counter=eng_at_read(fd, databuf, length);
	ENG_LOG("%s[%d]:%s",__FUNCTION__, counter, databuf);
	return counter;
}

static int eng_modem2pc(int client_server_fd, int pc_client_fd, char *databuf, int length)
{
	int len;
	

	memset(databuf, 0, length);
	ENG_LOG("%s: Waitting AT response from Server\n", __FUNCTION__);

	len = eng_modem2client(client_server_fd, databuf, length);

	ENG_LOG("%s: eng_rspbuf[%d]=%s\n",__FUNCTION__, \
		len, \
		databuf);

	write(pc_client_fd, databuf, len);

	return 0;
}

static int eng_linux2pc(int pc_client_fd, char *databuf)
{
	int len;

	ENG_LOG("%s: eng response = %s\n", __FUNCTION__, databuf);
	len = write(pc_client_fd, databuf, strlen(databuf));
	if (len <= 0) {
		restart_gser();
	}

	return 0;
}

#define VLOG_PRI  -20
static void set_vlog_priority(void)
{
	int inc = VLOG_PRI;
	int res = 0;

	errno = 0;
	res = nice(inc);
	if (res < 0){
		printf("cannot set vlog priority, res:%d ,%s\n", res,
				strerror(errno));
		return;
	}
	int pri = getpriority(PRIO_PROCESS, getpid());
	printf("now vlog priority is %d\n", pri);
	return;
}




static void *eng_pcclient_hdlr(void *_param)
{
	char databuf[ENG_BUFFER_SIZE];
	int fd, ret;
	int status;
	
	ENG_LOG("%s: Run",__FUNCTION__);
	for( ; ; ){
		memset(databuf, 0, ENG_BUFFER_SIZE);

		//read cmd from pc to client
		if(eng_pc2client(pc_client_fd, databuf)==-1) {
			restart_gser();
			continue;
		}

		//write cmd from client to modem
		status = eng_atreq(client_server_fd, databuf, ENG_BUFFER_SIZE);
 
		//write response from client to pc
		switch(status) {
			case ENG_CMD4LINUX:
				eng_linux2pc(pc_client_fd, databuf);
				break;
			case ENG_CMD4MODEM:
				eng_modem2pc(client_server_fd, pc_client_fd, databuf, ENG_BUFFER_SIZE);
				break;
		}
	}

	return NULL;
}

int eng_get_csclient_fd(void)
{
	return client_server_fd;
}
int eng_atcali_hdlr(char* buf)
{
	int index;
	ENG_LOG("%s: %s",__FUNCTION__, buf);
	if((index=eng_at2linux(buf))>=0) {
		memset(eng_rspbuf, 0, ENG_BUFFER_SIZE);
		eng_linuxcmd_hdlr(index, buf, eng_rspbuf);	
		return 0;
	}
	return -1;
}

static void eng_atcali_thread(void)
{
	int fd, n, ret;
	fd_set readfds;

	ENG_LOG("%s",__FUNCTION__);

	while((client_server_fd = eng_at_open(0)) < 0){
		ENG_LOG("%s: open server socket failed!, error[%d][%s]\n",\
			__FUNCTION__, errno, strerror(errno));
		usleep(500*1000);
	}

	eng_atauto_thread(NULL);
}

static void *eng_atauto_thread(void *par)
{
	int fd, usb_fd, n, j, ret;
	int usb_status=0;
	char buffer[32];
	fd_set readfds;

	while((fd=open(ENG_ATAUTO_DEV, O_RDWR|O_NONBLOCK)) < 0){
		ENG_LOG("%s: open %s failed!, error[%d][%s]\n",\
			__func__,  ENG_ATAUTO_DEV, errno, strerror(errno));
		usleep(500*1000);
	}

	usb_fd = open(ENG_USBIN, O_RDONLY);
	ENG_LOG("%s: Thread Start\n",__FUNCTION__);

	for( ; ; ) {
		
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);	
		
		n = select(fd+1, &readfds, NULL, NULL, NULL);
		ENG_LOG("%s: At Auto Report\n",__func__);
		if(n > 0) {
			memset(eng_atautobuf, 0, ENG_BUFFER_SIZE);
			n = read(fd, eng_atautobuf, ENG_BUFFER_SIZE);
			ENG_LOG("%s: eng_atautobuf=%s; usb_fd=%d\n",__func__, eng_atautobuf, usb_fd);
			if (n > 0) {
				j = eng_atcali_hdlr(eng_atautobuf);
				ENG_LOG("%s: j=%d",__FUNCTION__,j);
				if(j==0) {
					ENG_LOG("%s: Handle Auto AT",__FUNCTION__);
				} else {
					if(usb_fd>0) {
						memset(buffer, 0, sizeof(buffer));
						lseek(usb_fd, 0, SEEK_SET);
						ret = read(usb_fd, buffer, sizeof(buffer));
						ENG_LOG("%s: %s",__func__, buffer);
						if(strncmp(buffer, ENG_USBCONNECTD, strlen(ENG_USBCONNECTD))==0)
							usb_status = 1;
					}
					ENG_LOG("%s: usb_status=%d\n",__func__, usb_status);
					if(usb_status > 0) {
						ENG_LOG("%s: write at auto report to PC\n",__func__);
						write(pc_client_fd, eng_atautobuf, n);
						usb_status = 0;
					}
				}
			}
		} else {
			usleep(500*1000);
			ENG_LOG("%s: select error %d\n",__func__, n);
		}
	}

	if(usb_fd > 0)
		close(usb_fd);

	if(fd > 0)
		close(fd);

	return NULL;
}


#define MODEM_SOCKET_NAME	"modemd"
#define MODEM_SOCKET_BUFFER_SIZE	128
static void *eng_modemreset_thread(void *par)
{
    int soc_fd,pipe_fd, n, ret, status;
	char cmdrst[2]={'z',0x0a};
	char modemrst_property[8];
	char buffer[MODEM_SOCKET_BUFFER_SIZE];

	memset(modemrst_property, 0, sizeof(modemrst_property));
	property_get(ENG_MODEMRESET_PROPERTY, modemrst_property, "");
	n = atoi(modemrst_property);
	ALOGD("%s: %s is %s, n=%d\n",__func__, ENG_MODEMRESET_PROPERTY, modemrst_property,n);

	if(n!=1) {
		ALOGD("%s: Modem Won't Reset after assert\n",__func__);
		return NULL;
	}
	
	pipe_fd = open("/dev/vbpipe0",O_WRONLY);
	if(pipe_fd < 0) {
		ALOGD("%s: cannot open vpipe0\n",__func__);
		return NULL;	
	}
	
    soc_fd = socket_local_client( MODEM_SOCKET_NAME,
                         ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);

	while(soc_fd < 0) {
		ALOGD("%s: Unable bind server %s, waiting...\n",__func__, MODEM_SOCKET_NAME);
		usleep(10*1000);
    	soc_fd = socket_local_client( MODEM_SOCKET_NAME,
                         ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);		
	}

	ALOGD("%s, fd=%d, pipe_fd=%d\n",__func__, soc_fd, pipe_fd);

	for(;;) {
		memset(buffer, 0, MODEM_SOCKET_BUFFER_SIZE);
		ALOGD("%s: waiting for server %s\n",__func__, MODEM_SOCKET_NAME);
		usleep(10*1000);
		n = read(soc_fd, buffer, MODEM_SOCKET_BUFFER_SIZE);
		ALOGD("%s: get %d bytes %s\n", __func__, n, buffer);
		if(n>0) {
			if(strstr(buffer, "Assert") != NULL) {
				n = write(pipe_fd, cmdrst, 2);
				ALOGD("%s: write vbpip %d bytes RESET Modem\n",__func__, n);
			}
		}
	}

	close(soc_fd);
	close(pipe_fd);
}

void eng_check_factorymode_fornand(void)
{
	int ret;
	int fd;
	int status = eng_sql_string2int_get(ENG_TESTMODE);
	char status_buf[8];
	char config_property[64];


#ifdef USE_BOOT_AT_DIAG
	ENG_LOG("%s: status=%x\n",__func__, status);
	property_get("persist.sys.usb.config", config_property, "");
	if((status==1)||(status == ENG_SQLSTR2INT_ERR)) {
		fd=open(ENG_FACOTRYMODE_FILE, O_RDWR|O_CREAT|O_TRUNC);
		if(fd > 0)
			close(fd);
		if (strstr(config_property, "adb")) { 
			property_set("sys.usb.config","adb,vser,gser");
			property_set("persist.sys.usb.config","mass_storage,adb,vser,gser");
		} else {
			property_set("sys.usb.config","vser,gser");
			property_set("persist.sys.usb.config","vser,gser");
		}
	} else if (status == 0) {
		if (strstr(config_property, "vser,gser")) {
			if (strstr(config_property, "adb")) {
				property_set("sys.usb.config","adb");
				property_set("persist.sys.usb.config","adb");
			} else {
				property_set("sys.usb.config","");
				property_set("persist.sys.usb.config","");
			}
		} 
		remove(ENG_FACOTRYMODE_FILE);
	} else {
		remove(ENG_FACOTRYMODE_FILE);
	}
#endif

	fd=open(ENG_FACOTRYSYNC_FILE, O_RDWR|O_CREAT|O_TRUNC);
	if(fd > 0)
		close(fd);
}


void eng_check_factorymode_formmc(void)
{
	int ret;
	int fd;
	int status = eng_sql_string2int_get(ENG_TESTMODE);
	char status_buf[8];

	do {
		usleep(100*1000);
		memset(status_buf, 0, sizeof(status_buf));
		property_get(RAWDATA_PROPERTY, status_buf, "");
		ret = atoi(status_buf);
		LOGD("%s: %s is %s, n=%d\n",__FUNCTION__, RAWDATA_PROPERTY, status_buf,ret);
	}while(ret!=1);
	
	fd=open(ENG_FACOTRYMODE_FILE, O_RDWR|O_CREAT|O_TRUNC);

	LOGD("%s: fd=%d, status=%x\n",__FUNCTION__, fd, status);

	if(fd >= 0) {
		if((status==1)||(status == ENG_SQLSTR2INT_ERR)) {
			sprintf(status_buf, "%s", "1");
		} else if (status == 0) {
			sprintf(status_buf, "%s", "0");
		} else {
			sprintf(status_buf, "%s", "0");
		}

		ret = write(fd, status_buf, strlen(status_buf)+1);

		LOGD("%s: write %d bytes to %s",__FUNCTION__, ret, ENG_FACOTRYMODE_FILE);

		close(fd);
	}

	fd=open(ENG_FACOTRYSYNC_FILE, O_RDWR|O_CREAT|O_TRUNC);
	if(fd > 0)
		close(fd);
}

void eng_ctpcali(void)
{
	int fd, ret;
	char cali_cmd[]="1";
	char cali_note[]="/sys/devices/platform/sc8810-i2c.2/i2c-2/2-005c/calibrate";

	fd = open(cali_note, O_RDWR);

	ALOGD("%s: fd=%d",__FUNCTION__, fd);
	if(fd >= 0) {
		ret = write(fd, cali_cmd, strlen(cali_cmd));
		ALOGD("%s: ret=%d",__FUNCTION__, ret);
		if(ret == strlen(cali_cmd))
			ALOGD("%s: Success!",__FUNCTION__);
		else
			ALOGD("%s: Fail!",__FUNCTION__);
	}

}


int main(void)
{
	int fd, rc, califlag=0;
	int engtest=0;
	char cmdline[ENG_CMDLINE_LEN];
    eng_thread_t t1,t2, t3,t4, t5;

#if 0
	int index;
	index = eng_sql_string2int_get("index");
	if(index == ENG_SQLSTR2INT_ERR){
		index = 0;
	} else {
		index++;
	}
	eng_sql_string2int_set("index", index);
	memset(cmdline, 0, ENG_CMDLINE_LEN);
	sprintf(cmdline, "logcat > /data/eng_%d.log &", index);
	system(cmdline);

#endif
	eng_sqlite_create();

	memset(cmdline, 0, ENG_CMDLINE_LEN);
	fd = open("/proc/cmdline", O_RDONLY);
	if(fd > 0) {
		rc = read(fd, cmdline, sizeof(cmdline));
		ENG_LOG("ENGTEST_MODE: cmdline=%s\n", cmdline);
		if(rc > 0) {
			if(strstr(cmdline,ENG_CALISTR) != NULL)
				califlag = 1;
			if(strstr(cmdline,"engtest") != NULL)
				engtest = 1;
		}
	}
	ALOGD("eng_pcclient califlag=%d, engtest=%d\n",califlag, engtest);

	if(engtest == 1) {
		eng_ctpcali();
	}

	if(califlag == 1){ //at handler in calibration mode
		eng_atcali_thread();
		return 0;
	}

#ifdef CONFIG_EMMC
	eng_check_factorymode_formmc();
#else
	eng_check_factorymode_fornand();
#endif

	set_vlog_priority();
	
	if (0 != eng_thread_create( &t1, eng_vlog_thread, NULL)){
		ENG_LOG("vlog thread start error");
	}

	if (0 != eng_thread_create( &t2, eng_vdiag_thread, NULL)){
		ENG_LOG("vdiag thread start error");
	}

	if (0 != eng_thread_create( &t3, eng_modemreset_thread, NULL)){
		ENG_LOG("vdiag thread start error");
	}

	if (0 != eng_thread_create( &t4, eng_sd_log, NULL)){
		ENG_LOG("sd log thread start error");
	}


	rc = eng_pcclient_init();

	if(rc == -1) {
		ENG_LOG("%s: init fail, exit\n",__func__);
		return -1;
	}

	if (0 != eng_thread_create( &t5, eng_atauto_thread, NULL)){
		ENG_LOG("atauto thread start error");
	}	

	eng_pcclient_hdlr(NULL);

	return 0;

}


