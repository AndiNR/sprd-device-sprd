
#include "nvitem_common.h"
#include "nvitem_channel.h"
#include "nvitem_sync.h"
#include "nvitem_packet.h"
#include "nvitem_os.h"
#include "nvitem_buf.h"

static void *pSaveTask(void* ptr)
{
	do
	{
		waiteEvent();
		printf("pSaveTask up\n");
		saveToDisk();
	}while(1);
	return 0;
}

int main(void)
{
#ifndef WIN32
	pthread_t pTheadHandle;
#endif
	initEvent();
	initBuf();

//---------------------------------------------------
#ifndef WIN32
// create another task
	pthread_create(&pTheadHandle, NULL, (void*)pSaveTask, NULL);
#endif
//---------------------------------------------------

	do
	{
		channel_open();
		printf("NVITEM:channel open\n");
		_initPacket();
		_syncInit();
		syncAnalyzer();
		printf("NVITEM:channel close\n");
		channel_close();
	}while(1);
	return 0;
}


