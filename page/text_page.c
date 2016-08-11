#include <page_manag.h>
#include <pic_operation.h>
#include <render.h>

static struct DisLayout g_atTextPageIconsLayout[] = {
	{0, 0, 0, 0, "select_fold.bmp"},
	{0, 0, 0, 0, "interval.bmp"},
	{0, 0, 0, 0, "return.bmp"},
	{0, 0, 0, 0, NULL},
};

static struct PageLayout g_tTextPageLayout = {
	.iMaxTotalBytes = 0,
	.atDisLayout = g_atTextPageIconsLayout,
};

static void TextRunPage(struct PageIdetify *ptParentPageIdentify);
static int TextGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent);

static struct PageOpr g_tTextPageOpr = {
	.name = "text",
	.RunPage = TextRunPage,
	.GetInputEvent = TextGetInputEvent,
//	.Prepare  =    /* 后台准备函数，待实现 */
};

static void CalcTextPageLayout(struct PageLayout *ptPageLayout)
{
	int iXres;
	int iYres;
	int iBpp;

	int iHeight;
	int iWidth;
	int iVerticalDis;
	int iProportion;    /* 图像与各个图像间距的比例 */
	int iTmpTotalBytes;
	int iIconNum;
	int iIconTotal;     /* 图标总的个数 */
	
	struct DisLayout *atDisLayout;

	/* 获得图标数组 */
	atDisLayout = ptPageLayout->atDisLayout;
	GetDisDeviceSize(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	iIconTotal = sizeof(g_atTextPageIconsLayout) / sizeof(struct DisLayout) - 1;
	iProportion = 4;	/* 图像与间隔的比例 */

	/* 计算高，图像间距，宽 */
	iHeight = iYres * iProportion / 
		(iProportion * iIconTotal + iIconTotal + 1);
	iVerticalDis = iHeight / iProportion;
	iWidth  = iHeight * 2;	/* 固定宽高比为 2:1 */
	iIconNum = 0;

	

	/* 循环作图，结束标志是名字为 NULL */
	while(atDisLayout->pcIconName != NULL){
		
		atDisLayout->iTopLeftX  = (iXres - iWidth) / 2;
		atDisLayout->iTopLeftY  = iVerticalDis * (iIconNum + 1)
			+ iHeight * iIconNum;
		atDisLayout->iBotRightX = atDisLayout->iTopLeftX + iWidth - 1;
		atDisLayout->iBotRightY = atDisLayout->iTopLeftY + iHeight - 1;
		
		iTmpTotalBytes = (atDisLayout->iBotRightX -  atDisLayout->iTopLeftX + 1) * iBpp / 8
			* (atDisLayout->iBotRightY - atDisLayout->iTopLeftY + 1);

		/* 这个是为了生成图像的时候为每个图标分配空间用 */
		if(ptPageLayout->iMaxTotalBytes < iTmpTotalBytes){
			ptPageLayout->iMaxTotalBytes = iTmpTotalBytes;
		}

		iIconNum ++;
		atDisLayout ++;
	}
}

static void ShowTextPage(struct PageLayout *ptPageLayout)
{
	int iError = 0;

	struct VideoMem *ptTextPageVM;
	struct DisLayout *atDisLayout;

	atDisLayout = ptPageLayout->atDisLayout;

	/* 获得一块内存以显示 Text 页面 */
	ptTextPageVM = GetVideoMem(GetID("text"), 1);
	if(NULL == ptTextPageVM){
		DebugPrint(DEBUG_ERR"Get Text-page video memory error\n");
		return;
	}

	/* 把三个图标画上去 */
	if(atDisLayout[0].iTopLeftX == 0){
		CalcTextPageLayout(ptPageLayout);
	}

	iError = GeneratePage(ptPageLayout, ptTextPageVM);
	
	FlushVideoMemToDev(ptTextPageVM);

	/* 释放用完的内存，以供别的程序使用 */
	ReleaseVideoMem(ptTextPageVM);
}

static void TextRunPage(struct PageIdetify *ptParentPageIdentify)
{
	int iIndex;
	int iError = 0;
	int iIndexPressed = -1;	/* 判断是否是在同一个图标上按下与松开 */
	int bPressedFlag = 0;
	struct InputEvent tInputEvent;
	struct PageIdetify tPageIdentify;

	tPageIdentify.iPageID = GetID("text");
	ShowTextPage(&g_tTextPageLayout);

	while(1){
		/* 该函数会休眠 */
		iIndex = TextGetInputEvent(&g_tTextPageLayout, &tInputEvent);

		DebugPrint(DEBUG_DEBUG"text page index = %d****************\n", iIndex);
		if(tInputEvent.bPressure == 0){
			/* 说明曾经有按下，这里是松开 */
			if(bPressedFlag){
				bPressedFlag = 0;
				ReleaseButton(&g_atTextPageIconsLayout[iIndexPressed]);
				DebugPrint(DEBUG_DEBUG"Release button****************\n");

				/* 在同一个按钮按下与松开 */
//				if(iIndexPressed == iIndex){
//					goto nextwhilecircle;
//				}
					switch(iIndexPressed){
						case 0: {   /* 选择目录 */
							GetPageOpr("browse")->RunPage(&tPageIdentify);
							ShowTextPage(&g_tTextPageLayout);
							break;
						}
						case 1: {   /* 设置间隔 */
							GetPageOpr("interval")->RunPage(&tPageIdentify);
							ShowTextPage(&g_tTextPageLayout);
							break;
						}
						case 2: {
							return; /* 退出整个程序 */
						}
						default: {
							DebugPrint(DEBUG_EMERG"Somthing wrong\n");
							break;
						}
					}
				iIndexPressed = -1;
			}
		}else{
			if(iIndex != -1){
				if(0 == bPressedFlag){
					bPressedFlag = 1;
					iIndexPressed = iIndex;
					PressButton(&g_atTextPageIconsLayout[iIndexPressed]);
				}			
			}
		}	
nextwhilecircle:
	iError = 0;		
	}
}

static int TextGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

int TextPageInit(void)
{
	return RegisterPageOpr(&g_tTextPageOpr);
}


