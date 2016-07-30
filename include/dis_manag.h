/* LCD 显示控制管理头文件
 * 包括显示模块的结构体以及相应的操作函数
 *
 */

#ifndef __DIS_MANAG_H__
#define __DIS_MANAG_H__

#include <common_st.h>
#include <common_config.h>
#include <picfmt_manag.h>

struct DisLayout
{
	int iTopLeftX;	/* 左上角 */
	int iTopLeftY;
	int iBotRightX;	/* 右下角 */
	int iBotRightY;
	char *pcIconName;	/* 图标的名字 */
};

enum VideoMemStat
{
	VMS_FREE = 0,
	VMS_BUSY_FOR_PREPARE,	/* 被准备子线程使用 */
	VMS_BUSY_FOR_MAIN,	/* 被当前的主线程使用 */
};

enum PicStat
{
	PS_BLANK = 0,
	PS_GENERATING,	/* 正在生成 */
	PS_GENERATED,	/* 已经生成 */
};

struct VideoMem
{
	int iID;
	int bIsDevFrameBuffer;               /* 标识是否是设备的显存，用于无其余内存空间的情况 */
	enum VideoMemStat eVMStat;
	enum PicStat ePStat;
	struct PiexlDatasDesc tPiexlDatas; /* 图像描述 */
	struct VideoMem *ptVideoMemNext;     /* 链表 */
};

struct DisOpr{
	char *name;
	int iXres;
	int iYres;
	int iBpp;
	unsigned char *pucDisMem;    /* 显示设备的显存地址 */
	int (*DisDeviceInit)(void);
	int (*ShowPiexl)(int iPenX, int iPeny, unsigned int dwColor);
	int (*CleanScreen)(unsigned int dwBackColor);
	void (*DrawPage)(struct VideoMem *ptPageVideoMem);
	struct list_head tDisOpr;
};

int RegisterDispOpr(struct DisOpr *pt_DisOpr);
void ShowDisOpr(void);
struct DisOpr *GetDisOpr(char *pcName);
int DisplayInit(void);
int FBInit(void);
int GetDisDeviceSize(int *piXres, int *piYres, int *iBpp);
struct VideoMem *GetDevVideoMem();
int AllocVideoMem(int iMemNum);
void FreeAllVideoMem(void);
struct VideoMem *GetVideoMem(int iMemID, int bCur);
void ReleaseVideoMem(struct VideoMem *ptVideoMem);
int SelectAndInitDefaultDisOpr(char *pcName);
struct DisOpr *GetDefaultDisOpr(void);
void ClearVideoMemRegion(struct VideoMem *ptVideoMem, struct DisLayout *ptDisLayout, unsigned int dwColor);

#endif

