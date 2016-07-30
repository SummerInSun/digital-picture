#include <print_manag.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>  
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

static int NetPrintDeviceInit(void);
static int NetPrintDeviceExit(void);
static int NetDoPrint(char *pcPrintBuf);

/* socket */
#define LISTEN_PORT 12345

static int g_iSocketServerFd;

static struct sockaddr_in g_tServerAddr;
static struct sockaddr_in g_tClientAddr;

static int g_iHasClientConnected = 0;

static char g_cCtrlFormClient[512]; /* 来自客户端的控制字符 */

/* thread */
static pthread_t g_NetPrintToClientThreadId;
static pthread_t g_NetGetFormClientThreadId;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;

static struct PrintOpr *g_ptCtrlPrintOpr;

/* 环形缓冲区 */
#define RECVBUFF_LEN (16*1024)
static int g_iReadPos  = 0;
static int g_iWritePos = 0;
static char *g_cRecvBuff = NULL;

struct PrintOpr g_tNetPrintOpr = {
	.name = "net-print",
	.PrintDeviceInit = NetPrintDeviceInit,
	.PrintDeviceExit = NetPrintDeviceExit,
	.DoPrint = NetDoPrint,
	.isSelected = 1,
};

static int DataIsFull()
{
	return ((g_iWritePos + 1) % RECVBUFF_LEN == g_iReadPos); 
}

static int DataIsEmpty()
{
	return (g_iWritePos == g_iReadPos);
}

static int PutData(char cData)
{
	if(DataIsFull()){
		return -1;
	}
	g_cRecvBuff[g_iWritePos] = cData;
	g_iWritePos = (g_iWritePos + 1) % RECVBUFF_LEN;

	return 0;
}

static int GetData(char *cData)
{
	if(DataIsEmpty()){
		return -1;
	}
	*cData = g_cRecvBuff[g_iReadPos];
	g_iReadPos = (g_iReadPos + 1) % RECVBUFF_LEN;

	return 0;
}

/* 数据发送线程 */
static void *NetPrintToClientThread(void *pVoid)
{
	char cSendBufPack[512];
	int iSendBufLen;
	int iSendBufNum = 0;

	while(1){
		pthread_mutex_lock(&g_tMutex);
		pthread_cond_wait(&g_tConVar, &g_tMutex);

		/* 说明接受到了 debug 信号 */
		if(g_iHasClientConnected){
			while(!DataIsEmpty()){
				while((iSendBufNum < 512) && !DataIsEmpty()){
					GetData(&cSendBufPack[iSendBufNum]);
					iSendBufNum ++;
				}
				
				iSendBufLen =  sendto(g_iSocketServerFd, cSendBufPack, iSendBufNum, 0,
							  (const struct sockaddr *)&g_tClientAddr, sizeof(struct sockaddr));
				iSendBufNum = 0;
				if(iSendBufLen <= 0){
					break;
				}	
			}
		}
		
		pthread_mutex_unlock(&g_tMutex);
	}

	return NULL;
}

