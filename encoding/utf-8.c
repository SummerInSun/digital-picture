#include <encoding_manag.h>
#include <string.h>

static int Utf8isSupported(unsigned char *pucTextHead);
static int Utf8GetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);

struct EncodingOpr g_tUtf8EncodingOpr = {
	.name = "utf8-encoding",
	.iHeadLen = 3,
	.isSupported = Utf8isSupported,
	.GetCodeFromText = Utf8GetCodeFromText,
};

static int Utf8isSupported(unsigned char *pucTextHead)
{
	const char aStrUtf8[] = {0xEF, 0xBB, 0xBF, 0};
	
	if (strncmp((const char*)pucTextHead, aStrUtf8, 3) == 0)
	{
		/* is utf8 */
		return 1;
	}
	return 0;
}

static int GetUtf8CodeLen(unsigned char ucCode)
{
	int iNum = 0;
	int iFlag = 7;

	while((ucCode & (1 << iFlag)) && (iFlag > 0)){
		iFlag --;
		iNum ++;
	}

	return iNum;
}

/* 返回值是从原来文本文件中取到的字节个数 */
static int Utf8GetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode)
{
	int i = 0;
	int iUtf8CodeNum;
	unsigned char ucCodeTmp = 0;

	iUtf8CodeNum = GetUtf8CodeLen(pucTextStart[0]);

	if(pucTextStart + iUtf8CodeNum > pucTextEnd){
		return 0;
	}

	if(iUtf8CodeNum == 0){
		/* ascii code, but almostly impossiable */
		*pdwCode = pucTextStart[0];
		return 1;
	}

	ucCodeTmp = pucTextStart[0];

	ucCodeTmp = ucCodeTmp << iUtf8CodeNum;
	ucCodeTmp = ucCodeTmp >> iUtf8CodeNum;
	*pdwCode  = ucCodeTmp;
	for(i = 1; i < iUtf8CodeNum; i ++){
		ucCodeTmp = pucTextStart[i] & 0x3f;
		*pdwCode  = ((*pdwCode) << 6) + ucCodeTmp;
	}

	return iUtf8CodeNum;

}

int  Utf8EncodingInit(void)
{
	AddFontOprForEncoding(&g_tUtf8EncodingOpr, GetFontOpr("freetype"));
	AddFontOprForEncoding(&g_tUtf8EncodingOpr, GetFontOpr("ascii"));
	return RegisterEncodingOpr(&g_tUtf8EncodingOpr);
}


