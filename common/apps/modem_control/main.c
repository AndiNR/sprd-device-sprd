#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <pthread.h>
#include <utils/Log.h>
#include <errno.h>        
#include "packet.h"

#define LOG_TAG 	"MODEMD"
#define MODEM_SOCKET_NAME	"modemstate"
#define MAX_CLIENT_NUM	10
#define MODEM_PATH	"/dev/vbpipe2"
#define	MODEM_STATE_PATH	"/sys/devices/platform/modem_interface/state"
#define MODEM_ASSERT_PATH	"/mnt/sdcard/logs4modem/assert00.bin"

#define	DLOADER_PATH	"/dev/dloader"

#define DATA_BUF_SIZE (128)
#define TEST_BUFFER_SIZE (33 * 1024)


typedef enum _MOMDEM_STATE{
	MODEM_STA_SHUTDOWN,
	MODEM_STA_BOOT,
	MODEM_STA_INIT,
	MODEM_STA_ALIVE,
	MODEM_STA_ASSERT,
	MODEM_STA_RDY,
	MODEM_STA_MAX
} MODEM_STA_E;

extern int modem_boot(void);
extern int  send_start_message(int fd,int size,unsigned long addr,int flag);
static char test_buffer[TEST_BUFFER_SIZE]={0};
static char log_data[DATA_BUF_SIZE];
static int  client_fd[MAX_CLIENT_NUM];
static MODEM_STA_E modem_state = MODEM_STA_SHUTDOWN;