/* 数据接收线程,此线程一直运行，会在recvfrom处阻塞休眠 */
static void *NetGetFromClientThread(void *pVoid)
{	
	socklen_t iSockLen;
	int iRecvBufLen;

	while(1){
		iSockLen = sizeof(struct sockaddr);
		iRecvBufLen = recvfrom(g_iSocketServerFd, g_cCtrlFormClient, sizeof(g_cCtrlFormClient), 0,
							   (struct sockaddr *)&g_tClientAddr, &iSockLen);
		if(iRecvBufLen > 0){
			g_cCtrlFormClient[iRecvBufLen] = '\0';
			/* debug	    发送打印所有的环形缓冲区里面的数据
			 * console=0  关闭串口打印
			 * console=1  开启串口打印
			 * net=0      关闭网络打印
			 * net=1      开启网络打印
			 */
			DebugPrint(DEBUG_DEBUG"Receive buff %s\n", g_cCtrlFormClient);
			if(!strncmp(g_cCtrlFormClient, "debuglevel=", 11)){
				DebugPrint(DEBUG_DEBUG"Get here\n");
				if(g_cCtrlFormClient[12] != '\0'){
					goto next_circle;
				}
				if(g_cCtrlFormClient[11] < '0' || g_cCtrlFormClient[11] > '7'){
					goto next_circle;
				}
				DebugPrint(DEBUG_DEBUG"Get here\n");				
				SetGlobalDebugLevel(g_cCtrlFormClient[11] - '0');
			}
			if(!strncmp(g_cCtrlFormClient, "debug", 5)){
				if(g_cCtrlFormClient[5] != '\0'){
					goto next_circle;
				}
				g_iHasClientConnected = 1;	/* 当收到 debug 的时候才认为有客户端的连接 */
				DebugPrint(DEBUG_DEBUG"Branch 1\n");
				DebugPrint("");
			}
			if(!strncmp(g_cCtrlFormClient, "console=", 8)){
				if(g_cCtrlFormClient[9] != '\0'){
					goto next_circle;
				}
				if(g_cCtrlFormClient[8] == '0'){
					g_ptCtrlPrintOpr = GetPrintOpr("console-print");
					if(NULL != g_ptCtrlPrintOpr){
						g_ptCtrlPrintOpr->isSelected = 0;
					}
				}
				if(g_cCtrlFormClient[8] == '1'){
					g_ptCtrlPrintOpr = GetPrintOpr("console-print");
					if(NULL != g_ptCtrlPrintOpr){
						g_ptCtrlPrintOpr->isSelected = 1;
					}
				}
				DebugPrint(DEBUG_DEBUG"Branch 2\n");
			}
			if(!strncmp(g_cCtrlFormClient, "net=", 4)){
				if(g_cCtrlFormClient[5] != '\0'){
					goto next_circle;
				}
				if(g_cCtrlFormClient[4] == '0'){
					g_ptCtrlPrintOpr = GetPrintOpr("net-print");
					if(NULL != g_ptCtrlPrintOpr){
						g_ptCtrlPrintOpr->isSelected = 0;
					}
				}
				if(g_cCtrlFormClient[4] == '1'){
					g_ptCtrlPrintOpr = GetPrintOpr("net-print");
					if(NULL != g_ptCtrlPrintOpr){
						g_ptCtrlPrintOpr->isSelected = 1;
					}
				}
				DebugPrint(DEBUG_DEBUG"Branch 3\n");
			}
			
		}
next_circle:
		iRecvBufLen = 0;
	}
	return NULL;
}

static int NetPrintDeviceInit(void)
{	
	int iError = 0;
	
	/*
	 * SOCK_STREAM : Provides sequenced, reliable, two-way, connection-based byte streams
	 *				 连续的，可靠的，双向的，字节流
	 */
	g_iSocketServerFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == g_iSocketServerFd){
		printf("build socket error\n");
		return -1;
	}

	g_tServerAddr.sin_family	  = AF_INET;	/* 属于IPv4 */
	g_tServerAddr.sin_port		  = htons(LISTEN_PORT); /* host to net, short */
	g_tServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* 监控所有的ip INADDR_ANY = 0.0.0.0*/
	/* 绑定地址到socket */
	iError = bind(g_iSocketServerFd, (const struct sockaddr *)&g_tServerAddr,
					sizeof(struct sockaddr));
	if(-1 == iError){
		printf("Bind error\n");
		return -1;
	}

	/* 创建网络数据发送线程 */
	pthread_create(&g_NetPrintToClientThreadId, NULL, NetPrintToClientThread, NULL);

	/* 创建数据接收线程 */
	pthread_create(&g_NetGetFormClientThreadId, NULL, NetGetFromClientThread, NULL);

	g_cRecvBuff = malloc(RECVBUFF_LEN);
	if(NULL == g_cRecvBuff){
		printf("Has no space for receive data\n");
		return -1;
	}
		
	return 0;
}

static int NetPrintDeviceExit(void)
{
	close(g_iSocketServerFd);
	free(g_cRecvBuff);
	
	return 0;
}

static int NetDoPrint(char *pcPrintBuf)
{
	int iBufNum = 0;

	while(pcPrintBuf[iBufNum] != '\0'){
		if(DataIsFull()){
			return -1;
		}
		PutData(pcPrintBuf[iBufNum]);
		iBufNum++;
	}

	/* 唤醒发送线程 */
	pthread_mutex_lock(&g_tMutex);
	pthread_cond_signal(&g_tConVar);
	pthread_mutex_unlock(&g_tMutex);
	
	return iBufNum;
}

int NetPrintInit(void)
{
	int iError = 0;
	
	iError = NetPrintDeviceInit();
	iError |= RegisterPrintOpr(&g_tNetPrintOpr);

	return iError;
}

void NetPrintExit(void)
{
	NetPrintDeviceExit();
	UnregisterPrintOpr(&g_tNetPrintOpr);
}

