#include <page_manag.h>
#include <pic_operation.h>
#include <print_manag.h>
#include <dis_manag.h>
#include <file.h>
#include <font_manag.h>
#include <render.h>
#include <stdlib.h>
#include <string.h>
#include <render.h>

static struct DisLayout g_atBrowsePageIconsLayout[] = {
	{0, 0, 0, 0, "up.bmp"},
	{0, 0, 0, 0, "select.bmp"},
	{0, 0, 0, 0, "pre_page.bmp"},
	{0, 0, 0, 0, "next_page.bmp"},
	{0, 0, 0, 0, NULL},
};

static struct PageLayout g_tBrowsePageLayout = {
	.iMaxTotalBytes = 0,
	.atDisLayout = g_atBrowsePageIconsLayout,
};

static void BrowseRunPage(struct PageIdetify *ptParentPageIdentify);
static int BrowseGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent);

#define DEFAULT_DIR "/"        /* Ĭ��Ŀ¼�Ǹ�Ŀ¼ */

/* ͼ����һ�������Σ������ļ�����������Ҳ�������ε� */
#define DIR_FILE_ICON_WIDTH  50
#define DIR_FILE_ICON_HEIGHT DIR_FILE_ICON_WIDTH
#define DIR_FILE_NAME_HEIGHT 20
#define DIR_FILE_NAME_WIDTH  (DIR_FILE_ICON_HEIGHT + DIR_FILE_NAME_HEIGHT)
/* all ָ���ǰ����ļ������ļ�ͼ��������ο� */
#define DIR_FILE_ALL_WIDTH   DIR_FILE_NAME_WIDTH
#define DIR_FILE_ALL_HEIGHT  DIR_FILE_ALL_WIDTH

static char g_strCurDirPath[256] = DEFAULT_DIR;
static char g_strSelDirPath[256] = DEFAULT_DIR;

/* �ļ���Ŀ¼���¼���� */
#define DIRFILE_ICON_BASE 256

/* Ŀ¼���ļ� */
static struct DirContent *g_ptDirContents;
static int g_iDirContentsNumber;
static int g_iFileStartIndex = 0;
static char *g_strOpenedDirIconPath = "fold_opened.bmp";
static char *g_strClosedDirIconPath = "fold_closed.bmp";
static char *g_strFileIconPath = "file.bmp";

/* ͼ�� */
static int g_iDirFileNumPerRow;
static int g_iDirFileNumPerCol;

static struct PiexlDatasDesc g_tDirClosedIconPiexlDatas;
static struct PiexlDatasDesc g_tDirOpenedIconPiexlDatas;
static struct PiexlDatasDesc g_tFileIconPiexlDatas;

static struct DisLayout  *g_atFileDirIconLayout;
static struct PageLayout g_tFileDirPageLayout = {
	.iMaxTotalBytes = 0,
};

static struct PageOpr g_tBrowsePageOpr = {
	.name = "browse",
	.RunPage = BrowseRunPage,
	.GetInputEvent = BrowseGetInputEvent,
//	.Prepare  =    /* ��̨׼����������ʵ�� */
};

static void CalcBrowsePageMenusLayout(struct PageLayout *ptPageLayout)
{
	int iXres;
	int iYres;
	int iBpp;

	int iHeight;
	int iWidth;
	int iVerticalDis;
	int iProportion;    /* ͼ�������ͼ����ı��� */
	int iTmpTotalBytes;
	int iIconNum;
	int iIconTotal;     /* ͼ���ܵĸ��� */
	
	struct DisLayout *atDisLayout;

	/* ���ͼ������ */
	atDisLayout = ptPageLayout->atDisLayout;
	GetDisDeviceSize(&iXres, &iYres, &iBpp);
	ptPageLayout->iBpp = iBpp;

	iIconTotal = sizeof(g_atBrowsePageIconsLayout) / sizeof(struct DisLayout) - 1;
	iProportion = 2000;	/* ͼ�������ı��� */

	/* ����ߣ�ͼ���࣬�� */
	iHeight = iYres * iProportion / 
		(iProportion * iIconTotal + iIconTotal + 1);
	iVerticalDis = iHeight / iProportion;
	iWidth  = iXres / 6;	/* ��Ϊ LCD ���� 1/4 */
	iIconNum = 0;

	/* ѭ����ͼ��������־������Ϊ NULL */
	while(atDisLayout->pcIconName != NULL){
		
		atDisLayout->iTopLeftX  = 5;
		atDisLayout->iTopLeftY  = iVerticalDis * (iIconNum + 1)
			+ iHeight * iIconNum;
		atDisLayout->iBotRightX = atDisLayout->iTopLeftX + iWidth - 1;
		atDisLayout->iBotRightY = atDisLayout->iTopLeftY + iHeight - 1;
		
		iTmpTotalBytes = (atDisLayout->iBotRightX -  atDisLayout->iTopLeftX + 1) * iBpp / 8
			* (atDisLayout->iBotRightY - atDisLayout->iTopLeftY + 1);

		/* �����Ϊ������ͼ���ʱ��Ϊÿ��ͼ�����ռ��� */
		if(ptPageLayout->iMaxTotalBytes < iTmpTotalBytes){
			ptPageLayout->iMaxTotalBytes = iTmpTotalBytes;
		}

		iIconNum ++;
		atDisLayout ++;
	}
}

