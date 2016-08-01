#include <picfmt_manag.h>
#include <pic_operation.h>
#include <print_manag.h>
#include <stdlib.h>
#include <string.h>

/* 近邻插值法进行缩放
 *
 * Sx 原图的x坐标 Sw 原图的宽
 * Sy 原图的y坐标 Sh 原图的高
 * Dx 缩放图的x坐标 Dw 缩放图的宽
 * Dy 缩放图的y坐标 Dh 缩放图的高
 * 
 * 等于说仅仅是抽样取值，放大缩小倍数过大的话效果不好，可以改进，根据周围的像素
 * 取其像素的平均值来进行缩放与放大
 * 
 * Sx=Dx*Sw/Dw 
 * Sy=Dy*Sh/Dh
 */
int PicZoomOpr(struct PiexlDatasDesc *ptOrigin, struct PiexlDatasDesc *ptZoom)
{
	unsigned int dwSrcY;
	unsigned int dwSrcX;

	unsigned int dwZoomWidth;
	unsigned int dwZoomHeight;
	unsigned int dwPiexlBytes;
	unsigned int dwZoomX;
	unsigned int dwZoomY;

	unsigned char *pucDesDatas;
	unsigned char *pucSrcDatas;
	float fFw;
	float fFh;

	dwZoomWidth  = ptZoom->iWidth;
	dwZoomHeight = ptZoom->iHeight;
	dwPiexlBytes = ptOrigin->iBpp / 8;

	/* 如果两者的像素宽度不一致就不进行缩放 */
	if(ptOrigin->iBpp != ptZoom->iBpp){
		return -1;
	}

	fFw = (float)(ptOrigin->iWidth) / (float)(dwZoomWidth);
	fFh = (float)(ptOrigin->iHeight) / (float)(dwZoomHeight);

	/* 对 Y 轴进行抽样，并拷贝 X 轴的抽样数据 */
	for(dwZoomY = 0; dwZoomY < dwZoomHeight; dwZoomY ++){
		dwSrcY = (unsigned int)(dwZoomY * fFh);

		pucDesDatas = ptZoom->pucPiexlDatasMem + dwZoomY * ptZoom->iLineLength;
		pucSrcDatas = ptOrigin->pucPiexlDatasMem + dwSrcY * ptOrigin->iLineLength;

		for(dwZoomX = 0; dwZoomX < dwZoomWidth; dwZoomX ++){
			dwSrcX = (unsigned int)(dwZoomX * fFw);
			memcpy(pucDesDatas + dwZoomX * dwPiexlBytes, 
				pucSrcDatas + dwSrcX * dwPiexlBytes, dwPiexlBytes);
		}
	}

	return 0;
}


/* 近邻插值法进行缩放
 *
 * Sx 原图的x坐标 Sw 原图的宽
 * Sy 原图的y坐标 Sh 原图的高
 * Dx 缩放图的x坐标 Dw 缩放图的宽
 * Dy 缩放图的y坐标 Dh 缩放图的高
 * 
 * 等于说仅仅是抽样取值，放大缩小倍数过大的话效果不好，可以改进，根据周围的像素
 * 取其像素的平均值来进行缩放与放大
 * 
 * Sx=Dx*Sw/Dw 
 * Sy=Dy*Sh/Dh
 */
int PicZoomOprBackup(struct PiexlDatasDesc *ptOrigin, struct PiexlDatasDesc *ptZoom)
{
	unsigned int *pdwSrcXTable;
	unsigned int dwSrcY;

	unsigned int dwZoomWidth;
	unsigned int dwZoomHeight;
	unsigned int dwPiexlBytes;
	unsigned int dwZoomX;
	unsigned int dwZoomY;

	unsigned char *pucDesDatas;
	unsigned char *pucSrcDatas;

	dwZoomWidth  = ptZoom->iWidth;
	dwZoomHeight = ptZoom->iHeight;
	dwPiexlBytes = ptOrigin->iBpp / 8;

	/* 如果两者的像素宽度不一致就不进行缩放 */
	if(ptOrigin->iBpp != ptZoom->iBpp){
		return -1;
	}

	pdwSrcXTable = malloc(sizeof(unsigned int) * (dwZoomWidth));
	if(NULL == pdwSrcXTable){
		DebugPrint(DEBUG_ERR"pdwSrcXTable Malloc error\n");
		return -1;
	}

	/* 对 X 轴进行抽样 */
	for(dwZoomX = 0; dwZoomX < dwZoomWidth; dwZoomX ++){
		pdwSrcXTable[dwZoomX] = (dwZoomX * ptOrigin->iWidth / ptZoom->iWidth);
	}

	/* 对 Y 轴进行抽样，并拷贝 X 轴的抽样数据 */
	for(dwZoomY = 0; dwZoomY < dwZoomHeight; dwZoomY ++){
		dwSrcY = (dwZoomY * ptOrigin->iHeight / ptZoom->iHeight);

		pucDesDatas = ptZoom->pucPiexlDatasMem + dwZoomY * ptZoom->iLineLength;
		pucSrcDatas = ptOrigin->pucPiexlDatasMem + dwSrcY * ptOrigin->iLineLength;

		for(dwZoomX = 0; dwZoomX < dwZoomWidth; dwZoomX ++){
			memcpy(pucDesDatas + dwZoomX * dwPiexlBytes, 
				pucSrcDatas + pdwSrcXTable[dwZoomX] * dwPiexlBytes, dwPiexlBytes);
			
		}
	}

	free(pdwSrcXTable);

	return 0;
}


/* 
 * 把小的图片合并到大的图片里面
 * iXPos 与 iYPos 确定图片左上角的位置(大图片里面)
 * 上面两个坐标都是相对于大图片的左上角得到的
 * 
 */
int PicMergeOpr(int iXPos, int iYPos, struct PiexlDatasDesc *ptSmallPic, struct PiexlDatasDesc *ptBigPic)
{
	unsigned char *pucDesDatas;
	unsigned char *pucSrcDatas;
	unsigned int dwPiexlLineNum;
	int dwYSize;
	int dwXSize;

	if((ptSmallPic->iHeight > ptBigPic->iHeight) || 
		(ptSmallPic->iWidth > ptBigPic->iWidth) || 
		(ptSmallPic->iBpp != ptBigPic->iBpp))
	{
		return -1;
	}

	dwXSize = ptSmallPic->iWidth;
	dwYSize = ptSmallPic->iHeight;

	if(iYPos < 0){
		iYPos = 0;
	}

	if(iXPos < 0){
		iXPos = 0;
	}

	if(ptSmallPic->iHeight + iYPos > ptBigPic->iHeight){
		dwYSize = ptBigPic->iHeight - iYPos;
	}

	if(ptSmallPic->iWidth + iXPos > ptBigPic->iWidth){
		dwXSize = ptBigPic->iWidth - iXPos;
	}

	pucSrcDatas = ptSmallPic->pucPiexlDatasMem;
	pucDesDatas = ptBigPic->pucPiexlDatasMem + iYPos * ptBigPic->iLineLength + iXPos * ptBigPic->iBpp / 8;

	for(dwPiexlLineNum = 0; dwPiexlLineNum < dwYSize; dwPiexlLineNum ++){
		memcpy(pucDesDatas, pucSrcDatas, dwXSize * ptBigPic->iBpp / 8);
		pucSrcDatas += ptSmallPic->iLineLength;
		pucDesDatas += ptBigPic->iLineLength;
	}

	return 0;
}

