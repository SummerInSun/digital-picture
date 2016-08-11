#include <file.h>
#include <print_manag.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

int GetFileDesc(struct FileDesc *ptFileDesc)
{
	int iFd;
	int iError;
	FILE *tFp;
	struct stat tFileStat;

	tFp = fopen(ptFileDesc->cFileName, "r+");
	if(NULL == tFp){
		DebugPrint(DEBUG_ERR"Can't open this file %s\n", ptFileDesc->cFileName);
		return -1;
	}

	iFd = fileno(tFp);	/* 获取文件句柄 */

	iError = fstat(iFd, &tFileStat);
	if(iError){
		DebugPrint(DEBUG_ERR"Can't get file stat %s\n", ptFileDesc->cFileName);
		fclose(tFp);
		return -1;
	}

	
	ptFileDesc->pucFileMem= (unsigned char *)mmap(NULL , tFileStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, iFd, 0);
	if (ptFileDesc->pucFileMem == (unsigned char *)-1)
	{
		DebugPrint(DEBUG_ERR"mmap file error!\n");
		return -1;
	}

	ptFileDesc->ptFp = tFp;
	ptFileDesc->iFileSize = tFileStat.st_size;
	
	return 0;
}

void ReleaseFileDesc(struct FileDesc *ptFileDesc)
{
	fclose(ptFileDesc->ptFp);
	munmap(ptFileDesc->pucFileMem, ptFileDesc->iFileSize);
}

static int isNormalFile(char *strFilePath, char *strFileName)
{
	char strTmp[256];
	struct stat tFileStat;

	snprintf(strTmp, 256, "%s/%s", strFilePath, strFileName);
	strTmp[255] = '\0';

	if((stat(strTmp, &tFileStat) == 0) && S_ISREG(tFileStat.st_mode)){
		return 1;
	}

	return 0;
}

static int isDir(char *strFilePath, char *strFileName)
{
	char strTmp[256];
	struct stat tFileStat;

	snprintf(strTmp, 256, "%s/%s", strFilePath, strFileName);
	strTmp[255] = '\0';

	if((stat(strTmp, &tFileStat) == 0) && S_ISDIR(tFileStat.st_mode)){
		return 1;
	}

	return 0;
}

#if 0
static int isNormalDir(char *strFilePath, char *strSubDirName)
{
	static const char *aSpecialDirs[] = {"sbin", "bin", "usr", "lib", "proc", "tmp", "dev", "sys", NULL};

	int i = 0;

	/* 必须是根目录下面的 */
	if(0 == strcmp(strFilePath, "/")){
		while(aSpecialDirs[i]){
			if(0 == strcmp(aSpecialDirs[i], strSubDirName)){
				return 0;
			}

			i ++;
		}
	}

	return 1;
}
#endif

int GetDirContents(char *strcDirName, struct DirContent **aptDirContents, int *piFileNum)
{
	struct DirContent *atDirContents;
	struct dirent **aptNameList;
	int iFileNum;
	int i;
	int iGetFileNum;

	iFileNum = scandir(strcDirName, &aptNameList, 0, alphasort);
	if(iFileNum == -1){
		DebugPrint(DEBUG_ERR"Scan directory %s error\n", strcDirName);
		return -1;
	}

	/* 不显示 "." ".." 文件夹 */
	atDirContents = malloc(sizeof(struct DirContent) * (iFileNum - 2));
	if(NULL == atDirContents){
		DebugPrint(DEBUG_ERR"malloc aptDirContents error\n");
		return -1;
	}

	*aptDirContents = atDirContents;

	iGetFileNum = 0;
	for(i = 0; i < iFileNum; i ++){
		if((0 == strcmp(aptNameList[i]->d_name, "."))
			|| (0 == strcmp(aptNameList[i]->d_name, "..")))
		{
			continue;
		}

		if(isDir(strcDirName, aptNameList[i]->d_name)){
			strncpy(atDirContents[iGetFileNum].strDirName, aptNameList[i]->d_name, 256);
			atDirContents[iGetFileNum].strDirName[255] = '\0';
			atDirContents[iGetFileNum].eFileType       = DIR_FILE;
			free(aptNameList[i]);
			aptNameList[i] = NULL;

			iGetFileNum ++;
		}
	}

	for(i = 0; i < iFileNum; i ++){
		if(aptNameList[i] == NULL){
			continue;
		}
		
		if(isNormalFile(strcDirName, aptNameList[i]->d_name)){
			strncpy(atDirContents[iGetFileNum].strDirName, aptNameList[i]->d_name, 256);
			atDirContents[iGetFileNum].strDirName[255] = '\0';
			atDirContents[iGetFileNum].eFileType       = NORMAL_FILE;
			free(aptNameList[i]);
			aptNameList[i] = NULL;

			iGetFileNum ++;
		}
	}

	for(i = 0; i < iFileNum; i ++){
		if(aptNameList[i] != NULL){
			free(aptNameList[i]);
		}
	}

	free(aptNameList);

	*piFileNum = iGetFileNum;

	return 0;
}

void FreeDirContents(struct DirContent **aptDirContents)
{
	free(*aptDirContents);
}

