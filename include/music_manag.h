#ifndef __MUSIC_MANAG_H__
#define __MUSIC_MANAG_H__

#include <common_config.h>
#include <common_st.h>
#include <file.h>
#include <alsa/asoundlib.h>

enum MusicCtrlCode
{
	MUSIC_CTRL_CODE_PLAY = 0,
	MUSIC_CTRL_CODE_START,
	MUSIC_CTRL_CODE_HALT,
	MUSIC_CTRL_CODE_EXIT,
	MUSIC_CTRL_CODE_ADD_VOL,
	MUSIC_CTRL_CODE_DEC_VOL,
	MUSIC_CTRL_CODE_GET_VOL,
	MUSIC_CTRL_CODE_GET_RUNTIME,
	MUSIC_CTRL_CODE_MAX,
};

struct PcmHardwareParams
{
	snd_pcm_access_t eAccessMod;
	unsigned int eFmtMod;
	unsigned int dwSampRate;
	unsigned int dwChannels;
};

struct MusicParser
{
	char *name;
	int (*MusicDecoderInit)(void);
	void (*MusicDecodeExit)(void);
	int (*isSupport)(struct FileDesc *ptFileDesc);
	int (*MusicPlay)(struct FileDesc *ptFileDesc);
	int (*MusicCtrl)(enum MusicCtrlCode eCtrlCode);
	struct list_head tMusicParser;
};

int RegisterMusicParser(struct MusicParser *ptMusicParser);
void UnregisterMusicParser(struct MusicParser *ptMusicParser);
struct MusicParser *GetMusicParser(char *pcName);
struct MusicParser *GetSupportedMusicParser(struct FileDesc *ptFileDesc);
int isMusicSupported(char *pcName);
struct MusicParser *PlayMusic(struct FileDesc *ptFileDesc, char *strMusicPath);
void StopMusic(struct FileDesc *ptFileDesc, struct MusicParser *ptMusicParser);
int CtrlMusic(struct MusicParser *ptMusicParser, enum MusicCtrlCode eCtrlCode);
void ShowMusicParser(void);
int WAVParserInit(void);
void WAVParserExit(void);
int MP3ParserInit(void);
void MP3ParserExit(void);
int MusicParserInit(void);

#endif