/* �ļ���ͼ�� layout Ҫ��������з��� */
static int CalcBrowsePageFilesLayout()
{
	int iXres, iYres, iBpp;
	int iTopLeftX, iTopLeftY;
	int iTopLeftXBak;
	int iBotRightX, iBotRightY;
	int iIconWidth, iIconHeight;
	int iNumPerCol, iNumPerRow;
	int iDeltaX, iDeltaY;
	int i, j, k = 0;

	GetDisDeviceSize(&iXres, &iYres, &iBpp);

	/* �����Ų˵�ͼ����ұ�����,һֱ�� LCD �����ұ� */
	/* _____________________________
	 *|       _____________________ |
	 *| menu |   _     _     _     ||
	 *|      |  |_|   |_|   |_|    ||
	 *| menu |                     ||
	 *|      |     �ļ���Ŀ¼      ||
	 *| menu |   _     _     _     ||
	 *|      |  |_|   |_|   |_|    ||
	 *| menu |_____________________||
	 *|_____________________________|
	 */
	iTopLeftX  = g_atBrowsePageIconsLayout[0].iBotRightX + 1;
	iBotRightX = iXres - 1;
	iTopLeftY  = 0;
	iBotRightY = iYres - 1;

	/* ȷ����ʾ���ļ�����������һ�м�����һ�м��� 
	 * ��Ӧ�궨���е� DIR_FILE_ALL
	 */
	iIconWidth  = DIR_FILE_NAME_WIDTH;
	iIconHeight = iIconWidth;

	/* �������ͼ����� */
	iNumPerRow = (iBotRightX - iTopLeftX + 1) / iIconWidth;
	while(1){
		/* ������϶�Ŀ�� */
		iDeltaX = (iBotRightX - iTopLeftX + 1) - iIconWidth * iNumPerRow;
		if(iDeltaX / (iNumPerRow + 1) < 10){
			iNumPerRow --;
		}else{
			break;
		}
	}

	/* ��������ͼ����� */
	iNumPerCol = (iBotRightY - iTopLeftY + 1) / iIconHeight;
	while(1){
		/* ������϶�Ŀ�� */
		iDeltaY = (iBotRightY - iTopLeftY + 1) - iIconHeight * iNumPerCol;
		if(iDeltaY / (iNumPerCol + 1) < 10){
			iNumPerCol --;
		}else{
			break;
		}
	}

	iDeltaX = iDeltaX / (iNumPerRow + 1);
	iDeltaY = iDeltaY / (iNumPerCol + 1);

	g_iDirFileNumPerRow = iNumPerRow;
	g_iDirFileNumPerCol = iNumPerCol;

	/* ���� DisLayout �ṹ�壬����ͼ��������һ�� 
	 * 2 * g_iDirFileNumPerRow * g_iDirFileNumPerCol ��
	 * ���Ͻ�����ʶ��һ��
	 * 2 * g_iDirFileNumPerRow * g_iDirFileNumPerCol + 1 ��
	 */

	g_atFileDirIconLayout = malloc(sizeof(struct DisLayout) * (2 * g_iDirFileNumPerRow * g_iDirFileNumPerCol + 1));
	if(NULL == g_atFileDirIconLayout){
		DebugPrint(DEBUG_ERR"malloc g_atFileDirIconLayout error\n");
		return -1;
	}

	g_tFileDirPageLayout.iTopLeftX  = iTopLeftX;
	g_tFileDirPageLayout.iTopLeftY  = iTopLeftY;
	g_tFileDirPageLayout.iBotRightX = iBotRightX;
	g_tFileDirPageLayout.iBotRightY = iBotRightY;
	g_tFileDirPageLayout.iBpp       = iBpp;
	g_tFileDirPageLayout.iMaxTotalBytes = DIR_FILE_ALL_HEIGHT * DIR_FILE_ALL_WIDTH * iBpp / 8;
	g_tFileDirPageLayout.atDisLayout    = g_atFileDirIconLayout;

	/* ȷ��ÿһ��ͼ���λ�� */
	iTopLeftX = iTopLeftX + iDeltaX;
	iTopLeftY = iTopLeftY + iDeltaY;
	iTopLeftXBak = iTopLeftX;

	DebugPrint("iDeltaX = %d\n", iDeltaX);
	DebugPrint("iDeltaY = %d\n", iDeltaY);
	DebugPrint("iTopLeftX = %d\n", iTopLeftX);
	DebugPrint("iTopLeftY = %d\n", iTopLeftY);

	k = 0;
	for(i = 0; i < g_iDirFileNumPerCol; i ++){
		for(j = 0;j < g_iDirFileNumPerRow; j ++){
			/* �ļ����Ǳ�ͼ���� */
			g_atFileDirIconLayout[k].iTopLeftX  = iTopLeftX + (DIR_FILE_NAME_WIDTH - DIR_FILE_ICON_WIDTH) / 2;
			g_atFileDirIconLayout[k].iBotRightX = g_atFileDirIconLayout[k].iTopLeftX + DIR_FILE_ICON_WIDTH - 1;
			g_atFileDirIconLayout[k].iTopLeftY  = iTopLeftY;
			g_atFileDirIconLayout[k].iBotRightY = g_atFileDirIconLayout[k].iTopLeftY + DIR_FILE_ICON_HEIGHT - 1;

			/* �ļ��� */
			g_atFileDirIconLayout[k+1].iTopLeftX  = iTopLeftX;
			g_atFileDirIconLayout[k+1].iBotRightX = g_atFileDirIconLayout[k+1].iTopLeftX + DIR_FILE_NAME_WIDTH - 1;
			g_atFileDirIconLayout[k+1].iTopLeftY  = g_atFileDirIconLayout[k].iBotRightY + 1;
			g_atFileDirIconLayout[k+1].iBotRightY = g_atFileDirIconLayout[k+1].iTopLeftY + DIR_FILE_NAME_HEIGHT - 1;

			iTopLeftX = iTopLeftX + DIR_FILE_ALL_WIDTH + iDeltaX;
			k += 2;
		}

		iTopLeftX = iTopLeftXBak;
		iTopLeftY = iTopLeftY + DIR_FILE_ALL_HEIGHT + iDeltaY;
	}

	/* ͼ�������־ */
	g_atFileDirIconLayout[k].iTopLeftX  = 0;
	g_atFileDirIconLayout[k].iBotRightX = 0;
	g_atFileDirIconLayout[k].iTopLeftY  = 0;
	g_atFileDirIconLayout[k].iBotRightY = 0;
	g_atFileDirIconLayout[k].pcIconName = NULL;

	return 0;
}

