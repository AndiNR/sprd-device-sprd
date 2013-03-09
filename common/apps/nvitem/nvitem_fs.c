
#include "nvitem_common.h"
#include "nvitem_fs.h"
#include "nvitem_config.h"

#define CRC_16_L_SEED			0x80
#define CRC_16_L_POLYNOMIAL	0x8000
#define CRC_16_POLYNOMIAL		0x1021
unsigned short __crc_16_l_calc (uint8 *buf_ptr,uint32 len)
{
	unsigned int i;
	unsigned short crc = 0;

	while (len--!=0)	{
		for (i = CRC_16_L_SEED; i !=0 ; i = i>>1){
			if ( (crc & CRC_16_L_POLYNOMIAL) !=0){
				crc = crc << 1 ;
				crc = crc ^ CRC_16_POLYNOMIAL;
			}
			else	{
				crc = crc << 1 ;
			}
			if ( (*buf_ptr & i) != 0){
				crc = crc ^ CRC_16_POLYNOMIAL;
			}
		}
		buf_ptr++;
	}
	return (crc);
}

static unsigned short calc_checksum(unsigned char *dat, unsigned long len)
{
	unsigned long checksum = 0;
	unsigned short *pstart, *pend;
	if (0 == (unsigned long)dat % 2)  {
		pstart = (unsigned short *)dat;
		pend = pstart + len / 2;
		while (pstart < pend) {
			checksum += *pstart;
			pstart ++;
		}
		if (len % 2)
			checksum += *(unsigned char *)pstart;
		} else {
		pstart = (unsigned char *)dat;
		while (len > 1) {
			checksum += ((*pstart) | ((*(pstart + 1)) << 8));
			len -= 2;
			pstart += 2;
		}
		if (len)
			checksum += *pstart;
	}
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);
	return (~checksum);
}



/*
	TRUE(1): pass
	FALSE(0): fail
*/
BOOLEAN _chkEcc(uint8* buf, uint32 size)
{
	uint16 crc,crcOri;
//	crc = __crc_16_l_calc(buf, size-2);
//	crcOri = (uint16)((((uint16)buf[size-2])<<8) | ((uint16)buf[size-1]) );

	crc = calc_checksum(buf,size-4);
	crcOri = (uint16)((((uint16)buf[size-3])<<8) | ((uint16)buf[size-4]) );

	return (crc == crcOri);
}


void _makEcc(uint8* buf, uint32 size)
{
	uint16 crc;
	//crc = __crc_16_l_calc(buf, size-2);
	crc = calc_checksum(buf,size-4);
	buf[size-4] = (uint8)(0xFF&crc);
	buf[size-3] = (uint8)(0xFF&(crc>>8));
	buf[size-2] = 0;
	buf[size-1] = 0;

	return;
}


#ifndef WIN32
static RAM_NV_CONFIG _ramdiskCfg[RAMNV_NUM+1] = 
{
        {1,	"/fixnv/fixnv.bin",				"",	0x20000	},
        {2,	"/runtimenv/runtimenv.bin",		"",	0x40000	},
        {0,	"",							"",	0		}
};

const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	FILE *fp;
	char line[512];
	uint32 cnt = 2;

	if (!(fp = fopen(RAMDISK_CFG, "r"))) {
		printf("NVITEM: use default config!\n");
		return _ramdiskCfg;
	}

	cnt = 0;
	memset(_ramdiskCfg,0,sizeof(_ramdiskCfg));
	printf("NVITEM:\tpartId\toriginImage\t\tbackupImage\t\tlength\n");
	while(fgets(line, sizeof(line), fp)) {
		line[strlen(line)-1] = '\0';
		if (line[0] == '#' || line[0] == '\0'){
		    continue;
		}
		if(-1 == sscanf(
				line,"%x %s %s %x",
				&_ramdiskCfg[cnt].partId,
				_ramdiskCfg[cnt].image_path,
				_ramdiskCfg[cnt].imageBak_path,
				&_ramdiskCfg[cnt].image_size
			)
		){
			continue;
		}
		printf("NVITEM:\t0x%8x\t%32s\t%32s\t0x%8x\n",_ramdiskCfg[cnt].partId,_ramdiskCfg[cnt].image_path,_ramdiskCfg[cnt].imageBak_path,_ramdiskCfg[cnt].image_size);
		cnt++;
		if(RAMNV_NUM <= cnt){
			printf("NVITEM: Max support %d disk, this config has too many disk item!!!\n",RAMNV_NUM);
			break;
		}
	}
	fclose(fp);
	_ramdiskCfg[cnt].partId = 0;
	printf("NVITEM get config finished! \n");
	return _ramdiskCfg;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	return (RAMDISK_HANDLE)partId;
}

int _getIdx(RAMDISK_HANDLE handle)
{
	uint32 i;
	uint32 partId = (uint32)handle;

	for(i = 0; i < sizeof(_ramdiskCfg)/sizeof(RAM_NV_CONFIG); i++){
		if(0 == _ramdiskCfg[i].partId){
			return -1;
		}
		else if(partId == _ramdiskCfg[i].partId){
			return i;
		}
	}
	return -1;
}