static void print_log_data(char *buf, int cnt)
{
	int i;

	if (cnt > DATA_BUF_SIZE)
		cnt = DATA_BUF_SIZE;

	printf("received:\n");
	for(i = 0; i < cnt; i++) {
		printf("%c ", buf[i]);
	}
	printf("\n");
}
static int get_modem_state(char *buffer,size_t size)
{
	int modem_state_fd = 0;
	ssize_t numRead;
	modem_state_fd = open(MODEM_STATE_PATH, O_RDONLY);
	if(modem_state_fd < 0)
		return 0;
	numRead = read(modem_state_fd,buffer,size);
	close(modem_state_fd);
	if(numRead > 0){
		printf("modem_state = %s\n",buffer);
	} else{
		return 0;
	}
	return numRead;
}
static void reset_modem_state(void)
{
	int modem_state_fd = 0;
	ssize_t numRead;

	modem_state_fd = open(MODEM_STATE_PATH, O_WRONLY);
	if(modem_state_fd < 0)
		return ;
	numRead = write(modem_state_fd,"0",2);
	close(modem_state_fd);
}
static void *modemd_listenaccept_thread(void *par)
{
	int sfd, n, i;
	char buffer[64] = {0};
	unsigned short *data = (unsigned short *)buffer;
	
	for(i=0; i<MAX_CLIENT_NUM; i++)
		client_fd[i]=-1;
	
	sfd = socket_local_server(MODEM_SOCKET_NAME, 
		ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
	if (sfd < 0) {
		printf("%s: cannot create local socket server", __FUNCTION__);
		exit(-1);
	}

	for(; ;){

		printf("%s: Waiting for new connect ...", __FUNCTION__);
		if ( (n=accept(sfd,NULL,NULL)) == -1)
		{			
			printf("engserver accept error\n");
			continue;
		}

		printf("%s: accept client n=%d",__FUNCTION__, n);
		for(i=0; i<MAX_CLIENT_NUM; i++) {
			if(client_fd[i]==-1){
				client_fd[i]=n;
				printf("%s: fill %d to client[%d]\n",__FUNCTION__, n, i);
				break;
			}
		}
	}
}
static void dump_modem_memory(void)
{
	int  read_len;
	int  length=0;
	char read_buffer[4096] = {0};
	int assert_file_fd = -1;
	int modem_interface_fd = open(DLOADER_PATH, O_RDONLY);
	
	printf("dump modem assert information \n");
	assert_file_fd = open(MODEM_ASSERT_PATH,O_WRONLY|O_CREAT);
	if(assert_file_fd < 0){
		printf("open assert file failed in %s \n",__FUNCTION__);
	}
	if(modem_interface_fd > 0){
		do{
			read_len = read(modem_interface_fd,read_buffer,sizeof(read_buffer));
			printf("dump_read(0x%x ,0x%x) (0x%08x,0x%08x)\n",read_len,length,*(unsigned int *)read_buffer,*(unsigned int *)(read_buffer+256));
			if(read_len == 4)
				break;
			if(assert_file_fd > 0)
				write(assert_file_fd,read_buffer,read_len);
			length+=read_len;
		}while(1);
		close(modem_interface_fd);
		if(assert_file_fd > 0)
			close(assert_file_fd);
	} else printf("open dloader device failed in %s \n",__FUNCTION__);
}
static void dump_modem_assert_information(void)
{
	extern int open_uart_device(int mode,int speed);
	char read_buffer[4] = {0};
	char *buffer;
	int	read_len=0,length,count=0;
	int uart_fd = open_uart_device(1,921600);
	int time_out ;
	int assert_file_fd = -1;


        time_out = 0;
	if(uart_fd < 0){
		printf("open_uart_device failed in %s \n",__FUNCTION__);
		return;
	}
	assert_file_fd = open(MODEM_ASSERT_PATH,O_WRONLY|O_CREAT);
	if(assert_file_fd < 0){
		printf("open assert file failed in %s \n",__FUNCTION__);
	}
	length = sizeof(read_buffer);
	buffer = read_buffer;
	printf("dump modem assert information \n");
	read_buffer[0] = 't';
	read_buffer[1] = 0x0a;
	read_len = write(uart_fd,read_buffer,2);
	read_buffer[0]=0;
	do{
		
		read_len = read(uart_fd,buffer,length);
		if(read_len > 0){
		//	printf("read_len = %d \n",read_len);
			if(assert_file_fd > 0)
				write(assert_file_fd,buffer,read_len);
			time_out=0;
			length -= read_len;
			buffer += read_len;
		} else {
			time_out++;
			if(time_out > 10000)
				break;
		}
		if(length == 0)
		{
			printf("read_len = %d*4KB \n",count++);
			length = sizeof(read_buffer);
			buffer = read_buffer;
		}
	}while(1);
	printf("memory dump finished :0x%x  \n",count * 4096 + read_len);
	read_buffer[0] = 'O';
	write(uart_fd,read_buffer,1);
	if(assert_file_fd > 0)
		close(assert_file_fd);
}
static int translate_modem_state_message(char *message)
{
	int modem_gpio_mask;
	sscanf(message, "%d", &modem_gpio_mask);
	return modem_gpio_mask;
}
static void broadcast_modem_state(char *message,int size)
{
	int i,ret;
	for(i=0; i<MAX_CLIENT_NUM; i++) {
		if(client_fd[i] > 0) {
			printf("client_fd[%d]=%d\n",i, client_fd[i]);
			ret = write(client_fd[i], message, size);
			printf("write %d bytes to client socket [%d] to reset modem\n", ret, client_fd[i]);
		}
	}
}
static void process_modem_state_message(char *message,int size)
{
	int alive_status=0,assert_status = 0;
	int ret=0;
	ret = translate_modem_state_message(message);
	if(ret < 0)
		return;
	alive_status = ret & 0x1;
	assert_status = ret >> 1;
	printf("alive=%d assert=%d\n",alive_status,assert_status);
	switch(modem_state){
		case MODEM_STA_SHUTDOWN:
		break;
        	case MODEM_STA_INIT:
		{
			if(assert_status==0){
				if(alive_status == 1){
					modem_state = MODEM_STA_ALIVE;
					printf("modem_state0 = MODEM_STA_ALIVE\n");
					//broadcast_modem_state(message,size);
				} else {
					modem_state = MODEM_STA_ASSERT;
					printf("modem_state1 = MODEM_STA_ASSERT\n");
					//	broadcast_modem_state(message,size);
					//	dump_modem_assert_information();
					modem_state = MODEM_STA_BOOT;
				}
			}
		}
		break;
		case MODEM_STA_ASSERT:
			if((assert_status==0)&&(alive_status==0)){
				modem_state = MODEM_STA_BOOT;
				printf("modem_state2 = MODEM_STA_BOOT\n");
			}
		break;
        	case MODEM_STA_ALIVE:
			if(assert_status==1){
				modem_state = MODEM_STA_ASSERT;
				printf("modem_state3 = MODEM_STA_ASSERT\n");
				dump_modem_memory();
				//sleep(2);
				//modem_state = MODEM_STA_BOOT;
				//broadcast_modem_state(message,size);
				//dump_modem_assert_information();
				//modem_state = MODEM_STA_BOOT;
			}
		break;
		default:
		break;
	}
}
int main(int argc, char *argv[])
{
	int ret, i;
	char buf[DATA_BUF_SIZE]={0};	
	pthread_t t1;
	int modem_state_fd = open(MODEM_STATE_PATH, O_RDONLY);

	if(modem_state_fd < 0){
		printf("!!! Open %d failed, modem_reboot exit\n",MODEM_STATE_PATH);
		return 0;
	}
	close(modem_state_fd);

	pthread_create(&t1, NULL, (void*)modemd_listenaccept_thread, NULL);
	printf(">>>>>> start modem manager program ......\n");
	
	modem_state = MODEM_STA_INIT;
	do{
		memset(buf,0,sizeof(buf));
		ret = get_modem_state(buf,sizeof(buf));
		if(ret > 0){
			process_modem_state_message(buf,ret);
		}
		if(modem_state == MODEM_STA_BOOT){
			sleep(1);
			modem_boot();
			modem_state = MODEM_STA_INIT;
			//reset_modem_state();
			continue;
		}

		if(modem_state == MODEM_STA_SHUTDOWN)
			;
	}while(1);
	return 0;
}
