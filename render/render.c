#include <render.h>
#include <file.h>
#include <dis_manag.h>
#include <encoding_manag.h>
#include <print_manag.h>
#include <pic_operation.h>
#include <stdlib.h>
#include <string.h>

int GetPiexlDatasForIcons(char *pcName, struct PiexlDatasDesc *ptPiexlData)
{
	int iError = 0;
	struct FileDesc tIconFileDesc;
	struct PicFmtParser *ptIconParser;
	int iXres;
	int iYres;
	int iBpp;

	snprintf(tIconFileDesc.cFileName, 256, "%s/%s", ICON_PATH, pcName);
	tIconFileDesc.cFileName[255] = '\0';

	iError = GetFileDesc(&tIconFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"GetFileDesc error\n");
		return -1;
	}

	ptIconParser = GetSupportedParser(&tIconFileDesc);
	if(NULL == ptIconParser){
		DebugPrint(DEBUG_ERR"GetPicFmtParser error\n");
		return -1;
	}
	
	GetDisDeviceSize(&iXres, &iYres, &iBpp);
	ptPiexlData->iBpp = iBpp;
	iError = ptIconParser->GetPiexlDatas(&tIconFileDesc, ptPiexlData);
	if(iError){
		DebugPrint(DEBUG_ERR"Can't GetPiexlDatas\n");
		ReleaseFileDesc(&tIconFileDesc);
		return -1;
	}

	ReleaseFileDesc(&tIconFileDesc);

	return 0;
}

int GetPiexlDatasForPic(char *pcName, struct PiexlDatasDesc *ptPiexlData)
{
	int iError = 0;
	struct FileDesc tIconFileDesc;
	struct PicFmtParser *ptIconParser;
	int iXres;
	int iYres;
	int iBpp;

	snprintf(tIconFileDesc.cFileName, 256, "%s", pcName);
	tIconFileDesc.cFileName[255] = '\0';

	iError = GetFileDesc(&tIconFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"GetFileDesc error\n");
		return -1;
	}
	
	ptIconParser = GetSupportedParser(&tIconFileDesc);
	if(NULL == ptIconParser){
		DebugPrint(DEBUG_ERR"GetPicFmtParser error\n");
		return -1;
	}
	
	GetDisDeviceSize(&iXres, &iYres, &iBpp);
	ptPiexlData->iBpp = iBpp;
	iError = ptIconParser->GetPiexlDatas(&tIconFileDesc, ptPiexlData);
	if(iError){
		DebugPrint(DEBUG_ERR"Can't GetPiexlDatas\n");
		ReleaseFileDesc(&tIconFileDesc);
		return -1;
	}

	ReleaseFileDesc(&tIconFileDesc);

	return 0;
}

void FreePiexlDatasForIcon(struct PiexlDatasDesc *ptPiexlData)
{
	free(ptPiexlData->pucPiexlDatasMem);
}

void FlushVideoMemToDev(struct VideoMem *ptFlushVideoMem)
{
	/* 如果该缓冲区已经是设备显存的话就不需要再进行拷贝显示了 */
	if(!ptFlushVideoMem->bIsDevFrameBuffer){
		GetDefaultDisOpr()->DrawPage(ptFlushVideoMem);
	}
}

int GeneratePage(struct PageLayout *ptPageLayout, struct VideoMem *ptVideoMem)
{
	int iError = 0;

	struct PiexlDatasDesc tOriginIconDatas;
	struct PiexlDatasDesc tIcondatas;
	struct DisLayout *aIconDisLayout;

	aIconDisLayout = ptPageLayout->atDisLayout;
	
	/* 一次生成就可以多次使用 */
	if(ptVideoMem->ePStat != PS_GENERATED){

		ClearVideoMemRegion(ptVideoMem, NULL, CONFIG_BACKGROUND_COLOR);
	
		tIcondatas.iBpp  = ptPageLayout->iBpp;

		tIcondatas.pucPiexlDatasMem = malloc(ptPageLayout->iMaxTotalBytes);
		if(NULL == tIcondatas.pucPiexlDatasMem){
			DebugPrint(DEBUG_ERR"tIcondatas.pucPiexlDatasMem malloc error\n");
			return -1;
		}

		/* 循环缩放图标 */
		while(aIconDisLayout->pcIconName){
			if(aIconDisLayout->pcIconName[0] != ' '){
				
				iError = GetPiexlDatasForIcons(aIconDisLayout->pcIconName, &tOriginIconDatas);
				if(iError){
					DebugPrint(DEBUG_ERR"GetPiexlDatasForIcons error\n");
					free(tIcondatas.pucPiexlDatasMem);
					return -1;
				}
				
				tIcondatas.iHeight = aIconDisLayout->iBotRightY - aIconDisLayout->iTopLeftY;
				tIcondatas.iWidth  = aIconDisLayout->iBotRightX - aIconDisLayout->iTopLeftX;
				tIcondatas.iLineLength	= tIcondatas.iWidth * tIcondatas.iBpp / 8;
				tIcondatas.iTotalLength = tIcondatas.iLineLength * tIcondatas.iHeight;
				
				PicZoomOpr(&tOriginIconDatas, &tIcondatas);
				PicMergeOpr(aIconDisLayout->iTopLeftX, aIconDisLayout->iTopLeftY, 
					&tIcondatas, &ptVideoMem->tPiexlDatas);
				
			}
			
			aIconDisLayout ++;
		}

		free(tIcondatas.pucPiexlDatasMem);
		ptVideoMem->ePStat = PS_GENERATED;
	}

	return 0;
}