static int GenerateDirAndFileIcons(struct PageLayout *ptPageLayout)
{
	struct PiexlDatasDesc tOriginIconPiexlDatas;
	int iError;
	int iXres;
	int iYres;
	int iBpp;
	struct DisLayout *atLayout;

	/* ����ȷ��ͼ��Ĵ�С */
	atLayout = ptPageLayout->atDisLayout;

	iError = GetDisDeviceSize(&iXres, &iYres, &iBpp);
	if(iError){
		DebugPrint("GetDisDeviceSize error \n");
		return -1;
	}

	/* Ϊ��/�ر�Ŀ¼�Լ���ͨ�ļ�����ռ� */
	g_tDirOpenedIconPiexlDatas.iBpp = iBpp;
	g_tDirOpenedIconPiexlDatas.pucPiexlDatasMem = malloc(ptPageLayout->iMaxTotalBytes);
	DebugPrint("malloc size %d", ptPageLayout->iMaxTotalBytes);
		
	if(NULL == g_tDirOpenedIconPiexlDatas.pucPiexlDatasMem){
		DebugPrint(DEBUG_ERR"malloc g_tDirOpenedIconPiexlDatas error\n");
		return -1;
	}

	g_tDirClosedIconPiexlDatas.iBpp = iBpp;
	g_tDirClosedIconPiexlDatas.pucPiexlDatasMem = malloc(ptPageLayout->iMaxTotalBytes);
	if(NULL == g_tDirClosedIconPiexlDatas.pucPiexlDatasMem){
		DebugPrint(DEBUG_ERR"malloc g_tDirClosedIconPiexlDatas error\n");
		return -1;
	}

	g_tFileIconPiexlDatas.iBpp = iBpp;
	g_tFileIconPiexlDatas.pucPiexlDatasMem = malloc(ptPageLayout->iMaxTotalBytes);
	if(NULL == g_tFileIconPiexlDatas.pucPiexlDatasMem){
		DebugPrint(DEBUG_ERR"malloc g_tFileIconPiexlDatas error\n");
		return -1;
	}

	/* ������֮����ȡ���� */
	iError = GetPiexlDatasForIcons(g_strOpenedDirIconPath, &tOriginIconPiexlDatas);
	if(iError){
		DebugPrint(DEBUG_ERR"get g_tDirOpenedIconPiexlDatas error\n");
		return -1;
	}

	g_tDirOpenedIconPiexlDatas.iHeight = atLayout[0].iBotRightY - atLayout[0].iTopLeftY + 1;
	g_tDirOpenedIconPiexlDatas.iWidth  = atLayout[0].iBotRightX - atLayout[0].iTopLeftX + 1;
	g_tDirOpenedIconPiexlDatas.iLineLength = g_tDirOpenedIconPiexlDatas.iWidth * iBpp / 8;
	g_tDirOpenedIconPiexlDatas.iTotalLength = g_tDirOpenedIconPiexlDatas.iLineLength * g_tDirOpenedIconPiexlDatas.iLineLength;

	DebugPrint("GenerateDirAndFileIcons get here 1\n");
	
	PicZoomOpr(&tOriginIconPiexlDatas, &g_tDirOpenedIconPiexlDatas);
	FreePiexlDatasForIcon(&tOriginIconPiexlDatas);
	
	iError = GetPiexlDatasForIcons(g_strClosedDirIconPath, &tOriginIconPiexlDatas);
	if(iError){
		DebugPrint(DEBUG_ERR"get g_tDirClosedIconPiexlDatas error\n");
		return -1;
	}

	g_tDirClosedIconPiexlDatas.iHeight = atLayout[0].iBotRightY - atLayout[0].iTopLeftY + 1;
	g_tDirClosedIconPiexlDatas.iWidth  = atLayout[0].iBotRightX - atLayout[0].iTopLeftX + 1;
	g_tDirClosedIconPiexlDatas.iLineLength = g_tDirClosedIconPiexlDatas.iWidth * iBpp / 8;
	g_tDirClosedIconPiexlDatas.iTotalLength = g_tDirClosedIconPiexlDatas.iLineLength * g_tDirClosedIconPiexlDatas.iLineLength;

	PicZoomOpr(&tOriginIconPiexlDatas, &g_tDirClosedIconPiexlDatas);
	FreePiexlDatasForIcon(&tOriginIconPiexlDatas);

	iError = GetPiexlDatasForIcons(g_strFileIconPath, &tOriginIconPiexlDatas);
	if(iError){
		DebugPrint(DEBUG_ERR"get g_strFileIconPath error\n");
		return -1;
	}
	g_tFileIconPiexlDatas.iHeight = atLayout[0].iBotRightY - atLayout[0].iTopLeftY + 1;
	g_tFileIconPiexlDatas.iWidth  = atLayout[0].iBotRightX - atLayout[0].iTopLeftX + 1;
	g_tFileIconPiexlDatas.iLineLength = g_tFileIconPiexlDatas.iWidth * iBpp / 8;
	g_tFileIconPiexlDatas.iTotalLength = g_tFileIconPiexlDatas.iLineLength * g_tFileIconPiexlDatas.iLineLength;

	PicZoomOpr(&tOriginIconPiexlDatas, &g_tFileIconPiexlDatas);
	FreePiexlDatasForIcon(&tOriginIconPiexlDatas);

	return 0;
}

