#ifndef __PICFMT_MANAG_H__
#define __PICFMT_MANAG_H__

#include <common_config.h>
#include <common_st.h>
#include <file.h>

struct PiexlDatasDesc
{
	int iWidth;
	int iHeight;
	int iBpp;
	int iLineLength;
	int iTotalLength;
	unsigned char *pucPiexlDatasMem;
};

struct PicFmtParser
{
	char *name;
	int (*isSupport)(struct FileDesc *ptFileDesc);
	int (*GetPiexlDatas)(struct FileDesc *ptFileDesc, struct PiexlDatasDesc *ptPiexlDatasDesc);
	int (*FreePiexlDatas)(struct PiexlDatasDesc *ptPiexlDatasDesc);
	struct list_head ptPicFmtParser;
};

int RegisterPicFmtParser(struct PicFmtParser *ptPicFmtParser);
struct PicFmtParser *GetPicFmtParser(char *pcName);
void ShowPicFmtParser(void);
struct PicFmtParser *GetSupportedParser(struct FileDesc *ptFileDesc);
int BMPInit(void);
int JPGInit(void);
int PicFmtInit(void);

#endif

