#include <print_manag.h>
#include <stdio.h>

static int ConsolePrintDeviceInit(void);
static int ConsolePrintDeviceExit(void);
static int ConsoleDoPrint(char *pcPrintBuf);

struct PrintOpr g_tConsolePrintOpr = {
	.name = "console-print",
	.PrintDeviceInit = ConsolePrintDeviceInit,
	.PrintDeviceExit = ConsolePrintDeviceExit,
	.DoPrint = ConsoleDoPrint,
	.isSelected = 1,
};

static int ConsolePrintDeviceInit(void)
{
	return 0;
}

static int ConsolePrintDeviceExit(void)
{
	return 0;
}

static int ConsoleDoPrint(char *pcPrintBuf)
{
	printf("%s", pcPrintBuf);

	return 0;
}

int ConsolePrintInit(void)
{
	int iError = 0;
	
	iError = ConsolePrintDeviceInit();
	iError |= RegisterPrintOpr(&g_tConsolePrintOpr);

	return iError;
}

void ConsolePrintExit(void)
{
	ConsolePrintDeviceExit();
	UnregisterPrintOpr(&g_tConsolePrintOpr);
}

