#ifndef __DRAW_H__
#define __DRAW_H__

#include <print_manag.h>

#define NORMAL_FONT    0
#define LEAN_FONT      1
#define UNDERLINE_FONT 2
#define OVERSTRIKING_FONT   4

struct FontDisOpt{
	unsigned int  dwBackGroundColor;
	unsigned int  dwFontColor;
	unsigned char ucFontMode;
};

int OpenTextFile(char *pcFileName);
int SetTextDetail(char *pcHZKFile, char *pcFileFreetype, unsigned int dwFontSize);
int SelectAndInitDisplay(char *pcName);
int ShowNextPage(void);
int ShowPrePage(void);
int ReShowCurPage();
int RowDownOneLine(void);
int RowUpOneLine(void);

struct FontDisOpt *GetFontDisOpt(void);

//int ShowOneWord(struct FontBitMap *ptFontBitMap);

#endif
