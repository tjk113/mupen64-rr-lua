//#include "../config.h"
#include <cassert>

#include "win/main_win.h"
#include "win/features/Statusbar.hpp"
#include "win/features/Toolbar.hpp"

#include "../lua/LuaConsole.h"

#include "vcr.h"
#include "vcr_compress.h"
#include "vcr_resample.h"

//ffmpeg
#include "ffmpeg_capture/ffmpeg_capture.hpp"
#include <memory>

#include "plugin.hpp"
#include "rom.h"
#include "savestates.h"
#include "../memory/memory.h"

#include <filesystem>
#include <cerrno>
#include <malloc.h>
#include <cstdio>
#include <cstring>
#ifdef _MSC_VER
#define SNPRINTF	_snprintf
#define STRCASECMP	_stricmp
#define STRNCASECMP	_strnicmp
#else
#include <unistd.h>
#endif
//#include <zlib.h>
#include <ctime>
#include <chrono>
#include <commctrl.h> // for SendMessage, SB_SETTEXT
#include <Windows.h> // for truncate functions
#include <../../winproject/resource.h> // for EMU_RESET
#include "win/Config.hpp" //config struct
#include <WinUser.h>
#include "win/Commandline.h"

#ifdef _DEBUG
#include "../r4300/macros.h"
#endif

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif

// M64\0x1a
enum
{
	mup_magic = (0x1a34364d),
	mup_version = (3),
	mup_header_size_old = (512)
};

#define MUP_HEADER_SIZE (sizeof(t_movie_header))
#define MUP_HEADER_SIZE_CUR (m_header.version <= 2 ? mup_header_size_old : MUP_HEADER_SIZE)

enum { max_avi_size = 0x7B9ACA00 };
BOOL dont_play = false;
enum { buffer_growth_size = 4096 };

static const char* m_err_code_name[] =
{
	"Success",
	"Wrong Format",
	"Wrong Version",
	"File Not Found",
	"Not From This Movie",
	"Not From A Movie",
	"Invalid Frame",
	"Unknown Error"
};

e_task m_task = e_task::idle;
std::filesystem::path movie_path;

static char m_filename[PATH_MAX];
static char avi_file_name[PATH_MAX];
static FILE* m_file = nullptr;
static t_movie_header m_header;

static BOOL m_read_only = FALSE;
long m_current_sample = -1;
// should = length_samples when recording, and be < length_samples when playing
int m_current_vi = -1;
static int m_vis_per_second = -1;
static char* m_input_buffer = nullptr;
static unsigned long m_input_buffer_size = 0;
static char* m_input_buffer_ptr = nullptr;

static int m_capture = 0; // capture movie
static int m_audio_freq = 33000; //0x30018;
static int m_audio_bitrate = 16; // 16 bits
static long double m_video_frame = 0;
static long double m_audio_frame = 0;
#define SOUND_BUF_SIZE (44100*2*2) // 44100=1s sample, soundbuffer capable of holding 4s future data in circular buffer

static char sound_buf[SOUND_BUF_SIZE];
static char sound_buf_empty[SOUND_BUF_SIZE];
static int sound_buf_pos = 0;
long last_sound = 0;

int avi_increment = 0;
int title_length;
char vcr_lastpath[MAX_PATH];
bool is_restarting_flag = false;

bool capture_with_f_fmpeg = true;
std::unique_ptr<FFmpegManager> capture_manager;
uint64_t screen_updates = 0;

static int start_playback(const char* filename, const char* author_utf8,
                         const char* description_utf8, const bool restarting);
static int restart_playback();
static int stop_playback(const bool bypass_loop_setting);

static void write_movie_header(FILE* file, int)
{
	//	assert(ftell(file) == 0); // we assume file points to beginning of movie file
	fseek(file, 0L, SEEK_SET);

	m_header.version = mup_version; // make sure to update the version!
	///	m_header.length_vis = m_header.length_samples / m_header.num_controllers; // wrong

	fwrite(&m_header, 1, MUP_HEADER_SIZE, file);

	fseek(file, 0L, SEEK_END);
}

char* strtrimext(char* myStr)
{
	char* ret_str;
	if (myStr == nullptr) return nullptr;
	if ((ret_str = (char*)malloc(strlen(myStr) + 1)) == nullptr) return nullptr;
	strcpy(ret_str, myStr);
	if (char* last_ext = strrchr(ret_str, '.'); last_ext != nullptr)
		*last_ext = '\0';
	return ret_str;
}

void print_warning(const char* str)
{
	MessageBox(nullptr, str, "Warning", MB_OK | MB_ICONWARNING);
}

void print_error(const char* str)
{
	MessageBox(nullptr, str, "Error", MB_OK | MB_ICONERROR);
}


static void hard_reset_and_clear_all_save_data(const bool clear)
{
	extern BOOL clear_sram_on_restart_mode;
	clear_sram_on_restart_mode = clear;
	continue_vcr_on_restart_mode = TRUE;
	if (clear)
		printf("Clearing save data...\n");
	else
		printf("Playing movie without clearing save data\n");
	SendMessage(mainHWND, WM_COMMAND, EMU_RESET, 0);
}

static int vis_by_countrycode()
{
	if (m_vis_per_second == -1)
	{
		switch (ROM_HEADER.Country_code & 0xFF)
		{
		case 0x44:
		case 0x46:
		case 0x49:
		case 0x50:
		case 0x53:
		case 0x55:
		case 0x58:
		case 0x59:
			m_vis_per_second = 50;
			break;

		case 0x37:
		case 0x41:
		case 0x45:
		case 0x4a:
			m_vis_per_second = 60;
			break;
		default:
			print_warning(
				"[VCR]: Warning - unknown country code, using 60 FPS for video.\n");
			m_vis_per_second = 60;
			break;
		}
	}

	return m_vis_per_second;
}

static void set_rom_info(t_movie_header* header)
{
	// FIXME
	switch (ROM_HEADER.Country_code & 0xFF)
	{
	case 0x37:
	case 0x41:
	case 0x45:
	case 0x4a:
	default:
		header->vis_per_second = 60; // NTSC
		break;
	case 0x44:
	case 0x46:
	case 0x49:
	case 0x50:
	case 0x53:
	case 0x55:
	case 0x58:
	case 0x59:
		header->vis_per_second = 50; // PAL
		break;
	}

	header->controller_flags = 0;
	header->num_controllers = 0;
	if (Controls[0].Present)
		header->controller_flags |= CONTROLLER_1_PRESENT, header->num_controllers
			++;
	if (Controls[1].Present)
		header->controller_flags |= CONTROLLER_2_PRESENT, header->num_controllers
			++;
	if (Controls[2].Present)
		header->controller_flags |= CONTROLLER_3_PRESENT, header->num_controllers
			++;
	if (Controls[3].Present)
		header->controller_flags |= CONTROLLER_4_PRESENT, header->num_controllers
			++;
	if (Controls[0].Plugin == controller_extension::mempak)
		header->controller_flags |= CONTROLLER_1_MEMPAK;
	if (Controls[1].Plugin == controller_extension::mempak)
		header->controller_flags |= CONTROLLER_2_MEMPAK;
	if (Controls[2].Plugin == controller_extension::mempak)
		header->controller_flags |= CONTROLLER_3_MEMPAK;
	if (Controls[3].Plugin == controller_extension::mempak)
		header->controller_flags |= CONTROLLER_4_MEMPAK;
	if (Controls[0].Plugin == controller_extension::rumblepak)
		header->controller_flags |= CONTROLLER_1_RUMBLE;
	if (Controls[1].Plugin == controller_extension::rumblepak)
		header->controller_flags |= CONTROLLER_2_RUMBLE;
	if (Controls[2].Plugin == controller_extension::rumblepak)
		header->controller_flags |= CONTROLLER_3_RUMBLE;
	if (Controls[3].Plugin == controller_extension::rumblepak)
		header->controller_flags |= CONTROLLER_4_RUMBLE;

	strncpy(header->rom_name, (const char*)ROM_HEADER.nom, 32);
	header->rom_crc1 = ROM_HEADER.CRC1;
	header->rom_country = ROM_HEADER.Country_code;

	header->input_plugin_name[0] = '\0';
	header->video_plugin_name[0] = '\0';
	header->audio_plugin_name[0] = '\0';
	header->rsp_plugin_name[0] = '\0';

	strncpy(header->video_plugin_name, video_plugin->name.c_str(),
	        64);
	strncpy(header->input_plugin_name, input_plugin->name.c_str(),
	        64);
	strncpy(header->audio_plugin_name, audio_plugin->name.c_str(),
	        64);
	strncpy(header->rsp_plugin_name, rsp_plugin->name.c_str(), 64);
}

