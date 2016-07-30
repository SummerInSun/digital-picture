#include <input_manag.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static int ConsoleInputDeviceExit(void);
static int ConsoleInputDeviceInit(void);
static int ConsoleGetInputEvent(struct InputEvent *ptInputEvent);

static struct termios g_tOldTermIos;
int g_iConsoleFd;
fd_set g_tFdSet;

struct InputOpr g_tConsoleInputOpr = {
	.name = "console-input",
	.InputDeviceExit = ConsoleInputDeviceExit,
	.InputDeviceInit = ConsoleInputDeviceInit,
	.GetInputEvent   = ConsoleGetInputEvent,
};

static int ConsoleInputDeviceExit(void)
{
	/* 还原控制台状态 */
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tOldTermIos);

	return 0;

}

static int ConsoleInputDeviceInit(void)
{
	struct termios tNewTermIos;

    /* read terminal settings */
    tcgetattr(STDIN_FILENO, &g_tOldTermIos); 

	/* init new settings */
    tNewTermIos = g_tOldTermIos;  
    /* change settings */
    tNewTermIos.c_lflag &= ~(ICANON|ECHO);
	tNewTermIos.c_cc[VMIN] = 1;	
    /* apply */
    tcsetattr(STDIN_FILENO, TCSANOW, &tNewTermIos);

	g_tConsoleInputOpr.iInputDeviceFd = STDIN_FILENO;

	return 0;
}

static int ConsoleGetInputEvent(struct InputEvent *ptInputEvent)
{
	char cOprCode;

	DebugPrint(DEBUG_DEBUG"Keyboard read data\n");
	
	cOprCode = getchar();
	DEBUG_PRINTF("%c", cOprCode);
	gettimeofday(&ptInputEvent->tTimeVal, NULL);
	
	ptInputEvent->iInputType = INPUT_TYPE_CONSOLE;
	ptInputEvent->iInputCode = INPUT_CODE_UNKNOW;
	if(cOprCode == 'n'){
		ptInputEvent->iInputCode = INPUT_CODE_DOWN;			
	}
	if(cOprCode == 'u'){
		ptInputEvent->iInputCode = INPUT_CODE_UP;
	}
	if(cOprCode == 'l'){
		ptInputEvent->iInputCode = INPUT_CTRL_LEAN;
	}
	if(cOprCode == 'o'){
		ptInputEvent->iInputCode = INPUT_CTRL_OVERSTRIKING;
	}
	if(cOprCode == 'e'){
		ptInputEvent->iInputCode = INPUT_CTRL_UNDERLINE;
	}
	if(cOprCode == 'r'){
		ptInputEvent->iInputCode = INPUT_CTRL_RECOVERY;
	}
	if(cOprCode == 'q'){
		ptInputEvent->iInputCode = INPUT_CODE_EXIT;
	}
	return 0;
	
}

int ConsoleInit(void)
{
	int iError = 0;

	iError = ConsoleInputDeviceInit();
	iError |= RegisterInputOpr(&g_tConsoleInputOpr);

	 return iError;
}

void ConsoleExit(void)
{
	
	ConsoleInputDeviceExit();
	UnregisterInputOpr(&g_tConsoleInputOpr);
}
