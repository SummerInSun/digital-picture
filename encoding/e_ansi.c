#include <encoding_manag.h>
#include <font_manag.h>
#include <string.h>

static int AnsiisSupported(unsigned char *pucTextHead);
static int AnsiGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode);

struct EncodingOpr g_tAnsiEncodingOpr = {
	.name = "ansi-encoding",
	.iHeadLen = 0,
	.isSupported     = AnsiisSupported,
	.GetCodeFromText = AnsiGetCodeFromText,
};

static int AnsiisSupported(unsigned char *pucTextHead)
{
	const char aStrUtf8[]    = {0xEF, 0xBB, 0xBF, 0};
	const char aStrUtf16le[] = {0xFF, 0xFE, 0};
	const char aStrUtf16be[] = {0xFE, 0xFF, 0};
	
	if (strncmp((const char*)pucTextHead, aStrUtf8, 3) == 0)
	{
		/* UTF-8 */
		return 0;
	}
	else if (strncmp((const char*)pucTextHead, aStrUtf16le, 2) == 0)
	{
		/* UTF-16 little endian */
		return 0;
	}
	else if (strncmp((const char*)pucTextHead, aStrUtf16be, 2) == 0)
	{
		/* UTF-16 big endian */
		return 0;
	}
	else
	{
		return -1;
	}
}

static int AnsiGetCodeFromText(unsigned char *pucTextStart, unsigned char *pucTextEnd, unsigned int *pdwCode)
{
	unsigned char *pucBuf = pucTextStart;
	unsigned char c = *pucBuf;
	
	if ((pucBuf < pucTextEnd) && (c < (unsigned char)0x80))
	{
		/* 返回ASCII码 */
		*pdwCode = (unsigned int)c;
		return 1;
	}

	if (((pucBuf + 1) < pucTextEnd) && (c >= (unsigned char)0x80))
	{
		/* 返回GBK码 */
		*pdwCode = pucBuf[0] + (((unsigned int)pucBuf[1]) << 8);
		return 2;
	}

	if (pucBuf < pucTextEnd)
	{
		/* 可能文件有损坏, 但是还是返回一个码, 即使它是错误的 */
		*pdwCode = (unsigned int)c;
		return 1;
	}
	else
	{
		/* 文件处理完毕 */
		return 0;
	}
}


int AnsiEncodingInit(void)
{
//	AddFontOprForEncoding(&g_tAnsiEncodingOpr, GetFontOpr("freetype"));
	AddFontOprForEncoding(&g_tAnsiEncodingOpr, GetFontOpr("ascii"));
	AddFontOprForEncoding(&g_tAnsiEncodingOpr, GetFontOpr("gbk"));

	return RegisterEncodingOpr(&g_tAnsiEncodingOpr);
}
