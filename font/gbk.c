#include <font_manag.h>
#include <common_config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

static int GbkFontInit(char *pucFontFile, unsigned int dwFontSize);
static int GbkGetFontBitMap(unsigned int dwCode, struct FontBitMap *ptFontBitMap);

static int g_iHzkFd;
static struct stat g_tHzkStat;
static unsigned char *g_ucHzkMemStart;
static unsigned char *g_ucHzkMemEnd;


struct FontOpr g_tGbkFontOpr = {
	.name = "gbk",
	.FontInit = GbkFontInit,
	.GetFontBitMap = GbkGetFontBitMap,
};

static int GbkFontInit(char *pucFontFile, unsigned int dwFontSize)
{
	int iError = 0;

	if(dwFontSize != 16){
		DebugPrint(DEBUG_NOTICE"Gbk font can only support '16' size\n");
		return -1;
	}

	g_iHzkFd = open(pucFontFile, O_RDONLY);
	if(g_iHzkFd < 0){
		DebugPrint(DEBUG_ERR"Can't open %s\n", pucFontFile);
		return -1;
	}
	iError = fstat(g_iHzkFd, &g_tHzkStat);	/* Get file status */
	if(iError){
		DebugPrint(DEBUG_ERR"Can't load status of %s\n", pucFontFile);
		return -1;
	}

	g_ucHzkMemStart = (unsigned char *)mmap(NULL, g_tHzkStat.st_size, 
									PROT_READ, MAP_SHARED, g_iHzkFd, 0);
	if(g_ucHzkMemStart == (unsigned char *)-1){
		DebugPrint(DEBUG_ERR"Mmap g_ucHzkMem failed\n");
		return -1;
	}

	g_ucHzkMemEnd = g_ucHzkMemStart + g_tHzkStat.st_size;

	return 0;
}

static int GbkGetFontBitMap(unsigned int dwCode, struct FontBitMap *ptFontBitMap)
{
	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;

	int iArea  = (int)(dwCode & 0xff) - 0xA1;
	int iWhere = (int)((dwCode >> 8) & 0xff) - 0xA1;

	if(dwCode > 0xffff){
		DebugPrint(DEBUG_NOTICE"Gbk font has no this code %d\n", dwCode);
		return -1;
	}

	if(iArea < 0 || iWhere < 0){
		DebugPrint(DEBUG_NOTICE"Gbk font code is too small\n");
		return -1;
	}

	ptFontBitMap->iXLeft = iPenX;
	ptFontBitMap->iYTop  = iPenY - 16;
	ptFontBitMap->iXMax  = iPenX + 16;
	ptFontBitMap->iYMax  = iPenY;
	ptFontBitMap->iBpp   = 1;
	ptFontBitMap->iPatch = 2;
	ptFontBitMap->iNextOriginX = iPenX + 16;
	ptFontBitMap->iNextOriginY = iPenY;
	
	ptFontBitMap->pucBuffer    = (unsigned char *)
		(g_ucHzkMemStart + (iArea * 94 + iWhere) * 32);

	return 0;
}

int GbkInit(void)
{
	return RegisterFontOpr(&g_tGbkFontOpr);
}

