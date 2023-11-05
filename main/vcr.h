#ifndef VCR_H_
#define VCR_H_

# include <Windows.h>

#include "plugin.hpp"

#ifdef __cplusplus	//don't include cpp headers when .c compilation unit includes the file

#include <string>	//(also done later for ffmpeg functions)
#endif

#define SUCCESS (0)
#define WRONG_FORMAT (1)
#define WRONG_VERSION (2)
#define FILE_NOT_FOUND (3)
#define NOT_FROM_THIS_MOVIE (4)
#define NOT_FROM_A_MOVIE (5)
#define INVALID_FRAME (6)
#define UNKNOWN_ERROR (7)

#define VCR_PLAYBACK_SUCCESS (0)
#define VCR_PLAYBACK_ERROR (-1)
#define VCR_PLAYBACK_SAVESTATE_MISSING (-2)
#define VCR_PLAYBACK_FILE_BUSY (-3)
#define VCR_PLAYBACK_INCOMPATIBLE (-4)

#define MOVIE_START_FROM_SNAPSHOT			(1<<0)
#define MOVIE_START_FROM_NOTHING			(1<<1)
#define MOVIE_START_FROM_EEPROM				(1<<2)
#define MOVIE_START_FROM_EXISTING_SNAPSHOT	(1<<3)

#define CONTROLLER_X_PRESENT(x)	(1<<(x))
#define CONTROLLER_1_PRESENT	(1<<0)
#define CONTROLLER_2_PRESENT	(1<<1)
#define CONTROLLER_3_PRESENT	(1<<2)
#define CONTROLLER_4_PRESENT	(1<<3)
#define CONTROLLER_X_MEMPAK(x)	(1<<((x)+4))
#define CONTROLLER_1_MEMPAK 	(1<<4)
#define CONTROLLER_2_MEMPAK 	(1<<5)
#define CONTROLLER_3_MEMPAK 	(1<<6)
#define CONTROLLER_4_MEMPAK 	(1<<7)
#define CONTROLLER_X_RUMBLE(x)	(1<<((x)+8))
#define CONTROLLER_1_RUMBLE 	(1<<8)
#define CONTROLLER_2_RUMBLE 	(1<<9)
#define CONTROLLER_3_RUMBLE 	(1<<10)
#define CONTROLLER_4_RUMBLE 	(1<<11)

#define MOVIE_AUTHOR_DATA_SIZE (222)
#define MOVIE_DESCRIPTION_DATA_SIZE (256)
#define MOVIE_MAX_METADATA_SIZE (MOVIE_DESCRIPTION_DATA_SIZE > MOVIE_AUTHOR_DATA_SIZE ? MOVIE_DESCRIPTION_DATA_SIZE : MOVIE_AUTHOR_DATA_SIZE)

#define VCR_SYNC_AUDIO_DUPL 0
#define VCR_SYNC_VIDEO_SNDROP 1
#define VCR_SYNC_NONE 2

/*
	(0x0001)	Directional Right
	(0x0002)	Directional Left
	(0x0004)	Directional Down
	(0x0008)	Directional Up
	(0x0010)	Start
	(0x0020)	Z
	(0x0040)	B
	(0x0080)	A
	(0x0100)	C Right
	(0x0200)	C Left
	(0x0400)	C Down
	(0x0800)	C Up
	(0x1000)	R
	(0x2000)	L
	(0x00FF0000) Analog X
	(0xFF000000) Analog Y
*/

extern bool gStopAVI;

extern void VCR_getKeys(int Control, BUTTONS* Keys);
extern void VCR_updateScreen();
extern void VCR_aiDacrateChanged(system_type type);
extern void VCR_aiLenChanged();

extern BOOL VCR_isActive();
extern BOOL VCR_isIdle(); // not the same as !isActive()
extern BOOL VCR_isStarting();
extern BOOL VCR_isStartingAndJustRestarted();
extern BOOL VCR_isPlaying();
extern BOOL VCR_isRecording();
extern BOOL VCR_isCapturing();
extern void VCR_invalidatedCaptureFrame();
extern const char* VCR_getMovieFilename();
extern BOOL VCR_getReadOnly();
extern bool VCR_isLooping();
extern bool VCR_isRestarting();
extern void VCR_setReadOnly(BOOL val);
extern unsigned long VCR_getLengthVIs();
extern unsigned long VCR_getLengthSamples();
extern void VCR_setLengthVIs(unsigned long val);
extern void VCR_setLengthSamples(unsigned long val);
extern void VCR_toggleReadOnly();
extern void VCR_toggleLoopMovie();

extern void VCR_movieFreeze(char** buf, unsigned long* size);
extern int VCR_movieUnfreeze(const char* buf, unsigned long size);


