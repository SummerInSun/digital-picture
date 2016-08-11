#include <music_manag.h>
#include <alsa/asoundlib.h>
#include <print_manag.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static int WAVMusicDecoderInit(void);
static void WAVMusicDecodeExit(void);
static int isSupportWAV(struct FileDesc *ptFileDesc);
static int WAVMusicPlay(struct FileDesc *ptFileDesc);
static int WAVMusicCtrl(enum MusicCtrlCode eCtrlCode);

#pragma pack(push)
#pragma pack(1)

struct WAVFmtHeader
{
	char  strRIFF[4];    /* 'RIFF' 资源文件标志，固定字符 */
	unsigned long  dwTotalByte;   /* 从下一个成员开始到文件结尾的总字节数 */
	char  strWAVE[4];    /* 'WAVE' 表示是 WAVE 文件，固定字符 */
	char  strFmt[4];     /* 'fmt ' 波形格式标志，固定字符 */
	unsigned long  dwPcmFmt;      /* 数据格式 16LE,32LE等等 */
	unsigned short bIsCompress;   /* 是否是压缩的 PCM 编码，大于 1 表示有压缩，等于 1 表示无压缩 */
	unsigned short wChannels;     /* 声道数量 */
	unsigned long  dwPcmSampRate; /* 采样频率 */
	unsigned long  dwByteRate;    /* byte 率， byterate = 采样频率 * 音频通道数量 * 数据位数 / 8 */
	unsigned short wFrameSize;    /* 块对齐/帧大小 framesize = 通道数 * 数据位数 / 8 */
	unsigned short wSampBitWidth;  /* 样本数据位数 */
	char  strData[4];    /* 'data' 数据标志，固定字符 */
	unsigned long  dwPcmDataSize; /* 实际音频数据的大小 */
};

#pragma pack(pop)

/* 线程里面会用到的参数 */
struct MusicThreadParams
{
	unsigned char *pucFilePosStart;
	unsigned char *pucFileCurPos;
	int iFilesize;
	int iWAVFrameSize;
	unsigned char *pucPeriodBuf;
};

#define VOL_CHANGE_NUM 2
#define DEFAULT_VOL_VAL "10"
static char g_cVolValue = 10;    /* 默认的声音值 */

#define PCM_PERIOD_SIZE 128
static struct MusicThreadParams g_tWAVThreadParams;
static int g_iRuntimeRatio = 0;

static snd_pcm_t *g_ptWAVPlaybackHandle;
static pthread_t g_WAVPlayThreadId;
static pthread_once_t g_PthreadOnce = PTHREAD_ONCE_INIT;
static char g_bMusicPalyFinish = 1;

struct MusicParser g_tWAVMusicParser = {
	.name = "wav",
	.MusicDecoderInit = WAVMusicDecoderInit,
	.MusicDecodeExit  = WAVMusicDecodeExit,
	.isSupport = isSupportWAV,
	.MusicPlay = WAVMusicPlay,
	.MusicCtrl = WAVMusicCtrl,
};

static snd_pcm_t *OpenPcmDev(char *strDevName, snd_pcm_stream_t eSndPcmStream)
{
	snd_pcm_t *SndPcmHandle;
	int iError = 0;

	iError = snd_pcm_open (&SndPcmHandle, strDevName, eSndPcmStream, 0);
	if (iError < 0) {
		return NULL;
	}

	return SndPcmHandle;
}