static void reserve_buffer_space(const unsigned long space_needed)
{
	if (space_needed > m_input_buffer_size)
	{
		const unsigned long ptr_offset = m_input_buffer_ptr - m_input_buffer;
		const unsigned long alloc_chunks = space_needed / buffer_growth_size;
		m_input_buffer_size = buffer_growth_size * (alloc_chunks + 1);
		m_input_buffer = (char*)realloc(m_input_buffer, m_input_buffer_size);
		m_input_buffer_ptr = m_input_buffer + ptr_offset;
	}
}

static void truncate_movie()
{
	// truncate movie controller data to header.length_samples length

	const long trunc_len = MUP_HEADER_SIZE + sizeof(BUTTONS) * (m_header.
		length_samples);

	if (const HANDLE file_handle = CreateFile(m_filename, GENERIC_WRITE, 0, nullptr,
	                                          OPEN_EXISTING, 0, nullptr); file_handle != nullptr)
	{
		SetFilePointer(file_handle, trunc_len, nullptr, FILE_BEGIN);
		SetEndOfFile(file_handle);
		CloseHandle(file_handle);
	}
}

static int read_movie_header(FILE* file, t_movie_header* header)
{
	fseek(file, 0L, SEEK_SET);

	t_movie_header new_header = {};

	if (fread(&new_header, 1, mup_header_size_old, file) != mup_header_size_old)
		return WRONG_FORMAT;

	if (new_header.magic != mup_magic)
		return WRONG_FORMAT;

	if (new_header.version <= 0 || new_header.version > mup_version)
		return WRONG_VERSION;

	if (new_header.version == 1 || new_header.version == 2)
	{
		// attempt to recover screwed-up plugin data caused by
		// version mishandling and format problems of first versions

#define IS_ALPHA(x) (((x) >= 'A' && (x) <= 'Z') || ((x) >= 'a' && (x) <= 'z') || ((x) == '1'))
		int i;
		for (i = 0; i < 56 + 64; i++)
			if (IS_ALPHA(new_header.reserved_bytes[i])
				&& IS_ALPHA(new_header.reserved_bytes[i + 64])
				&& IS_ALPHA(new_header.reserved_bytes[i + 64 + 64])
				&& IS_ALPHA(new_header.reserved_bytes[i + 64 + 64 + 64]))
				break;
		if (i != 56 + 64)
		{
			memmove(new_header.video_plugin_name, new_header.reserved_bytes + i,
			        256);
		} else
		{
			for (i = 0; i < 56 + 64; i++)
				if (IS_ALPHA(new_header.reserved_bytes[i])
					&& IS_ALPHA(new_header.reserved_bytes[i + 64])
					&& IS_ALPHA(new_header.reserved_bytes[i + 64 + 64]))
					break;
			if (i != 56 + 64)
				memmove(new_header.audio_plugin_name, new_header.reserved_bytes + i,
				        256 - 64);
			else
			{
				for (i = 0; i < 56 + 64; i++)
					if (IS_ALPHA(new_header.reserved_bytes[i])
						&& IS_ALPHA(new_header.reserved_bytes[i + 64]))
						break;
				if (i != 56 + 64)
					memmove(new_header.input_plugin_name,
					        new_header.reserved_bytes + i, 256 - 64 - 64);
				else
				{
					for (i = 0; i < 56 + 64; i++)
						if (IS_ALPHA(new_header.reserved_bytes[i]))
							break;
					if (i != 56 + 64)
						memmove(new_header.rsp_plugin_name,
						        new_header.reserved_bytes + i,
						        256 - 64 - 64 - 64);
					else
						strncpy(new_header.rsp_plugin_name, "(unknown)", 64);

					strncpy(new_header.input_plugin_name, "(unknown)", 64);
				}
				strncpy(new_header.audio_plugin_name, "(unknown)", 64);
			}
			strncpy(new_header.video_plugin_name, "(unknown)", 64);
		}
		// attempt to convert old author and description to utf8
		strncpy(new_header.author, new_header.old_author_info, 48);
		strncpy(new_header.description, new_header.old_description, 80);
	}
	if (new_header.version == 3)
	{
		// read rest of header
		if (fread((char*)(&new_header) + mup_header_size_old, 1,
		          MUP_HEADER_SIZE - mup_header_size_old,
		          file) != MUP_HEADER_SIZE - mup_header_size_old)
			return WRONG_FORMAT;
	}

	*header = new_header;

	return SUCCESS;
}

void flush_movie()
{
	if (m_file && (m_task == e_task::recording || m_task == e_task::start_recording || m_task ==
		e_task::start_recording_from_snapshot || m_task ==
		e_task::start_recording_from_existing_snapshot))
	{
		// (over-)write the header
		write_movie_header(m_file, MUP_HEADER_SIZE);

		// (over-)write the controller data
		fseek(m_file, MUP_HEADER_SIZE, SEEK_SET);
		fwrite(m_input_buffer, 1, sizeof(BUTTONS) * (m_header.length_samples),
		       m_file);

		fflush(m_file);
	}
}

t_movie_header vcr_get_header_info(const char* filename)
{
	char buf[PATH_MAX];
	char temp_filename[PATH_MAX];
	t_movie_header temp_header = {};
	temp_header.rom_country = -1;
	strcpy(temp_header.rom_name, "(no ROM)");

	flush_movie();

	strncpy(temp_filename, filename, PATH_MAX);
	if (char* p = strrchr(temp_filename, '.'))
	{
		if (!STRCASECMP(p, ".m64") || !STRCASECMP(p, ".st"))
			*p = '\0';
	}
	// open record file
	strncpy(buf, temp_filename, PATH_MAX);
	FILE* temp_file = fopen(buf, "rb+");
	if (temp_file == nullptr && (temp_file = fopen(buf, "rb")) == nullptr)
	{
		strncat(buf, ".m64", 4);
		temp_file = fopen(buf, "rb+");
		if (temp_file == nullptr && (temp_file = fopen(buf, "rb")) == nullptr)
		{
			fprintf(
				stderr,
				"[VCR]: Could not get header info of .m64 file\n\"%s\": %s\n",
				filename, strerror(errno));
			return temp_header;
		}
	}

	read_movie_header(temp_file, &temp_header);
	fclose(temp_file);
	return temp_header;
}

void vcr_clear_save_data()
{
	{
		if (FILE* f = fopen(get_sram_path().string().c_str(), "wb"))
		{
			extern unsigned char sram[0x8000];
			for (unsigned char& i : sram) i = 0;
			fwrite(sram, 1, 0x8000, f);
			fclose(f);
		}
	}
	{
		if (FILE* f = fopen(get_eeprom_path().string().c_str(), "wb"))
		{
			extern unsigned char eeprom[0x8000];
			for (int i = 0; i < 0x800; i++) eeprom[i] = 0;
			fwrite(eeprom, 1, 0x800, f);
			fclose(f);
		}
	}
	{
		if (FILE* f = fopen(get_mempak_path().string().c_str(), "wb"))
		{
			extern unsigned char mempack[4][0x8000];
			for (auto& j : mempack)
			{
				for (int i = 0; i < 0x800; i++) j[i] = 0;
				fwrite(j, 1, 0x800, f);
			}
			fclose(f);
		}
	}
}


BOOL
vcr_is_active()
{
	return (m_task == e_task::recording || m_task == e_task::playback) ? TRUE : FALSE;
}

BOOL
vcr_is_idle()
{
	return (m_task == e_task::idle) ? TRUE : FALSE;
}

BOOL
vcr_is_starting()
{
	return (m_task == e_task::start_playback || m_task == e_task::start_playback_from_snapshot)
		       ? TRUE
		       : FALSE;
}

BOOL
vcr_is_starting_and_just_restarted()
{
	if (extern BOOL just_restarted_flag; m_task == e_task::start_playback && !continue_vcr_on_restart_mode &&
		just_restarted_flag)
	{
		just_restarted_flag = FALSE;
		m_current_sample = 0;
		m_current_vi = 0;
		m_task = e_task::playback;
		return TRUE;
	}

	return FALSE;
}

BOOL
vcr_is_playing()
{
	return (m_task == e_task::playback) ? TRUE : FALSE;
}

BOOL
vcr_is_recording()
{
	return (m_task == e_task::recording) ? TRUE : FALSE;
}

