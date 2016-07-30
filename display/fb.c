#include <dis_manag.h>
#include <common_st.h>
#include <print_manag.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>


static int FBDisDeviceInit(void);
static int FBShowPiexl(int iPenX, int iPeny, unsigned int dwColor);
static int FBCleanScreen(unsigned int dwBackColor);
static void FBDrawPage(struct VideoMem *ptPageVideoMem);

static struct fb_var_screeninfo g_tFBVar;	
static struct fb_fix_screeninfo g_tFBFix;
static unsigned int g_dwFBScreenSize = 0;
static unsigned char *g_pucFBMem = NULL;

static int g_FBfd;
static unsigned int g_dwScreenWidth;
static unsigned int g_dwScreenHight;
static unsigned int g_dwPixelWidth;

static struct DisOpr g_tFBDisOpr = {
	.name = "fb",
	.DisDeviceInit = FBDisDeviceInit,
	.ShowPiexl     = FBShowPiexl,
	.CleanScreen   = FBCleanScreen,
	.DrawPage      = FBDrawPage,
};

static int FBDisDeviceInit(void)
{
	int iError;
	
	g_FBfd = open("/dev/fb0", O_RDWR);
	if (g_FBfd < 0){
		printf("Can't open /dev/fb0\n");
		return -1;
	}

	iError = ioctl(g_FBfd, FBIOGET_VSCREENINFO, &g_tFBVar);
	if (iError){
		printf("Can't get the framebuffer screen info\n");
		return -1;
	}
	
	iError = ioctl(g_FBfd, FBIOGET_FSCREENINFO, &g_tFBFix);
	if (iError){
		printf("Can't get the fix info\n");
		return -1;
	}
	
	g_dwFBScreenSize = g_tFBVar.xres * g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;
	
	g_pucFBMem = (unsigned char *)mmap(NULL, g_dwFBScreenSize, 
									PROT_READ | PROT_WRITE, MAP_SHARED, g_FBfd, 0);
	if(g_pucFBMem == (unsigned char *)-1){
		printf("mmap g_pucFBMem failed\n");
		return -1;
	}

	g_tFBDisOpr.iXres = g_tFBVar.xres;
	g_tFBDisOpr.iYres = g_tFBVar.yres;
	g_tFBDisOpr.iBpp  = g_tFBVar.bits_per_pixel;
	g_tFBDisOpr.pucDisMem = g_pucFBMem;

	g_dwScreenWidth = g_tFBVar.xres * g_tFBVar.bits_per_pixel / 8;
	g_dwScreenHight = g_tFBVar.yres * g_tFBVar.bits_per_pixel / 8;
	g_dwPixelWidth  = g_tFBVar.bits_per_pixel / 8;

	return 0;
}
static int FBShowPiexl(int iPenX, int iPeny, unsigned int dwColor)
{
	int iRed, iGreen, iBlue;
	unsigned char *pucPen8bpp;
	unsigned short *pwPen16bpp;
	unsigned int *pdwPen32bpp;

	if(iPenX > g_dwScreenWidth || iPeny > g_dwScreenHight){
		DebugPrint(DEBUG_NOTICE"Out of screen\n");
		return -1;
	}

	pucPen8bpp  = (unsigned char *)
		g_pucFBMem + (iPeny * g_tFBVar.xres + iPenX) * g_tFBVar.bits_per_pixel / 8;
	pwPen16bpp  = (unsigned short *)
		g_pucFBMem + (iPeny * g_tFBVar.xres + iPenX) * g_tFBVar.bits_per_pixel / 16;
	pdwPen32bpp = (unsigned int *)
		g_pucFBMem + (iPeny * g_tFBVar.xres + iPenX) * g_tFBVar.bits_per_pixel / 32;

	switch(g_tFBVar.bits_per_pixel){
		case 8: {
			*pucPen8bpp = dwColor;
			break;
		}
		case 16: {
			iRed   = (dwColor >> 16) & 0xff;
			iGreen = (dwColor >> 8) & 0xff;
			iBlue  = (dwColor >> 0) & 0xff;

			dwColor = ((iRed >> 3) << 11) | ((iGreen >> 2) << 5) | ((iBlue >> 3));
			*pwPen16bpp = dwColor;
			break;
		}
		case 32: {
			*pdwPen32bpp = dwColor;
			break;
		}
		default:{
			DebugPrint(DEBUG_NOTICE"Can't support bpp %d\n", g_tFBVar.bits_per_pixel);
			break;
		}
	}
	
	return 0;
}
static int FBCleanScreen(unsigned int dwBackColor)
{
	int iRed, iGreen, iBlue;
	unsigned char *pucPen8bpp;
	unsigned short *pwPen16bpp;
	unsigned int *pdwPen32bpp;

	unsigned int i = 0;

	pucPen8bpp  = (unsigned char *) g_pucFBMem;
	pwPen16bpp  = (unsigned short *) g_pucFBMem;
	pdwPen32bpp = (unsigned int *) g_pucFBMem;

	switch(g_tFBVar.bits_per_pixel){
		case 8: {
			memset(pucPen8bpp, dwBackColor, g_dwFBScreenSize);
			break;
		}
		case 16: {
			iRed   = (dwBackColor >> 16) & 0xff;
			iGreen = (dwBackColor >> 8) & 0xff;
			iBlue  = (dwBackColor >> 0) & 0xff;

			dwBackColor = ((iRed >> 3) << 11) | ((iGreen >> 2) << 5) | ((iBlue >> 3));

			for(i = 0; i < g_dwFBScreenSize; i += 2){
				pwPen16bpp[i/2] = dwBackColor;
			}	
			break;
		}
		case 32: {
			for(i = 0; i < g_dwFBScreenSize; i += 4){
				pdwPen32bpp[i/4] = dwBackColor;
			}
			break;
		}
		default:{
			DebugPrint(DEBUG_NOTICE"Can't support bpp %d\n", g_tFBVar.bits_per_pixel);
			break;
		}
	}
	
	return 0;
}

static void FBDrawPage(struct VideoMem *ptPageVideoMem)
{
	memcpy(g_pucFBMem, ptPageVideoMem->tPiexlDatas.pucPiexlDatasMem,
		ptPageVideoMem->tPiexlDatas.iTotalLength);	
}

int FBInit(void)
{
	return RegisterDispOpr(&g_tFBDisOpr);
}

