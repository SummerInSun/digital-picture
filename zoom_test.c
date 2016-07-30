#include <dis_manag.h>
#include <common_st.h>
#include <common_config.h>
#include <font_manag.h>
#include <encoding_manag.h>
#include <print_manag.h>
#include <input_manag.h>
#include <picfmt_manag.h>
#include <pic_operation.h>
#include <file.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <string.h>
#include <termios.h>

static struct DisOpr *g_ptFBDisOpr;
static struct PicFmtParser *g_ptBMPFmtParser;
static struct PiexlDatasDesc *g_ptBMPPiexlDatasDesc;
static struct FileDesc g_tBMPFileDesc;
static struct PiexlDatasDesc g_tPiexlDatas;
static struct PiexlDatasDesc g_tZoomPic;

int FbDrawLines(int iXStart, int iXEnd, int iYStart, unsigned char *pucLineBuff, int iPiexlLength)
{
	int iNPiexlNum;
	unsigned int dwColor;
	int iRed;
	int iGreen;
	int iBlue;

	unsigned short *pwLineBuff = (unsigned short *)(pucLineBuff);

	if(iYStart >= 272){
		return -1;
	}

	for(iNPiexlNum = iXStart; iNPiexlNum < iXEnd; iNPiexlNum ++){
		if(iNPiexlNum >= 480){
			return -1;
		}

		dwColor = pwLineBuff[iNPiexlNum];
		iRed    = dwColor >> 11 << 3;
		iGreen  = ((dwColor >> 5) & 0x3f) << 2;
		iBlue   = (dwColor & 0x1f) << 3;
		dwColor = (iRed << 16) | (iGreen << 8) | (iBlue);
		g_ptFBDisOpr->ShowPiexl(iNPiexlNum, iYStart, dwColor);
	}

	return 0;
}


int main(int argc, char *argv[])  
{
	int iError = 0;
	int Xres;
	int Yres;
	int YPos;

	if(argc != 2){
		printf("exe <pic.bmp>\n");
		goto error_exit;
	}
	
	iError = PrintDeviceInit();
	if(iError){
		goto error_exit;
	}

	iError = DisplayInit();
	if(iError){
		DebugPrint(DEBUG_ERR"DisplayInit error\n");
		goto error_exit;
	}

	g_ptFBDisOpr = GetDisOpr("fb");
	g_ptFBDisOpr->DisDeviceInit();
	g_ptFBDisOpr->CleanScreen(0);

	iError = PicFmtInit();
	if(iError){
		DebugPrint(DEBUG_ERR"PicFmtInit error\n");
		goto error_exit;
	}
	g_ptBMPFmtParser = GetPicFmtParser("bmp");

	ShowPicFmtParser();

	/* ÏÔÊ¾Ò»¸± BMP Í¼Æ¬ */
	strcpy(g_tBMPFileDesc.cFileName, argv[1]);
	DebugPrint("File name %s\n", g_tBMPFileDesc.cFileName);
	iError = GetFileDesc(&g_tBMPFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"GetFileDesc error\n");
		goto error_exit;
	}
	g_tPiexlDatas.iBpp = 16;
	g_ptBMPFmtParser->GetPiexlDatas(&g_tBMPFileDesc, &g_tPiexlDatas);
	
	for(YPos = 0; YPos < g_tPiexlDatas.iHeight; YPos ++){
		FbDrawLines(0, g_tPiexlDatas.iWidth, YPos, 
			g_tPiexlDatas.pucPiexlDatasMem+YPos*g_tPiexlDatas.iLineLength, 2);
	}

	g_tZoomPic.iBpp    = g_tPiexlDatas.iBpp;
	g_tZoomPic.iWidth  = g_tPiexlDatas.iWidth / 2;
	g_tZoomPic.iHeight = g_tPiexlDatas.iHeight / 2;
	g_tZoomPic.iLineLength  = g_tZoomPic.iWidth * g_tZoomPic.iBpp / 8;
	g_tZoomPic.iTotalLength = g_tZoomPic.iLineLength * g_tZoomPic.iHeight;

	g_tZoomPic.pucPiexlDatasMem = malloc(g_tZoomPic.iTotalLength * 20);
	if(NULL == g_tZoomPic.pucPiexlDatasMem){
		DebugPrint(DEBUG_ERR"g_tZoomPic.pucPiexlDatasMem malloc error\n");
		goto error_exit;
	}

	DebugPrint("Find segment fault\n");

	PicZoomOpr(&g_tPiexlDatas, &g_tZoomPic);

	DebugPrint("Find segment fault\n");
	
	for(YPos = 128; YPos < 128 + g_tZoomPic.iHeight; YPos ++){
		FbDrawLines(256, 256 + g_tZoomPic.iWidth, YPos, 
			g_tZoomPic.pucPiexlDatasMem+(YPos-128)*g_tZoomPic.iLineLength, 2);
	}

	g_ptBMPFmtParser->FreePiexlDatas(&g_tZoomPic);
	g_ptBMPFmtParser->FreePiexlDatas(&g_tPiexlDatas);
	fclose(g_tBMPFileDesc.ptFp);

exit:
	return 0;
error_exit:
	return -1;

}


