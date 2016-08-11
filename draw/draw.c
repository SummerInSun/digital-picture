#include <draw.h>
#include <dis_manag.h>
#include <common_st.h>
#include <common_config.h>
#include <font_manag.h>
#include <encoding_manag.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct FilePageDesc{
	int iPageNum;
	unsigned char *pucCurPageFirstPos;
	unsigned char *pucNextPageFirstPos;
	struct FilePageDesc *ptPrePage;
	struct FilePageDesc *ptNextPage;
};

static int g_iTextFileFd;
static unsigned char *g_pucTextFileMemStart;
static unsigned char *g_pucTextFileMemEnd;
static struct stat g_tTextFileStatus;

static struct EncodingOpr *g_ptTextFileEncodingOpr;
static unsigned char *g_pucEncodingFirstPos;
static unsigned char *g_pucEncodingNextPos;

static struct FontDisOpt g_tFontDisOpt;
static int g_dwFontSize;
static struct DisOpr *g_ptDisOpr;
static struct FilePageDesc *g_ptPages   = NULL;
static struct FilePageDesc * g_ptCurPage = NULL;

int OpenTextFile(char *pcFileName)
{
	int iError = 0;

	g_iTextFileFd = open(pcFileName, O_RDONLY);
	if(g_iTextFileFd < 0){
		printf("Can't open %s\n", pcFileName);
		return -1;
	}
	iError = fstat(g_iTextFileFd, &g_tTextFileStatus);	
	if(iError){
		printf("Can't load %s's status\n", pcFileName);
		return -1;
	}

	g_pucTextFileMemStart = (unsigned char *)mmap(NULL, g_tTextFileStatus.st_size, 
									PROT_READ, MAP_SHARED, g_iTextFileFd, 0);
	if(g_pucTextFileMemStart == (unsigned char *)-1){
		printf("mmap g_pucTextFileMem failed\n");
		return -1;
	}
	
	g_ptTextFileEncodingOpr = SelectEncodingOprForFile(g_pucTextFileMemStart);
	if(g_ptTextFileEncodingOpr == NULL){
		printf("We can't support this format\n");
		return -1;
	}

	DebugPrint(DEBUG_NOTICE"Selected encoding is %s\n", g_ptTextFileEncodingOpr->name);
	g_pucEncodingFirstPos = g_pucTextFileMemStart + g_ptTextFileEncodingOpr->iHeadLen;
	g_pucTextFileMemEnd = g_pucTextFileMemStart + g_tTextFileStatus.st_size;

	close(g_iTextFileFd);

	return 0;
}

int SetTextDetail(char *pcHZKFile,char *pcFileFreetype,unsigned int dwFontSize)
{
	int iError = 0;
	struct FontOpr *ptFontOpr;
	struct FontOpr *ptFontOprTmp;
	int iRet = -1;

	g_dwFontSize = dwFontSize;

	ptFontOpr = g_ptTextFileEncodingOpr->ptFontOprSupportedHead;
	while(ptFontOpr){
		
		/* 所有的支持的字体都初始化一遍，不支持的就删掉 */
		if(strcmp(ptFontOpr->name, "ascii") == 0){
			iError = ptFontOpr->FontInit(NULL, dwFontSize);

		}else if(strcmp(ptFontOpr->name, "gbk") == 0){
			iError = ptFontOpr->FontInit(pcHZKFile, dwFontSize);

		}else{
			iError = ptFontOpr->FontInit(pcFileFreetype, dwFontSize);
			
		}

		ptFontOprTmp = ptFontOpr->ptFontOprNext;

		if(iError == 0){
			iRet = 0;
		}else{
			DelFontOprForEncoding(g_ptTextFileEncodingOpr, ptFontOpr);
		}
		ptFontOpr = ptFontOprTmp;
	}

	return iRet;
}

/* 个性化的字体设置 */
struct FontDisOpt *GetFontDisOpt(void)
{
	return &g_tFontDisOpt;
}

static void InitFontDisOpt()
{
	/* Set to default value */
	g_tFontDisOpt.dwBackGroundColor = CONFIG_BACKGROUND_COLOR;
	g_tFontDisOpt.dwFontColor       = CONFIG_FONT_COLOR;
	g_tFontDisOpt.ucFontMode        = NORMAL_FONT;
}

int SelectAndInitDisplay(char *pcName)
{
	int iError;

	g_ptDisOpr = GetDisOpr(pcName);
	if(g_ptDisOpr == 0){
		DebugPrint(DEBUG_ERR"GetDisOpr of %s failed\n", pcName);
		return -1;
	}

	iError = g_ptDisOpr->DisDeviceInit();
	InitFontDisOpt();
	
	return iError;
}

int IncLcdX(int iX)
{
	if (iX + 1 < g_ptDisOpr->iXres)
		return (iX + 1);
	else
		return 0;
}

int IncLcdY(int iY)
{
	if (iY + g_dwFontSize < g_ptDisOpr->iYres)
		return (iY + g_dwFontSize);
	else
		return 0;
}

