#ifndef __INPUT_MANAG_H__
#define __INPUT_MANAG_H__

#include <common_st.h>
#include <common_config.h>
#include <sys/time.h>
#include <pthread.h>
#include <print_manag.h>

/* Input type */
#define INPUT_TYPE_CONSOLE 0
#define INPUT_TYPE_TS      1
#define INPUT_TYPE_BUTTON  2

#define INPUT_CODE_DOWN    0
#define INPUT_CODE_UP      1
#define INPUT_CODE_EXIT    2
#define INPUT_CTRL_LEAN    3
#define INPUT_CTRL_OVERSTRIKING    4
#define INPUT_CTRL_UNDERLINE       5
#define INPUT_CTRL_RECOVERY        6

#define INPUT_CODE_UNKNOW  -1

struct InputEvent{
	struct timeval tTimeVal;
	int iInputType;
	int iInputCode;
	int iXPos;
	int iYPos;
	int bPressure;	/* 1为按下，0为松开 */
};

struct InputOpr{
	char *name;
	int (*InputDeviceExit)(void);
	int (*InputDeviceInit)(void);
	int (*GetInputEvent)(struct InputEvent *ptInputEvent);
	int iInputDeviceFd;
	pthread_t tThreadId;
	struct list_head tInputOpr;
};

int RegisterInputOpr(struct InputOpr *ptInputOpr);
void UnregisterInputOpr(struct InputOpr *ptInputOpr);
struct InputOpr *GetInputOpr(char *pcName);
void ShowInputOpr(void);
int GetInputEvent(struct InputEvent *ptInputEvent);
int ConsoleInit(void);
void ConsoleExit(void);
int TouchScreenInit(void);
void TouchScreenExit(void);
int BoardButtonInit(void);
void BoardButtonExit(void);
int InputDeviceInit(void);
int InputDeviceExit(void);
int AllInputFdInit();

#endif
