#include <fcntl.h>
#include <stdio.h>
#include <errno.h>        
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>      
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <time.h>
#include "packet.h"

struct image_info {
	char *image_path;
	int image_size;
	unsigned long address;
};
//#define __TEST_SPI_ONLY__
#define	MODEM_POWER_PATH	"/sys/devices/platform/modem_interface/modempower"
#define	DLOADER_PATH		"/dev/dloader"
#define	UART_DEVICE_NAME	"/dev/ttyS1"

#define FDL_DL_ADDRESS		0x40000000

#define FDL_IMAGE_PATH		"/dev/block/mmcblk0p1"
#define FDL_OFFSET		0
#define	FDL_PACKET_SIZE 	256
#define	FDL_IMAGE_SIZE  	(12*1024)
#define HS_PACKET_SIZE		0x8000

#define	DL_FAILURE		(-1)
#define DL_SUCCESS		(0)
char test_buffer[HS_PACKET_SIZE]={0};
unsigned long fdl_image_data[FDL_IMAGE_SIZE+4];

struct image_info download_image_info[]={
	{ //fixvn
		"/dev/block/mmcblk0p4",
		0x1E000,
		0x400,
	},
	{ //fixvn
		"/dev/block/mmcblk0p4",
		0x1E000,
		0x02100000,
	},
	{ //DSP code
		"/dev/block/mmcblk0p3",
		0x200000,
		0x00020000,
	},
	{ //ARM code
		"/dev/block/mmcblk0p19",
		0x009F8000,
		0x00400000,
	},
	{ //running nv
		"/dev/block/mmcblk0p6",
		0x20000,
		0x02120000,
	},
};