BOOL
vcr_is_capturing()
{
	return m_capture ? TRUE : FALSE;
}

BOOL
vcr_get_read_only()
{
	return m_read_only;
}

// Returns the filename of the last-played movie
const char* vcr_get_movie_filename()
{
	return m_filename;
}

void
vcr_set_read_only(const BOOL val)
{
	extern HWND mainHWND;
	if (m_read_only != val)
		CheckMenuItem(GetMenu(mainHWND), EMU_VCRTOGGLEREADONLY,
		              MF_BYCOMMAND | (val ? MFS_CHECKED : MFS_UNCHECKED));
	m_read_only = val;
}

bool vcr_is_looping()
{
	return Config.is_movie_loop_enabled;
}

bool vcr_is_restarting()
{
	return is_restarting_flag;
}

void vcr_set_loop_movie(const bool val)
{
	if (vcr_is_looping() != val)
		CheckMenuItem(GetMenu(mainHWND), ID_LOOP_MOVIE,
		              MF_BYCOMMAND | (val ? MFS_CHECKED : MFS_UNCHECKED));
	Config.is_movie_loop_enabled = val;
}

unsigned long vcr_get_length_v_is()
{
	return vcr_is_active() ? m_header.length_vis : 0;
}

unsigned long vcr_get_length_samples()
{
	return vcr_is_active() ? m_header.length_samples : 0;
}

void vcr_set_length_v_is(const unsigned long val)
{
	m_header.length_vis = val;
}

void vcr_set_length_samples(const unsigned long val)
{
	m_header.length_samples = val;
}

void
vcr_movie_freeze(char** buf, unsigned long* size)
{
	// sanity check
	if (!vcr_is_active())
	{
		return;
	}

	*buf = nullptr;
	*size = 0;

	// compute size needed for the buffer
	unsigned long size_needed = sizeof(m_header.uid) + sizeof(m_current_sample) +
		sizeof(m_current_vi) + sizeof(m_header.length_samples);
	// room for header.uid, currentFrame, and header.length_samples
	size_needed += (unsigned long)(sizeof(BUTTONS) * (m_header.length_samples +
		1));
	*buf = (char*)malloc(size_needed);
	*size = size_needed;

	char* ptr = *buf;
	if (!ptr)
	{
		return;
	}

	*reinterpret_cast<unsigned long*>(ptr) = m_header.uid;
	ptr += sizeof(m_header.uid);
	*reinterpret_cast<unsigned long*>(ptr) = m_current_sample;
	ptr += sizeof(m_current_sample);
	*reinterpret_cast<unsigned long*>(ptr) = m_current_vi;
	ptr += sizeof(m_current_vi);
	*reinterpret_cast<unsigned long*>(ptr) = m_header.length_samples;
	ptr += sizeof(m_header.length_samples);

	memcpy(ptr, m_input_buffer, sizeof(BUTTONS) * (m_header.length_samples + 1));
}

int vcr_movie_unfreeze(const char* buf, const unsigned long size)
{
	// sanity check
	if (vcr_is_idle())
	{
		return -1; // doesn't make sense to do that
	}

	const char* ptr = buf;
	if (size < sizeof(m_header.uid) + sizeof(m_current_sample) + sizeof(
		m_current_vi) + sizeof(m_header.length_samples))
	{
		return WRONG_FORMAT;
	}

	const unsigned long movie_id = *reinterpret_cast<const unsigned long*>(ptr);
	ptr += sizeof(unsigned long);
	const unsigned long current_sample = *reinterpret_cast<const unsigned long*>
		(ptr);
	ptr += sizeof(unsigned long);
	const unsigned long current_vi = *reinterpret_cast<const unsigned long*>(
		ptr);
	ptr += sizeof(unsigned long);
	const unsigned long max_sample = *reinterpret_cast<const unsigned long*>(
		ptr);
	ptr += sizeof(unsigned long);

	const unsigned long space_needed = (sizeof(BUTTONS) * (max_sample + 1));

	if (movie_id != m_header.uid)
		return NOT_FROM_THIS_MOVIE;

	if (current_sample > max_sample)
		return INVALID_FRAME;

	if (space_needed > size)
		return WRONG_FORMAT;

	const e_task last_task = m_task;
	if (!m_read_only)
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentFrame pointer
		//		change_state(MOVIE_STATE_RECORD);
		m_task = e_task::recording;
		flush_movie();

		// update header with new ROM info
		if (last_task == e_task::playback)
			set_rom_info(&m_header);

		enable_emulation_menu_items(TRUE);
		m_current_sample = (long)current_sample;
		m_header.length_samples = current_sample;
		m_current_vi = (int)current_vi;

		m_header.rerecord_count++;

		reserve_buffer_space(space_needed);
		memcpy(m_input_buffer, ptr, space_needed);
		flush_movie();
		fseek(m_file,
		      MUP_HEADER_SIZE_CUR + (sizeof(BUTTONS) * (m_current_sample + 1)),
		      SEEK_SET);
	} else
	{
		// here, we are going to keep the input data from the movie file
		// and simply rewind to the currentFrame pointer
		// this will cause a desync if the savestate is not in sync
		// with the on-disk recording data, but it's easily solved
		// by loading another savestate or playing the movie from the beginning

		// and older savestate might have a currentFrame pointer past
		// the end of the input data, so check for that here
		if (current_sample > m_header.length_samples)
			return INVALID_FRAME;

		m_task = e_task::playback;
		flush_movie();

		enable_emulation_menu_items(TRUE);

		m_current_sample = (long)current_sample;
		m_current_vi = (int)current_vi;
	}

	m_input_buffer_ptr = m_input_buffer + (sizeof(BUTTONS) * m_current_sample);

	///	for(int controller = 0 ; controller < MOVIE_NUM_OF_POSSIBLE_CONTROLLERS ; controller++)
	///		if((m_header.controllerFlags & MOVIE_CONTROLLER(controller)) != 0)
	///			read_frame_controller_data(controller);
	///	read_frame_controller_data(0); // correct if we can assume the first controller is active, which we can on all GBx/xGB systems

	return SUCCESS;
}

extern BOOL just_restarted_flag;

