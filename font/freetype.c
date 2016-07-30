#include <font_manag.h>
#include <common_config.h>
#include <draw.h>
#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H

static int FreeTypeFontInit(char *pucFontFile, unsigned int dwFontSize);
static int FreeTypeGetFontBitMap(unsigned int dwCode, struct FontBitMap *ptFontBitMap);
static int FreeTypeSetFontSize(unsigned int dwFontSize);

static FT_Library   g_tFTLibrary;
static FT_Face      g_tFTFace;
static FT_GlyphSlot g_tFTSlot;
static FT_Vector  g_tFTPen;
static FT_Matrix  g_tFTMatrix;
static double     g_dbFTAngle;

static struct FontDisOpt *g_tFreetypeFontDisOpt;

struct FontOpr g_tFreetypeFontOpr = {
	.name = "freetype",
	.FontInit = FreeTypeFontInit,
	.GetFontBitMap = FreeTypeGetFontBitMap,
	.SetFontSize = FreeTypeSetFontSize,
};

static int FreeTypeFontInit(char *pucFontFile, unsigned int dwFontSize)
{
	int iError = 0;
	
	iError = FT_Init_FreeType(&g_tFTLibrary);              /* initialize library */
	if(iError){
		DebugPrint(DEBUG_ERR"FT_Init_FreeType error\n");
		return -1;
	}
	
	iError = FT_New_Face( g_tFTLibrary, pucFontFile, 0, &g_tFTFace );/* create face object */
	if(iError){
		DebugPrint(DEBUG_ERR"FT_New_Face error\n");
		return -1;
	}
	
	iError = FT_Set_Pixel_Sizes(g_tFTFace, dwFontSize, 0);
	if(iError){
		DebugPrint(DEBUG_ERR"FT_Set_Pixel_Sizes error\n");
		return -1;
	}
	
	g_tFTSlot = g_tFTFace->glyph;

	return 0;
}

static int FreeTypeGetFontBitMap(unsigned int dwCode, struct FontBitMap *ptFontBitMap)
{
	int iError = 0;
	int iPenX = ptFontBitMap->iCurOriginX;
	int iPenY = ptFontBitMap->iCurOriginY;

	g_tFreetypeFontDisOpt = GetFontDisOpt();
	
#if 1
	g_dbFTAngle = (0 / 360.0 ) * 3.14159 * 2;      /* use 25 degrees     */
	/* the pen position in 26.6 cartesian space coordinates; */
	/* start at (300,200) relative to the upper left corner  */
	g_tFTPen.x = iPenX;
	g_tFTPen.y = (272 - iPenY);
	DebugPrint(DEBUG_DEBUG"%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
	
	/* set up matrix */
	g_tFTMatrix.xx = (FT_Fixed)( cos( g_dbFTAngle ) * 0x10000L );
	g_tFTMatrix.xy = (FT_Fixed)( -sin( g_dbFTAngle ) * 0x10000L );
	if(g_tFreetypeFontDisOpt->ucFontMode & LEAN_FONT){
		g_tFTMatrix.xy = (FT_Fixed)( 0.2 * 0x10000L );
	}
	g_tFTMatrix.yx = (FT_Fixed)( sin( g_dbFTAngle ) * 0x10000L );
	g_tFTMatrix.yy = (FT_Fixed)( cos( g_dbFTAngle ) * 0x10000L );

	/* set transformation */
    FT_Set_Transform(g_tFTFace, &g_tFTMatrix, &g_tFTPen);
#endif
    /* load glyph image into the slot (erase previous one) */
    iError = FT_Load_Char(g_tFTFace, dwCode, FT_LOAD_RENDER | FT_LOAD_MONOCHROME);
	if(iError){
		DebugPrint(DEBUG_ERR"FT_Load_Char error\n");
		return -1;
	}

//	DEBUG_PRINTF("iPenX = %d, iPenY = %d, bitmap_left = %d, bitmap_top = %d, width = %d, rows = %d\n", iPenX, iPenY, g_tFTSlot->bitmap_left, g_tFTSlot->bitmap_top, g_tFTSlot->bitmap.width, g_tFTSlot->bitmap.rows);
#if 1
	ptFontBitMap->iXLeft = iPenX + g_tFTSlot->bitmap_left;
	ptFontBitMap->iYTop  = iPenY - g_tFTSlot->bitmap_top;
	ptFontBitMap->iXMax  = ptFontBitMap->iXLeft + g_tFTSlot->bitmap.width;
	ptFontBitMap->iYMax  = ptFontBitMap->iYTop  + g_tFTSlot->bitmap.rows;
	ptFontBitMap->iBpp	 = 1;
	ptFontBitMap->iPatch = g_tFTSlot->bitmap.pitch;
	ptFontBitMap->iNextOriginX = iPenX + g_tFTSlot->advance.x / 64;
	ptFontBitMap->iNextOriginY = iPenY;
	ptFontBitMap->pucBuffer    = g_tFTSlot->bitmap.buffer;
#else
	ptFontBitMap->iXLeft = g_tFTSlot->bitmap_left;
	ptFontBitMap->iYTop  = g_tFTSlot->bitmap_top;
	ptFontBitMap->iXMax  = ptFontBitMap->iXLeft + g_tFTSlot->bitmap.width;
	ptFontBitMap->iYMax  = ptFontBitMap->iYTop  + g_tFTSlot->bitmap.rows;
	ptFontBitMap->iBpp	 = 1;
	ptFontBitMap->iPatch = g_tFTSlot->bitmap.pitch;
	ptFontBitMap->iNextOriginX = ptFontBitMap->iXMax;
	ptFontBitMap->iNextOriginY = iPenY;
	ptFontBitMap->pucBuffer    = g_tFTSlot->bitmap.buffer;

	DEBUG_PRINTF("iYTop = %d\n ", ptFontBitMap->iYTop);
	DEBUG_PRINTF("iXLeft = %d\n ", ptFontBitMap->iXLeft);
	DEBUG_PRINTF("iXMax = %d\n ", ptFontBitMap->iXMax);
	DEBUG_PRINTF("iYMax = %d\n ", ptFontBitMap->iYMax);
	DEBUG_PRINTF("iPatch = %d\n ", ptFontBitMap->iPatch);
	DEBUG_PRINTF("iPenX = %d\n ", iPenX);
	DEBUG_PRINTF("iPenY = %d\n ", iPenY);
#endif
	
	return 0;
}

static int FreeTypeSetFontSize(unsigned int dwFontSize)
{
	int iError = 0;

	iError = FT_Set_Pixel_Sizes(g_tFTFace, dwFontSize, 0);
	if(iError){
		DebugPrint(DEBUG_ERR"FT_Set_Pixel_Sizes error\n");
		return -1;
	}

	return 0;
}

int FreeTypeInit(void)
{
	return RegisterFontOpr(&g_tFreetypeFontOpr);
}


