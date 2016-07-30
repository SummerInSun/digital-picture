#ifndef __ENCODING_MANAG_H__
#define __ENCODING_MANAG_H__

#include <common_st.h>
#include <common_config.h>
#include <font_manag.h>
#include <print_manag.h>

struct EncodingOpr{
	char *name;
	int iHeadLen;
	struct FontOpr *ptFontOprSupportedHead;
	int (*isSupported)(unsigned char *pucTextHead);
	int (*GetCodeFromText)(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);
	struct list_head tEncodingOpr;
};

int RegisterEncodingOpr(struct EncodingOpr *ptEncodingOpr);
void ShowEncodingOpr(void);
struct EncodingOpr *GetEncodingOpr(char *pcName);
struct EncodingOpr *SelectEncodingOprForFile(unsigned char *pucFileHead);
int AddFontOprForEncoding(struct EncodingOpr *ptEncodingOpr, struct FontOpr *ptFontOpr);
int DelFontOprForEncoding(struct EncodingOpr *ptEncodingOpr, struct FontOpr *ptFontOpr);
int EncodingInit(void);
int AnsiEncodingInit(void);
int  Utf16beEncodingInit(void);
int  Utf16leEncodingInit(void);
int  Utf8EncodingInit(void);
int Utf8GetCodeBuf(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);

#endif
