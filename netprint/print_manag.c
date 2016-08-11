#include <print_manag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static int g_iDebugLevelForAllPrint = DEBUG_DEFAULT;

DECLARE_HEAD(g_tPrintOprHead);

int RegisterPrintOpr(struct PrintOpr *tPrintOpr)
{
	ListAddTail(&tPrintOpr->tPrintOpr, &g_tPrintOprHead);

	return 0;
}

void UnregisterPrintOpr(struct PrintOpr *tPrintOpr)
{
	if(NULL != tPrintOpr){
		ListDelTail(&tPrintOpr->tPrintOpr);
	}
}

void ShowPrintOpr(void)
{
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PrintOpr *ptPOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPrintOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PrintOpr, tPrintOpr);

		printf("%d <---> %s\n", iPTNum++, ptPOTmpPos->name);
	}	
}

struct PrintOpr *GetPrintOpr(char *pcName)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PrintOpr *ptPOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPrintOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PrintOpr, tPrintOpr);

		if(0 == strcmp(pcName, ptPOTmpPos->name)){
			return ptPOTmpPos;
		}
	}

	return NULL;
}

void PrintDeviceExit(void)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PrintOpr *ptPOTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPrintOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PrintOpr, tPrintOpr);

		ptPOTmpPos->PrintDeviceExit();
	}	
}

int PrintDeviceInit(void)
{
	int iError = 0;

	iError = ConsolePrintInit();
	if(iError){
		DebugPrint(DEBUG_NOTICE"ConsolePrintInit error\n");
		ConsolePrintExit();
	}

	iError &= NetPrintInit();
	if(iError){
		DebugPrint(DEBUG_NOTICE"ConsolePrintInit error\n");
		NetPrintExit();
	}

	return 0;
}

int DebugPrint(const char *cFormat, ...)
{
	va_list tArg;
	int iNum;
	
	char pcPrintStrTmp[1024];
	int iDebugLevel = g_iDebugLevelForAllPrint;	/* 自动添加默认的打印级别 */

	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PrintOpr *ptPOTmpPos;

	/* 判断打印级别，如果够打印级别的话就打印，否则不打印 */
	if(cFormat[0] == '<' && cFormat[2] == '>'){
		if(cFormat[1] >= '0' && cFormat[1] <= '7'){
			iDebugLevel = cFormat[1] - '0';
		}
		cFormat += 3;	/* 跳过打印级别 */
	}

	if(iDebugLevel > g_iDebugLevelForAllPrint){
		return 0;
	}
	
	va_start (tArg, cFormat);
	iNum = vsprintf(pcPrintStrTmp, cFormat, tArg);
	va_end (tArg);
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPrintOprHead){
		ptPOTmpPos = LIST_ENTRY(ptLHTmpPos, struct PrintOpr, tPrintOpr);
		if(ptPOTmpPos->isSelected){
			ptPOTmpPos->DoPrint(pcPrintStrTmp);
		}
	}	
	
	return iNum;
}

void SetGlobalDebugLevel(int iDebugLevel)
{
	g_iDebugLevelForAllPrint = iDebugLevel;
}