static void InvertButton(struct DisLayout *ptButtonLayout)
{
	int iXPos;
	int iYPos;
	int iBpp;
	int iInvertWidth;

	unsigned char *pucVideoMem;
	struct DisOpr *ptDefalutDisDev;

	ptDefalutDisDev = GetDefaultDisOpr();
#if 0
	if(NULL == ptButtonLayout->pcIconName){
		/* 未知的按键，不处理 */
		return;
	}
#endif
	pucVideoMem  = ptDefalutDisDev->pucDisMem;
	iBpp         = ptDefalutDisDev->iBpp;
	pucVideoMem  = pucVideoMem 
		+ ptButtonLayout->iTopLeftY * ptDefalutDisDev->iXres * iBpp / 8 
		+ ptButtonLayout->iTopLeftX * iBpp / 8;
	iInvertWidth = (ptButtonLayout->iBotRightX - ptButtonLayout->iTopLeftX + 1) * iBpp / 8;

	for(iYPos = ptButtonLayout->iTopLeftY; iYPos <= ptButtonLayout->iBotRightY; iYPos ++){
		for(iXPos = 0; iXPos < iInvertWidth; iXPos ++){
			pucVideoMem[iXPos] = ~pucVideoMem[iXPos];
		}
		pucVideoMem += ptDefalutDisDev->iXres * iBpp / 8;
	}
	
}

void ReleaseButton(struct DisLayout *ptButtonLayout)
{
	InvertButton(ptButtonLayout);
}

void PressButton(struct DisLayout *ptButtonLayout)
{
	InvertButton(ptButtonLayout);
}

int SetColorForOnePiexl(int iPenX, int iPeny, unsigned int dwColor, struct VideoMem *ptVideoMem)
{
	int iRed, iGreen, iBlue;
	unsigned char *pucMemStart;
	unsigned char *pucPen8bpp;
	unsigned short *pwPen16bpp;
	unsigned int *pdwPen32bpp;
	int iXres, iYres, iBpp;

	GetDisDeviceSize(&iXres, &iYres, &iBpp);

	if(iPenX > iXres || iPeny > iYres){
		DebugPrint(DEBUG_NOTICE"Out of screen\n");
		return -1;
	}

	pucMemStart = ptVideoMem->tPiexlDatas.pucPiexlDatasMem;
	pucPen8bpp  = (unsigned char *)
		pucMemStart + (iPeny * iXres + iPenX) * iBpp / 8;
	pwPen16bpp  = (unsigned short *)
		pucMemStart + (iPeny * iXres + iPenX) * iBpp / 16;
	pdwPen32bpp = (unsigned int *)
		pucMemStart + (iPeny * iXres + iPenX) * iBpp / 32;

	switch(iBpp){
		case 8: {
			*pucPen8bpp = dwColor;
			break;
		}
		case 16: {
			iRed   = (dwColor >> 16) & 0xff;
			iGreen = (dwColor >> 8) & 0xff;
			iBlue  = (dwColor >> 0) & 0xff;

			dwColor = ((iRed >> 3) << 11) | ((iGreen >> 2) << 5) | ((iBlue >> 3));
			*pwPen16bpp = dwColor;
			break;
		}
		case 32: {
			*pdwPen32bpp = dwColor;
			break;
		}
		default:{
			DebugPrint(DEBUG_NOTICE"Can't support bpp %d\n", iBpp);
			break;
		}
	}
	
	return 0;
}