/* iNum ���ܵ�ͼ������� */
static int GenerateBrowsePageDirAndFile(int iStart, int iNum, struct DirContent *atDirContent, struct VideoMem *ptVideoMem)
{
	struct PageLayout *ptPageLayout;
	struct DisLayout *atDisLayout;
	struct DisLayout tCleanDisLayout;
	int i, j, k;
	int iIndex;

	ptPageLayout = &g_tFileDirPageLayout;
	atDisLayout = ptPageLayout->atDisLayout;

	/* ���Ҫ���õ����� */
	tCleanDisLayout.iTopLeftX  = g_tFileDirPageLayout.iTopLeftX;
	tCleanDisLayout.iTopLeftY  = g_tFileDirPageLayout.iTopLeftY;
	tCleanDisLayout.iBotRightX = g_tFileDirPageLayout.iBotRightX;
	tCleanDisLayout.iBotRightY = g_tFileDirPageLayout.iBotRightY;
	tCleanDisLayout.pcIconName = NULL;
	ClearVideoMemRegion(ptVideoMem, &tCleanDisLayout, CONFIG_BACKGROUND_COLOR);

	SetFontSize(atDisLayout[1].iBotRightY- atDisLayout[1].iTopLeftY - 5);

	k = 0;
	iIndex = iStart;
	for(i = 0; i < g_iDirFileNumPerCol; i ++){
		for(j = 0; j < g_iDirFileNumPerRow; j ++){
			if(iIndex < iNum){
				if(atDirContent[iIndex].eFileType == DIR_FILE){
					PicMergeOpr(atDisLayout[k].iTopLeftX, atDisLayout[k].iTopLeftY, &g_tDirClosedIconPiexlDatas, &ptVideoMem->tPiexlDatas);
				}else{
					PicMergeOpr(atDisLayout[k].iTopLeftX, atDisLayout[k].iTopLeftY, &g_tFileIconPiexlDatas, &ptVideoMem->tPiexlDatas);					
				}

				k ++;
				
				/* ��ʾ���� */
				MergeString(atDisLayout[k].iTopLeftX, atDisLayout[k].iTopLeftY,
								atDisLayout[k].iBotRightX, atDisLayout[k].iBotRightY,
								(unsigned char *)atDirContent[iIndex].strDirName, ptVideoMem);

				k ++;
				iIndex ++;
			}else{
				return 0;
			}
		}
	}

	return 0;
}