void vcr_on_controller_poll(int index, BUTTONS* input)
{
	// if we aren't playing back a movie, our data source isn't movie
	if (!is_task_playback(m_task))
	{
		getKeys(index, input);
		last_controller_data[index] = *input;
		main_dispatcher_invoke([index] {
			AtInputLuaCallback(index);
		});

		// if lua requested a joypad change, we overwrite the data with lua's changed value for this cycle
		if (overwrite_controller_data[index])
		{
			*input = new_controller_data[index];
			last_controller_data[index] = *input;
			overwrite_controller_data[index] = false;
		}
	}

	if (m_task == e_task::idle)
		return;

	if (m_task == e_task::start_recording)
	{
		if (!continue_vcr_on_restart_mode)
		{
			if (just_restarted_flag)
			{
				just_restarted_flag = FALSE;
				m_current_sample = 0;
				m_current_vi = 0;
				m_task = e_task::recording;
				*input = {0};
				EnableMenuItem(GetMenu(mainHWND), ID_STOP_RECORD, MF_ENABLED);
			} else
			{
				printf("[VCR]: Starting recording...\n");
				hard_reset_and_clear_all_save_data(
					!(m_header.startFlags & MOVIE_START_FROM_EEPROM));
			}
		}
	}

	if (m_task == e_task::start_recording_from_snapshot)
	{
		// TODO: maybe call st generation like normal and remove the "start_x" states
		if (savestates_job == e_st_job::none)
		{
			printf("[VCR]: Starting recording from Snapshot...\n");
			m_task = e_task::recording;
			*input = {0};
		}
	}

	if (m_task == e_task::start_recording_from_existing_snapshot)
	{
		// TODO: maybe call st generation like normal and remove the "start_x" states
		if (savestates_job == e_st_job::none)
		{
			printf("[VCR]: Starting recording from Existing Snapshot...\n");
			m_task = e_task::recording;
			*input = {0};
		}
	}


	if (m_task == e_task::start_playback)
	{
		if (!continue_vcr_on_restart_mode)
		{
			if (just_restarted_flag)
			{
				just_restarted_flag = FALSE;
				m_current_sample = 0;
				m_current_vi = 0;
				m_task = e_task::playback;
			} else
			{
				printf("[VCR]: Starting playback...\n");
				hard_reset_and_clear_all_save_data(
					!(m_header.startFlags & MOVIE_START_FROM_EEPROM));
			}
		}
	}

	if (m_task == e_task::start_playback_from_snapshot)
	{
		// TODO: maybe call st generation like normal and remove the "start_x" states
		if (savestates_job == e_st_job::none)
		{
			if (!savestates_job_success)
			{
				m_task = e_task::idle;
				if (!dont_play)
				{
					// TODO: reset titlebar properly
					char title[MAX_PATH];
					GetWindowText(mainHWND, title, MAX_PATH);
					title[title_length] = '\0'; //remove movie being played part
					SetWindowText(mainHWND, title);
				}
				getKeys(index, input);
				return;
			}
			printf("[VCR]: Starting playback...\n");
			m_task = e_task::playback;
		}
	}

	if (m_task == e_task::recording)
	{
		// TODO: as old comments already state, remove vcr flush mechanism (reasons for it are long gone, at this point it's just huge complexity for no reason)
		reserve_buffer_space(
			(unsigned long)(m_input_buffer_ptr + sizeof(BUTTONS) -
				m_input_buffer));

		extern bool scheduled_restart;
		if (scheduled_restart)
		{
			// reserved 1 and 2 pressed simultaneously = reset flag
			*input = {
				.Reserved1 = 1,
				.Reserved2 = 1,
			};
		}

		*reinterpret_cast<BUTTONS*>(m_input_buffer_ptr) = *input;

		m_input_buffer_ptr += sizeof(BUTTONS);
		m_header.length_samples++;
		m_current_sample++;

		// flush data every 5 seconds or so
		if ((m_header.length_samples % (m_header.num_controllers * 150)) == 0)
		{
			flush_movie();
		}

		if (scheduled_restart)
		{
			resetEmu();
			scheduled_restart = false;
		}
		return;
	}

	// our input source is movie, input plugin is overriden
	if (m_task == e_task::playback)
	{
		// This if previously also checked for if the VI is over the amount specified in the header,
		// but that can cause movies to end playback early on laggy plugins.
		// TODO: only rely on samples for movie termination
		if (m_current_sample >= (long)m_header.length_samples)
		{
			stop_playback(false);
			commandline_on_movie_playback_stop();
			setKeys(index, {0});
			getKeys(index, input);
			return;
		}

		if (m_header.controller_flags & CONTROLLER_X_PRESENT(index))
		{
			*input = *reinterpret_cast<BUTTONS*>(m_input_buffer_ptr);
			m_input_buffer_ptr += sizeof(BUTTONS);
			setKeys(index, *input);

			//no readable code because 120 star tas can't get this right >:(
			if (input->Value == 0xC000)
			{
				continue_vcr_on_restart_mode = true;
				resetEmu();
			}

			last_controller_data[index] = *input;
			main_dispatcher_invoke([index] {
				AtInputLuaCallback(index);
			});

			// if lua requested a joypad change, we overwrite the data with lua's changed value for this cycle
			if (overwrite_controller_data[index])
			{
				*input = new_controller_data[index];
				last_controller_data[index] = *input;
				overwrite_controller_data[index] = false;
			}
			m_current_sample++;
		} else
		{
			// disconnected controls are forced to have no input during playback
			*input = {0};
		}

	}
}


