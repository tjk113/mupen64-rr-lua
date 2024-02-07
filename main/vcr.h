#ifndef VCR_H
#define VCR_H

# include <Windows.h>

#include "plugin.hpp"

#ifdef __cplusplus	//don't include cpp headers when .c compilation unit includes the file

#include <string>	//(also done later for ffmpeg functions)
#include <functional>

#endif

enum
{
	SUCCESS = (0),
	WRONG_FORMAT = (1),
	WRONG_VERSION = (2),
	FILE_NOT_FOUND = (3),
	NOT_FROM_THIS_MOVIE = (4),
	NOT_FROM_A_MOVIE = (5),
	INVALID_FRAME = (6),
	UNKNOWN_ERROR = (7)
};

enum
{
	VCR_PLAYBACK_SUCCESS = (0),
	VCR_PLAYBACK_ERROR = (-1),
	VCR_PLAYBACK_SAVESTATE_MISSING = (-2),
	VCR_PLAYBACK_FILE_BUSY = (-3),
	VCR_PLAYBACK_INCOMPATIBLE = (-4)
};

enum
{
	MOVIE_START_FROM_SNAPSHOT = (1<<0),
	MOVIE_START_FROM_NOTHING = (1<<1),
	MOVIE_START_FROM_EEPROM = (1<<2),
	MOVIE_START_FROM_EXISTING_SNAPSHOT = (1<<3)
};

#define CONTROLLER_X_PRESENT(x)	(1<<(x))

enum
{
	CONTROLLER_1_PRESENT = (1<<0),
	CONTROLLER_2_PRESENT = (1<<1),
	CONTROLLER_3_PRESENT = (1<<2),
	CONTROLLER_4_PRESENT = (1<<3)
};

#define CONTROLLER_X_MEMPAK(x)	(1<<((x)+4))

enum
{
	CONTROLLER_1_MEMPAK = (1<<4),
	CONTROLLER_2_MEMPAK = (1<<5),
	CONTROLLER_3_MEMPAK = (1<<6),
	CONTROLLER_4_MEMPAK = (1<<7)
};

#define CONTROLLER_X_RUMBLE(x)	(1<<((x)+8))

enum
{
	CONTROLLER_1_RUMBLE = (1<<8),
	CONTROLLER_2_RUMBLE = (1<<9),
	CONTROLLER_3_RUMBLE = (1<<10),
	CONTROLLER_4_RUMBLE = (1<<11)
};

enum
{
	MOVIE_AUTHOR_DATA_SIZE = (222),
	MOVIE_DESCRIPTION_DATA_SIZE = (256)
};

#define MOVIE_MAX_METADATA_SIZE (MOVIE_DESCRIPTION_DATA_SIZE > MOVIE_AUTHOR_DATA_SIZE ? MOVIE_DESCRIPTION_DATA_SIZE : MOVIE_AUTHOR_DATA_SIZE)

enum
{
	VCR_SYNC_AUDIO_DUPL = 0,
	VCR_SYNC_VIDEO_SNDROP = 1,
	VCR_SYNC_NONE = 2
};

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


extern void vcr_update_screen();
extern void vcr_ai_dacrate_changed(system_type type);
extern void vcr_ai_len_changed();

extern BOOL vcr_is_active();
extern BOOL vcr_is_idle(); // not the same as !isActive()
extern BOOL vcr_is_starting();
extern BOOL vcr_is_starting_and_just_restarted();
extern BOOL vcr_is_playing();
extern BOOL vcr_is_recording();
extern BOOL vcr_is_capturing();
extern const char* vcr_get_movie_filename();
extern BOOL vcr_get_read_only();
extern bool vcr_is_looping();
extern void vcr_set_read_only(BOOL val);
extern unsigned long vcr_get_length_v_is();
extern unsigned long vcr_get_length_samples();
extern void vcr_set_length_v_is(unsigned long val);
extern void vcr_set_length_samples(unsigned long val);
extern void vcr_toggle_read_only();

extern void vcr_movie_freeze(char** buf, unsigned long* size);
extern int vcr_movie_unfreeze(const char* buf, unsigned long size);


extern int vcr_start_record(const char* filename, unsigned short flags,
                           const char* author_utf8, const char* description_utf8);
extern int vcr_stop_record();
extern int vcr_start_playback(const std::string &filename, const char* author_utf8,
                             const char* description_utf8);
extern int vcr_stop_playback();
extern int vcr_stop_capture();

//ffmpeg
#ifdef __cplusplus
int vcr_start_f_fmpeg_capture(const std::string& output_name,
                           const std::string& arguments);
void vcr_stop_f_fmpeg_capture();
#endif

extern void vcr_core_stopped();

/**
 * \brief Updates the statusbar with the current VCR state
 */
void vcr_update_statusbar();

/**
 * \brief Clears all save data related to the current rom, such as SRAM, EEP and mempak
 */
void vcr_clear_save_data();

/**
 * \brief Starts an AVI capture
 * \param path The path to the AVI output file
 * \param show_codec_dialog Whether the user should be presented with a dialog to pick the capture codec
 * \return Whether the operation succeded
 */
bool vcr_start_capture(const char* path, bool show_codec_dialog);

/**
 * \brief Notifies VCR engine about controller being polled
 * \param index The polled controller's index
 * \param input The controller's input data
 */
void vcr_on_controller_poll(int index, BUTTONS* input);

/**
 * \brief Notifies VCR engine about a new VI
 */
void vcr_on_vi();

#ifdef __cplusplus
/**
 * \brief Initializes the VCR engine
 * \param on_task_changed Callback which is invoked whenever the VCR task changes
 */
void vcr_init(std::function<void(e_task)> on_task_changed);
#endif

void vcr_recent_movies_build(int32_t reset = 0);
void vcr_recent_movies_add(const std::string path);
int32_t vcr_recent_movies_play(uint16_t menu_item_id);


bool is_frame_skipped();

extern t_movie_header vcr_get_header_info(const char* filename);
extern char vcr_lastpath[MAX_PATH];
extern uint64_t screen_updates;
extern std::filesystem::path movie_path;

#endif // VCR_H
