#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>  
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 12345

int iSocketFd;

struct sockaddr_in tServerAddr;
socklen_t iSockLen;

char cRecvBuf[1024];
char cSendBuf[1024];
int iSendBufLen = 0; 
int iRecvBufLen = 0;

static int g_bDebugFlag = 0;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;	// 互斥锁
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;	//条件变量

static void *RecvThread(void *pVoid)
{
	while(1){
		/* 获取锁并休眠 */
		pthread_mutex_lock(&g_tMutex);
		pthread_cond_wait(&g_tConVar, &g_tMutex);

		while(g_bDebugFlag){
			iSockLen = sizeof(struct sockaddr);
			iRecvBufLen = recvfrom(iSocketFd, cRecvBuf, sizeof(cRecvBuf), 0,
									(struct sockaddr *)&tServerAddr, &iSockLen);
			if(iRecvBufLen <= 0){
				printf("receive buffer error\n");
				break;
			}
			cRecvBuf[iRecvBufLen] = '\0';
			printf("%s", cRecvBuf);
		}

		pthread_mutex_unlock(&g_tMutex);
	}

	return NULL;
}


int main(int argc, char *argv[])
{
	int iError = 0; 

	pthread_t tRecvThreadId;

	if(argc != 2){
		printf("exe <ipaddr>\n");
		return -1;
	}
	
	iSocketFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(-1 == iSocketFd){
		printf("build socket error\n");
		return -1;
	}

	tServerAddr.sin_family      = AF_INET;	/* 属于IPv4 */
	tServerAddr.sin_port        = htons(SERVER_PORT); /* host to net, short */
	if(0 == inet_aton(argv[1], (struct in_addr *)&tServerAddr.sin_addr)){
		printf("Wrong addr\n");
		return -1;
	}

	pthread_create(&tRecvThreadId, NULL, RecvThread, 0);

	while(1){
		printf("<ctrl : d|c0|c1|n0|n1|l<0-7>|cd>\n");
		scanf("%s", cSendBuf);

		if(!strncmp(cSendBuf, "d", 1)){
			if(cSendBuf[1] != '\0'){
				goto next_circle;
			}
			sprintf(cSendBuf, "debug");
		}
		if(!strncmp(cSendBuf, "cd", 2)){
			if(cSendBuf[2] != '\0'){
				goto next_circle;
			}
			sprintf(cSendBuf, "closedebug");
		}
		if(!strncmp(cSendBuf, "l", 1)){
			if(cSendBuf[2] != '\0'){
				goto next_circle;
			}
			if(cSendBuf[1] < '0' || cSendBuf[1] > '7'){
				goto next_circle;
			}
			sprintf(cSendBuf, "debuglevel=%d", cSendBuf[1] - '0');
		}

		if(!strncmp(cSendBuf, "c", 1)){
			if(cSendBuf[2] != '\0'){
				goto next_circle;
			}
			if(cSendBuf[1] < '0' || cSendBuf[1] > '1'){
				goto next_circle;
			}
			sprintf(cSendBuf, "console=%d", cSendBuf[1] - '0');
		}
		if(!strncmp(cSendBuf, "n", 1)){
			if(cSendBuf[2] != '\0'){
				goto next_circle;
			}
			if(cSendBuf[1] < '0' || cSendBuf[1] > '1'){
				goto next_circle;
			}
			sprintf(cSendBuf, "net=%d", cSendBuf[1] - '0');
		}

		iSendBufLen =  sendto(iSocketFd, cSendBuf, 1024, 0,
                  (const struct sockaddr *)&tServerAddr, sizeof(struct sockaddr));

		if(iSendBufLen <= 0){
			printf("Send buffer error\n");
			return -1;
		}
		
		if(!strcmp(cSendBuf, "debug")){
			/* Get lock */
			pthread_mutex_lock(&g_tMutex);

			g_bDebugFlag = 1;
			
			pthread_cond_signal(&g_tConVar);	/* Weak the main thread */
			pthread_mutex_unlock(&g_tMutex);	/* Release the lock */			
		}
		if(!strcmp(cSendBuf, "closedebug")){
			g_bDebugFlag = 0;	
		}
next_circle:
		iSendBufLen = 0;
	}

	return 0;
}