static int modem_interface_fd = -1;
static int boot_status = 0;
int speed_arr[] = {B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
                   B921600,B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {921600,115200,38400,  19200, 9600,  4800,  2400,  1200,  300,
        921600, 115200,38400,  19200,  9600, 4800, 2400, 1200,  300, };

static void reset_modem(void)
{
	int modem_power_fd;
	//return;
	printf("reset modem ...\n");
	modem_power_fd = open(MODEM_POWER_PATH, O_RDWR);
        if(modem_power_fd < 0)
		return;
        write(modem_power_fd,"0",2);
	sleep(2);
        write(modem_power_fd,"1",2);
        close(modem_power_fd);
}
void delay_ms(int ms)
{
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000; 
	select(0, NULL, NULL, NULL, &delay);
}
#define UART_TIMEOUT    200
static void try_to_connect_modem(int uart_fd)
{
	unsigned long hand_shake = 0x7E7E7E7E;
	int buffer[16]={0};
	char *version_string = (char *)buffer;
	char *data = version_string;
	int modem_connect = 0;
	int i,ret,delay;
	long options;
	struct timespec delay_time;

	delay_time.tv_sec = 0;
    	printf("try to connect modem(%d)......\n",boot_status);
        modem_connect = UART_TIMEOUT;

	for(;;){
		
		if(modem_connect >= UART_TIMEOUT){
        		if(boot_status == 0){
				reset_modem();
				usleep(180*1000);
			}
		}

		data = version_string;
		if(boot_status == 0){
			i=0;
			if(modem_connect >= UART_TIMEOUT){
				do{
					ret = read(uart_fd,data,1);
					i++;
					if((ret ==1)&&(*data == 0x55))
						break;
					if((i%1000) == 0)
						write(uart_fd,&hand_shake,3);
				}while(i<2000000);
				if(i >= 2000000)
					continue;
				modem_connect = 0;
			}
		}
		usleep(5*1000);
		write(uart_fd,&hand_shake,1);
		
		data = version_string;
		ret = read(uart_fd,version_string,1);
		if(ret == 1)
			printf("end %d 0x%x\n",ret,version_string[0]);
		if(ret == 1){
			if(version_string[0]==0x7E){
				modem_connect=0;
				data++;
				do{
					ret = read(uart_fd,data,1);
					if(ret == 1){ 
				 		if(*data == 0x7E){
							modem_connect = 1;	
							printf("Version string received:");
							i=0;
							do{
								printf("0x%02x",version_string[i]);
								i++;
							}while(data > &version_string[i]);
							printf("0x%02x",version_string[i]);
							printf("\n");
							break;
						}
						data++;
					}  else {
						modem_connect += 2;
					}
				
				}while(modem_connect < UART_TIMEOUT);
			} else {
				if(version_string[0] == 0x55){
					write(uart_fd,&hand_shake,3);
					modem_connect += 40;
				}
				modem_connect += 2;
			}
		}
		if(modem_connect == 1)
			break;
		modem_connect += 2;
	}
}

int download_image(int channel_fd,struct image_info *info)
{
	int packet_size;
	int image_fd;
	int read_len;
	char *buffer;
	int i,image_size;
	int count = 0;
	int ret;
	
	image_fd = open(info->image_path, O_RDONLY,0);

	if(image_fd < 0){
		printf("open file: %s error %s\n", info->image_path, strerror(errno));
		return -1;
	}

	printf("Start download image %s image_size 0x%x address 0x%x\n",info->image_path,info->image_size,info->address);
	image_size = info->image_size;
	count = (image_size+HS_PACKET_SIZE-1)/HS_PACKET_SIZE;
	ret = send_start_message(channel_fd,count*HS_PACKET_SIZE,info->address,1);
	if(ret != DL_SUCCESS){
		close(image_fd);
		return DL_FAILURE;
	}
        for(i=0;i<count;i++){
		packet_size = HS_PACKET_SIZE;
		buffer = (char *)test_buffer;
		do{
			read_len = read(image_fd,buffer,packet_size);
			if(read_len > 0){
				packet_size -= read_len;
				buffer += read_len;
			} else break;
		}while(packet_size > 0);
		if(image_size < HS_PACKET_SIZE){
			for(i=image_size;i<HS_PACKET_SIZE;i++)
				test_buffer[i] = 0xFF;
			image_size = 0;
		}else { image_size -= HS_PACKET_SIZE;}
		//memset(test_buffer,i,HS_PACKET_SIZE);
		ret = send_data_message(channel_fd,test_buffer,HS_PACKET_SIZE,1);
		if(ret != DL_SUCCESS){
			close(image_fd);
			return DL_FAILURE;
		}
	}

	ret = send_end_message(channel_fd,1);

	close(image_fd);
	return ret;
}

int download_images(int channel_fd)
{
	struct image_info *info;
	int i ,ret;
	int image_count = sizeof(download_image_info)/sizeof(download_image_info[0]);
	
	info = &download_image_info[0];
	for(i=0;i<image_count;i++){
		ret = download_image(channel_fd,info);
		if(ret != DL_SUCCESS)
			break;
		info++;
	}
	send_exec_message(channel_fd,0x400000,1);
	return ret;
}

void * load_fdl2memory(int *length)
{
	int fdl_fd;
	int read_len,size;
	char *buffer = (char *)fdl_image_data;
	
	fdl_fd = open(FDL_IMAGE_PATH, O_RDONLY,0);
	if(fdl_fd < 0){
		printf("open file %s error %s\n", FDL_IMAGE_PATH, strerror(errno));
		return NULL;
	}
	size = FDL_IMAGE_SIZE;
	do{
		read_len = read(fdl_fd,buffer,size);
		if(read_len > 0)
		{
			size -= read_len;
			buffer += read_len;
		}
	}while(size > 0);
	close(fdl_fd);
	if(length)
		*length = FDL_IMAGE_SIZE;
	return fdl_image_data;
}
static int download_fdl(int uart_fd)
{
	int size=0,ret;
	int data_size=0;
	int offset=0;
	int translated_size=0;
	int ack_size = 0;
	char *buffer,data = 0;
	char test_buffer1[256]={0};

	buffer = load_fdl2memory(&size);
	printf("fdl image info : address %p size %x\n",buffer,size);
	printf("fdl_content:  0x%x 0x%x 0x%x\n",*(int *)buffer,*(int*)(buffer+4),*(int *)(buffer+8));
	if(buffer == NULL)
		return -1;
	ret = send_start_message(uart_fd,size,FDL_DL_ADDRESS,0);
	if(ret == DL_FAILURE)
		return ret;
	while(size){
		ret = send_data_message(uart_fd,buffer,FDL_PACKET_SIZE,0);
		if(ret == DL_FAILURE)
			return ret;		
		buffer += FDL_PACKET_SIZE;
		size -= FDL_PACKET_SIZE;
	}
	ret = send_end_message(uart_fd,0);
	if(ret == DL_FAILURE)
		return ret;
	ret = send_exec_message(uart_fd,FDL_DL_ADDRESS,0);
	if(ret == DL_FAILURE)
		return ret;

	return ret;
}
static void print_log_data(char *buf, int cnt)
{
}
void set_raw_data_speed(int fd, int speed)  
{  
    unsigned long   	i;  
    int   		status;  
    struct termios   	Opt;  
   
    tcflush(fd,TCIOFLUSH);  
    tcgetattr(fd, &Opt);  
    for ( i= 0;  i  < sizeof(speed_arr) / sizeof(int);  i++){  
        if  (speed == name_arr[i])  
        {  
	    //set raw data mode 
            Opt.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
            Opt.c_oflag &= ~OPOST;
            Opt.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
            Opt.c_cflag &= ~(CSIZE | PARENB);
            Opt.c_cflag |= CS8;

	    Opt.c_iflag = ~(ICANON|ECHO|ECHOE|ISIG);
	    Opt.c_oflag = ~OPOST;
	    cfmakeraw(&Opt);
            //set baudrate
            cfsetispeed(&Opt, speed_arr[i]);  
            cfsetospeed(&Opt, speed_arr[i]);  
            status = tcsetattr(fd, TCSANOW, &Opt);  
            if  (status != 0)  
                perror("tcsetattr fd1");  
            return;  
        }  
    }  
 }

   
int open_uart_device(int polling_mode,int speed)
{
    int fd;
    if(polling_mode == 1)	
    	fd = open( UART_DEVICE_NAME, O_RDWR|O_NONBLOCK );         //| O_NOCTTY | O_NDELAY  
    else
    	fd = open( UART_DEVICE_NAME, O_RDWR);          
    if(fd > 0)
    	set_raw_data_speed(fd,speed);  
    	//set_raw_data_speed(fd,115200);  
    return fd;
}
int modem_boot(void)  
{  
    int uart_fd;
    int ret=0;
    unsigned int i;  
    int nread;  
    char buff[512]; 
    unsigned long offset = 0,step = 4*1024;

reboot_modem:    
    uart_fd = open_uart_device(1,115200);
    if(uart_fd < 0)
	return -1;  

#ifndef __TEST_SPI_ONLY__
    do{
    	boot_status = 0;
    	try_to_connect_modem(uart_fd);
    	boot_status = 1;
    	ret = send_connect_message(uart_fd,0);
    }while(ret < 0);

    ret = download_fdl(uart_fd);
    if(ret == DL_FAILURE){
	close(uart_fd);
	goto reboot_modem;
    }
    try_to_connect_modem(uart_fd);
    close(uart_fd);
    uart_fd = open_uart_device(0,115200);
    if(uart_fd< 0) 
	return -1; 
    uart_send_change_spi_mode_message(uart_fd);
#endif
    
    modem_interface_fd = open(DLOADER_PATH, O_RDWR);
    if(modem_interface_fd < 0){
	printf("open dloader device failed ......\n");
        for(;;)
        {
            	modem_interface_fd = open(DLOADER_PATH, O_RDWR);
                if(modem_interface_fd==0)
                        break;
        }
    }
    printf("open dloader device successfully ... \n");
    
    ret = download_images(modem_interface_fd);
    close(uart_fd);
    close(modem_interface_fd);
    if(ret == DL_FAILURE){
	sleep(2);
	goto reboot_modem;
    }
    
    printf("MODEM boot finished ......\n");
    return 0;
} 