int
vcr_start_record(const char* filename, const unsigned short flags,
                const char* author_utf8, const char* description_utf8, const int def_ext)
{
	vcr_core_stopped();

	char buf[PATH_MAX];

	vcr_recent_movies_add(std::string(filename));

	// m_filename will be overwritten later in the function if
	// MOVIE_START_FROM_EXISTING_SNAPSHOT is true, but it doesn't
	// matter enough to make this a conditional thing
	strncpy(m_filename, filename, PATH_MAX);

	// open record file
	strcpy(buf, m_filename);
	{
		const char* dot = strrchr(buf, '.');
		const char* s1 = strrchr(buf, '\\');
		if (const char* s2 = strrchr(buf, '/'); !dot || ((s1 && s1 > dot) || (s2 && s2 > dot)))
		{
			strncat(buf, ".m64", PATH_MAX);
		}
	}
	m_file = fopen(buf, "wb");
	if (m_file == nullptr)
	{
		fprintf(
			stderr,
			"[VCR]: Cannot start recording, could not open file '%s': %s\n",
			filename, strerror(errno));
		return -1;
	}

	for (auto& [Present, RawData, Plugin] : Controls)
	{
		if (Present && RawData)
		{
			if (MessageBox(
				nullptr,
				"Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".\nThis can cause issues when recording and playing movies. Proceed?",
				"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING) == IDNO) return -
				1;
			break; //
		}
	}

	vcr_set_read_only(FALSE);

	memset(&m_header, 0, MUP_HEADER_SIZE);

	m_header.magic = mup_magic;
	m_header.version = mup_version;
	m_header.uid = (unsigned long)time(nullptr);
	m_header.length_vis = 0;
	m_header.length_samples = 0;

	m_header.rerecord_count = 0;
	m_header.startFlags = flags;

	reserve_buffer_space(4096);

	if (flags & MOVIE_START_FROM_SNAPSHOT)
	{
		// save state
		printf("[VCR]: Saving state...\n");
		strcpy(buf, m_filename);

		// remove extension
		for (;;)
		{
			if (char* dot = strrchr(buf, '.'); dot && (dot > strrchr(buf, '\\') && dot > strrchr(buf, '/')))
				*dot = '\0';
			else
				break;
		}

		if (def_ext)
			strncat(buf, ".st", 4);
		else
			strncat(buf, ".savestate", 12);

		savestates_do(buf, e_st_job::save);
		m_task = e_task::start_recording_from_snapshot;
	} else if (flags & MOVIE_START_FROM_EXISTING_SNAPSHOT)
	{
		printf("[VCR]: Loading state...\n");
		strncpy(m_filename, filename, MAX_PATH);
		strncpy(buf, m_filename, MAX_PATH);
		strip_extension(buf);
		if (def_ext)
			strncat(buf, ".st", 4);
		else
			strncat(buf, ".savestate", 12);

		savestates_do(buf, e_st_job::load);

		// set this to the normal snapshot flag to maintain compatibility
		m_header.startFlags = MOVIE_START_FROM_SNAPSHOT;

		m_task = e_task::start_recording_from_existing_snapshot;
	} else
	{
		m_task = e_task::start_recording;
	}

	movie_path = buf;
	set_rom_info(&m_header);
	update_titlebar();

	// utf8 strings are also null-terminated so this method still works
	if (author_utf8)
		strncpy(m_header.author, author_utf8, MOVIE_AUTHOR_DATA_SIZE);
	m_header.author[MOVIE_AUTHOR_DATA_SIZE - 1] = '\0';
	if (description_utf8)
		strncpy(m_header.description, description_utf8,
		        MOVIE_DESCRIPTION_DATA_SIZE);
	m_header.description[MOVIE_DESCRIPTION_DATA_SIZE - 1] = '\0';

	write_movie_header(m_file, MUP_HEADER_SIZE);

	m_current_sample = 0;
	m_current_vi = 0;

	return 0;
}


int
vcr_stop_record(const int def_ext)
{
	int ret_val = -1;

	if (m_task == e_task::start_recording)
	{
		char buf[PATH_MAX];

		m_task = e_task::idle;
		if (m_file)
		{
			fclose(m_file);
			m_file = nullptr;
		}
		printf("[VCR]: Removing files (nothing recorded)\n");

		strcpy(buf, m_filename);

		if (def_ext)
			strncat(m_filename, ".st", PATH_MAX);
		else
			strncat(m_filename, ".savestate", PATH_MAX);

		if (_unlink(buf) < 0)
			fprintf(stderr, "[VCR]: Couldn't remove save state: %s\n",
			        strerror(errno));

		strcpy(buf, m_filename);
		strncat(m_filename, ".m64", PATH_MAX);
		if (_unlink(buf) < 0)
			fprintf(stderr, "[VCR]: Couldn't remove recorded file: %s\n",
			        strerror(errno));

		ret_val = 0;
	}

	if (m_task == e_task::recording)
	{
		//		long end = -1;

		set_rom_info(&m_header);

		flush_movie();

		m_task = e_task::idle;

		//		fwrite( &end, 1, sizeof (long), m_file );
		//		fwrite( &m_header.length_samples, 1, sizeof (long), m_file );
		fclose(m_file);

		m_file = nullptr;

		truncate_movie();

		printf("[VCR]: Record stopped. Recorded %ld input samples\n",
		       m_header.length_samples);

		enable_emulation_menu_items(TRUE);

		statusbar_post_text("", 1);
		statusbar_post_text("Stopped recording");

		ret_val = 0;
	}

	if (m_input_buffer)
	{
		free(m_input_buffer);
		m_input_buffer = nullptr;
		m_input_buffer_ptr = nullptr;
		m_input_buffer_size = 0;
	}

	movie_path = "";
	update_titlebar();
	return ret_val;
}

bool get_savestate_path(const char* filename, char* out_buffer)
{
	bool found = true;

	const auto filename_with_extension = (char*)malloc(strlen(filename) + 11);
	if (!filename_with_extension)
		return false;

	strcpy(filename_with_extension, filename);
	strncat(filename_with_extension, ".st", 4);

	if (std::filesystem::path st_path = filename_with_extension; std::filesystem::exists(st_path))
		strcpy(out_buffer, filename_with_extension);
	else
	{
		/* try .savestate (old extension created bc of discord
		trying to display a preview of .st data when uploaded) */
		strcpy(filename_with_extension, filename);
		strncat(filename_with_extension, ".savestate", 11);
		st_path = filename_with_extension;

		if (std::filesystem::exists(st_path))
			strcpy(out_buffer, filename_with_extension);
		else
			found = false;
	}

	free(filename_with_extension);
	return found;
}

int
vcr_start_playback(const std::string &filename, const char* author_utf8,
                  const char* description_utf8)
{
	vcr_recent_movies_add(filename);
	return start_playback(filename.c_str(), author_utf8, description_utf8, false);
}

static int start_playback(const char* filename, const char* author_utf8,
              const char* description_utf8, const bool restarting)
{
	vcr_core_stopped();
	is_restarting_flag = false;

	extern HWND mainHWND;
	char buf[PATH_MAX];

	strncpy(m_filename, filename, PATH_MAX);
	printf("m_filename = %s\n", m_filename);
	char* p = strrchr(m_filename, '.');
	// gets a string slice from the final "." to the end

	if (p)
	{
		if (!STRCASECMP(p, ".m64") || !STRCASECMP(p, ".st"))
			*p = '\0';
	}
	// open record file
	strcpy(buf, m_filename);
	m_file = fopen(buf, "rb+");
	if (m_file == nullptr && (m_file = fopen(buf, "rb")) == nullptr)
	{
		strncat(buf, ".m64", PATH_MAX);
		m_file = fopen(buf, "rb+");
		if (m_file == nullptr && (m_file = fopen(buf, "rb")) == nullptr)
		{
			fprintf(
				stderr,
				"[VCR]: Cannot start playback, could not open .m64 file '%s': %s\n",
				filename, strerror(errno));
			if (m_file != nullptr)
				fclose(m_file);
			return VCR_PLAYBACK_FILE_BUSY;
		}
	}

	// can crash when looping + fast forward, no need to change this
	// this creates a bug, so i changed it -auru
	{
		const int code = read_movie_header(m_file, &m_header);

		switch (code)
		{
		case SUCCESS:
			{
			char warning_str[8092]{};
				warning_str[0] = '\0';

				dont_play = FALSE;

				for (auto& [Present, RawData, Plugin] : Controls)
				{
					if (Present && RawData)
					{
						if (MessageBox(
								nullptr,
								"Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".\nThis can cause issues when recording and playing movies. Proceed?",
								"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING)
							==
							IDNO) dont_play = TRUE;
						break; //
					}
				}
				for (int i = 0; i < 4; ++i)
				{
					if (!Controls[i].Present && (m_header.controller_flags &
						CONTROLLER_X_PRESENT(i))) {
						sprintf(warning_str,
							   "Error: You have controller %d disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you fix this first (in your input settings).\n", (i+1));
						dont_play = TRUE;
					}
					if (Controls[i].Present && !(m_header.controller_flags &
						CONTROLLER_X_PRESENT(i)))
						sprintf(warning_str,
							   "Warning: You have controller %d enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you change this first (in your input settings).\n", (i+1));
					else {
						if (Controls[i].Present && (Controls[i].Plugin !=
							controller_extension::mempak) && (m_header.
								controller_flags & CONTROLLER_X_MEMPAK(i)))
							sprintf(warning_str,
								   "Warning: Controller %d has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i+1));
						if (Controls[i].Present && (Controls[i].Plugin !=
							controller_extension::rumblepak) && (m_header.
								controller_flags & CONTROLLER_X_RUMBLE(i)))
							sprintf(warning_str,
								   "Warning: Controller %d has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i+1));
						if (Controls[i].Present && (Controls[i].Plugin !=
							controller_extension::none) && !(m_header.
								controller_flags & (CONTROLLER_X_MEMPAK(i) |
									CONTROLLER_X_RUMBLE(i))))
							sprintf(warning_str,
								   "Warning: Controller %d does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i+1));
					}
				}
				char str[512], name[512];

				if (_stricmp(m_header.rom_name,
				                           (const char*)ROM_HEADER.nom) != 0)
				{
					sprintf(
						str,
						"The movie was recorded with the ROM \"%s\",\nbut you are using the ROM \"%s\",\nso the movie probably won't play properly.\n",
						m_header.rom_name, (char*)ROM_HEADER.nom);
					strcat(warning_str, str);
					dont_play = Config.is_rom_movie_compatibility_check_enabled
						           ? dont_play
						           : TRUE;
				} else
				{
					if (m_header.rom_country != ROM_HEADER.
						Country_code)
					{
						sprintf(
							str,
							"The movie was recorded with a ROM with country code \"%d\",\nbut you are using a ROM with country code \"%d\",\nso the movie may not play properly.\n",
							m_header.rom_country, ROM_HEADER.Country_code);
						strcat(warning_str, str);
						dont_play =
							Config.is_rom_movie_compatibility_check_enabled
								? dont_play
								: TRUE;
					} else if (m_header.rom_crc1 != ROM_HEADER.
						CRC1)
					{
						sprintf(
							str,
							"The movie was recorded with a ROM that has CRC \"0x%X\",\nbut you are using a ROM with CRC \"0x%X\",\nso the movie may not play properly.\n",
							(unsigned int)m_header.rom_crc1,
							(unsigned int)ROM_HEADER.CRC1);
						strcat(warning_str, str);
						dont_play =
							Config.is_rom_movie_compatibility_check_enabled
								? dont_play
								: TRUE;
					}
				}

				if (warning_str[0] != '\0')
				{
					if (dont_play)
						print_error(warning_str);
					else
						print_warning(warning_str);
				}

				strncpy(name, input_plugin->name.c_str(), 64);
				if (name[0] && m_header.input_plugin_name[0] && _stricmp(
					m_header.input_plugin_name, name) != 0)
				{
					printf(
						"Warning: The movie was recorded with the input plugin \"%s\",\nbut you are using the input plugin \"%s\",\nso the movie may not play properly.\n",
						m_header.input_plugin_name, name);
				}
				strncpy(name, video_plugin->name.c_str(), 64);
				if (name[0] && m_header.video_plugin_name[0] && _stricmp(
					m_header.video_plugin_name, name) != 0)
				{
					printf(
						"Warning: The movie was recorded with the graphics plugin \"%s\",\nbut you are using the graphics plugin \"%s\",\nso the movie might not play properly.\n",
						m_header.video_plugin_name, name);
				}
				strncpy(name, audio_plugin->name.c_str(), 64);
				if (name[0] && m_header.audio_plugin_name[0] && _stricmp(
					m_header.audio_plugin_name, name) != 0)
				{
					printf(
						"Warning: The movie was recorded with the sound plugin \"%s\",\nbut you are using the sound plugin \"%s\",\nso the movie might not play properly.\n",
						m_header.audio_plugin_name, name);
				}
				strncpy(name, rsp_plugin->name.c_str(), 64);
				if (name[0] && m_header.rsp_plugin_name[0] && _stricmp(
					m_header.rsp_plugin_name, name) != 0)
				{
					printf(
						"Warning: The movie was recorded with the RSP plugin \"%s\",\nbut you are using the RSP plugin \"%s\",\nso the movie probably won't play properly.\n",
						m_header.rsp_plugin_name, name);
				}


				if (dont_play)
				{
					if (m_file != nullptr)
						fclose(m_file);
					return VCR_PLAYBACK_INCOMPATIBLE;
				}

				// recalculate length of movie from the file size
				//				fseek(m_file, 0, SEEK_END);
				//				int fileSize = ftell(m_file);
				//				m_header.length_samples = (fileSize - MUP_HEADER_SIZE) / sizeof(BUTTONS) - 1;
				if (m_file == nullptr) return 0;
				fseek(m_file, MUP_HEADER_SIZE_CUR, SEEK_SET);

				// read controller data
				m_input_buffer_ptr = m_input_buffer;
				const unsigned long to_read = sizeof(BUTTONS) * (m_header.
					length_samples + 1);
				reserve_buffer_space(to_read);
				fread(m_input_buffer_ptr, 1, to_read, m_file);

				fseek(m_file, 0, SEEK_END);
				char buffer[50];
				sprintf(buffer, "%lu rr", m_header.rerecord_count);

				statusbar_post_text(std::string(buffer), 1);
			}
			break;
		default:
			char buffer[100];
			sprintf(buffer, "[VCR]: Error playing movie: %s.\n",
			        m_err_code_name[code]);
			print_error(buffer);
			dont_play = code != 0; // should be stable enough
			break;
		}

		m_current_sample = 0;
		m_current_vi = 0;
		strcpy(vcr_lastpath, filename);
		movie_path = buf;
		update_titlebar();

		if (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
		{
			// load state
			printf("[VCR]: Loading state...\n");
			strcpy(buf, m_filename);

			const auto untruncated_name = (char*)malloc(strlen(buf)+1);
			strcpy(untruncated_name, buf);
			// remove everything after the first `.` (dot)
			for (;;)
			{
				if (char* dot = strrchr(buf, '.'); dot && (dot > strrchr(buf, '\\') && dot >
					strrchr(buf, '/')))
					*dot = '\0';
				else
					break;
			}
			if (!get_savestate_path(buf, buf) && !get_savestate_path(
				untruncated_name, buf))
			{
				printf(
					"[VCR]: Precautionary movie respective savestate exist check failed. No .savestate or .st found for movie!\n");
				if (m_file != nullptr)
					fclose(m_file);
				return VCR_PLAYBACK_SAVESTATE_MISSING;
			}

			savestates_do(buf, e_st_job::load);
			m_task = e_task::start_playback_from_snapshot;
		} else
		{
			m_task = e_task::start_playback;
		}

		// utf8 strings are also null-terminated so this method still works
		if (author_utf8)
			strncpy(m_header.author, author_utf8, MOVIE_AUTHOR_DATA_SIZE);
		m_header.author[MOVIE_AUTHOR_DATA_SIZE - 1] = '\0';
		if (description_utf8)
			strncpy(m_header.description, description_utf8,
			        MOVIE_DESCRIPTION_DATA_SIZE);
		m_header.description[MOVIE_DESCRIPTION_DATA_SIZE - 1] = '\0';
		main_dispatcher_invoke(AtPlayMovieLuaCallback);
		return code;
	}
}

int restart_playback()
{
	is_restarting_flag = true;
	return vcr_start_playback(vcr_lastpath, nullptr, nullptr);
}

int vcr_stop_playback()
{
	return stop_playback(true);
}

static int stop_playback(const bool bypass_loop_setting)
{
	if (!bypass_loop_setting && vcr_is_looping())
	{
		return restart_playback();
	}

	movie_path = "";
	update_titlebar();

	if (m_file && m_task != e_task::start_recording && m_task != e_task::recording)
	{
		fclose(m_file);
		m_file = nullptr;
	}

	if (m_task == e_task::start_playback)
	{
		m_task = e_task::idle;
		return 0;
	}

	if (m_task == e_task::playback)
	{
		m_task = e_task::idle;
		printf("[VCR]: Playback stopped (%ld samples played)\n",
		       m_current_sample);

		extern void enable_emulation_menu_items(BOOL flag);
		enable_emulation_menu_items(TRUE);

		statusbar_post_text("", 1);
		statusbar_post_text("Stopped playback");

		if (m_input_buffer)
		{
			free(m_input_buffer);
			m_input_buffer = nullptr;
			m_input_buffer_ptr = nullptr;
			m_input_buffer_size = 0;
		}

		main_dispatcher_invoke(AtStopMovieLuaCallback);
		return 0;
	}
	return -1;
}


void vcr_update_screen()
{
	if (!vcr_is_capturing())
	{
		if (!is_frame_skipped()) {
			updateScreen();
		}

		// we always want to invoke atvi, regardless of ff optimization
		if (!hwnd_lua_map.empty())
		{
			main_dispatcher_invoke(AtVILuaCallback);
		}
		return;
	}

	// capturing, update screen and call readscreen, call avi/ffmpeg stuff
	updateScreen();
	if (!hwnd_lua_map.empty())
	{
		main_dispatcher_invoke(AtVILuaCallback);
	}
	void* image = nullptr;
	long width = 0, height = 0;

	const auto start = std::chrono::high_resolution_clock::now();
	readScreen(&image, &width, &height);
	const auto end = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double, std::milli> time = (end - start);
	printf("ReadScreen (ffmpeg): %lf ms\n", time.count());

	if (image == nullptr)
	{
		printf("[VCR]: Couldn't read screen (out of memory?)\n");
		return;
	}

	if (capture_with_f_fmpeg)
	{
		capture_manager->WriteVideoFrame((unsigned char*)image,
		                                width * height * 3);

		//free only with external capture, since plugin can't reuse same buffer...
		if (readScreen != vcrcomp_internal_read_screen)
			DllCrtFree(image);

		return;
	} else
	{
		if (VCRComp_GetSize() > max_avi_size)
		{
			static char* endptr;
			VCRComp_finishFile(1);
			if (!avi_increment)
				endptr = avi_file_name + strlen(avi_file_name) - 4;
			//AVIIncrement
			sprintf(endptr, "%d.avi", ++avi_increment);
			VCRComp_startFile(avi_file_name, width, height, vis_by_countrycode(),
			                  0);
		}
	}

	if (Config.synchronization_mode == VCR_SYNC_AUDIO_DUPL || Config.
		synchronization_mode == VCR_SYNC_NONE)
	{
		// AUDIO SYNC
		// This type of syncing assumes the audio is authoratative, and drops or duplicates frames to keep the video as close to
		// it as possible. Some games stop updating the screen entirely at certain points, such as loading zones, which will cause
		// audio to drift away by default. This method of syncing prevents this, at the cost of the video feed possibly freezing or jumping
		// (though in practice this rarely happens - usually a loading scene just appears shorter or something).

		int audio_frames = (int)(m_audio_frame - m_video_frame + 0.1);
		// i've seen a few games only do ~0.98 frames of audio for a frame, let's account for that here

		if (Config.synchronization_mode == VCR_SYNC_AUDIO_DUPL)
		{
			if (audio_frames < 0)
			{
				print_error("Audio frames became negative!");
				vcr_stop_capture();
				goto cleanup;
			}

			if (audio_frames == 0)
			{
				printf("\nDropped Frame! a/v: %Lg/%Lg", m_video_frame,
				       m_audio_frame);
			} else if (audio_frames > 0)
			{
				if (!VCRComp_addVideoFrame((unsigned char*)image))
				{
					print_error(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
					vcr_stop_capture();
					goto cleanup;
				} else
				{
					m_video_frame += 1.0;
					audio_frames--;
				}
			}

			// can this actually happen?
			while (audio_frames > 0)
			{
				if (!VCRComp_addVideoFrame((unsigned char*)image))
				{
					print_error(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
					vcr_stop_capture();
					goto cleanup;
				} else
				{
					printf("\nDuped Frame! a/v: %Lg/%Lg", m_video_frame,
					       m_audio_frame);
					m_video_frame += 1.0;
					audio_frames--;
				}
			}
		} else /*if (Config.synchronization_mode == VCR_SYNC_NONE)*/
		{
			if (!VCRComp_addVideoFrame((unsigned char*)image))
			{
				print_error(
					"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
				vcr_stop_capture();
				goto cleanup;
			} else
			{
				m_video_frame += 1.0;
			}
		}
	}

cleanup:
	if (readScreen != vcrcomp_internal_read_screen)
	{
		if (image)
			DllCrtFree(image);
	}
}


void
vcr_ai_dacrate_changed(const system_type type)
{
	if (vcr_is_capturing())
	{
		printf("Fatal error, audio frequency changed during capture\n");
		vcr_stop_capture();
		return;
	}
	aiDacrateChanged(type);

	m_audio_bitrate = (int)ai_register.ai_bitrate + 1;
	switch (type)
	{
	case ntsc:
		m_audio_freq = (int)(48681812 / (ai_register.ai_dacrate + 1));
		break;
	case pal:
		m_audio_freq = (int)(49656530 / (ai_register.ai_dacrate + 1));
		break;
	case mpal:
		m_audio_freq = (int)(48628316 / (ai_register.ai_dacrate + 1));
		break;
	default:
		assert(false);
		break;
	}
}

// assumes: len <= writeSize
static void write_sound(char* buf, int len, const int min_write_size, const int max_write_size,
                        const BOOL force)
{
	if ((len <= 0 && !force) || len > max_write_size)
		return;

	if (sound_buf_pos + len > min_write_size || force)
	{
		if (int len2 = vcr_get_resample_len(44100, m_audio_freq, m_audio_bitrate,
		                                    sound_buf_pos); (len2 % 8) == 0 || len > max_write_size)
		{
			static short* buf2 = nullptr;
			len2 = vcr_resample(&buf2, 44100,
			                    reinterpret_cast<short*>(sound_buf), m_audio_freq,
			                    m_audio_bitrate, sound_buf_pos);
			if (len2 > 0)
			{
				if ((len2 % 4) != 0)
				{
					printf(
						"[VCR]: Warning: Possible stereo sound error detected.\n");
					fprintf(
						stderr,
						"[VCR]: Warning: Possible stereo sound error detected.\n");
				}
				if (!VCRComp_addAudioData((unsigned char*)buf2, len2))
				{
					print_error(
						"Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?");
					vcr_stop_capture();
				}
			}
			sound_buf_pos = 0;
		}
	}

	if (len > 0)
	{
		if ((unsigned int)(sound_buf_pos + len) > SOUND_BUF_SIZE * sizeof(char))
		{
			MessageBox(nullptr, "Fatal error", "Sound buffer overflow", MB_ICONERROR);
			printf("SOUND BUFFER OVERFLOW\n");
			return;
		}
#ifdef _DEBUG
		else
		{
			long double pro = (long double)(sound_buf_pos + len) * 100 / (
				SOUND_BUF_SIZE * sizeof(char));
			if (pro > 75) printf("---!!!---");
			printf("sound buffer: %.2f%%\n", pro);
		}
#endif
		memcpy(sound_buf + sound_buf_pos, (char*)buf, len);
		sound_buf_pos += len;
		m_audio_frame += ((len / 4) / (long double)m_audio_freq) *
			vis_by_countrycode();
	}
}

// calculates how long the audio data will last
float get_percent_of_frame(const int ai_len, const int audio_freq, const int audio_bitrate)
{
	const int limit = vis_by_countrycode();
	const float vi_len = 1.f / (float)limit; //how much seconds one VI lasts
	const float time = (float)(ai_len * 8) / ((float)audio_freq * 2.f * (float)
		audio_bitrate); //how long the buffer can play for
	return time / vi_len; //ratio
}

void vcr_ai_len_changed()
{
	const auto p = reinterpret_cast<short*>((char*)rdram + (ai_register.ai_dram_addr
		& 0xFFFFFF));
	const auto buf = (char*)p;
	const int ai_len = (int)ai_register.ai_len;
	aiLenChanged();

	// hack - mupen64 updates bitrate after calling aiDacrateChanged
	m_audio_bitrate = (int)ai_register.ai_bitrate + 1;

	if (m_capture == 0)
		return;

	if (capture_with_f_fmpeg)
	{
		capture_manager->WriteAudioFrame(buf, ai_len);
		return;
	}

	if (ai_len > 0)
	{
		const int len = ai_len;
		const int write_size = 2 * m_audio_freq;
		// we want (writeSize * 44100 / m_audioFreq) to be an integer

		/*
				// TEMP PARANOIA
				if(len > writeSize)
				{
					char str [256];
					printf("Sound AVI Output Failure: %d > %d", len, writeSize);
					fprintf(stderr, "Sound AVI Output Failure: %d > %d", len, writeSize);
					sprintf(str, "Sound AVI Output Failure: %d > %d", len, writeSize);
					printError(str);
					printWarning(str);
					exit(0);
				}
		*/

		if (Config.synchronization_mode == VCR_SYNC_VIDEO_SNDROP || Config.
			synchronization_mode == VCR_SYNC_NONE)
		{
			// VIDEO SYNC
			// This is the original syncing code, which adds silence to the audio track to get it to line up with video.
			// The N64 appears to have the ability to arbitrarily disable its sound processing facilities and no audio samples
			// are generated. When this happens, the video track will drift away from the audio. This can happen at load boundaries
			// in some games, for example.
			//
			// The only new difference here is that the desync flag is checked for being greater than 1.0 instead of 0.
			// This is because the audio and video in mupen tend to always be diverged just a little bit, but stay in sync
			// over time. Checking if desync is not 0 causes the audio stream to to get thrashed which results in clicks
			// and pops.

			long double desync = m_video_frame - m_audio_frame;

			if (Config.synchronization_mode == VCR_SYNC_NONE) // HACK
				desync = 0;

			if (desync > 1.0)
			{
				printf("[VCR]: Correcting for A/V desynchronization of %+Lf frames\n",desync);
				int len3 = (int)(m_audio_freq / (long double)vis_by_countrycode()) * (int)desync;
				len3 <<= 2;
				const int empty_size = len3 > write_size ? write_size : len3;

				for (int i = 0; i < empty_size; i += 4)
					*reinterpret_cast<long*>(sound_buf_empty + i) = last_sound;

				while (len3 > write_size)
				{
					write_sound(sound_buf_empty, write_size, m_audio_freq, write_size,
					           FALSE);
					len3 -= write_size;
				}
				write_sound(sound_buf_empty, len3, m_audio_freq, write_size, FALSE);
			} else if (desync <= -10.0)
			{
				printf(
					"[VCR]: Waiting from A/V desynchronization of %+Lf frames\n",
					desync);
			}
		}

		write_sound(buf, len, m_audio_freq, write_size, FALSE);

		last_sound = *(reinterpret_cast<long*>(buf + len) - 1);
	}
}

void update_title_bar_capture(const char* filename)
{
	// Setting titlebar to include currently capturing AVI file
	char title[PATH_MAX];
	char avi[PATH_MAX];
	char ext[PATH_MAX];
	strncpy(avi, avi_file_name, PATH_MAX);
	_splitpath(avi, nullptr, nullptr, avi, ext);

	if (vcr_is_playing())
	{
		char m64[PATH_MAX];
		strncpy(m64, m_filename, PATH_MAX);
		_splitpath(m64, nullptr, nullptr, m64, nullptr);
		sprintf(title, MUPEN_VERSION " - %s | %s.m64 | %s%s",
		        (char*)ROM_HEADER.nom, m64, avi, ext);
	} else
	{
		sprintf(title, MUPEN_VERSION " - %s | %s%s", (char*)ROM_HEADER.nom,
		        avi, ext);
	}
	printf("title %s\n", title);
	SetWindowText(mainHWND, title);
}

bool vcr_start_capture(const char* path, const bool show_codec_dialog)
{
	extern BOOL emu_paused;
	const BOOL was_paused = emu_paused;
	if (!emu_paused)
	{
		pauseEmu(TRUE);
	}

	if (readScreen == nullptr)
	{
		printf("readScreen not implemented by graphics plugin. Falling back...\n");
		readScreen = vcrcomp_internal_read_screen;
	}

	memset(sound_buf_empty, 0, std::size(sound_buf_empty));
	memset(sound_buf, 0, std::size(sound_buf));
	last_sound = 0;
	m_video_frame = 0.0;
	m_audio_frame = 0.0;

	// fill in window size at avi start, which can't change
	// scrap whatever was written there even if window didnt change, for safety
	vcrcomp_window_info = {0};
	get_window_info(mainHWND, vcrcomp_window_info);
	const long width = vcrcomp_window_info.width & ~3;
	const long height = vcrcomp_window_info.height & ~3;

	VCRComp_startFile(path, width, height, vis_by_countrycode(), show_codec_dialog);
	m_capture = 1;
	capture_with_f_fmpeg = false;
	strncpy(avi_file_name, path, PATH_MAX);
	Config.avi_capture_path = path;


	// toolbar could get captured in AVI, so we disable it
	toolbar_set_visibility(0);
	update_title_bar_capture(avi_file_name);
	enable_emulation_menu_items(TRUE);
	SetWindowLong(mainHWND, GWL_STYLE, GetWindowLong(mainHWND, GWL_STYLE) & ~WS_MINIMIZEBOX);
	// we apply WS_EX_LAYERED to fix off-screen blitting (off-screen window portions are not included otherwise)
	SetWindowLong(mainHWND, GWL_EXSTYLE, GetWindowLong(mainHWND, GWL_EXSTYLE) | WS_EX_LAYERED);

	if (!was_paused || (m_task == e_task::playback || m_task == e_task::start_playback || m_task
		== e_task::start_playback_from_snapshot))
	{
		resumeEmu(TRUE);
	}

	printf("[VCR]: Starting capture...\n");

	return true;
}

/// <summary>
/// Start capture using ffmpeg, if this function fails, manager (process and pipes) isn't created and error is returned.
/// </summary>
/// <param name="output_name">name of video file output</param>
/// <param name="arguments">additional ffmpeg params (output stream only)</param>
/// <returns></returns>
int vcr_start_f_fmpeg_capture(const std::string& output_name,
                           const std::string& arguments)
{
	if (!emu_launched) return INIT_EMU_NOT_LAUNCHED;
	if (capture_manager != nullptr)
	{
#ifdef _DEBUG
		printf(
			"[VCR] Attempted to start ffmpeg capture when it was already started\n");
#endif
		return INIT_ALREADY_RUNNING;
	}
	t_window_info s_info{};
	get_window_info(mainHWND, s_info);

	InitReadScreenFFmpeg(s_info);
	capture_manager = std::make_unique<FFmpegManager>(
		s_info.width, s_info.height, vis_by_countrycode(), m_audio_freq,
		arguments + " " + output_name);

	auto err = capture_manager->initError;
	if (err != INIT_SUCCESS)
		capture_manager.reset();
	else
	{
		update_title_bar_capture(output_name.data());
		m_capture = 1;
		capture_with_f_fmpeg = 1;
	}
#ifdef _DEBUG
	if (err == INIT_SUCCESS)
		printf("[VCR] ffmpeg capture started\n");
	else
		printf("[VCR] Could not start ffmpeg capture\n");
#endif
	return err;
}

void vcr_stop_f_fmpeg_capture()
{
	if (capture_manager == nullptr) return; // no error code but its no big deal
	m_capture = 0;
	//this must be first in case object is being destroyed and other thread still sees m_capture=1
	capture_manager.reset();
	//apparently closing the pipes is enough to tell ffmpeg the movie ended.
#ifdef _DEBUG
	printf("[VCR] ffmpeg capture stopped\n");
#endif
}

void
vcr_toggle_read_only()
{
	if (m_task == e_task::recording)
	{
		flush_movie();
	}
	vcr_set_read_only(!m_read_only);

	statusbar_post_text(m_read_only ? "Read" : "Read-write");
}

int vcr_stop_capture()
{
	if (capture_with_f_fmpeg)
	{
		vcr_stop_f_fmpeg_capture();
		return 0;
	}

	m_capture = 0;
	m_vis_per_second = -1;
	write_sound(nullptr, 0, m_audio_freq, m_audio_freq * 2, TRUE);

	// re-enable the toolbar (m_capture==0 causes this call to do that)
	// check previous update_toolbar_visibility call
	toolbar_set_visibility(Config.is_toolbar_enabled);

	SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	if (vcr_is_playing())
	{
		char title[PATH_MAX];
		char m64[PATH_MAX];
		strncpy(m64, m_filename, PATH_MAX);
		_splitpath(m64, nullptr, nullptr, m64, nullptr);
		sprintf(title, MUPEN_VERSION " - %s | %s.m64", (char*)ROM_HEADER.nom,
		        m64);
		SetWindowText(mainHWND, title);
	} else
	{
		update_titlebar();
	}
	SetWindowLong(mainHWND, GWL_STYLE, GetWindowLong(mainHWND, GWL_STYLE) | WS_MINIMIZEBOX);
	// we remove WS_EX_LAYERED again, because dwm sucks at dealing with layered top-level windows
	SetWindowLong(mainHWND, GWL_EXSTYLE, GetWindowLong(mainHWND, GWL_EXSTYLE) & ~WS_EX_LAYERED);

	VCRComp_finishFile(0);
	avi_increment = 0;
	printf("[VCR]: Capture finished.\n");
	return 0;
}


void
vcr_core_stopped()
{
	extern BOOL continue_vcr_on_restart_mode;
	if (continue_vcr_on_restart_mode)
		return;

	switch (m_task)
	{
	case e_task::start_recording:
	case e_task::start_recording_from_snapshot:
	case e_task::recording:
		vcr_stop_record(1);
		break;
	case e_task::start_playback:
	case e_task::start_playback_from_snapshot:
	case e_task::playback:
		vcr_stop_playback();
		break;
	}

	if (m_capture != 0)
		vcr_stop_capture();
}

void vcr_update_statusbar()
{
	BUTTONS b = last_controller_data[0];
	std::string input_string = std::format("({}, {}) ", (int)b.Y_AXIS, (int)b.X_AXIS);
	if (b.START_BUTTON) input_string += "S";
	if (b.Z_TRIG) input_string += "Z";
	if (b.A_BUTTON) input_string += "A";
	if (b.B_BUTTON) input_string += "B";
	if (b.L_TRIG) input_string += "L";
	if (b.R_TRIG) input_string += "R";
	if (b.U_CBUTTON || b.D_CBUTTON || b.L_CBUTTON ||
		b.R_CBUTTON)
	{
		input_string += " C";
		if (b.U_CBUTTON) input_string += "^";
		if (b.D_CBUTTON) input_string += "v";
		if (b.L_CBUTTON) input_string += "<";
		if (b.R_CBUTTON) input_string += ">";
	}
	if (b.U_DPAD || b.D_DPAD || b.L_DPAD || b.
		R_DPAD)
	{
		input_string += " D";
		if (b.U_DPAD) input_string += "^";
		if (b.D_DPAD) input_string += "v";
		if (b.L_DPAD) input_string += "<";
		if (b.R_DPAD) input_string += ">";
	}

	if (vcr_is_recording())
	{
		std::string text = std::format("{} ({}) ", m_current_vi, m_current_sample);
		statusbar_post_text(text + input_string);
		statusbar_post_text(std::format("{} rr", m_header.rerecord_count), 1);
	}

	if (vcr_is_playing())
	{
		std::string text = std::format("{} / {} ({} / {}) ", m_current_vi, vcr_get_length_v_is(), m_current_sample, vcr_get_length_samples());
		statusbar_post_text(text + input_string);
	}

	if (!vcr_is_active())
	{
		statusbar_post_text(input_string);
	}
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////// Recent Movies //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void vcr_recent_movies_build(const int32_t reset)
{
	const HMENU h_menu = GetMenu(mainHWND);

	for (size_t i = 0; i < Config.recent_movie_paths.size(); i++)
	{
		if (Config.recent_movie_paths[i].empty())
		{
			continue;
		}
		DeleteMenu(h_menu, ID_RECENTMOVIES_FIRST + i, MF_BYCOMMAND);
	}

	if (reset)
	{
		Config.recent_movie_paths.clear();
	}

	HMENU h_sub_menu = GetSubMenu(h_menu, 3);
	h_sub_menu = GetSubMenu(h_sub_menu, 6);

	MENUITEMINFO menu_info = {0};
	menu_info.cbSize = sizeof(MENUITEMINFO);
	menu_info.fMask = MIIM_TYPE | MIIM_ID;
	menu_info.fType = MFT_STRING;
	menu_info.fState = MFS_ENABLED;

	for (size_t i = 0; i < Config.recent_movie_paths.size(); i++)
	{
		if (Config.recent_movie_paths[i].empty())
		{
			continue;
		}
		menu_info.dwTypeData = const_cast<LPSTR>(Config.recent_movie_paths[i].c_str());
		menu_info.cch = strlen(menu_info.dwTypeData);
		menu_info.wID = ID_RECENTMOVIES_FIRST + i;
		InsertMenuItem(h_sub_menu, i + 3, TRUE, &menu_info);
	}
}

void vcr_recent_movies_add(const std::string path) // TODO: change to const reference
{
	if (Config.is_recent_movie_paths_frozen)
	{
		return;
	}
	if (Config.recent_movie_paths.size() > 5)
	{
		Config.recent_movie_paths.pop_back();
	}
	std::erase(Config.recent_movie_paths, path);
	Config.recent_movie_paths.insert(Config.recent_movie_paths.begin(), path);
	vcr_recent_movies_build();
}

int32_t vcr_recent_movies_play(const uint16_t menu_item_id)
{
	if (const int index = menu_item_id - ID_RECENTMOVIES_FIRST; index >= 0 && index < Config.recent_movie_paths.size())
	{
		vcr_set_read_only(TRUE);
		return vcr_start_playback(Config.recent_movie_paths[index], "", "");
	}
	return 0;
}
