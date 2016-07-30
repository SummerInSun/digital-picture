#include <encoding_manag.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

DECLARE_HEAD(g_tEncodingOprHead);

int RegisterEncodingOpr(struct EncodingOpr *ptEncodingOpr)
{
	ListAddTail(&ptEncodingOpr->tEncodingOpr, &g_tEncodingOprHead);

	return 0;
}

void ShowEncodingOpr(void)
{
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct EncodingOpr *ptEOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tEncodingOprHead){
		ptEOTmpPos = LIST_ENTRY(ptLHTmpPos, struct EncodingOpr, tEncodingOpr);

		printf("%d <---> %s\n", iPTNum++, ptEOTmpPos->name);
	}
}
struct EncodingOpr *GetEncodingOpr(char *pcName)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct EncodingOpr *ptEOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tEncodingOprHead){
		ptEOTmpPos = LIST_ENTRY(ptLHTmpPos, struct EncodingOpr, tEncodingOpr);

		if(0 == strcmp(pcName, ptEOTmpPos->name)){
			return ptEOTmpPos;
		}
	}

	return NULL;
}

struct EncodingOpr *SelectEncodingOprForFile(unsigned char *pucFileHead)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct EncodingOpr *ptEOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tEncodingOprHead){
		ptEOTmpPos = LIST_ENTRY(ptLHTmpPos, struct EncodingOpr, tEncodingOpr);

		if(ptEOTmpPos->isSupported(pucFileHead)){
			return ptEOTmpPos;
		}
	}

	return NULL;
}

int AddFontOprForEncoding(struct EncodingOpr *ptEncodingOpr, struct FontOpr *ptFontOpr)
{
	struct FontOpr *ptFOTmpPos;

	if(ptEncodingOpr == NULL || ptFontOpr == NULL){
		return -1;
	}

	ptFOTmpPos = malloc(sizeof(struct FontOpr));
	if(ptFOTmpPos == NULL){
		DebugPrint(DEBUG_ERR"Malloc FontOpr failed, no space\n");
		return -1;
	}

	memcpy(ptFOTmpPos, ptFontOpr, sizeof(struct FontOpr));
	ptFOTmpPos->ptFontOprNext = ptEncodingOpr->ptFontOprSupportedHead;
	ptEncodingOpr->ptFontOprSupportedHead = ptFOTmpPos;

	return 0;
}

int DelFontOprForEncoding(struct EncodingOpr *ptEncodingOpr, struct FontOpr *ptFontOpr)
{
	struct FontOpr *ptFOTmpPos;
	struct FontOpr *ptFOTmpPrePos;

	if(ptEncodingOpr == NULL || ptFontOpr == NULL){
		return -1;
	}
	
	ptFOTmpPos = ptEncodingOpr->ptFontOprSupportedHead;
	if (strcmp(ptFOTmpPos->name, ptFontOpr->name) == 0)
	{
		/* 删除头节点 */
		ptEncodingOpr->ptFontOprSupportedHead = ptFOTmpPos->ptFontOprNext;
		free(ptFOTmpPos);
		return 0;
	}

	ptFOTmpPrePos = ptEncodingOpr->ptFontOprSupportedHead;
	ptFOTmpPos = ptFOTmpPrePos->ptFontOprNext;
	while (ptFOTmpPos)
	{
		if (strcmp(ptFOTmpPos->name, ptFontOpr->name) == 0)
		{
			/* 从链表里取出、释放 */
			ptFOTmpPrePos->ptFontOprNext = ptFOTmpPos->ptFontOprNext;
			free(ptFOTmpPos);
			return 0;
		}
		else
		{
			ptFOTmpPrePos = ptFOTmpPos;
			ptFOTmpPos = ptFOTmpPos->ptFontOprNext;
		}
	}

	return -1;
}

/* 返回值是得到的编码个数 */
int Utf8GetCodeBuf(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode)
{
	return GetEncodingOpr("utf8-encoding")->GetCodeFromText(pucTextStart, pucTextEnd, pdwCode);
}

int EncodingInit(void)
{
	int iError = 0;

	iError = AnsiEncodingInit();
	if(iError){
		DebugPrint(DEBUG_ERR"AsciiEncodingInit error\n");
		return -1;
	}
	
	iError = Utf16beEncodingInit();
	if(iError){
		DebugPrint(DEBUG_ERR"Utf16beEncodingInit error\n");
		return -1;
	}
	
	iError = Utf16leEncodingInit();
	if(iError){
		DebugPrint(DEBUG_ERR"Utf16leEncodingInit error\n");
		return -1;
	}
	
	iError = Utf8EncodingInit();
	if(iError){
		DebugPrint(DEBUG_ERR"Utf8EncodingInit error\n");
		return -1;
	}

	return 0;
}