int RelocateFontPos(struct FontBitMap *ptFontBitMap)
{
	int iLcdY;
//	int iDeltaX;
	int iDeltaY;

	if (ptFontBitMap->iYMax > g_ptDisOpr->iYres)
	{
		/* 满页了 */
		return -1;
	}

	/* 超过LCD最右边 */
	if (ptFontBitMap->iXMax > g_ptDisOpr->iXres)
	{
		/* 换行 */		
		iLcdY = IncLcdY(ptFontBitMap->iCurOriginY);
		if (0 == iLcdY)
		{
			/* 满页了 */
			return -1;
		}
		else
		{
			/* 设置新的位置 */
#if 0
			iDeltaX = 0 - ptFontBitMap->iCurOriginX;
			iDeltaY = iLcdY - ptFontBitMap->iCurOriginY;

			ptFontBitMap->iCurOriginX  += iDeltaX;
			ptFontBitMap->iCurOriginY  += iDeltaY;

			ptFontBitMap->iNextOriginX += iDeltaX;
			ptFontBitMap->iNextOriginY += iDeltaY;

			ptFontBitMap->iXLeft += iDeltaX;
			ptFontBitMap->iXMax  += iDeltaX;

			ptFontBitMap->iYTop  += iDeltaY;
			ptFontBitMap->iYMax  += iDeltaY;
#else
			iDeltaY = iLcdY - ptFontBitMap->iCurOriginY;

			ptFontBitMap->iNextOriginX = ptFontBitMap->iXMax - ptFontBitMap->iCurOriginX;
			ptFontBitMap->iXLeft = ptFontBitMap->iXLeft - ptFontBitMap->iCurOriginX;
			ptFontBitMap->iCurOriginX  = 0;
			ptFontBitMap->iXMax  = ptFontBitMap->iNextOriginX;		
			
			ptFontBitMap->iCurOriginY  += iDeltaY;
			ptFontBitMap->iNextOriginY += iDeltaY;
			ptFontBitMap->iYTop  += iDeltaY;
			ptFontBitMap->iYMax  += iDeltaY;
#endif
			return 0;	
		}
	}
	
	return 0;
}

static int ShowOneWord(struct FontBitMap *ptFontBitMap)
{
	unsigned int dwXCod, dwYCod;
	unsigned int dw8PiexlNum;
	int dwPiexlNum;
	int dwAddPiexlForFont = 1;
	int dwAddPiexlNum = 0;

	if(ptFontBitMap == NULL){
		return -1;
	}

	if(g_tFontDisOpt.ucFontMode & OVERSTRIKING_FONT){
		dwAddPiexlForFont = 3;
	}
	for(dwAddPiexlNum = 0; dwAddPiexlNum < dwAddPiexlForFont; dwAddPiexlNum ++){
		if(ptFontBitMap->iBpp == 1){
			
			/* mono bitmap, 1bit per piexl */
			for(dwYCod = ptFontBitMap->iYTop; dwYCod < ptFontBitMap->iYMax; dwYCod ++){
				/* align piexl */
				dw8PiexlNum = (dwYCod - ptFontBitMap->iYTop) * ptFontBitMap->iPatch;

				for(dwXCod = ptFontBitMap->iXLeft + dwAddPiexlNum, dwPiexlNum = 7; dwXCod < ptFontBitMap->iXMax + dwAddPiexlNum; dwXCod ++){
					if(ptFontBitMap->pucBuffer[dw8PiexlNum] & (1 << dwPiexlNum)){
						g_ptDisOpr->ShowPiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR);
					}else{
						/* keep background */
					}

					if((dwYCod == ptFontBitMap->iYMax - 1) && (g_tFontDisOpt.ucFontMode & UNDERLINE_FONT))
					{
						g_ptDisOpr->ShowPiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR);
					}
					
					dwPiexlNum --;
					if(dwPiexlNum == -1){
						dwPiexlNum = 7;
						dw8PiexlNum ++;
					}
				}
			}
		}else if(ptFontBitMap->iBpp == 8){

			/* normal bitmap, 8bit per piexl */
			for(dwYCod = ptFontBitMap->iYTop; dwYCod < ptFontBitMap->iYMax; dwYCod ++){
				/* align piexl */
				dw8PiexlNum = (dwYCod - ptFontBitMap->iYTop) * ptFontBitMap->iPatch;

				for(dwXCod = ptFontBitMap->iXLeft + dwAddPiexlNum; dwXCod < ptFontBitMap->iXMax + dwAddPiexlNum; dwXCod ++){
					if(ptFontBitMap->pucBuffer[dw8PiexlNum]){
						g_ptDisOpr->ShowPiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR);
					}else{
						/* keep background */
					}

					if((dwYCod == ptFontBitMap->iYMax - 1) && (g_tFontDisOpt.ucFontMode & UNDERLINE_FONT))
					{
						g_ptDisOpr->ShowPiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR);
					}
					
					dw8PiexlNum ++;
				}
			}
		}else{
			DebugPrint(DEBUG_ERR"We can't support this BPP format\n");
			return -1;
		}
	}
	return 0;
}

