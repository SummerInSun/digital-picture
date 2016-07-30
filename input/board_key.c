#include <input_manag.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>

static int ButtonInputDeviceExit(void);
static int ButtonInputDeviceInit(void);
static int ButtonGetInputEvent(struct InputEvent *ptInputEvent);

static int g_iButtonFd;
static int g_iButtonNum = 0;
static int g_bExitFlag = 0;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

struct InputOpr g_tButtonInputOpr = {
	.name = "button-input",
	.InputDeviceExit = ButtonInputDeviceExit,
	.InputDeviceInit = ButtonInputDeviceInit,
	.GetInputEvent   = ButtonGetInputEvent,
};

static int ButtonInputDeviceExit(void)
{
	close(g_iButtonFd);

	return 0;
}

static void BoardButtonSync(int iSigNum)
{
	static int iPressFlag = 0;

	g_iButtonNum = 0;
	read(g_iButtonFd, &g_iButtonNum, 1);
	if(g_iButtonNum == 0){
		return;
	}
	
	/* 因为按键是双边沿触发，所以要过滤掉第二次读取到的数据，这个不好，但是暂时保留 */
	if(0 == (iPressFlag % 2)){
		iPressFlag = iPressFlag + 1;
		iPressFlag = iPressFlag % 2;
	}else if(1 == (iPressFlag % 2)){
		iPressFlag = iPressFlag + 1;
		iPressFlag = iPressFlag % 2;
		return;
	}

	if((g_iButtonNum == 11) && (0 == g_bExitFlag)){
		g_bExitFlag ++;
		return;	/* 这里要等待两次按键之后再退出 */
	}
	g_bExitFlag = 0;	/* 重新记录，但是基本不会产生这样的情况 */
	
	/* Get lock */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_signal(&g_tConVar);	/* Weak the main thread */
	pthread_mutex_unlock(&g_tMutex);	/* Release the lock */
	
}

static int ButtonInputDeviceInit(void)
{
	int iOflags;

	g_iButtonFd = open("/dev/by_button", O_RDONLY);	/* 默认为阻塞方式打开 */
	if(g_iButtonFd < 0){
		DebugPrint(DEBUG_NOTICE"Open button device error\n");
		return -1;
	}

	fcntl(g_iButtonFd, F_SETOWN, getpid());
	iOflags = fcntl(g_iButtonFd, F_GETFL);
	fcntl(g_iButtonFd, F_SETFL, iOflags | FASYNC);

	signal(SIGIO, BoardButtonSync);

	return 0;
}

static int ButtonGetInputEvent(struct InputEvent *ptInputEvent)
{
	DebugPrint(DEBUG_DEBUG"Button read data\n");

	/* 获取锁并休眠 */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_wait(&g_tConVar, &g_tMutex);
	pthread_mutex_unlock(&g_tMutex);

	if(g_iButtonNum == 0){	/* 一般情况到这里就说明有了按键数据 */
		return -1;
	}
	
	gettimeofday(&ptInputEvent->tTimeVal, NULL);
	ptInputEvent->iInputType = INPUT_TYPE_BUTTON;
	ptInputEvent->iInputCode = INPUT_CODE_UNKNOW;

	if(g_iButtonNum == 1){
		ptInputEvent->iInputCode = INPUT_CODE_DOWN;
	}
	if(g_iButtonNum == 2){
		ptInputEvent->iInputCode = INPUT_CODE_UP;
	}
	if(g_iButtonNum == 11){
		ptInputEvent->iInputCode = INPUT_CODE_EXIT;
	}

	return 0;
}

int BoardButtonInit(void)
{
	int iError = 0;
	
	iError = ButtonInputDeviceInit();
	iError |= RegisterInputOpr(&g_tButtonInputOpr);

	return iError;
}

void BoardButtonExit(void)
{
	ButtonInputDeviceExit();
	UnregisterInputOpr(&g_tButtonInputOpr);
}