extern int VCR_startRecord(const char* filename, unsigned short flags,
                           const char* authorUTF8, const char* descriptionUTF8,
                           int defExt);
extern int VCR_stopRecord(int defExt);
extern int VCR_startPlayback(const std::string &filename, const char* authorUTF8,
                             const char* descriptionUTF8);
extern int VCR_stopPlayback();
extern int VCR_startCapture(const char* recFilename, const char* aviFilename,
                            bool codecDialog);
extern int VCR_stopCapture();

//ffmpeg
#ifdef __cplusplus
int VCR_StartFFmpegCapture(const std::string& outputName,
                           const std::string& arguments);
void VCR_StopFFmpegCapture();
#endif

extern void VCR_coreStopped();

extern void printWarning(const char*);
extern void printError(const char*);

/**
 * \brief Updates the statusbar with the current VCR state
 */
void vcr_update_statusbar();

/**
 * \brief Clears all save data related to the current rom, such as SRAM, EEP and mempak
 */
extern void vcr_clear_save_data();

void vcr_recent_movies_build(int32_t reset = 0);
void vcr_recent_movies_add(const std::string path);
int32_t vcr_recent_movies_play(uint16_t menu_item_id);

#pragma pack(push, 1)
/**
 * \brief
 */
typedef struct
{
	/**
	 * \brief <c>M64\0x1a</c>
	 */
	unsigned long magic;

	/**
	 * \brief Currently <c>3</c>
	 */
	unsigned long version;

	/**
	 * \brief Movie creation time, used to correlate savestates to a movie
	 */
	unsigned long uid;

	/**
	 * \brief Amount of VIs in the movie
	 */
	unsigned long length_vis;

	/**
	 * \brief Amount of loaded states during recording
	 */
	unsigned long rerecord_count;

	/**
	 * \brief The VIs per second of the rom this movie was recorded on
	 * \remarks (unused)
	 * \remarks This field makes no sense, as VI/s are already known via ROM cc
	 */
	unsigned char vis_per_second;

	/**
	 * \brief The amount of controllers connected during recording
	 * \remarks (pretty much unused)
	 * \remarks This field makes no sense, as this value can already be figured out by checking controller flags post-load
	 */
	unsigned char num_controllers;

	unsigned short reserved1;

	/**
	 * \brief Amount of input samples in the movie, ideally equal to <c>(file_length - 1024) / 4</c>
	 */
	unsigned long length_samples;

	/**
	 * \brief What state the movie is expected to start from
	 * \remarks vcr.h:32
	 */
	unsigned short startFlags;

	unsigned short reserved2;

	/**
	 * \brief Bitfield of flags for each controller. Sequence of present, mempak and rumble bits with per-controller values respectively (c1,c2,c3,c4,r1,...)
	 */
	unsigned long controller_flags;

	unsigned long reserved_flags[8];

	/**
	 * \brief The name of the movie's author
	 * \remarks Used in .rec (version 2)
	 */
	char old_author_info[48];

	/**
	 * \brief A description of what the movie is about
	 * \remarks Used in .rec (version 2)
	 */
	char old_description[80];

	/**
	 * \brief The internal name of the rom the movie was recorded on
	 */
	char rom_name[32];

	/**
	 * \brief The primary checksum of the rom the movie was recorded on
	 */
	unsigned long rom_crc1;

	/**
	 * \brief The rom country of the rom the movie was recorded on
	 */
	unsigned short rom_country;

	char reserved_bytes[56];

	/**
	 * \brief The name of the video plugin the movie was recorded with, as specified by the plugin
	 */
	char video_plugin_name[64];

	/**
	 * \brief The name of the audio plugin the movie was recorded with, as specified by the plugin
	 */
	char audio_plugin_name[64];

	/**
	 * \brief The name of the input plugin the movie was recorded with, as specified by the plugin
	 */
	char input_plugin_name[64];

	/**
	 * \brief The name of the RSP plugin the movie was recorded with, as specified by the plugin
	 */
	char rsp_plugin_name[64];

	/**
	 * \brief The name of the movie's author as a UTF-8 string
	 */
	char author[MOVIE_AUTHOR_DATA_SIZE];

	/**
	 * \brief A description of what the movie is about as a UTF-8 string
	 */
	char description[MOVIE_DESCRIPTION_DATA_SIZE];
} t_movie_header;
#pragma pack(pop)

enum class e_task
{
	idle,
	start_recording,
	start_recording_from_snapshot,
	start_recording_from_existing_snapshot,
	recording,
	start_playback,
	start_playback_from_snapshot,
	playback
};

extern e_task m_task;

extern t_movie_header VCR_getHeaderInfo(const char* filename);
extern char VCR_Lastpath[MAX_PATH];
extern uint64_t screen_updates;

#endif // VCR_H_