int ShowOnePage(unsigned char *pucTextFileMemCurPos)
{
	int iLen;
	int iError;
	unsigned char *pucBufStart;
	unsigned int dwCode;
	struct FontOpr *ptFontOpr;
	struct FontBitMap tFontBitMap;
	
	int bHasNotClrSceen = 1;
	int bHasGetCode = 0;

	tFontBitMap.iCurOriginX = 0;
	tFontBitMap.iCurOriginY = g_dwFontSize;
	pucBufStart = pucTextFileMemCurPos;

	while (1)
	{
		iLen = g_ptTextFileEncodingOpr->GetCodeFromText(pucBufStart, g_pucTextFileMemEnd, &dwCode);
		if (0 == iLen)
		{
			/* 文件结束 */
			if (!bHasGetCode)
			{
				return -1;
			}
			else
			{
				return 0;
			}
		}

		bHasGetCode = 1;
		
		pucBufStart += iLen;

		/* 有些文本, \n\r两个一起才表示回车换行
		 * 碰到这种连续的\n\r, 只处理一次
		 */
		if (dwCode == '\n')
		{
			g_pucEncodingNextPos= pucBufStart;
			
			/* 回车换行 */
			tFontBitMap.iCurOriginX = 0;
			tFontBitMap.iCurOriginY = IncLcdY(tFontBitMap.iCurOriginY);
			if (0 == tFontBitMap.iCurOriginY)
			{
				/* 显示完当前一屏了 */
				return 0;
			}
			else
			{
				continue;
			}
		}
		else if (dwCode == '\r')
		{
			continue;
		}
		else if (dwCode == '\t')
		{
			/* TAB键用一个空格代替 */
			dwCode = ' ';
		}
		
		/* 循环采用不同的字体进行显示，如果显示成功的话就直接返回，否则返回错误 */
		ptFontOpr = g_ptTextFileEncodingOpr->ptFontOprSupportedHead;
		while (ptFontOpr)
		{
			iError = ptFontOpr->GetFontBitMap(dwCode, &tFontBitMap);

			if (0 == iError)
			{
				if (RelocateFontPos(&tFontBitMap))
				{
					/* 剩下的LCD空间不能满足显示这个字符 */
					return 0;
				}

				if (bHasNotClrSceen)
				{
					/* 首先清屏 */
					bHasNotClrSceen = 0;
					g_ptDisOpr->CleanScreen(CONFIG_BACKGROUND_COLOR);
				}

				/* 显示一个字符 */
				if (ShowOneWord(&tFontBitMap))
				{
					return -1;
				}
				
				tFontBitMap.iCurOriginX = tFontBitMap.iNextOriginX;
				tFontBitMap.iCurOriginY = tFontBitMap.iNextOriginY;
				g_pucEncodingNextPos = pucBufStart;

				/* 继续取出下一个编码来显示 */
				break;
			}
			ptFontOpr = ptFontOpr->ptFontOprNext;
		}		
	}

	return 0;
}

static void RecordPage(struct FilePageDesc* ptPageNew)
{
	struct FilePageDesc* ptPage;
		
	if (!g_ptPages)
	{
		g_ptPages = ptPageNew;
	}
	else
	{
		ptPage = g_ptPages;
		while (ptPage->ptNextPage)
		{
			ptPage = ptPage->ptNextPage;
		}
		ptPage->ptNextPage   = ptPageNew;
		ptPageNew->ptPrePage = ptPage;
	}
}


int ShowNextPage(void)
{
	int iError;
	struct FilePageDesc *ptPage;
	unsigned char *pucTextFileMemCurPos;

	if (g_ptCurPage)
	{
		pucTextFileMemCurPos = g_ptCurPage->pucNextPageFirstPos;
	}
	else
	{
		pucTextFileMemCurPos = g_pucEncodingFirstPos;
	}
	
	iError = ShowOnePage(pucTextFileMemCurPos);

	if (iError == 0)
	{
		if (g_ptCurPage && g_ptCurPage->ptNextPage)
		{
			g_ptCurPage = g_ptCurPage->ptNextPage;
			return 0;
		}
		
		ptPage = malloc(sizeof(struct FilePageDesc));
		if (ptPage)
		{
			ptPage->pucCurPageFirstPos  = pucTextFileMemCurPos;
			ptPage->pucNextPageFirstPos = g_pucEncodingNextPos;
			ptPage->ptPrePage                    = NULL;
			ptPage->ptNextPage                   = NULL;
			g_ptCurPage = ptPage;

			RecordPage(ptPage);
			return 0;
		}
		else
		{
			return -1;
		}
	}
	return iError;
}


int ShowPrePage(void)
{
	int iError;

	if (!g_ptCurPage || !g_ptCurPage->ptPrePage)
	{
		return -1;
	}

	iError = ShowOnePage(g_ptCurPage->ptPrePage->pucCurPageFirstPos);
	if (iError == 0)
	{
		g_ptCurPage = g_ptCurPage->ptPrePage;
	}
	return iError;
}

int ReShowCurPage()
{
	return ShowOnePage(g_ptCurPage->pucCurPageFirstPos);
}


