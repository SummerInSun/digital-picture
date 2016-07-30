#include <encoding_manag.h>
#include <string.h>
#include <stdio.h>

static int Utf16leisSupported(unsigned char *pucTextHead);
static int Utf16leGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);

struct EncodingOpr g_tUtf16leEncodingOpr = {
	.name = "uft16le-encoding",
	.iHeadLen = 2,
	.isSupported = Utf16leisSupported,
	.GetCodeFromText = Utf16leGetCodeFromText,
};

static int Utf16leisSupported(unsigned char *pucTextHead)
{
	
	const char aStrUtf16le[] = {0xFF, 0xFE, 0};
	if (strncmp((const char *)pucTextHead, aStrUtf16le, 2) == 0)
	{
		/* UTF-16 little endian */
		return 1;
	}

	return 0;
}

static int Utf16leGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode)
{
	if(pucTextStart + 1 > pucTextEnd){
		/* End of file */
		return 0;
	}
	DEBUG_PRINTF("Utf16leGetCodeFromText\n");
	*pdwCode = (((unsigned int)(pucTextStart[1])) << 8) + pucTextStart[0];
	DEBUG_PRINTF("Utf16leGetCodeFromText\n");
	return 2;
}

int  Utf16leEncodingInit(void)
{
	AddFontOprForEncoding(&g_tUtf16leEncodingOpr, GetFontOpr("freetype"));
	AddFontOprForEncoding(&g_tUtf16leEncodingOpr, GetFontOpr("ascii"));
	return RegisterEncodingOpr(&g_tUtf16leEncodingOpr);
}