static void ShowBrowsePage(struct PageLayout *ptPageLayout)
{
	struct DisLayout *atDisLayout;
	struct VideoMem *ptVideoMem;

	atDisLayout = ptPageLayout->atDisLayout;

	ptVideoMem = GetVideoMem(GetID("browse"), 1);
	if(NULL == ptVideoMem){
		DebugPrint(DEBUG_ERR"malloc browse VideoMem error\n");
		return;
	}

	if(atDisLayout[0].iTopLeftX == 0){
		CalcBrowsePageMenusLayout(ptPageLayout);
		CalcBrowsePageFilesLayout();
	}
	
	/* ��ȡ���������ļ�ͼ��ĵ�����Ϣ����ŵ�ȫ�ֱ������� */
	if(!g_tDirClosedIconPiexlDatas.pucPiexlDatasMem){
		GenerateDirAndFileIcons(&g_tFileDirPageLayout);
	}

	GeneratePage(ptPageLayout, ptVideoMem);
	DebugPrint("ShowBrowsePage get here \n");
	GenerateBrowsePageDirAndFile(g_iFileStartIndex, g_iDirContentsNumber, g_ptDirContents, ptVideoMem);

	DebugPrint("ShowBrowsePage get here \n");

	FlushVideoMemToDev(ptVideoMem);

	ReleaseVideoMem(ptVideoMem);
}

static int GetInputPosition(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent)
{
	int iFileNum = 0;
	struct DisLayout *atLayout = ptPageLayout->atDisLayout;

	while(atLayout[iFileNum].pcIconName){
		if(ptInputEvent->iXPos <= atLayout[iFileNum].iBotRightX
			&& ptInputEvent->iXPos >= atLayout[iFileNum].iTopLeftX
			&& ptInputEvent->iYPos <= atLayout[iFileNum].iBotRightY
			&& ptInputEvent->iYPos >= atLayout[iFileNum].iTopLeftY)
		{
			return iFileNum;
		}

		iFileNum ++;  /* ��������Ҳ����Ϊ�ǵ����˸��ļ� */
	}

	return -1;
}

