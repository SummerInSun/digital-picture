#include <dis_manag.h>
#include <common_st.h>
#include <common_config.h>
#include <font_manag.h>
#include <encoding_manag.h>
#include <print_manag.h>
#include <input_manag.h>
#include <picfmt_manag.h>
#include <pic_operation.h>
#include <page_manag.h>
#include <music_manag.h>
#include <file.h>
#include <draw.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <string.h>
#include <termios.h>

int main(int argc, char *argv[])  
{
	int iError;
	
	struct PageIdetify tPageIdetify;
	struct PageOpr *ptMainPageOpr;

	iError = PrintDeviceInit();
	if(iError){
		DebugPrint("Print device init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported print device\n");
	ShowPrintOpr();

	if(argc != 2){
		DebugPrint(DEBUG_ERR"exe <fontfile>\n");
		return -1;
	}

	iError = EncodingInit();
	if(iError){
		DebugPrint("Encoding init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported encoding\n");
	ShowEncodingOpr();

	iError = FontInit();
	if(iError){
		DebugPrint("Font init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported font\n");
	ShowFontOpr();
	
	iError = SetFontsDetail("freetype", argv[1], 20);
	if(iError){
		DebugPrint("This font can't be support. check the file format or file size\n");
		goto error_exit;
	}

	iError = DisplayInit();
	if(iError){
		DebugPrint("Display init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported display device\n");
	ShowDisOpr();
	
	SelectAndInitDefaultDisOpr("fb");
	if(iError){
		DebugPrint("Init default display opration error\n");
		goto error_exit;
	}

	iError = InputDeviceInit();
	AllInputFdInit();
	if(iError){
		DebugPrint("Input device init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported input device\n");
	ShowInputOpr();
	
	iError = PicFmtInit();
	if(iError){
		DebugPrint("Picture format init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported picture format\n");
	ShowPicFmtParser();

	iError = AllocVideoMem(5);
	if(iError){
		DebugPrint("Alloc video memory error\n");
		goto error_exit;
	}

	iError = MusicParserInit();
	if(iError){
		DebugPrint("MusicParser init error\n");
		goto error_exit;
	}
	DebugPrint("\nSupported music\n");
	ShowMusicParser();
	
	iError = PagesInit();
	if(iError){
		DebugPrint("Page init error\n");
		goto error_exit;
	}
	DebugPrint("\nCreated pages\n");
	ShowPageOpr();
	
	ptMainPageOpr = GetPageOpr("main");
	if(NULL == ptMainPageOpr){
		DebugPrint("GetPageOpr error\n");
		goto error_exit;
	}
	
	ptMainPageOpr->RunPage(&tPageIdetify);

	FreeAllVideoMem();
	FreeDirAndFileIcons();
	PrintDeviceExit();

exit:
	return 0;
error_exit:
	return -1;

}


