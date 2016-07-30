#include <picfmt_manag.h>
#include <print_manag.h>
#include <string.h>

DECLARE_HEAD(g_tPicFmtParserHead);

int RegisterPicFmtParser(struct PicFmtParser *ptPicFmtParser)
{
	ListAddTail(&ptPicFmtParser->ptPicFmtParser, &g_tPicFmtParserHead);

	return 0;
}

struct PicFmtParser *GetPicFmtParser(char *pcName)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PicFmtParser *ptPFPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPicFmtParserHead){
		ptPFPTmpPos = LIST_ENTRY(ptLHTmpPos, struct PicFmtParser, ptPicFmtParser);

		if(0 == strcmp(pcName, ptPFPTmpPos->name)){
			return ptPFPTmpPos;
		}
	}

	return NULL;
}

struct PicFmtParser *GetSupportedParser(struct FileDesc *ptFileDesc)
{
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PicFmtParser *ptPFPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPicFmtParserHead){
		ptPFPTmpPos = LIST_ENTRY(ptLHTmpPos, struct PicFmtParser, ptPicFmtParser);

		if(ptPFPTmpPos->isSupport(ptFileDesc)){
			return ptPFPTmpPos;
		}
	}

	return NULL;
}

void ShowPicFmtParser(void)
{
	int iPTNum = 0;
	struct list_head *ptLHTmpPos;	//LH = lis_head
	struct PicFmtParser *ptPFPTmpPos;
	
	LIST_FOR_EACH_ENTRY_H(ptLHTmpPos, &g_tPicFmtParserHead){
		ptPFPTmpPos = LIST_ENTRY(ptLHTmpPos, struct PicFmtParser, ptPicFmtParser);

		printf("%d <---> %s\n", iPTNum++, ptPFPTmpPos->name);
	}
}

int PicFmtInit(void)
{
	int iError = 0;

	iError = BMPInit();
	if(iError){
		DebugPrint(DEBUG_ERR"BMPInit error\n");
		return -1;
	}


	iError = JPGInit();
	if(iError){
		DebugPrint(DEBUG_ERR"JPGInit error\n");
		return -1;
	}

	return 0;
}



