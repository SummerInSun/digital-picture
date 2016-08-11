#include <page_manag.h>
#include <print_manag.h>
#include <render.h>
#include <stdio.h>
#include <string.h>

DECLARE_HEAD(g_tPageOprHead);

int RegisterPageOpr(struct PageOpr *ptPageOpr)
{
	ListAddTail(&ptPageOpr->tPageOpr, &g_tPageOprHead);

	return 0;
}

struct PageOpr *GetPageOpr(char *pcName)
{	
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PageOpr *ptPOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPageOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PageOpr, tPageOpr);

		if(0 == strcmp(pcName, ptPOTmpPos->name)){
			return ptPOTmpPos;
		}
	}

	return NULL;
}

void ShowPageOpr(void)
{
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PageOpr *ptPOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPageOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PageOpr, tPageOpr);

		printf("%d <---> %s\n", iPTNum++, ptPOTmpPos->name);
	}
}

int PagesInit(void)
{
	int iError;

	iError = MainPageInit();
	if(iError){
		DebugPrint("MainPageInit error\n");
		return -1;
	}

	iError = SettingPageInit();
	if(iError){
		DebugPrint("SettingPageInit error\n");
		return -1;
	}

	iError = BrowsePageInit();
	if(iError){
		DebugPrint("BrowsePageInit error\n");
		return -1;
	}

	iError = IntervalPageInit();
	if(iError){
		DebugPrint("IntervalPageInit error\n");
		return -1;
	}

	iError = AutoPageInit();
	if(iError){
		DebugPrint("AutoPageInit error\n");
		return -1;
	}

	iError = PicturePageInit();
	if(iError){
		DebugPrint("PicturePageInit error\n");
		return -1;
	}

	iError = MusicPageInit();
	if(iError){
		DebugPrint("MusicPageInit error\n");
		return -1;
	}

	return 0;
}

int GetID(char *pcName)
{
	return (int)(pcName[0] + pcName[1] + pcName[2] + pcName[3] + pcName[4]);
}

void GetPageConfig(struct PageConfig *ptPageConfig)
{
	GetSelectedAutoPageDir(ptPageConfig->strDirName);
	GetIntervalTime(&ptPageConfig->iIntervalSec);
}

int GenericGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent)
{
	struct InputEvent tInputEvent;
	struct DisLayout *atIconsLayout;
	int iError = 0;
	int IconNum = 0;

	/* 该函数会休眠，直到有数据到来的时候才会继续运行 */
	iError = GetInputEvent(&tInputEvent);
	if(iError){
		DebugPrint(DEBUG_EMERG"You can't get here, or your system was died\n");
		return -1;
	}

	if(tInputEvent.iInputType != INPUT_TYPE_TS){
		DebugPrint(DEBUG_DEBUG"It is not a TS event\n");
		return -1;
	}

	*ptInputEvent = tInputEvent;
	atIconsLayout = ptPageLayout->atDisLayout;

	/* 确定触点在哪个图标上面 */
	while(atIconsLayout[IconNum].pcIconName != NULL){
		if(ptInputEvent->iXPos > atIconsLayout[IconNum].iTopLeftX 
			&& ptInputEvent->iXPos < atIconsLayout[IconNum].iBotRightX
			&& ptInputEvent->iYPos > atIconsLayout[IconNum].iTopLeftY
			&& ptInputEvent->iYPos < atIconsLayout[IconNum].iBotRightY)
		{
			return IconNum;
		}

		IconNum ++;
	}

	return -1;
}

int DisOfTwoPoint(struct InputEvent *ptPreEvent, struct InputEvent *ptCurEvent)
{
	int iDistanceX;
	int iDistanceY;

	iDistanceX = ptCurEvent->iXPos - ptPreEvent->iXPos;
	iDistanceY = ptCurEvent->iYPos - ptPreEvent->iYPos;

	return (iDistanceX*iDistanceX + iDistanceY*iDistanceY);
}

