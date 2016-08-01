#ifndef __PRINT_MANAG_H__
#define __PRINT_MANAG_H__

#include <common_config.h>
#include <common_st.h>

struct PrintOpr{
	char *name;
	int (*PrintDeviceInit)(void);
	int (*PrintDeviceExit)(void);
	int (*DoPrint)(char *pcPrintBuf);
	int isSelected;
	struct list_head tPrintOpr;
};

int RegisterPrintOpr(struct PrintOpr *tPrintOpr);
void UnregisterPrintOpr(struct PrintOpr *tPrintOpr);
void ShowPrintOpr(void);
struct PrintOpr *GetPrintOpr(char *pcName);
int ConsolePrintInit(void);
void ConsolePrintExit(void);
int NetPrintInit(void);
void PrintDeviceExit(void);
void NetPrintExit(void);
int PrintDeviceInit(void);
int DebugPrint(const char *cFormat, ...);
void SetGlobalDebugLevel(int iDebugLevel);

#endif

