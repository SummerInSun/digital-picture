#include <music_manag.h>
#include <print_manag.h>
#include <file.h>

#include <string.h>
#include <stdio.h>

DECLARE_HEAD(g_tMusicParserHead);

int RegisterMusicParser(struct MusicParser *ptMusicParser)
{
	ListAddTail(&ptMusicParser->tMusicParser, &g_tMusicParserHead);

	return 0;	
}

void UnregisterMusicParser(struct MusicParser *ptMusicParser)
{
	if(ptMusicParser != NULL){
		ListDelTail(&ptMusicParser->tMusicParser);
	}
}

struct MusicParser *GetMusicParser(char *pcName)
{	
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct MusicParser *ptMPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tMusicParserHead){
		ptMPTmpPos = LIST_ENTRY(ptLHTmpPos, struct MusicParser, tMusicParser);

		if(0 == strcmp(pcName, ptMPTmpPos->name)){
			return ptMPTmpPos;
		}
	}

	return NULL;
}

struct MusicParser *GetSupportedMusicParser(struct FileDesc *ptFileDesc)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct MusicParser *ptMPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tMusicParserHead){
		ptMPTmpPos = LIST_ENTRY(ptLHTmpPos, struct MusicParser, tMusicParser);

		if(ptMPTmpPos->isSupport(ptFileDesc)){
			return ptMPTmpPos;
		}
	}

	return NULL;
}

int isMusicSupported(char *pcName)
{
	struct FileDesc tMusicFileDesc;
	int iError = 0;

	snprintf(tMusicFileDesc.cFileName, 256, "%s", pcName);
	tMusicFileDesc.cFileName[255] = '\0';
		
	iError = GetFileDesc(&tMusicFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"Get file descript error int isMusicSupported\n");
		return 0;
	}

	if(NULL == GetSupportedMusicParser(&tMusicFileDesc)){
		ReleaseFileDesc(&tMusicFileDesc);
		return 0;
	}

	ReleaseFileDesc(&tMusicFileDesc);
	return 1;
}

/* 播放音乐，返回音频解码结构体 */
struct MusicParser *PlayMusic(struct FileDesc *ptFileDesc, char *strMusicPath)
{
	struct MusicParser *ptMusicParser;
	int iError = 0;

	snprintf(ptFileDesc->cFileName, 256, "%s", strMusicPath);
	ptFileDesc->cFileName[255] = '\0';
		
	iError = GetFileDesc(ptFileDesc);
	if(iError){
		DebugPrint(DEBUG_ERR"Get file descript error int isMusicSupported\n");
		return 0;
	}

	ptMusicParser = GetSupportedMusicParser(ptFileDesc);
	if(NULL == ptMusicParser){
		ReleaseFileDesc(ptFileDesc);
		return NULL;
	}

	ptMusicParser->MusicPlay(ptFileDesc);

	return ptMusicParser;
}

void StopMusic(struct FileDesc *ptFileDesc, struct MusicParser *ptMusicParser)
{
	/* bug : 
	 * ReleaseFileDesc(ptFileDesc);
	 * ptMusicParser->MusicCtrl(MUSIC_CTRL_CODE_EXIT);
	 * 
	 * 由于先释放了文件内存，在线程里面可能会再次访问，所以导致段错误，互换位置就好了
	 */

	ptMusicParser->MusicCtrl(MUSIC_CTRL_CODE_EXIT);
	ReleaseFileDesc(ptFileDesc);
}

int CtrlMusic(struct MusicParser *ptMusicParser, enum MusicCtrlCode eCtrlCode)
{
	return ptMusicParser->MusicCtrl(eCtrlCode);
}

void ShowMusicParser(void)
{	
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct MusicParser *ptMPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tMusicParserHead){
		ptMPTmpPos = LIST_ENTRY(ptLHTmpPos, struct MusicParser, tMusicParser);

		printf("%d <---> %s\n", iPTNum++, ptMPTmpPos->name);
	}
}

/* 初始化音乐解析模块
 * 只要有一个初始化成功，就返回正确
 */
int MusicParserInit(void)
{
	int iError = 0;
	int iParserNum = 0;

	iError = WAVParserInit();
	if(iError){
		DebugPrint("WAVParserInit error\n");
		WAVParserExit();
	}else{
		iParserNum ++;
	}
	
	iError = MP3ParserInit();
	if(iError){
		DebugPrint("WAVParserInit error\n");
		MP3ParserExit();
	}else{
		iParserNum ++;
	}

	if(iParserNum){
		return 0;
	}

	return -1;
}

