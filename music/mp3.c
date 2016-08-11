#include <music_manag.h>
#include <mad.h>
#include <alsa/asoundlib.h>
#include <print_manag.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct DataFrameHeader
{
	unsigned int bzFrameSyncFlag1:8;   /* 全为 1 */
	unsigned int bzProtectBit:1;       /* CRC */
	unsigned int bzVersionInfo:4;      /* 包括 mpeg 版本，layer 版本 */
	unsigned int bzFrameSyncFlag2:3;   /* 全为 1 */
	unsigned int bzPrivateBit:1;       /* 私有 */
	unsigned int bzPaddingBit:1;      /* 是否填充，1 填充，0 不填充
	layer1 是 4 字节，其余的都是 1 字节 */
	unsigned int bzSampleIndex:2;     /* 采样率索引 */
	unsigned int bzBitRateIndex:4;    /* bit 率索引 */
	unsigned int bzExternBits:6;      /* 版权等，不关心 */	
	unsigned int bzCahnnelMod:2;      /* 通道
	* 00 - Stereo 01 - Joint Stereo
	* 10 - Dual   11 - Single
	*/
};

/* IDxVx 的头部结构 */
struct IDxVxHeader
{
    unsigned char aucIDx[3];     /* 保存的值比如为"ID3"表示是ID3V2 */
    unsigned char ucVersion;     /* 如果是ID3V2.3则保存3,如果是ID3V2.4则保存4 */
    unsigned char ucRevision;    /* 副版本号 */
    unsigned char ucFlag;        /* 存放标志的字节 */
    unsigned char aucIDxSize[4]; /* 整个 IDxVx 的大小，除去本结构体的 10 个字节 */
    /* 只有后面 7 位有用 */
};

struct PcmFmtParams
{
    unsigned char *pucDataStart;
    unsigned long dwDataLength;
	/* 要统计歌曲播放长度，但是 dwDataLength
	 * 被解码函数的 input 置 0，只能在下面备份一下
	 */
	unsigned long dwDataLengthBak;
	unsigned char *pucDataEnd;    
};

static int MP3MusicDecoderInit(void);
static void MP3MusicDecodeExit(void);
static int isSupportMP3(struct FileDesc *ptFileDesc);
static int MP3MusicPlay(struct FileDesc *ptFileDesc);
static int MP3MusicCtrl(enum MusicCtrlCode eCtrlCode);

static int SetMp3OutPcmFmt(void);
static int SkipIDxVxContents(struct IDxVxHeader *ptIDxVxHeader);
static int GetPlayTimeForFile(unsigned char *ptDataFrameIndex, int iFileSize);

static int g_aiMP3BitRateIndex[] = {
	0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
};
static int g_aiMP3SampleRateIndex[] = {
	44100, 48000, 32000, 0,
};

#define VOL_CHANGE_NUM 2
#define DEFAULT_VOL_VAL "10"
static char g_cVolValue = 10;    /* 默认的声音值 */
static int g_iRuntimeRatio = 0;

static unsigned char g_bMp3DecoderExit = 0;
static snd_pcm_t *g_ptMP3PlaybackHandle;

static pthread_t g_MP3PlayThreadId;
static pthread_once_t g_PthreadOnce = PTHREAD_ONCE_INIT;
static struct PcmFmtParams g_tMP3PcmFmtParams;

static pthread_mutex_t g_tMutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_tConVar = PTHREAD_COND_INITIALIZER;
static pthread_t g_MP3TimeThreadId;
static int g_iMP3PlayTime;
static char g_bMusicHalt = 0;
static char g_bMP3TimeThreadExit = 1;
static char g_bMP3PlayThreadExit = 1;

