#include <font_manag.h>
#include <stdio.h>
#include <string.h>

DECLARE_HEAD(g_tFontOprHead);
static int g_dwFontSize;
	
int RegisterFontOpr(struct FontOpr *ptFontOpr)
{
	ListAddTail(&ptFontOpr->tFontOpr, &g_tFontOprHead);

	return 0;
}

struct FontOpr *GetFontOpr(char *pcName)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct FontOpr *ptFOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tFontOprHead){
		ptFOTmpPos = LIST_ENTRY(ptLHTmpPos, struct FontOpr, tFontOpr);

		if(0 == strcmp(pcName, ptFOTmpPos->name)){
			return ptFOTmpPos;
		}
	}

	return NULL;
}

void ShowFontOpr(void)
{
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct FontOpr *ptFOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tFontOprHead){
		ptFOTmpPos = LIST_ENTRY(ptLHTmpPos, struct FontOpr, tFontOpr);

		printf("%d <---> %s\n", iPTNum++, ptFOTmpPos->name);
	}
}

void SetFontSize(unsigned int dwFontSize)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct FontOpr *ptFOTmpPos;

	g_dwFontSize = dwFontSize;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tFontOprHead){
		ptFOTmpPos = LIST_ENTRY(ptLHTmpPos, struct FontOpr, tFontOpr);

		if(ptFOTmpPos->SetFontSize){
			ptFOTmpPos->SetFontSize(dwFontSize);
		}
	}
}

int SetFontsDetail(char *pcFontName, char *pcFontFile, unsigned int dwFontSize)
{
	struct FontOpr *ptFontOpr;

	ptFontOpr = GetFontOpr(pcFontName);
	if(NULL == ptFontOpr){
		DebugPrint(DEBUG_ERR"Get font operation error in font_manag.c\n");
		return -1;
	}

	g_dwFontSize = dwFontSize;

	return ptFontOpr->FontInit(pcFontFile, dwFontSize);	
}

int GetFontBitmap(unsigned int dwCode, struct FontBitMap *ptFontBitMap)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct FontOpr *ptFOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tFontOprHead){
		ptFOTmpPos = LIST_ENTRY(ptLHTmpPos, struct FontOpr, tFontOpr);

		if(0 == ptFOTmpPos->GetFontBitMap(dwCode, ptFontBitMap)){
			return 0;
		}
	}

	return -1;
}

int FontInit(void)
{
	int iError = 0;

	iError = AsciiInit();
	if(iError){
		DebugPrint(DEBUG_ERR"AsciiInit error\n");
		return -1;
	}

	iError = GbkInit();
	if(iError){
		DebugPrint(DEBUG_ERR"AsciiInit error\n");
		return -1;
	}

	iError = FreeTypeInit();
	if(iError){
		DebugPrint(DEBUG_ERR"AsciiInit error\n");
		return -1;
	}

	return 0;
}