static int SetHarwareParams(snd_pcm_t *ptPcmHandle ,struct PcmHardwareParams *ptPcmHWParams)
{
	int iError = 0;
	snd_pcm_hw_params_t *HardWareParams;
	snd_pcm_format_t ePcmFmt = SND_PCM_FORMAT_S16_LE; /* 默认值 */
	unsigned int dwSampRate = 44100; /* 默认值 */

	/* 格式识别 */
	switch(ptPcmHWParams->eFmtMod){
		case 8: {
			ePcmFmt = SND_PCM_FORMAT_S8;			
			break;
		}
		case 16: {
			ePcmFmt = SND_PCM_FORMAT_S16_LE;
			break;
		}
		case 24: {
			ePcmFmt = SND_PCM_FORMAT_S24_LE;
			break;
		}
		case 32: {
			ePcmFmt = SND_PCM_FORMAT_S32_LE;
			break;
		}
		default : {
			DebugPrint("Unsupported format bits : %d\n", ptPcmHWParams->eFmtMod);
			return -1;
			break;
		}
	}

	dwSampRate = ptPcmHWParams->dwSampRate;    /* 采样率 */

	/* 里面实际上调用 calloc 函数进行空间分配
	 * calloc 与 malloc 函数的区别在于，calloc 函数把申请到的空间全部清零
	 * calloc(n, size); n 要申请的变量个数， size 变量的大小
	 */
	iError = snd_pcm_hw_params_malloc(&HardWareParams);
	if (iError < 0){
		DebugPrint("Can't alloc snd_pcm_hw_params_t \n");
		return -1;
	}
	
	/* 把硬件参数设置为默认值 */
	iError = snd_pcm_hw_params_any (ptPcmHandle, HardWareParams);
	if (iError < 0) {
		DebugPrint("Initialize HardWareParams error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;
	}

	/* 设置模式
	 * 设置函数都是通过 snd_pcm_hw_param_set 来完成，不同设置使用不同的参数来指定
	 */
	iError = snd_pcm_hw_params_set_access(ptPcmHandle, HardWareParams, ptPcmHWParams->eAccessMod);
	if (iError < 0) {
		DebugPrint("Set access mod error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;		
	}

	/* 设置格式, 默认是 signed 16 位小端 */
	iError = snd_pcm_hw_params_set_format(ptPcmHandle, HardWareParams, ePcmFmt);
	if (iError < 0) {
		DebugPrint("Set format mod error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;		
	}

	/* 设置采样率，默认是 44100 */
	iError = snd_pcm_hw_params_set_rate_near(ptPcmHandle, HardWareParams, &dwSampRate, NULL);
	if (iError < 0) {
		DebugPrint("Set rate mod error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;		
	}

	/* 设置通道数，这里是两个，双声道 */
	iError = snd_pcm_hw_params_set_channels(ptPcmHandle, HardWareParams, ptPcmHWParams->dwChannels);
	if (iError < 0) {
		DebugPrint("Set channel mod error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;		
	}

	/* 写入硬件设置 */
	iError = snd_pcm_hw_params(ptPcmHandle, HardWareParams);
	if (iError < 0) {
		DebugPrint("Write hardware parameters error\n");
		free(HardWareParams);
		HardWareParams = NULL;
		return -1;		
	}

	snd_pcm_hw_params_free(HardWareParams);

	/* 此函数会被自动调用，在 snd_pcm_hw_params 之后 */
	iError = snd_pcm_prepare(ptPcmHandle);
	if (iError < 0) {
		DebugPrint("cannot prepare audio interface for use\n");		
		return -1;		
	}

	return 0;
}

/* 线程清理函数 */
void WAVThread1Clean(void *data)
{
	struct MusicThreadParams *ptWAVThreadParams = (struct MusicThreadParams *)data;

	free(ptWAVThreadParams->pucPeriodBuf);
	snd_pcm_close(g_ptWAVPlaybackHandle);    /* 播放完要关闭设备 */
	g_bMusicPalyFinish = 1;
}

void *WAVPlayThread(void *data)
{
	struct MusicThreadParams *ptWAVThreadParams = (struct MusicThreadParams *)data;
	unsigned char *pucWAVFileCurPos;
	unsigned char *pucWAVFilePos;
	unsigned char *pucPeriodBuf;
	int iFileSize;
	int iHasPlaySize = 0;
	int iFrameSize;
	int iChunkNum = 0;

	pucWAVFileCurPos = ptWAVThreadParams->pucFileCurPos;
	pucWAVFilePos    = ptWAVThreadParams->pucFilePosStart;
	pucPeriodBuf     = ptWAVThreadParams->pucPeriodBuf;
	iFileSize        = ptWAVThreadParams->iFilesize;
	iFrameSize       = ptWAVThreadParams->iWAVFrameSize;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	/* 设置清理线程 */
	pthread_cleanup_push(WAVThread1Clean, ptWAVThreadParams);

	g_bMusicPalyFinish = 0;

	while(1){
		pthread_testcancel();    /* 线程取消点 */

		if(pucWAVFileCurPos >= pucWAVFilePos + iFileSize){
			pthread_exit(NULL);
		}
		for(iChunkNum = 0; iChunkNum < PCM_PERIOD_SIZE; iChunkNum ++){
			if(pucWAVFileCurPos < pucWAVFilePos + iFileSize){
				memcpy(pucPeriodBuf, pucWAVFileCurPos, iFrameSize);				
				pucWAVFileCurPos += iFrameSize;
				pucPeriodBuf     += iFrameSize;

				/* 运行时间比例, 1 - 100 */
				iHasPlaySize     += iFrameSize;
				g_iRuntimeRatio = (float)iHasPlaySize / iFileSize * 1000;
			}else{
				break;
			}
		}

		pucPeriodBuf = pucPeriodBuf - iChunkNum * iFrameSize;

		/* 默认一个 period 长度是 128, 单位一帧 */
		snd_pcm_writei(g_ptWAVPlaybackHandle, pucPeriodBuf, iChunkNum);
	}

	pthread_cleanup_pop(0);
	pthread_exit(NULL);
}

static int WAVMusicDecoderInit(void)
{
	return 0;
}

static void WAVMusicDecodeExit(void)
{	
	
}

static int isSupportWAV(struct FileDesc *ptFileDesc)
{
	struct WAVFmtHeader *ptWAVFmtHeader = (struct WAVFmtHeader *)ptFileDesc->pucFileMem;
	int iError = 0;

	iError = strncmp("RIFF", ptWAVFmtHeader->strRIFF, 4);
	iError |= strncmp("WAVE", ptWAVFmtHeader->strWAVE, 4);

	return !iError;
}

static int SetVol(char *vol, int roflag)
{
	int err;
	snd_ctl_t *handle = NULL;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_info_alloca(&info);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_value_alloca(&control);

	if (snd_ctl_ascii_elem_id_parse(id, "numid=45")) {
		return -1;
	}
	
	if ((err = snd_ctl_open(&handle, "default", SND_CTL_NONBLOCK)) < 0) {
		DebugPrint("Control open error\n");
		return err;
	}
	snd_ctl_elem_info_set_id(info, id);
	if ((err = snd_ctl_elem_info(handle, info)) < 0) {
		DebugPrint("Cannot find the given element from control \n");
		snd_ctl_close(handle);
		handle = NULL;
		
		return err;
	}

	snd_ctl_elem_value_set_id(control, id);

	if(roflag)
		return (int)snd_ctl_elem_value_get_integer(control, 1);
	
	if ((err = snd_ctl_elem_read(handle, control)) < 0) {
		DebugPrint("Cannot read the given element from control\n");
		snd_ctl_close(handle);
		handle = NULL;

		return err;
	}
	err = snd_ctl_ascii_value_parse(handle, control, info, vol);
	if (err < 0) {
		DebugPrint("Control parse error: %s\n");
		snd_ctl_close(handle);
		handle = NULL;
		
		return err;
	}
	if ((err = snd_ctl_elem_write(handle, control)) < 0) {
		DebugPrint("Control element write error: %s\n");
		snd_ctl_close(handle);
		handle = NULL;
		return err;
	}

	snd_ctl_close(handle);
	handle = NULL;
	
	return 0;
}

static void SetVolThreadOnce(void)
{
	SetVol(DEFAULT_VOL_VAL, 0);
	g_cVolValue = atoi(DEFAULT_VOL_VAL);		
}

static int WAVMusicPlay(struct FileDesc *ptFileDesc)
{
	struct WAVFmtHeader *ptWAVFmtHeader = (struct WAVFmtHeader *)ptFileDesc->pucFileMem;
	int iError = 0;
	struct PcmHardwareParams tPcmHWParams;
	unsigned char *pucPeriodBuf;

	/* 初始化打开一个音频设备
	 */
	g_ptWAVPlaybackHandle = OpenPcmDev("default", SND_PCM_STREAM_PLAYBACK);
	if(NULL == g_ptWAVPlaybackHandle){
		DebugPrint("Can't open pcm device %s\n");
		return -1;
	}

	tPcmHWParams.eAccessMod = SND_PCM_ACCESS_RW_INTERLEAVED;
	tPcmHWParams.eFmtMod    = ptWAVFmtHeader->dwPcmFmt;
	tPcmHWParams.dwChannels = ptWAVFmtHeader->wChannels;
	tPcmHWParams.dwSampRate = ptWAVFmtHeader->dwPcmSampRate;
	
	iError = SetHarwareParams(g_ptWAVPlaybackHandle, &tPcmHWParams);
	if(iError){
		DebugPrint("Can't set dev params \n");
		return -1;
	}

	pucPeriodBuf = malloc(ptWAVFmtHeader->wFrameSize * PCM_PERIOD_SIZE);
	if(NULL == pucPeriodBuf){
		DebugPrint("Malloc pucPeriodBuf error \n");
		return -1;
	}

	/* 私有结构体，传递一些线程需要用到的数据，一定要是全局变量，否则该函数退出之后
	 * 某些变量就被释放了,再次访问会造成段错误
	 */
	g_tWAVThreadParams.pucFilePosStart = ptFileDesc->pucFileMem;
	g_tWAVThreadParams.pucFileCurPos = g_tWAVThreadParams.pucFilePosStart
								+ sizeof(struct WAVFmtHeader);
	g_tWAVThreadParams.iWAVFrameSize = ptWAVFmtHeader->wFrameSize;
	g_tWAVThreadParams.iFilesize     = ptFileDesc->iFileSize;
	g_tWAVThreadParams.pucPeriodBuf  = pucPeriodBuf;

	pthread_once(&g_PthreadOnce, SetVolThreadOnce);
	pthread_create(&g_WAVPlayThreadId, NULL, WAVPlayThread, &g_tWAVThreadParams);

	return 0;
}

static int WAVMusicCtrl(enum MusicCtrlCode eCtrlCode)
{
	char acVolStr[3];

	switch(eCtrlCode){
		case MUSIC_CTRL_CODE_HALT : {
			snd_pcm_pause(g_ptWAVPlaybackHandle, 1);

			break;
		}
		case MUSIC_CTRL_CODE_PLAY : {
			snd_pcm_pause(g_ptWAVPlaybackHandle, 0);

			break;
		}
		case MUSIC_CTRL_CODE_EXIT : {
			/* 有可能线程取消的时候音乐正在暂停
			 * 不管有没有暂停歌曲，都先运行歌曲，然后再取消线程
			 * 防止由于阻塞导致程序不响应
			 */
			if(!g_bMusicPalyFinish){
				snd_pcm_pause(g_ptWAVPlaybackHandle, 0);  
				pthread_cancel(g_WAVPlayThreadId); /* 线程取消 */
				pthread_join(g_WAVPlayThreadId, NULL);
				snd_pcm_drop(g_ptWAVPlaybackHandle);
			}
			break;
		}
		case MUSIC_CTRL_CODE_ADD_VOL : {
			g_cVolValue = g_cVolValue > 63 - VOL_CHANGE_NUM ? 63 : g_cVolValue + VOL_CHANGE_NUM;
			sprintf(acVolStr, "%d", g_cVolValue);
			SetVol(acVolStr, 0);
			break;
		}
		case MUSIC_CTRL_CODE_DEC_VOL : {
			g_cVolValue = g_cVolValue < VOL_CHANGE_NUM ? 0 : g_cVolValue - VOL_CHANGE_NUM;
			sprintf(acVolStr, "%d", g_cVolValue);
			SetVol(acVolStr, 0);
			break;
		}
		case MUSIC_CTRL_CODE_GET_VOL : {
			return g_cVolValue;
			break;
		}
		case MUSIC_CTRL_CODE_GET_RUNTIME : {
			return g_iRuntimeRatio;
			break;
		}
		
		default : break;
	}

	return 0;
}

int WAVParserInit(void)
{
	int iError = 0;

	iError = RegisterMusicParser(&g_tWAVMusicParser);
	iError |= WAVMusicDecoderInit();

	return iError;
}

void WAVParserExit(void)
{
	UnregisterMusicParser(&g_tWAVMusicParser);
	WAVMusicDecodeExit();
}