static int isInArea(int iTopLeftX, int iTopLeftY, int iBotRightX, int iBotRightY, struct FontBitMap *ptFontBitmap)
{
	if(ptFontBitmap->iXLeft >= iTopLeftX 
		&& ptFontBitmap->iXMax <= iBotRightX
		&& ptFontBitmap->iYTop >= iTopLeftY
		&& ptFontBitmap->iYMax <= iBotRightY)
	{
		return 1;
	}

	return 0;
}

static int MergeOneFontToVideoMem(struct FontBitMap *ptFontBitMap, struct VideoMem *ptVideoMem)
{
	unsigned int dwXCod, dwYCod;
	unsigned int dw8PiexlNum;
	int dwPiexlNum;

	if(ptFontBitMap == NULL){
		return -1;
	}

	if(ptFontBitMap->iBpp == 1){
		
		/* mono bitmap, 1bit per piexl */
		for(dwYCod = ptFontBitMap->iYTop; dwYCod < ptFontBitMap->iYMax; dwYCod ++){
			/* align piexl */
			dw8PiexlNum = (dwYCod - ptFontBitMap->iYTop) * ptFontBitMap->iPatch;

			for(dwXCod = ptFontBitMap->iXLeft, dwPiexlNum = 7; dwXCod < ptFontBitMap->iXMax; dwXCod ++){
				if(ptFontBitMap->pucBuffer[dw8PiexlNum] & (1 << dwPiexlNum)){
					SetColorForOnePiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR, ptVideoMem);
				}else{
					/* keep background */
//					SetColorForOnePiexl(dwXCod, dwYCod, 0, ptVideoMem);
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

			for(dwXCod = ptFontBitMap->iXLeft; dwXCod < ptFontBitMap->iXMax; dwXCod ++){
				if(ptFontBitMap->pucBuffer[dw8PiexlNum]){
					SetColorForOnePiexl(dwXCod, dwYCod, CONFIG_FONT_COLOR, ptVideoMem);
				}else{
					/* keep background */
				}
				
				dw8PiexlNum ++;
			}
		}
	}else{
		DebugPrint(DEBUG_ERR"We can't support this BPP format\n");
		return -1;
	}
	return 0;
}

int ShowRectangle(int iTopLeftX, int iTopLeftY, int iBotRightX, int iBotRightY, unsigned int dwColor, struct VideoMem *ptVideoMem)
{
	unsigned char *pucMemStart = ptVideoMem->tPiexlDatas.pucPiexlDatasMem;
	int iXres, iYres, iBpp;
	int iDrawLineIndex;
	int iDrawWidthIndex;

	GetDisDeviceSize(&iXres, &iYres, &iBpp);

	if(iTopLeftX < 0 || iBotRightX > iXres
		|| iTopLeftY < 0 || iBotRightY > iYres)
	{
		DebugPrint("Out of range in ShowRectangle\n");
		return -1;
	}

	for(iDrawLineIndex = iTopLeftY; iDrawLineIndex < iBotRightY; iDrawLineIndex ++){
		for(iDrawWidthIndex = iTopLeftX; iDrawWidthIndex < iBotRightX; iDrawWidthIndex ++){
			SetColorForOnePiexl(iDrawWidthIndex, iDrawLineIndex, dwColor, ptVideoMem);
		}
	}

	return 0;	
}

int MergeString(int iTopLeftX, int iTopLeftY, int iBotRightX, int iBotRightY, unsigned char *strTextString, struct VideoMem *ptVideoMem, unsigned int dwColor)
{
	struct DisLayout tClearRegionDisLayout;

	int iError = 0;
	unsigned char *pucBufStart;
	unsigned char *pucBufEnd;
	int iLenDelta;

	unsigned int dwCode;
	int bHasGetCode = 0;  /* 标记是否已经得到了编码 */
	struct FontBitMap tFontBitmap;

	int iXMax = -1;
	int iYMax = -1;
	int iXMin = 65536;
	int iYMin = 65536;
	
	int iStringWidth;
	int iStringHeight;
	int iStringTopLeftX;
	int iStringTopLeftY;

	int i = 0;

	/* 清除要显示的区域 */
	tClearRegionDisLayout.iTopLeftX  = iTopLeftX;
	tClearRegionDisLayout.iTopLeftY  = iTopLeftY;
	tClearRegionDisLayout.iBotRightX = iBotRightX;
	tClearRegionDisLayout.iBotRightY = iBotRightY;
	ClearVideoMemRegion(ptVideoMem, &tClearRegionDisLayout, dwColor);

	tFontBitmap.iCurOriginX = 0;
	tFontBitmap.iCurOriginY = 0;
	pucBufStart = strTextString;
	pucBufEnd   = pucBufStart + strlen((char *)strTextString) - 1;

	/* 计算字符串的整体高度以及宽度 */
	while(1){
		iLenDelta = Utf8GetCodeBuf(pucBufStart, pucBufEnd, &dwCode);
		if(0 == iLenDelta){
			if(0 == bHasGetCode){
				return -1;   /* 出错了 */
			}else{
				break;
			}
		}

		bHasGetCode = 1;
		pucBufStart += iLenDelta;

		iError = GetFontBitmap(dwCode, &tFontBitmap);
		if(0 == iError){
			if(iXMin > tFontBitmap.iXLeft){
				iXMin = tFontBitmap.iXLeft;
			}
			if(iYMin > tFontBitmap.iYTop){
				iYMin = tFontBitmap.iYTop;
			}
			if(iXMax < tFontBitmap.iXMax){
				iXMax = tFontBitmap.iXMax;
			}
			if(iYMax < tFontBitmap.iYMax){
				iYMax = tFontBitmap.iYMax;
			}

			tFontBitmap.iCurOriginX = tFontBitmap.iNextOriginX;
			tFontBitmap.iCurOriginY = tFontBitmap.iNextOriginY;
		}else{
			DebugPrint(DEBUG_DEBUG"Get font bitmap error\n");
		}
	}

	/* 字符串整体的宽高 */
	iStringWidth  = iXMax - iXMin;
	iStringHeight = iYMax - iYMin;

	if(iStringWidth > iBotRightX - iTopLeftX){
		/* 截断字符 */
		iStringWidth = iBotRightX - iTopLeftX;
	}

	if(iStringHeight > iBotRightY - iTopLeftY){
		DebugPrint(DEBUG_DEBUG"string is too high\n");
		return -1;
	}

	iStringTopLeftX = iTopLeftX + (iBotRightX - iTopLeftX - iStringWidth) / 2;
	iStringTopLeftY = iTopLeftY + (iBotRightY - iTopLeftY - iStringHeight) / 2;

	/* 确定字符的原点 */
	tFontBitmap.iCurOriginX = iStringTopLeftX - iXMin;
	tFontBitmap.iCurOriginY = iStringTopLeftY - iYMin;

	pucBufStart = strTextString;
	bHasGetCode = 0;

	while(1){
		iLenDelta = Utf8GetCodeBuf(pucBufStart, pucBufEnd, &dwCode);
		if(0 == iLenDelta){
			if(0 == bHasGetCode){
				return -1;   /* 出错了 */
			}else{
				break;
			}
		}

		bHasGetCode = 1;
		pucBufStart += iLenDelta;

		iError = GetFontBitmap(dwCode, &tFontBitmap);
		if(0 == iError){
			if(isInArea(iTopLeftX, iTopLeftY, iBotRightX, iBotRightY, &tFontBitmap)){
				if(MergeOneFontToVideoMem(&tFontBitmap, ptVideoMem)){
					DebugPrint(DEBUG_DEBUG"MergeOneFontToVideoMem error\n");
				}
			}else{

				/* 显示 3 个点，表示还有一部分名字没有显示完全 */
				for(i = 0; i < 3; i ++){
					GetFontBitmap(46, &tFontBitmap); /* "."的 unicode 码是 46 */
					MergeOneFontToVideoMem(&tFontBitmap, ptVideoMem);
					tFontBitmap.iCurOriginX = tFontBitmap.iNextOriginX;
					tFontBitmap.iCurOriginY = tFontBitmap.iNextOriginY;
				}
				return 0;
			}

			tFontBitmap.iCurOriginX = tFontBitmap.iNextOriginX;
			tFontBitmap.iCurOriginY = tFontBitmap.iNextOriginY;
		}else{
			DebugPrint(DEBUG_DEBUG"Get font bitmap error\n");
		}
	}

	return 0;
}

int isPictureSupported(char *pcName)
{
	struct FileDesc tPicFileDesc;
	int iError = 0;

	snprintf(tPicFileDesc.cFileName, 256, "%s", pcName);
	tPicFileDesc.cFileName[255] = '\0';
		
	iError = GetFileDesc(&tPicFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"Get file descript error int isPictureSupported\n");
		return 0;
	}

	if(NULL == GetSupportedParser(&tPicFileDesc)){
		ReleaseFileDesc(&tPicFileDesc);
		return 0;
	}

	ReleaseFileDesc(&tPicFileDesc);
	return 1;
}