struct MusicParser g_tMP3MusicParser = {
	.name = "mp3",
	.MusicDecoderInit = MP3MusicDecoderInit,
	.MusicDecodeExit  = MP3MusicDecodeExit,
	.isSupport = isSupportMP3,
	.MusicPlay = MP3MusicPlay,
	.MusicCtrl = MP3MusicCtrl,
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

static int MP3MusicDecoderInit(void)
{

	return 0;
}

static void MP3MusicDecodeExit(void)
{	
	
}

/* 由于 mp3 格式不是很固定，所以凭借后缀名来判断是不是 mp3 格式文件 */
static int isSupportMP3(struct FileDesc *ptFileDesc)
{
	char *pcMP3Fmt;
	int iError = 0;

	pcMP3Fmt = strrchr(ptFileDesc->cFileName, '.');

	if(NULL == pcMP3Fmt){
		return 0;
	}

	iError = strcmp(pcMP3Fmt + 1, "mp3");

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

static int SetMp3OutPcmFmt(void)
{
    struct PcmHardwareParams tPcmHWParams;
    int iError = 0;

    tPcmHWParams.eAccessMod = SND_PCM_ACCESS_RW_INTERLEAVED;
    tPcmHWParams.dwSampRate = 44100;
    tPcmHWParams.dwChannels = 2;
    tPcmHWParams.eFmtMod    = 16;    /* libmad 默认值 */
  
    /* 打开一个音频设备
     */
    g_ptMP3PlaybackHandle = OpenPcmDev("default", SND_PCM_STREAM_PLAYBACK);
    if(NULL == g_ptMP3PlaybackHandle){
  	  DebugPrint(DEBUG_ERR"Can't open dev\n");
  	  return -1;
    }   
      
    iError = SetHarwareParams(g_ptMP3PlaybackHandle, &tPcmHWParams);
    if(iError){
  	    DebugPrint(DEBUG_ERR"Can't set dev params \n");
		snd_pcm_close(g_ptMP3PlaybackHandle);
        return -1;
    }

	pthread_once(&g_PthreadOnce, SetVolThreadOnce);

	return 0;
}

static int SkipIDxVxContents(struct IDxVxHeader *ptIDxVxHeader)
{
    int iSkipNum;

    if(!strncmp("ID3", (char *)ptIDxVxHeader->aucIDx, 3)){
        iSkipNum = (ptIDxVxHeader->aucIDxSize[0] & 0x7f) * 0x200000
			+ (ptIDxVxHeader->aucIDxSize[1] & 0x7f) * 0x4000
			+ (ptIDxVxHeader->aucIDxSize[2] & 0x7f) * 0x80
			+ (ptIDxVxHeader->aucIDxSize[3] & 0x7f); 

		return iSkipNum + 10;    /* 结构体本身 10 个字节 */
    }

	return 0;
}

static int GetPlayTimeForFile(unsigned char *ptDataFrameIndex, int iFileSize)
{
	unsigned char *pucDataFrame = ptDataFrameIndex;
	int iBitRate = 0;
	int iSampleRate = 0;
	int iPlayTime = 0;
	unsigned char ucChannels = 0;
	struct DataFrameHeader *ptDataFrameHeader;

	/* 有些文件可能开头不是帧数据，而是填充的 0 */
	while(*pucDataFrame != 0xFF){
		if(pucDataFrame >= ptDataFrameIndex + iFileSize){
			/* 文件可能已经损坏 */
			return -1;
		}
		pucDataFrame ++;
	}

	ptDataFrameHeader = (struct DataFrameHeader *)pucDataFrame;

	iBitRate   = g_aiMP3BitRateIndex[ptDataFrameHeader->bzBitRateIndex];
	iSampleRate = g_aiMP3SampleRateIndex[ptDataFrameHeader->bzSampleIndex];
	ucChannels  = ptDataFrameHeader->bzCahnnelMod;

	iPlayTime = iFileSize * 8 / iBitRate / 1000;
	
	return iPlayTime;
}

/*
 * 数据转换
 */
static inline
signed int scale(mad_fixed_t sample)
{
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * 该回调函数用于输出解码的音频数据. 它在每一帧的数据解码完毕之后被回调
 * 目的是为了输出或者播放 PCM 音频数据
 */

static
enum mad_flow Mp3DataOutput(void *data, struct mad_header const *header,
		     struct mad_pcm *pcm)
{
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    unsigned char aucFrameData[1152*4];
	int iFrameDataNum = 0;
	int iError = 0;
	
    /* pcm->samplerate contains the sampling frequency */
    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];
	
    while (nsamples--) {
        signed int sample;
     
        /* output sample(s) in 16-bit signed little-endian PCM */
     
        sample = scale(*left_ch++);
        aucFrameData[iFrameDataNum ++] = ((sample >> 0) & 0xff);
        aucFrameData[iFrameDataNum ++] = ((sample >> 8) & 0xff);
   		           
   	    if(nchannels == 2){
            sample = scale(*right_ch++);
            aucFrameData[iFrameDataNum ++] = ((sample >> 0) & 0xff);
            aucFrameData[iFrameDataNum ++] = ((sample >> 8) & 0xff);
        }else if(nchannels == 1){
            /* 如果是单声道就扩充数据 */
            aucFrameData[iFrameDataNum ++] = ((sample >> 0) & 0xff);
   		    aucFrameData[iFrameDataNum ++] = ((sample >> 8) & 0xff); 
   	    }

    }

    /* 传送数据,四个字节为单位 */
	iError = snd_pcm_writei(g_ptMP3PlaybackHandle, aucFrameData, iFrameDataNum / 4);

	if(g_bMp3DecoderExit){
		return MAD_FLOW_STOP;
	}
  
    return MAD_FLOW_CONTINUE;
}

/*
 * 回调函数. mad_stream_buffer() 获得输入数据流
 * 当此函数第二次被调用的时候，音乐结束
 */
static
enum mad_flow Mp3GetInput(void *data, struct mad_stream *stream)
{
	struct PcmFmtParams *ptPcmFmtParams = data;

	if (!ptPcmFmtParams->dwDataLength){
		return MAD_FLOW_STOP;		
	}

	/* 要解码的数据流,因为之前已经映射数据到内存了，所以直接使用此函数即可，可能有别的
	 *  函数用于没有提前映射的数据解码
	 */
	mad_stream_buffer(stream, ptPcmFmtParams->pucDataStart, ptPcmFmtParams->dwDataLength);

	ptPcmFmtParams->dwDataLength = 0;

	return MAD_FLOW_CONTINUE;
}

/*
 * 错误处理函数 MAD_ERROR_* 错误在 mad.h (or stream.h) 中定义
 */
static
enum mad_flow Mp3DecoderError(void *data, struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct PcmFmtParams *ptPcmFmtParams = data;

  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - ptPcmFmtParams->pucDataStart);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

static void MP3Thread1Clean(void *data)
{
	g_iRuntimeRatio = 0;
	g_bMusicHalt = 0;
	pthread_mutex_unlock(&g_tMutex);
	g_bMP3TimeThreadExit = 1;
}

static void *MP3TimeThread(void *data)
{
	int iTotalPlayTimeUsec = g_iMP3PlayTime * 1000;
	int iHasPlayTimeUsec = 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	/* 设置清理线程 */
	pthread_cleanup_push(MP3Thread1Clean, NULL);

	g_bMP3TimeThreadExit = 0;

	while(iHasPlayTimeUsec < iTotalPlayTimeUsec){
		pthread_testcancel();    /* 线程取消点 */
		
		/* 获取锁并休眠 */
		pthread_mutex_lock(&g_tMutex);
		if(g_bMusicHalt){
			pthread_cond_wait(&g_tConVar, &g_tMutex);
		}		
		pthread_mutex_unlock(&g_tMutex);

		g_iRuntimeRatio = (float)iHasPlayTimeUsec / iTotalPlayTimeUsec * 1000;
		usleep(50 * 1000);		
		iHasPlayTimeUsec = iHasPlayTimeUsec + 50;
	}

	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}

static void *MP3PlayThread(void *data)
{
	struct mad_decoder decoder;
	int result;

	g_bMp3DecoderExit = 0;    /* 初始化为 0 */
	g_bMP3PlayThreadExit = 0;

	/* 配置输入、输出和错误回调函数 */
	mad_decoder_init(&decoder, data,
		   Mp3GetInput, 0, 0, Mp3DataOutput,
		   Mp3DecoderError, 0);

	/* 开始解码 */
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	/* 解码结束 */
	mad_decoder_finish(&decoder);

	snd_pcm_close(g_ptMP3PlaybackHandle);

	g_bMP3PlayThreadExit = 1;

	pthread_exit(NULL);
}

static int MP3MusicPlay(struct FileDesc *ptFileDesc)
{
	struct IDxVxHeader *ptIDxVxHeader = (struct IDxVxHeader *)ptFileDesc->pucFileMem;
	int iError = 0;
	int iSkipSize = 0;
	unsigned char *pucMemStart = ptFileDesc->pucFileMem;
	int iRealPcmDataSize = 0;

	iError = SetMp3OutPcmFmt();
	if(iError){
		DebugPrint("SetMp3OutPcmFmt error \n");
		return -1;
	}

	/* 有很长一段是歌词等等信息，要跳过 */
    ptIDxVxHeader = (struct IDxVxHeader *)pucMemStart;
	iSkipSize = SkipIDxVxContents(ptIDxVxHeader);

    /* 结尾有 ID3V1, 长 128 字节 */
    iRealPcmDataSize = ptFileDesc->iFileSize - iSkipSize - 128;
    pucMemStart = pucMemStart + iSkipSize;

	/* 初始化私有信息结构体，包括要解码的其起始地址以及长度 */
	g_tMP3PcmFmtParams.pucDataStart = pucMemStart;
	g_tMP3PcmFmtParams.dwDataLength = iRealPcmDataSize;
	g_tMP3PcmFmtParams.dwDataLengthBak = iRealPcmDataSize;
	g_tMP3PcmFmtParams.pucDataEnd  = g_tMP3PcmFmtParams.pucDataStart + iRealPcmDataSize;

	g_iMP3PlayTime = GetPlayTimeForFile(g_tMP3PcmFmtParams.pucDataStart, iRealPcmDataSize);

	pthread_create(&g_MP3TimeThreadId, NULL, MP3TimeThread, NULL);	
	pthread_create(&g_MP3PlayThreadId, NULL, MP3PlayThread, &g_tMP3PcmFmtParams);

	return 0;
}

static int MP3MusicCtrl(enum MusicCtrlCode eCtrlCode)
{
	char acVolStr[3];

	switch(eCtrlCode){
		case MUSIC_CTRL_CODE_HALT : {
			snd_pcm_pause(g_ptMP3PlaybackHandle, 1);
			pthread_mutex_lock(&g_tMutex);
			g_bMusicHalt = 1;
			pthread_mutex_unlock(&g_tMutex);	/* Release the lock */

			break;
		}
		case MUSIC_CTRL_CODE_PLAY : {
			snd_pcm_pause(g_ptMP3PlaybackHandle, 0);
			/* Get lock */
			pthread_mutex_lock(&g_tMutex);
			g_bMusicHalt = 0;
			pthread_cond_signal(&g_tConVar);	/* Weak the main thread */
			pthread_mutex_unlock(&g_tMutex);	/* Release the lock */

			break;
		}
		case MUSIC_CTRL_CODE_EXIT : {
			/* 有可能线程取消的时候音乐正在暂停
			 * 不管有没有暂停歌曲，都先运行歌曲，然后再取消线程
			 * 防止由于阻塞导致程序不响应
			 */
			if(!g_bMP3PlayThreadExit){
				snd_pcm_pause(g_ptMP3PlaybackHandle, 0);
				g_bMp3DecoderExit = 1;
				pthread_join(g_MP3PlayThreadId, NULL);
				snd_pcm_drop(g_ptMP3PlaybackHandle);
			}

			if(!g_bMP3TimeThreadExit){
				pthread_cancel(g_MP3TimeThreadId);
				pthread_join(g_MP3TimeThreadId, NULL);				
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

int MP3ParserInit(void)
{
	int iError = 0;

	iError = RegisterMusicParser(&g_tMP3MusicParser);
	iError |= MP3MusicDecoderInit();

	return iError;
}

void MP3ParserExit(void)
{
	UnregisterMusicParser(&g_tMP3MusicParser);
	MP3MusicDecodeExit();
}

