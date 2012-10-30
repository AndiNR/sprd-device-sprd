#ifndef _NVITEM_PACKAGE_H_
#define _NVITEM_PACKAGE_H_

#ifdef __cplusplus
extern "C" {
#endif 

#define NV_PACKAGE_HEADFLAG	0x5a5a5a5a

//little endian
#define ATTR_PARTITION_MASK	0xc0000000	//bit 30 31
#define ATTR_PARTITION_OFFSET	30			

#define ATTR_RW_MASK		0x20000000	//bit 29
#define ATTR_RW_OFFSET		29

#define ATTR_SYNC_MASK		0x10000000	//bit 28
#define ATTR_SYNC_OFFSET	28
	
#define ATTR_INFO_MASK		0x08000000	//bit 27
#define ATTR_INFO_OFFSET	27

#define FIXNV_BLOCK_SIZE	2*128
#define RUNNV_BLOCK_SIZE	504
#define BLOCK_SIZE		512

#define FIXNV_REALSIZE		2*120
#define RUNNV_REALSIZE		504
typedef struct _nvrequest_header_t{
        unsigned long int	flag;
        unsigned long int	attr;
        unsigned long int	offset;
        unsigned long int	size;
	unsigned long int	reserved;
} __attribute__ ((packed)) nvrequest_header_t;

#define DATA_BUF_LEN    1024
unsigned char data[DATA_BUF_LEN];
unsigned char ret_data[DATA_BUF_LEN];


#ifdef __cplusplus
}
#endif 

#endif	//_NVITEM_PACKAGE_H_
