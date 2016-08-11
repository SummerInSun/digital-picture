#include <page_manag.h>
#include <render.h>
#include <pic_operation.h>
#include <stdlib.h>
#include <file.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

static pthread_t g_tAutoPagePlayThreadId;
static pthread_mutex_t g_tAutoPagePlayThreadMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_bAutoPlayThreadShouldExit = 0;
static struct PageConfig g_tAutoPageConfig;

/* 做简单一点，只连播当前目录下面的图片文件 */
static int g_iCurFileNum = 0;
static int g_iFileNumTotal = 0;
static struct DirContent *g_atAutoPageDirContent;

static void AutoRunPage(struct PageIdetify *ptParentPageIdentify);
static int AutoGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent);

static struct PageOpr g_tAutoPageOpr = {
	.name = "auto",
	.RunPage = AutoRunPage,
	.GetInputEvent = AutoGetInputEvent,
};

static void ResetAutoPlayFile(void)
{
	g_iCurFileNum = 0;
	g_iFileNumTotal = 0;
}

/* 参数为 1 的时候是必须得到一个缓冲区，马上显示到 LCD 上面 */
static struct VideoMem *PrepareNextPicture(int bCur)
{
	int iXres, iYres, iBpp;
	char strTmp[256];
	struct VideoMem *ptVideoMem;
	struct PiexlDatasDesc tOriginPiexlDatas;
	struct PiexlDatasDesc tZoomPiexlDatas;
	int iTopLeftX, iTopLeftY;
	int iError = 0;
	float iRatio = 0;
	
	GetDisDeviceSize(&iXres, &iYres, &iBpp);

	ptVideoMem = GetVideoMem(GetID("auto"), bCur);
	if(NULL == ptVideoMem){
		DebugPrint(DEBUG_ERR"Can't get video memory in PrepareNextPicture\n");
		return NULL;
	}

	while(1){	

		DebugPrint(DEBUG_DEBUG"g_iCurFileNum = %d\n", g_iCurFileNum);
		
		if(g_atAutoPageDirContent[g_iCurFileNum].eFileType == NORMAL_FILE){
			snprintf(strTmp, 256, "%s/%s", g_tAutoPageConfig.strDirName, g_atAutoPageDirContent[g_iCurFileNum].strDirName);
			strTmp[255] = '\0';
			if(isPictureSupported(strTmp)){
				g_iCurFileNum ++;
				g_iCurFileNum = g_iCurFileNum % g_iFileNumTotal;
				break;
			}
		}

		g_iCurFileNum ++;
		g_iCurFileNum = g_iCurFileNum % g_iFileNumTotal;
	}
	iError = GetPiexlDatasForPic(strTmp, &tOriginPiexlDatas);
	if(iError){
		DebugPrint(DEBUG_ERR"Can't GetPiexlDatasForPic int PrepareNextPicture\n");
		return NULL;
	}

	tZoomPiexlDatas.iBpp    = tOriginPiexlDatas.iBpp;
	tZoomPiexlDatas.iWidth  = tOriginPiexlDatas.iWidth;
	tZoomPiexlDatas.iHeight = tOriginPiexlDatas.iHeight;
	/* 
	 * 计算宽高比，有两种情况，一种是图片宽比高大，一种与之相反
	 * 两种分别比较屏幕的宽与图片的宽，和屏幕的高与图片的高
	 */
	if(tZoomPiexlDatas.iWidth > tZoomPiexlDatas.iHeight){
		if(tZoomPiexlDatas.iWidth > iXres){
			iRatio = (float)tZoomPiexlDatas.iWidth / tZoomPiexlDatas.iHeight;
			tZoomPiexlDatas.iWidth  = iXres;
			tZoomPiexlDatas.iHeight = tZoomPiexlDatas.iWidth / iRatio;
			if(tZoomPiexlDatas.iHeight > iYres){
				tZoomPiexlDatas.iHeight = iYres;
			}
		}
	}else if(tZoomPiexlDatas.iWidth <= tZoomPiexlDatas.iHeight){
		if(tZoomPiexlDatas.iHeight > iYres){
			iRatio = (float)tZoomPiexlDatas.iHeight / tZoomPiexlDatas.iWidth;
			tZoomPiexlDatas.iHeight = iYres;
			tZoomPiexlDatas.iWidth  = tZoomPiexlDatas.iHeight / iRatio;
			if(tZoomPiexlDatas.iWidth > iXres){
				tZoomPiexlDatas.iWidth = iXres;
			}
		}
	}

	DebugPrint(DEBUG_DEBUG"tZoomPiexlDatas.iWidth = %d\n", tZoomPiexlDatas.iWidth);
	DebugPrint(DEBUG_DEBUG"tZoomPiexlDatas.iHeight = %d\n", tZoomPiexlDatas.iHeight);
	DebugPrint(DEBUG_DEBUG"iRatio = %f\n", iRatio);