static void BrowseRunPage(struct PageIdetify *ptParentPageIdentify)
{
	int iIndex;
	struct InputEvent tInputEvent;
	struct InputEvent tPrePressInputEvent;

	int bUsedToSelectDir = 0;    /* �Ƿ�������ѡ������ͼƬ�� */
	int bIconPressed = 0;        /* ��ǰҳ���Ƿ���ͼ�갴�� */

	int bHaveClickSelectIcon = 0;
	
	int iIndexPressed = -1;
	int iDirFileContentIndex;

	int iError;
	struct VideoMem *ptDevVideoMem;

	struct PageIdetify tBrowsePageIdentify;
	char strPathTmp[256];  /* ·�����ݴ��� */
	char *pcPathTmp;

	int bPressedFlag = 0;

	/* ��ʼ��Ϊ0 */
	tPrePressInputEvent.tTimeVal.tv_sec  = 0;
	tPrePressInputEvent.tTimeVal.tv_usec = 0;

	/* ˵����Ϊ����������Ŀ¼ */
	if(ptParentPageIdentify->iPageID == GetID("setting")){
		bUsedToSelectDir = 1;
	}

	ptDevVideoMem = GetDevVideoMem(); /* ���Ĭ����ʾ�豸���Դ� */

	iError = GetDirContents(g_strCurDirPath, &g_ptDirContents, &g_iDirContentsNumber);
	if(iError){
		DebugPrint(DEBUG_ERR"Can't get directory error\n");
		return;
	}

	tBrowsePageIdentify.iPageID = GetID("browse");
	ShowBrowsePage(&g_tBrowsePageLayout);

	while(1){
		/* �ú��������� */
		iIndex = BrowseGetInputEvent(&g_tBrowsePageLayout, &tInputEvent);

		if(-1 == iIndex){
			/* ˵�������ڲ˵�ͼ�����棬Ҫ�ж��ǲ������ļ������ļ������� */
			iIndex = GetInputPosition(&g_tFileDirPageLayout, &tInputEvent);
			if(-1 != iIndex){
				if(g_iFileStartIndex + iIndex / 2 < g_iDirContentsNumber){
					iIndex += DIRFILE_ICON_BASE;
				}else{
					iIndex = -1;
				}
			}
		}

		if(tInputEvent.bPressure == 0){		
			if(0 == bPressedFlag){
				goto nextwhilecircle;
			}

			/* ˵�������а��£��������ɿ� */
			if(iIndexPressed < DIRFILE_ICON_BASE){
				bPressedFlag = 0;
				ReleaseButton(&g_atBrowsePageIconsLayout[iIndexPressed]);

				/* �ڲ�ͬ�İ�ť�������ɿ���������һ��ѭ�� */
				if(iIndexPressed == iIndex){
					goto nextwhilecircle;
				}
				
				/* ��ͬһ����ť�������ɿ� */
				switch(iIndexPressed){
					/* ���� */
					case 0: {
						if(0 == strcmp(g_strCurDirPath, "/")){
							FreeDirContents(&g_ptDirContents);
							return; /* �˳�����ҳ�� */
						}

						/* �ҵ����һ��"/"ɾ�� */
						pcPathTmp = strrchr(g_strCurDirPath, '/');
						*pcPathTmp = '\0';
						FreeDirContents(&g_ptDirContents);

						/* ���»�ȡĿ¼��������� */
						iError = GetDirContents(g_strCurDirPath, &g_ptDirContents, &g_iDirContentsNumber);
						if(iError){
							DebugPrint(DEBUG_ERR"Get DirContents error\n");
							return;
						}
						g_iFileStartIndex = 0;
						GenerateBrowsePageDirAndFile(g_iFileStartIndex, g_iDirContentsNumber, g_ptDirContents, ptDevVideoMem);
						break;
					}
					/* ѡ�� */
					case 1: {
						break;
					}
					/* ��һҳ */
					case 2: {
						if(0 == g_iFileStartIndex){
							break;
						}else{
							g_iFileStartIndex -= g_iDirFileNumPerCol * g_iDirFileNumPerRow;
							GenerateBrowsePageDirAndFile(g_iFileStartIndex, g_iDirContentsNumber, g_ptDirContents, ptDevVideoMem);
							break;
						}
						break;
					}
					/* ��һҳ */
					case 3: {
						g_iFileStartIndex += g_iDirFileNumPerCol * g_iDirFileNumPerRow;
						
						if(g_iFileStartIndex > g_iDirContentsNumber){
							g_iFileStartIndex -= g_iDirFileNumPerCol * g_iDirFileNumPerRow;
							break;
						}else{	
							GenerateBrowsePageDirAndFile(g_iFileStartIndex, g_iDirContentsNumber, g_ptDirContents, ptDevVideoMem);
							break;
						}
						break;
					}
					default: {
						DebugPrint(DEBUG_EMERG"Somthing wrong\n");
						break;
					}
				}
			}else{
				bPressedFlag = 0;
				ReleaseButton(&g_atFileDirIconLayout[iIndexPressed - DIRFILE_ICON_BASE]);

				/* �����Ŀ¼�ͽ��� */
				iDirFileContentIndex = g_iFileStartIndex + (iIndexPressed - DIRFILE_ICON_BASE) / 2;
				if(g_ptDirContents[iDirFileContentIndex].eFileType == DIR_FILE){
					snprintf(strPathTmp, 256, "%s/%s", g_strCurDirPath, g_ptDirContents[iDirFileContentIndex].strDirName);
					strPathTmp[255] = '\0';
					strcpy(g_strCurDirPath, strPathTmp);  /* ���ĵ�ǰĿ¼ */
					FreeDirContents(&g_ptDirContents);
					/* ���»�ȡĿ¼��������� */
					iError = GetDirContents(g_strCurDirPath, &g_ptDirContents, &g_iDirContentsNumber);
					if(iError){
						DebugPrint(DEBUG_ERR"Get DirContents error\n");
						return;
					}
					g_iFileStartIndex = 0;
					GenerateBrowsePageDirAndFile(g_iFileStartIndex, g_iDirContentsNumber, g_ptDirContents, ptDevVideoMem);
				}else if(g_ptDirContents[iDirFileContentIndex].eFileType == NORMAL_FILE){
					snprintf(tBrowsePageIdentify.strCurPicFile, 256, "%s/%s", g_strCurDirPath, g_ptDirContents[iDirFileContentIndex].strDirName);
					tBrowsePageIdentify.strCurPicFile[255] = '\0';
					if(isPictureSupported(tBrowsePageIdentify.strCurPicFile)){
						tBrowsePageIdentify.iPageID = GetID("browse");
						GetPageOpr("picture")->RunPage(&tBrowsePageIdentify);
						ShowBrowsePage(&g_tBrowsePageLayout);
					}
				}
			}	
		}else{  /* ��ť���� */
			if(iIndex == -1){  /* û�������κ�һ��ͼ������ */
				goto nextwhilecircle;
			}
			
			if(0 == bPressedFlag){
				bPressedFlag = 1;
				iIndexPressed = iIndex;

				if(iIndexPressed == 0){
					tPrePressInputEvent = tInputEvent;   /* ��¼��һ�ΰ��µ��¼� */
				}
				
				if(iIndexPressed < DIRFILE_ICON_BASE){					
					PressButton(&g_atBrowsePageIconsLayout[iIndexPressed]);
				}else{
					PressButton(&g_atFileDirIconLayout[iIndexPressed - DIRFILE_ICON_BASE]);
				}

				goto nextwhilecircle;
			}else{
				if(iIndex != 0){  /* ˵�������������·��ؼ� */
					tPrePressInputEvent = tInputEvent;
					goto nextwhilecircle;
				}
				/* ��������Ҫֱ�ӷ��ص���ҳ�� */
				if(tInputEvent.tTimeVal.tv_sec - tPrePressInputEvent.tTimeVal.tv_sec >= 2){
					/* �´ν����ʱ���ǴӸ�Ŀ¼��ʼ */
					g_strCurDirPath[0] = '/';
					g_strCurDirPath[1] = '\0';
					FreeDirContents(&g_ptDirContents);
					return; /* �˳�����ҳ�� */
				}
			}			
		}	

nextwhilecircle:
		iError = 0;
	}
}

static int BrowseGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent)
{
	return GenericGetInputEvent(ptPageLayout, ptInputEvent);
}

int BrowsePageInit(void)
{
	return RegisterPageOpr(&g_tBrowsePageOpr);
}


