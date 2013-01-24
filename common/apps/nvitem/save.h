#ifndef _NVITEM_SAVE_H_
#define _NVITEM_SAVE_H_

#ifdef __cplusplus
extern "C" {
#endif 

#ifdef CONFIG_EMMC
#define FIXNV_FILENAME	"/dev/block/mmcblk0p4"
#define RUNNINGNV_FILENAME "/dev/block/mmcblk0p6"
#else
#define FIXNV_FILENAME	"/fixnv/fixnv.bin"
#define RUNNINGNV_FILENAME "/runtimenv/runtimenv.bin"
#endif

#ifdef CONFIG_EMMC
#define BACKUP_FIXNV_FILENAME	"/dev/block/mmcblk0p5"
#define BACKUP_RUNNINGNV_FILENAME "/dev/block/mmcblk0p7"
#else
#define BACKUP_FIXNV_FILENAME	"/backupfixnv/fixnv.bin"
#define BACKUP_RUNNINGNV_FILENAME "/runtimenv/runtimenvbkup.bin"
#endif

extern int fixnv_fd;
extern int runningnv_fd;

void init_save();
void run_save();

#ifdef __cplusplus
}
#endif 

#endif	//_NVITEM_SAVE_H_
