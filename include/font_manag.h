#ifndef __FONT_MANAG_H__
#define __FONT_MANAG_H__

#include <common_st.h>
#include <common_config.h>
#include <print_manag.h>

struct FontBitMap{
	int iXLeft;
	int iYTop;
	int iXMax;
	int iYMax;
	int iBpp;
	int iPatch; /* for single bitmap, the distance of two piexl line */
	int iCurOriginX;
	int iCurOriginY;
	int iNextOriginX;
	int iNextOriginY;
	unsigned char *pucBuffer;	/* all piexls */
};

struct FontOpr{
	char *name;
	int (*FontInit)(char *pucFontFile, unsigned int dwFontSize);
	int (*GetFontBitMap)(unsigned int dwCode, struct FontBitMap *ptFontBitMap);
	int (*SetFontSize)(unsigned int dwFontSize);
	struct list_head tFontOpr;
	struct FontOpr *ptFontOprNext;
};

int RegisterFontOpr(struct FontOpr *ptFontOpr);
struct FontOpr *GetFontOpr(char *pcName);
void ShowFontOpr(void);
void SetFontSize(unsigned int dwFontSize);
int GetFontBitmap(unsigned int dwCode, struct FontBitMap *ptFontBitMap);
int SetFontsDetail(char *pcFontName, char *pcFontFile, unsigned int dwFontSize);
int FontInit(void);
int AsciiInit(void);
int GbkInit(void);
int FreeTypeInit(void);

#endif
