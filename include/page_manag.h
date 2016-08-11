
#ifndef __PAGE_MANAG_H__
#define __PAGE_MANAG_H__

#include <common_config.h>
#include <common_st.h>
#include <input_manag.h>
#include <dis_manag.h>

/* 确定点击一个按键时按下与松开点的最大距离 */
//#define CONFIRM_CLICK_DISTANCE 200*200

struct PageIdetify
{
	int iPageID;
	char strCurPicFile[256];	/* 要处理的第一个图片文件 */
};

struct PageConfig
{
	int iIntervalSec;
	char strDirName[256];   /* 播放哪个目录下的图片文件 */
};

struct PageLayout
{
	int iTopLeftX;	/* 左上角 */
	int iTopLeftY;
	int iBotRightX;	/* 右下角 */
	int iBotRightY;
	int iBpp;
	int iMaxTotalBytes;
	struct DisLayout *atDisLayout;	/* layout 数组 */
};

struct PageOpr
{
	char *name;
	void (*RunPage)(struct PageIdetify *ptParentPageIdentify);
	int (*GetInputEvent)(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent);
	int (*Prepare)(void);	/* 后台准备函数，待实现 */
	struct list_head tPageOpr;
};

int RegisterPageOpr(struct PageOpr *ptPageOpr);
struct PageOpr *GetPageOpr(char *pcName);
void ShowPageOpr(void);
int PagesInit(void);
int MainPageInit(void);
int SettingPageInit(void);
int BrowsePageInit(void);
int PicturePageInit(void);
int MusicPageInit(void);
int IntervalPageInit(void);
int AutoPageInit(void);
int GetID(char *pcName);
int GenericGetInputEvent(struct PageLayout *ptPageLayout, struct InputEvent *ptInputEvent);
void GetSelectedAutoPageDir(char *strDirName);
void GetIntervalTime(int *piIntervalTime);
void GetPageConfig(struct PageConfig *ptPageConfig);
int DisOfTwoPoint(struct InputEvent *ptPreEvent, struct InputEvent *ptCurEvent);
void FreeDirAndFileIcons(void);

#endif