	tZoomPiexlDatas.iLineLength  = tZoomPiexlDatas.iWidth * iBpp / 8;
	tZoomPiexlDatas.iTotalLength = tZoomPiexlDatas.iLineLength * tZoomPiexlDatas.iHeight;
	tZoomPiexlDatas.pucPiexlDatasMem = malloc(tZoomPiexlDatas.iTotalLength);
	if(NULL == tZoomPiexlDatas.pucPiexlDatasMem){
		DebugPrint(DEBUG_ERR"Can't malloc memory in PrepareNextPicture\n");
		ReleaseVideoMem(ptVideoMem);
		return NULL;
	}

	PicZoomOpr(&tOriginPiexlDatas, &tZoomPiexlDatas);
	iTopLeftX = (iXres - tZoomPiexlDatas.iWidth) / 2;
	iTopLeftY = (iYres - tZoomPiexlDatas.iHeight) / 2;

	ClearVideoMemRegion(ptVideoMem, NULL, CONFIG_BACKGROUND_COLOR);

	PicMergeOpr(iTopLeftX, iTopLeftY, &tZoomPiexlDatas, &ptVideoMem->tPiexlDatas);
	free(tOriginPiexlDatas.pucPiexlDatasMem);
	free(tZoomPiexlDatas.pucPiexlDatasMem);

	return ptVideoMem;
}

static void *AutoPlayThreadFunc(void *pvoid)
{
	int bShouldExit = 0;
	struct VideoMem *ptVideoMem;

	while(1){
		pthread_mutex_lock(&g_tAutoPagePlayThreadMutex);
		bShouldExit = g_bAutoPlayThreadShouldExit;
		pthread_mutex_unlock(&g_tAutoPagePlayThreadMutex);

		if(bShouldExit){
			return NULL;
		}

		ptVideoMem = PrepareNextPicture(1);

		FlushVideoMemToDev(ptVideoMem);

		ReleaseVideoMem(ptVideoMem);
		
		sleep(g_tAutoPageConfig.iIntervalSec);
	}

	return NULL;
}

static void AutoRunPage(struct PageIdetify *ptParentPageIdentify)
{
	struct InputEvent tInputEvent;
	int iError;
	char strTmp[256];

	g_bAutoPlayThreadShouldExit = 0;

	ResetAutoPlayFile();

	GetPageConfig(&g_tAutoPageConfig);
	DebugPrint(DEBUG_DEBUG"g_tAutoPageConfig.iIntervalSec = %d\n", g_tAutoPageConfig.iIntervalSec);
	DebugPrint(DEBUG_DEBUG"g_tAutoPageConfig.strDirName = %s\n", g_tAutoPageConfig.strDirName);

	iError = GetDirContents(g_tAutoPageConfig.strDirName, &g_atAutoPageDirContent, &g_iFileNumTotal);
	if(iError){
		DebugPrint(DEBUG_ERR"Auto page GetDirContent error\n");
		return;
	}

	for(g_iCurFileNum = 0; g_iCurFileNum < g_iFileNumTotal; g_iCurFileNum ++){
		if(g_atAutoPageDirContent[g_iCurFileNum].eFileType == NORMAL_FILE){
			snprintf(strTmp, 256, "%s/%s", g_tAutoPageConfig.strDirName, g_atAutoPageDirContent[g_iCurFileNum].strDirName);
			strTmp[255] = '\0';
			if(isPictureSupported(strTmp)){
				break;
			}
		}
	}
	DebugPrint(DEBUG_DEBUG"g_iFileNumTotal = %d\n", g_iFileNumTotal);
	DebugPrint(DEBUG_DEBUG"g_iCurFileNum = %d\n", g_iCurFileNum);

	/* 如果没有找到图片文件就退出 */
	if(g_iCurFileNum >= g_iFileNumTotal){
		DebugPrint("No supported picture files in this directory\n");
		FreeDirContents(&g_atAutoPageDirContent);
		return;
	}

	pthread_create(&g_tAutoPagePlayThreadId, NULL, AutoPlayThreadFunc, NULL);

	while(1){
		iError = GetInputEvent(&tInputEvent);
		if(!iError){
			pthread_mutex_lock(&g_tAutoPagePlayThreadMutex);
			g_bAutoPlayThreadShouldExit = 1;
			pthread_mutex_unlock(&g_tAutoPagePlayThreadMutex);
			pthread_join(g_tAutoPagePlayThreadId, NULL);  /* 等待子线程退出 */

			FreeDirContents(&g_atAutoPageDirContent);
			return;
		}
	}
}

static int AutoGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

int AutoPageInit(void)
{
	return RegisterPageOpr(&g_tAutoPageOpr);
}