/*
	1 read imagPath first, if succes return , then
	2 read imageBakPath, if fail , return, then
	3 fix imagePath

	note:
		first imagePath then imageBakPath
*/
BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	int ret=0;
	int fileHandle = 0;;
	int idx;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
// 1 read origin image
	memset(buf,0xFF,size);
	fileHandle = open(_ramdiskCfg[idx].image_path, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	ret = read(fileHandle, buf, size);
	close(fileHandle);
	//check crc
	if(ret == size){
		if(_chkEcc(buf, size)){
			printf("NVITEM partId%x:origin image read success!\n",_ramdiskCfg[idx].partId);
			return 1;
		}
		printf("NVITEM partId%x:origin image ECC error!\n",_ramdiskCfg[idx].partId);
	}
	printf("NVITEM partId%x:origin image read error!\n",_ramdiskCfg[idx].partId);
// 2 read bakup image
	memset(buf,0xFF,size);
	fileHandle = open(_ramdiskCfg[idx].imageBak_path, O_RDWR, S_IRWXU | S_IRWXG | S_IRWXO);
	ret = read(fileHandle, buf, size);
	close(fileHandle);

	if(ret != size){
		printf("NVITEM partId%x:bakup image read error!\n",_ramdiskCfg[idx].partId);
		return 1;
	}
	if(!_chkEcc(buf, size)){
		printf("NVITEM partId%x:bakup image ECC error!\n",_ramdiskCfg[idx].partId);
		return 1;
	}

	fileHandle  = open(_ramdiskCfg[idx].image_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	write(fileHandle, buf, size);
	fsync(fileHandle);
	close(fileHandle);

	printf("NVITEM  partId%x:bakup image read success!\n",_ramdiskCfg[idx].partId);
	return 1;

}


/*
	1 get Ecc first
	2 write  imageBakPath
	3 write imagePath

	note:
		first imageBakPath then imagePath
*/
BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
	BOOLEAN ret;
	int fileHandle = 0;;
	int idx;

	idx = _getIdx(handle);
	if(-1 == idx){
		return 0;
	}
// 1 get Ecc
	_makEcc( buf, size);
// 2 write bakup image
	ret = 1;
	fileHandle = open(_ramdiskCfg[idx].imageBak_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(size != write(fileHandle, buf, size)){
		ret =0;
		printf("NVITEM partId%x:bakup image write fail!\n",_ramdiskCfg[idx].partId);
	}
	fsync(fileHandle);
	close(fileHandle);
// 3 write origin image
	fileHandle = open(_ramdiskCfg[idx].image_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
	if(size != write(fileHandle, buf, size)){
		printf("NVITEM partId%x:origin image write fail!\n",_ramdiskCfg[idx].partId);
		ret = 0;
	}
	fsync(fileHandle);
	close(fileHandle);
	printf("NVITEM partId%x:image write finished %d!\n",_ramdiskCfg[idx].partId,ret);
	return ret;

}

void		ramDisk_Close(RAMDISK_HANDLE handle)
{
	return;
}



#else

const RAM_NV_CONFIG ram_nv_config[] = 
{
	{RAMBSD_FIXNV_ID,			(uint8*)"D:/analyzer/commonTools/testCode/testdata/fixnv",		"",	0x20000	},
	{RAMBSD_RUNNV_ID,			(uint8*)"D:/analyzer/commonTools/testCode/testdata/runingnv",	"",	0x40000	},
	{RAMBSD_PRODUCTINFO_ID,	(uint8*)"D:/analyzer/commonTools/testCode/testdata/productnv",	"",	0x20000	},
	{0,							0,														0,	0		}
};

const RAM_NV_CONFIG*	ramDisk_Init(void)
{
	return ram_nv_config;
}

int _getIdx(uint32 partId)
{
	int i;

	for(i = 0; i < sizeof(ram_nv_config)/sizeof(RAM_NV_CONFIG); i++){
		if(partId == ram_nv_config[i].partId){
			return i;
		}
	}
	return -1;
}

RAMDISK_HANDLE	ramDisk_Open(uint32 partId)
{
	FILE* DiskImg = 0;
	uint32 idx;

	idx = _getIdx(partId);
	if(-1 == idx)
	{
		return (RAMDISK_HANDLE)0;
	}
	DiskImg = fopen((char*)ram_nv_config[idx].image_path, "wb+");
	if(NULL == DiskImg)
	{
		return 0;
	}
	fseek( DiskImg, 0, SEEK_SET );

	return (RAMDISK_HANDLE)DiskImg;
}

BOOLEAN		ramDisk_Read(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
 {
 	FILE* DiskImg = (FILE*)handle;

	fseek( DiskImg, 0, SEEK_SET );
	fread( buf, size, 1, DiskImg );

	return 1;
}


BOOLEAN		ramDisk_Write(RAMDISK_HANDLE handle, uint8* buf, uint32 size)
{
 	FILE* DiskImg = (FILE*)handle;

	fseek( DiskImg, 0, SEEK_SET );
	fwrite( buf, size, 1, DiskImg );
	fflush( DiskImg );

	return 1;
}


void			ramDisk_Close(RAMDISK_HANDLE handle)
{
 	FILE* DiskImg = (FILE*)handle;

	fclose(DiskImg);
	return;

}
#endif

