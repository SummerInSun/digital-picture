#include <input_manag.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

DECLARE_HEAD(g_tInputOprHead);

static struct InputEvent g_tInputEvent;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

int RegisterInputOpr(struct InputOpr *ptInputOpr)
{	
	ListAddTail(&ptInputOpr->tInputOpr, &g_tInputOprHead);

	return 0;
}

void UnregisterInputOpr(struct InputOpr *ptInputOpr)
{
	if(NULL != ptInputOpr){
		ListDelTail(&ptInputOpr->tInputOpr);
	}
}

struct InputOpr *GetInputOpr(char *pcName)
{
	
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct InputOpr *ptIOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tInputOprHead){
		ptIOTmpPos = LIST_ENTRY(ptLHTmpPos, struct InputOpr, tInputOpr);

		if(0 == strcmp(pcName, ptIOTmpPos->name)){
			return ptIOTmpPos;
		}
	}

	return NULL;
}

void ShowInputOpr(void)
{	
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct InputOpr *ptIOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tInputOprHead){
		ptIOTmpPos = LIST_ENTRY(ptLHTmpPos, struct InputOpr, tInputOpr);

		printf("%d <---> %s\n", iPTNum++, ptIOTmpPos->name);
	}
}

static void *GetInputEventThread(void *pVoid)
{
	struct InputEvent tInputEvent;
	int (*GetInputEvent)(struct InputEvent *ptInputEvent);
	GetInputEvent = (int (*)(struct InputEvent*))pVoid;

	while(1){
		if(0 == GetInputEvent(&tInputEvent)){
			/* Get lock */
			pthread_mutex_lock(&g_tMutex);

//			DebugPrint(DEBUG_DEBUG"InputCode = %d\n", tInputEvent.iInputCode);
			g_tInputEvent = tInputEvent;
			pthread_cond_signal(&g_tConVar);	/* Weak the main thread */
			pthread_mutex_unlock(&g_tMutex);	/* Release the lock */
		}
	}

	return NULL;
}

int AllInputFdInit()
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct InputOpr *ptIOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tInputOprHead){
		ptIOTmpPos = LIST_ENTRY(ptLHTmpPos, struct InputOpr, tInputOpr);

		pthread_create(&ptIOTmpPos->tThreadId, NULL, GetInputEventThread, ptIOTmpPos->GetInputEvent);
	}

	return 0;
}

int GetInputEvent(struct InputEvent *ptInputEvent)
{
	/* »ñÈ¡Ëø²¢ÐÝÃß */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_wait(&g_tConVar, &g_tMutex);

	*ptInputEvent = g_tInputEvent;
	pthread_mutex_unlock(&g_tMutex);
	
	return 0;
}

int InputDeviceInit(void)
{
	int iError = 0;
	int bisGetOneDev = 1;

	DebugPrint(DEBUG_NOTICE"Find segmentfault\n");
	
	iError = TouchScreenInit();
	if(iError){
		DebugPrint("TouchScreenInit error\n");
		//return -1;
		TouchScreenExit();
	}

	bisGetOneDev &= iError;
	DebugPrint(DEBUG_NOTICE"Find segmentfault\n");

	iError = BoardButtonInit();
	if(iError){
		DebugPrint("BoardButtonInit error\n");
		BoardButtonExit();
	}

	bisGetOneDev &= iError;

//	iError = ConsoleInit();
//	if(iError){
//		DebugPrint(DEBUG_NOTICE"ConsoleInit error\n");
//		ConsoleExit();
//	}

//	bisGetOneDev &= iError;

	if(bisGetOneDev){
		return -1;
	}

	return 0;
}

int InputDeviceExit(void)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct InputOpr *ptIOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tInputOprHead){
		ptIOTmpPos = LIST_ENTRY(ptLHTmpPos, struct InputOpr, tInputOpr);

		ptIOTmpPos->InputDeviceExit();
	}

	return 0;
}

