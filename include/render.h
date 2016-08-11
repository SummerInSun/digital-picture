
#ifndef __RENDER_H__
#define __RENDER_H__

#include <common_config.h>
#include <dis_manag.h>
#include <page_manag.h>

int GeneratePage(struct PageLayout *ptPageLayout, struct VideoMem *ptVideoMem);
void FlushVideoMemToDev(struct VideoMem *ptFlushVideoMem);
void ReleaseButton(struct DisLayout *ptButtonLayout);
void PressButton(struct DisLayout *ptButtonLayout);
int GetPiexlDatasForIcons(char *pcName, struct PiexlDatasDesc *ptPiexlData);
int GetPiexlDatasForPic(char *pcName, struct PiexlDatasDesc *ptPiexlData);
void FreePiexlDatasForIcon(struct PiexlDatasDesc *ptPiexlData);
int SetColorForOnePiexl(int iPenX, int iPeny, unsigned int dwColor, struct VideoMem *ptVideoMem);
int ShowRectangle(int iTopLeftX, int iTopLeftY, int iBotRightX, int iBotRightY, unsigned int dwColor, struct VideoMem *ptVideoMem);
int MergeString(int iTopLeftX, int iTopLeftY, int iBotRightX, int iBotRightY, unsigned char *strTextString, struct VideoMem *ptVideoMem, unsigned int dwColor);
int isPictureSupported(char *pcName);

#endif
