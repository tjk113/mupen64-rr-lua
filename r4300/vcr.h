#pragma once

#include <functional>
#include "../r4300/Plugin.hpp"

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
	VCR_PLAYBACK_INCOMPATIBLE = (-4),
	VCR_PLAYBACK_NO_MATCHING_ROM = (-5),
};

enum
{
	MOVIE_START_FROM_SNAPSHOT = (1 << 0),
	MOVIE_START_FROM_NOTHING = (1 << 1),
	MOVIE_START_FROM_EEPROM = (1 << 2),
	MOVIE_START_FROM_EXISTING_SNAPSHOT = (1 << 3)
};

#define CONTROLLER_X_PRESENT(x)	(1<<(x))

enum
{
	CONTROLLER_1_PRESENT = (1 << 0),
	CONTROLLER_2_PRESENT = (1 << 1),
	CONTROLLER_3_PRESENT = (1 << 2),
	CONTROLLER_4_PRESENT = (1 << 3)
};

#define CONTROLLER_X_MEMPAK(x)	(1<<((x)+4))

enum
{
	CONTROLLER_1_MEMPAK = (1 << 4),
	CONTROLLER_2_MEMPAK = (1 << 5),
	CONTROLLER_3_MEMPAK = (1 << 6),
	CONTROLLER_4_MEMPAK = (1 << 7)
};

#define CONTROLLER_X_RUMBLE(x)	(1<<((x)+8))

enum
{
	CONTROLLER_1_RUMBLE = (1 << 8),
	CONTROLLER_2_RUMBLE = (1 << 9),
	CONTROLLER_3_RUMBLE = (1 << 10),
	CONTROLLER_4_RUMBLE = (1 << 11)
};

enum
{
	MOVIE_AUTHOR_DATA_SIZE = (222),
	MOVIE_DESCRIPTION_DATA_SIZE = (256)
};

#define MOVIE_MAX_METADATA_SIZE (MOVIE_DESCRIPTION_DATA_SIZE > MOVIE_AUTHOR_DATA_SIZE ? MOVIE_DESCRIPTION_DATA_SIZE : MOVIE_AUTHOR_DATA_SIZE)

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


/**
 * \brief The movie freeze buffer, which is used to store the movie (with only essential data) associated with a savestate inside the savestate.
 */
struct t_movie_freeze
{
	unsigned long size;
	unsigned long uid;
	unsigned long current_sample;
	unsigned long current_vi;
	unsigned long length_samples;
	std::vector<uint8_t> input_buffer;
};

extern volatile e_task m_task;

bool task_is_playback(e_task task);
bool task_is_recording(e_task task);

extern bool vcr_is_active();
extern bool vcr_is_idle(); // not the same as !isActive()
extern bool vcr_is_starting();
extern bool vcr_is_playing();
extern bool vcr_is_recording();
extern bool vcr_is_looping();
extern unsigned long vcr_get_length_v_is();
extern unsigned long vcr_get_length_samples();
extern void vcr_set_length_v_is(unsigned long val);
extern void vcr_set_length_samples(unsigned long val);


extern int vcr_start_record(const char* filename, unsigned short flags,
                            const char* author_utf8, const char* description_utf8);
extern int vcr_stop_record();

extern int vcr_stop_playback();

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

namespace VCR
{
	enum class Result
	{
		// The operation completed successfully
		Ok,
		// The provided data has an invalid format
		InvalidFormat,
		// The provided file is inaccessible or does not exist
		BadFile,
		// The user cancelled the operation
		Cancelled,
		// The controller configuration is invalid
		InvalidControllers,
		// The movie's savestate is missing or invalid
		InvalidSavestate,
		// The resulting frame is outside the bounds of the movie
		InvalidFrame,
		// There is no rom which matches this movie
		NoMatchingRom,
		// The callee is already performing another task
		Busy,
		// The VCR engine is idle, but must be active to complete this operation
		Idle,
		// The provided freeze buffer is not from the currently active movie
		NotFromThisMovie,
	};

	/**
	 * \brief Initializes the VCR engine
	 */
	void init();

	/**
	 * \brief Parses a movie's header
	 * \param path The movie's path
	 * \param header The header to fill
	 * \return The operation result
	 */
	Result parse_header(std::filesystem::path path, t_movie_header* header);

	/**
 	* \brief Starts playing back a movie
 	* \param path The movie's path
 	* \return The error code
 	*/
	Result start_playback(std::filesystem::path path);

	/**
	 * \brief Computes whether a seek operation is possible
	 * \param frame The frame to seek to
	 * \param relative Whether the seek is relative to the current frame
	 * \return Whether the specified parameters allow a seek operation
	 */
	bool can_seek_to(int32_t frame, bool relative);

	/**
	 * \brief Begins seeking to a frame in the currently playing movie
	 * \param frame The frame to seek to
	 * \param relative Whether the seek is relative to the current frame
	 * \return The operation result
	 * \remarks When the seek operation completes, the SeekCompleted message will be sent
	 */
	Result begin_seek_to(int32_t frame, bool relative);

	/**
	 * \brief Stops the current seek operation
	 */
	void stop_seek();

	/**
	 * \brief Gets whether the VCR engine is currently performing a seek operation
	 */
	bool is_seeking();

	/**
	 * \brief Generates the current movie freeze buffer
	 * \return The movie freeze buffer, or nothing
	 */
	std::optional<t_movie_freeze> freeze();

	/**
	 * \brief Restores the movie from a freeze buffer
	 * \param freeze The freeze buffer
	 * \return The operation result
	 */
	Result unfreeze(t_movie_freeze freeze);

	/**
	 * \brief Stops all running tasks
	 * \return The operation result
	 */
	int stop_all();

	/**
	 * \brief Gets the text representation of the last frame's inputs
	 */
	const char* get_input_text();

	/**
	 * \brief Gets the text representation of the VCR state's inputs
	 */
	const char* get_status_text();
}

bool is_frame_skipped();
extern std::filesystem::path movie_path;
