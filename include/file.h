#ifndef __FILE_H__
#define __FILE_H__

#include <common_config.h>
#include <common_st.h>
#include <stdio.h>

struct FileDesc
{
	char cFileName[256];
	FILE *ptFp;
	int iFileSize;
	unsigned char *pucFileMem;	/* 映射地址 */
};

enum FileType
{
	DIR_FILE = 0,
	NORMAL_FILE    /* 普通的文件 */
};

struct DirContent
{
	char strDirName[256];
	enum FileType eFileType;
};

int GetFileDesc(struct FileDesc *ptFileDesc);
void ReleaseFileDesc(struct FileDesc *ptFileDesc);
int GetDirContents(char *strcDirName, struct DirContent **aptDirContents, int *piFileNum);
void FreeDirContents(struct DirContent **aptDirContents);

#endif

