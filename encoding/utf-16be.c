#include <encoding_manag.h>
#include <string.h>

static int Utf16beisSupported(unsigned char *pucTextHead);
static int Utf16beGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);

struct EncodingOpr g_tUtf16beEncodingOpr = {
	.name = "uft16be-encoding",
	.iHeadLen = 2,
	.isSupported = Utf16beisSupported,
	.GetCodeFromText = Utf16beGetCodeFromText,
};

static int Utf16beisSupported(unsigned char *pucTextHead)
{
	
	const char aStrUtf16be[] = {0xFE, 0xFF, 0};
	if (strncmp((const char *)pucTextHead, aStrUtf16be, 2) == 0)
	{
		/* UTF-16 big endian */
		return 1;
	}

	return 0;
}

static int Utf16beGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode)
{
	if(pucTextStart + 1 > pucTextEnd){
		/* End of file */
		return 0;
	}

	*pdwCode = (((unsigned int)pucTextStart[0]) << 8) + pucTextStart[1];
	return 2;
}

int  Utf16beEncodingInit(void)
{
	AddFontOprForEncoding(&g_tUtf16beEncodingOpr, GetFontOpr("freetype"));
	AddFontOprForEncoding(&g_tUtf16beEncodingOpr, GetFontOpr("ascii"));
	return RegisterEncodingOpr(&g_tUtf16beEncodingOpr);
}


