#include <cutils/log.h>
#include <fcntl.h>
#include <time.h>


#define BUFFER_LEN		(128)

char gch;
FILE *gfp;
char gbuffer[BUFFER_LEN];

int ghour, gminute, gsecond;
time_t  gtimep;
struct tm   *gp;

void write_time(void)
{
	time(&gtimep);
  	gp = localtime(&gtimep);
	memset(gbuffer, 0, BUFFER_LEN);
	ghour = gp->tm_hour;
	gminute = gp->tm_min;
	gsecond = gp->tm_sec;
	sprintf(gbuffer, "\nH:%02d  M:%02d  S:%02d\n", ghour, gminute, gsecond);
	fputs(gbuffer, gfp);
	fflush(gfp);
}

char select_log_file(void)
{
	int ret;
	FILE *fp;
	char ch;

	ret = access("/data/index.txt", 0);
	if (ret != 0) {
		printf("/data/index.txt is not exist, create it\n");
		fp = fopen("/data/index.txt", "w+");
		if (fp == NULL) {
			printf("can not creat /data/index.txt\n");
			return 0;
		} else {
			fputc('1', fp);
			fclose(fp);
			return '1';
		}
	}

	printf("/data/index.txt is exist\n");
	fp = fopen("/data/index.txt", "r+");
	fseek(fp, 0, SEEK_SET);
	ch = fgetc(fp);
	ch ++;
	if (ch == '6')
		ch = '1';
	fseek(fp, 0, SEEK_SET);
	fputc(ch, fp);
	fclose(fp);

	return ch;
}

//extern 
int init_nvitem_log()
{
	gch = select_log_file();
	gfp = NULL;

	if (gch == '1')
		gfp = fopen("/data/1.txt", "w+");
	else if (gch == '2')
		gfp = fopen("/data/2.txt", "w+");
	else if (gch == '3')
		gfp = fopen("/data/3.txt", "w+");
	else if (gch == '4')
		gfp = fopen("/data/4.txt", "w+");
	else if (gch == '5')
		gfp = fopen("/data/5.txt", "w+");
	else
		printf("nothing is selected\n");

	if (gfp == NULL)
		//release_wake_lock ("NvItemdLock");
		return 0;
	return 1;
}
