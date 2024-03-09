//#include "../config.h"
#include <cassert>

#include "win/main_win.h"
#include "win/features/Statusbar.hpp"
#include "../r4300/r4300.h"
#include "../lua/LuaConsole.h"

#include "vcr.h"
#include "vcr_compress.h"
#include "vcr_resample.h"

//ffmpeg
#include "ffmpeg_capture/ffmpeg_capture.hpp"
#include <memory>

#include "Plugin.hpp"
#include "../r4300/rom.h"
#include "savestates.h"
#include "../memory/memory.h"

#include <filesystem>
#include <cerrno>
#include <malloc.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <Windows.h>
#include "win/Config.hpp"
#include "guifuncs.h"
#include "LuaCallbacks.h"
#include "messenger.h"
#include "../memory/pif.h"
#include "win/features/MGECompositor.h"
#include "win/features/RomBrowser.hpp"

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

// The frame to seek to during playback, or an empty option if no seek is being performed
std::optional<size_t> seek_to_frame;

static char m_filename[MAX_PATH];
static char avi_file_name[MAX_PATH];
static FILE* m_file = nullptr;
static t_movie_header m_header;

long m_current_sample = -1;
// should = length_samples when recording, and be < length_samples when playing
int m_current_vi = -1;
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

bool capture_with_f_fmpeg = true;
std::unique_ptr<FFmpegManager> capture_manager;
uint64_t screen_updates = 0;

// Used for tracking VCR-invoked resets
bool just_reset;
// Used for tracking user-invoked resets
bool user_requested_reset;

std::recursive_mutex vcr_mutex;

bool is_task_playback(const e_task task)
{
	return task == e_task::start_playback || task == e_task::start_playback_from_snapshot || task == e_task::playback;
}

static void write_movie_header(FILE* file)
{
	//	assert(ftell(file) == 0); // we assume file points to beginning of movie file
	fseek(file, 0L, SEEK_SET);

	m_header.version = mup_version; // make sure to update the version!
	///	m_header.length_vis = m_header.length_samples / m_header.num_controllers; // wrong

	fwrite(&m_header, 1, MUP_HEADER_SIZE, file);

	fseek(file, 0L, SEEK_END);
}

static void set_rom_info(t_movie_header* header)
{
	header->vis_per_second = get_vis_per_second(ROM_HEADER.Country_code);
	header->controller_flags = 0;
	header->num_controllers = 0;

	for (int i = 0; i < 4; ++i)
	{
		if (Controls[i].Plugin == controller_extension::mempak)
		{
			header->controller_flags |= CONTROLLER_X_MEMPAK(i);
		}

		if (Controls[i].Plugin == controller_extension::rumblepak)
		{
			header->controller_flags |= CONTROLLER_X_RUMBLE(i);
		}

		if (!Controls[i].Present)
			continue;

		header->controller_flags |= CONTROLLER_X_PRESENT(i);
		header->num_controllers++;
	}

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

static int read_movie_header(std::vector<uint8_t> buf, t_movie_header* header)
{
	if (buf.size() < mup_header_size_old)
		return WRONG_FORMAT;

	t_movie_header new_header = {};
	memcpy(&new_header, buf.data(), mup_header_size_old);

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
	if (new_header.version == 3 && buf.size() < MUP_HEADER_SIZE)
	{
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
		write_movie_header(m_file);

		// (over-)write the controller data
		fseek(m_file, MUP_HEADER_SIZE, SEEK_SET);
		fwrite(m_input_buffer, 1, sizeof(BUTTONS) * (m_header.length_samples),
		       m_file);

		fflush(m_file);
	}
}

VCR::Result VCR::parse_header(std::filesystem::path path, t_movie_header* header)
{
	if (path.extension() != ".m64")
	{
		return Result::InvalidFormat;
	}

	t_movie_header new_header = {};
	new_header.rom_country = -1;
	strcpy(new_header.rom_name, "(no ROM)");

	auto buf = read_file_buffer(path);
	if (buf.empty())
	{
		return Result::BadFile;
	}

	if(read_movie_header(buf, &new_header) != SUCCESS)
	{
		return Result::InvalidFormat;
	}

	*header = new_header;
	return Result::Ok;
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

// Returns the filename of the last-played movie
const char* vcr_get_movie_filename()
{
	return m_filename;
}

bool vcr_is_looping()
{
	return Config.is_movie_loop_enabled;
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
	if (!Config.vcr_readonly)
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentFrame pointer
		//		change_state(MOVIE_STATE_RECORD);
		m_task = e_task::recording;
		Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
		flush_movie();

		// update header with new ROM info
		if (last_task == e_task::playback)
			set_rom_info(&m_header);

		m_current_sample = (long)current_sample;
		m_header.length_samples = current_sample;
		m_current_vi = (int)current_vi;

		m_header.rerecord_count++;
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);

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
		Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
		flush_movie();

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
	// NOTE: We mutate m_task and send task change messages in here, so we need to acquire the lock (what if playback start thread decides to beat us up midway through input poll? right...)
	std::scoped_lock lock(vcr_mutex);

	// When resetting during playback, we need to remind program of the rerecords
	if (m_task != e_task::idle && just_reset)
	{
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
	}

	// if we aren't playing back a movie, our data source isn't movie
	if (!is_task_playback(m_task))
	{
		getKeys(index, input);
		last_controller_data[index] = *input;
		LuaCallbacks::call_input(index);

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


	// We need to handle start tasks first, as logic after it depends on the task being modified
	if (m_task == e_task::start_recording)
	{
		if (just_reset)
		{
			m_current_sample = 0;
			m_current_vi = 0;
			m_task = e_task::recording;
			just_reset = false;
			Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
			Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
		} else
		{
			// We need to fully reset rom prior to actually pushing any samples to the buffer
			bool clear_eeprom = !(m_header.startFlags & MOVIE_START_FROM_EEPROM);
			std::thread([clear_eeprom] { reset_rom(clear_eeprom, false); }).detach();
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

	if (m_task == e_task::start_playback_from_snapshot)
	{
		// TODO: maybe call st generation like normal and remove the "start_x" states
		if (savestates_job == e_st_job::none)
		{
			if (!savestates_job_success)
			{
				m_task = e_task::idle;
				getKeys(index, input);
				return;
			}
			printf("[VCR]: Starting playback...\n");
			m_task = e_task::playback;
		}
	}


	if (m_task == e_task::start_playback)
	{
		if (just_reset)
		{
			m_current_sample = 0;
			m_current_vi = 0;
			m_task = e_task::playback;
			just_reset = false;
			Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
			Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
		} else
		{
			bool clear_eeprom = !(m_header.startFlags & MOVIE_START_FROM_EEPROM);
			std::thread([clear_eeprom] { reset_rom(clear_eeprom, false); }).detach();
		}
	}

	if (m_task == e_task::recording)
	{
		// TODO: as old comments already state, remove vcr flush mechanism (reasons for it are long gone, at this point it's just huge complexity for no reason)
		reserve_buffer_space(
			(unsigned long)(m_input_buffer_ptr + sizeof(BUTTONS) -
				m_input_buffer));

		if (user_requested_reset)
		{
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

		if (user_requested_reset)
		{
			user_requested_reset = false;
			std::thread([] { reset_rom(false, false); }).detach();
		}
		return;
	}

	// our input source is movie, input plugin is overriden
	if (m_task != e_task::playback)
		return;

	// This if previously also checked for if the VI is over the amount specified in the header,
	// but that can cause movies to end playback early on laggy plugins.
	if (m_current_sample >= (long)m_header.length_samples)
	{
		vcr_stop_playback();

		if (Config.is_movie_loop_enabled)
		{
			VCR::start_playback(movie_path);
			return;
		}

		setKeys(index, {0});
		getKeys(index, input);
		return;
	}

	// Off-by-one because we are already inside input poll, so pauseEmu will be delayed by one frame
	if (seek_to_frame.has_value() && m_current_sample >= seek_to_frame.value() - 1)
	{
		pause_emu();
		VCR::stop_seek();
	}

	if (!(m_header.controller_flags & CONTROLLER_X_PRESENT(index)))
	{
		// disconnected controls are forced to have no input during playback
		*input = {0};
		return;
	}

	// Use inputs from movie, also notify input plugin of override via setKeys
	*input = *reinterpret_cast<BUTTONS*>(m_input_buffer_ptr);
	m_input_buffer_ptr += sizeof(BUTTONS);
	setKeys(index, *input);

	//no readable code because 120 star tas can't get this right >:(
	if (input->Value == 0xC000)
	{
		printf("[VCR] Resetting during playback...\n");
		std::thread([] { reset_rom(false, false); }).detach();
		// NOTE: While it doesn't seem to happen in practice, we could theoretically get another input poll generation between us signalling reset and the emu actually stopping.
		// To prevent this, we pause it here as to not generate new frames.
		pause_emu();
	}

	last_controller_data[index] = *input;
	LuaCallbacks::call_input(index);

	// if lua requested a joypad change, we overwrite the data with lua's changed value for this cycle
	if (overwrite_controller_data[index])
	{
		*input = new_controller_data[index];
		last_controller_data[index] = *input;
		overwrite_controller_data[index] = false;
	}
	m_current_sample++;
}


int
vcr_start_record(const char* filename, const unsigned short flags,
                const char* author_utf8, const char* description_utf8)
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		printf("[VCR] vcr_start_record busy!\n");
		return -1;
	}

	vcr_core_stopped();

	char buf[MAX_PATH];

	// m_filename will be overwritten later in the function if
	// MOVIE_START_FROM_EXISTING_SNAPSHOT is true, but it doesn't
	// matter enough to make this a conditional thing
	strncpy(m_filename, filename, MAX_PATH);

	// open record file
	strcpy(buf, m_filename);
	{
		const char* dot = strrchr(buf, '.');
		const char* s1 = strrchr(buf, '\\');
		if (const char* s2 = strrchr(buf, '/'); !dot || ((s1 && s1 > dot) || (s2 && s2 > dot)))
		{
			strncat(buf, ".m64", MAX_PATH);
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

	// FIXME: Do we want to reset this every time?
	Config.vcr_readonly = 0;
	Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);

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
		savestates_do_file(strip_extension(m_filename) + ".st", e_st_job::save);
		m_task = e_task::start_recording_from_snapshot;
	} else if (flags & MOVIE_START_FROM_EXISTING_SNAPSHOT)
	{
		printf("[VCR]: Loading state...\n");
		strncpy(m_filename, filename, MAX_PATH);
		savestates_do_file(strip_extension(m_filename) + ".st", e_st_job::load);

		// set this to the normal snapshot flag to maintain compatibility
		m_header.startFlags = MOVIE_START_FROM_SNAPSHOT;

		m_task = e_task::start_recording_from_existing_snapshot;
	} else
	{
		m_task = e_task::start_recording;
	}

	// NOTE: Previously, movie_path was set to buf here, which is the st path
	movie_path = filename;
	set_rom_info(&m_header);

	// utf8 strings are also null-terminated so this method still works
	if (author_utf8)
		strncpy(m_header.author, author_utf8, MOVIE_AUTHOR_DATA_SIZE);
	m_header.author[MOVIE_AUTHOR_DATA_SIZE - 1] = '\0';
	if (description_utf8)
		strncpy(m_header.description, description_utf8,
		        MOVIE_DESCRIPTION_DATA_SIZE);
	m_header.description[MOVIE_DESCRIPTION_DATA_SIZE - 1] = '\0';

	write_movie_header(m_file);

	m_current_sample = 0;
	m_current_vi = 0;

	Messenger::broadcast(Messenger::Message::MovieRecordingStarted, movie_path);
	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
	return 0;
}


int
vcr_stop_record()
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		printf("[VCR] vcr_stop_record busy!\n");
		return -1;
	}

	int ret_val = -1;

	if (m_task == e_task::start_recording)
	{
		char buf[MAX_PATH];

		m_task = e_task::idle;
		if (m_file)
		{
			fclose(m_file);
			m_file = nullptr;
		}
		printf("[VCR]: Removing files (nothing recorded)\n");

		strcpy(buf, m_filename);

		strncat(m_filename, ".st", MAX_PATH);

		if (_unlink(buf) < 0)
			fprintf(stderr, "[VCR]: Couldn't remove save state: %s\n",
			        strerror(errno));

		strcpy(buf, m_filename);
		strncat(m_filename, ".m64", MAX_PATH);
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

		Statusbar::post("Stopped recording");

		ret_val = 0;
	}

	if (m_input_buffer)
	{
		free(m_input_buffer);
		m_input_buffer = nullptr;
		m_input_buffer_ptr = nullptr;
		m_input_buffer_size = 0;
	}

	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	return ret_val;
}

std::filesystem::path find_savestate_for_movie(std::filesystem::path path)
{
	// To allow multiple m64s to reference on st, we construct the st name from the m64 name up to the first point only
	// movie.a.m64 => movie.st
	// movie.m64   => movie.st
	char drive[MAX_PATH] = {0};
	char dir[MAX_PATH] = {0};
	char filename[MAX_PATH] = {0};
	_splitpath(path.string().c_str(), drive, dir, filename, nullptr);

	auto dot_index = std::string(filename).find_first_of(".");
	std::string name;
	if (dot_index == std::string::npos)
	{
		name = filename;
	} else
	{
		name = std::string(filename).substr(0, dot_index);
	}

	auto st = std::string(drive) + std::string(dir) + strip_extension(name) + ".st";
	if (std::filesystem::exists(st))
	{
		return st;
	}

	st = std::string(drive) + std::string(dir) + strip_extension(name) + ".savestate";
	if (std::filesystem::exists(st))
	{
		return st;
	}

	return "";
}


int check_warn_controllers(char* warning_str) {
	for (int i = 0; i < 4; ++i) {
		if (!Controls[i].Present && (m_header.controller_flags &
			CONTROLLER_X_PRESENT(i))) {
			sprintf(warning_str,
				   "Error: You have controller %d disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you fix this first (in your input settings).\n", (i + 1));
			return 0;
		}
		if (Controls[i].Present && !(m_header.controller_flags &
			CONTROLLER_X_PRESENT(i)))
			sprintf(warning_str,
				   "Warning: You have controller %d enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you change this first (in your input settings).\n", (i + 1));
		else {
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::mempak) && (m_header.
					controller_flags & CONTROLLER_X_MEMPAK(i)))
				sprintf(warning_str,
					   "Warning: Controller %d has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i + 1));
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::rumblepak) && (m_header.
					controller_flags & CONTROLLER_X_RUMBLE(i)))
				sprintf(warning_str,
					   "Warning: Controller %d has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i + 1));
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::none) && !(m_header.
					controller_flags & (CONTROLLER_X_MEMPAK(i) |
						CONTROLLER_X_RUMBLE(i))))
				sprintf(warning_str,
					   "Warning: Controller %d does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n", (i + 1));
		}
	}
	return 1;
}

VCR::Result VCR::start_playback(std::filesystem::path path)
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		printf("[VCR] vcr_start_playback busy!\n");
		return Result::Busy;
	}

	// Nope, doesnt work since some random user32 call in core somewhere converts it to "GUI" thread.
	// assert(!IsGUIThread(false));

	if (!emu_launched && !start_rom(path))
	{
		return Result::NoMatchingRom;
	}

	strncpy(m_filename, path.string().c_str(), MAX_PATH);

	auto movie_buf = read_file_buffer(path);

	if (movie_buf.empty())
	{
		return Result::BadFile;
	}

	// We can't call this after opening m_file, since it will potentially nuke it
	vcr_core_stopped();

	// NOTE: Previously, a code path would try to look for corresponding .m64 if a non-m64 extension file is provided.
	m_file = fopen(m_filename, "rb+");
	if (!m_file)
	{
		return Result::BadFile;
	}

	const int code = read_movie_header(movie_buf, &m_header);
	if (code != SUCCESS)
	{
		// FIXME: The results don't collide, but use typed errors anyway!!!
		fclose(m_file);
		return Result::InvalidFormat;
	}

	for (auto& [Present, RawData, Plugin] : Controls)
	{
		if (!Present || !RawData)
			continue;

		if (MessageBox(
				nullptr,
				"Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".\nThis can cause issues when recording and playing movies. Proceed?",
				"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING)
			==
			IDNO)
		{
			fclose(m_file);
			return Result::Cancelled;
		}

		break;
	}

	char dummy[1024] = {0};
	if(!check_warn_controllers(dummy))
	{
		fclose(m_file);
		return Result::InvalidControllers;
	}

	if (strlen(dummy) > 0)
	{
		MessageBox(
			nullptr,
			dummy, "VCR", MB_OK | MB_TOPMOST | MB_ICONWARNING);
	}

	if (_stricmp(m_header.rom_name,
	             (const char*)ROM_HEADER.nom) != 0)
	{
		if (MessageBox(
				nullptr,
				std::format("The movie was recorded on the rom '{}', but is being played back on '{}'.\r\nPlayback might desynchronize. Are you sure you want to continue?", m_header.rom_name, (const char*)ROM_HEADER.nom).c_str(),
				"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING)
			==
			IDNO)
		{
			fclose(m_file);
			return Result::Cancelled;
		}
	} else
	{
		if (m_header.rom_country != ROM_HEADER.
			Country_code)
		{
			if (MessageBox(
				nullptr,
				std::format("The movie was recorded on a rom with country {}, but is being played back on {}.\r\nPlayback might desynchronize. Are you sure you want to continue?", m_header.rom_country, ROM_HEADER.Country_code).c_str(),
				"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING)
			==
			IDNO)
			{
				fclose(m_file);
				return Result::Cancelled;
			}
		} else if (m_header.rom_crc1 != ROM_HEADER.
			CRC1)
		{
			char str[512] = {0};
			sprintf(
				str,
				"The movie was recorded with a ROM that has CRC \"0x%X\",\nbut you are using a ROM with CRC \"0x%X\".\r\nPlayback might desynchronize. Are you sure you want to continue?",
				(unsigned int)m_header.rom_crc1,
				(unsigned int)ROM_HEADER.CRC1);
			if (MessageBox(
				nullptr,
str,				"VCR", MB_YESNO | MB_TOPMOST | MB_ICONWARNING)
			==
			IDNO)
			{
				fclose(m_file);
				return Result::Cancelled;
			}
		}
	}

	fseek(m_file, MUP_HEADER_SIZE_CUR, SEEK_SET);

	// Reset VCR-related state
	m_current_sample = 0;
	m_current_vi = 0;
	strcpy(vcr_lastpath, m_filename);
	movie_path = path;

	// Read input stream into buffer, then point to first element
	m_input_buffer_ptr = m_input_buffer;
	const unsigned long to_read = sizeof(BUTTONS) * (m_header.length_samples + 1);
	reserve_buffer_space(to_read);
	fread(m_input_buffer_ptr, 1, to_read, m_file);
	fseek(m_file, 0, SEEK_END);

	if (m_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
	{
		printf("[VCR]: Loading state...\n");

		// Load appropriate state for movie
		auto st_path = find_savestate_for_movie(movie_path);

		if (st_path.empty())
		{
			fclose(m_file);
			return Result::InvalidSavestate;
		}

		savestates_do_file(st_path, e_st_job::load);
		m_task = e_task::start_playback_from_snapshot;
	} else
	{
		m_task = e_task::start_playback;
	}

	Messenger::broadcast(Messenger::Message::MoviePlaybackStarted, movie_path);
	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)m_header.rerecord_count);
	LuaCallbacks::call_play_movie();
	return Result::Ok;
}

bool VCR::can_seek_to(int32_t frame, bool relative)
{
	frame += relative ? m_current_sample : 0;
	return frame < m_header.length_samples && frame > 0;
}

VCR::Result VCR::begin_seek_to(int32_t frame, bool relative)
{
	frame += relative ? m_current_sample : 0;

	if (!can_seek_to(frame, relative))
	{
		return Result::InvalidFrame;
	}

	seek_to_frame = std::make_optional(frame);
	resume_emu();

	// We need to backtrack by restarting playback if we're ahead of the frame
	if (m_current_sample > frame)
	{
		vcr_stop_playback();
		start_playback(movie_path);
	}

	return Result::Ok;
}

void VCR::stop_seek()
{
	seek_to_frame.reset();
	Messenger::broadcast(Messenger::Message::SeekCompleted, nullptr);
}

bool VCR::is_seeking()
{
	return seek_to_frame.has_value();
}

int vcr_stop_playback()
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock()) {
		printf("[VCR] vcr_stop_playback busy!\n");
		return -1;
	}

	if (m_file && m_task != e_task::start_recording && m_task != e_task::recording)
	{
		fclose(m_file);
		m_file = nullptr;
	}

	if (is_task_playback(m_task))
	{
		m_task = e_task::idle;
		Statusbar::post("Stopped playback");

		if (m_input_buffer)
		{
			free(m_input_buffer);
			m_input_buffer = nullptr;
			m_input_buffer_ptr = nullptr;
			m_input_buffer_size = 0;
		}

		Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
		LuaCallbacks::call_stop_movie();
		return 0;
	}

	return -1;
}


bool task_is_playback(e_task task)
{
	return task == e_task::playback || task == e_task::start_playback || task ==
		e_task::start_playback_from_snapshot;
}
bool task_is_recording(e_task task)
{
	return task == e_task::recording || task == e_task::start_recording || task == e_task::start_recording_from_existing_snapshot || task == e_task::start_recording_from_snapshot;
}

void vcr_update_screen()
{
	if (!vcr_is_capturing())
	{
		if (!is_frame_skipped()) {
			if (get_video_size && read_video)
			{
				MGECompositor::update_screen();
			} else
			{
				updateScreen();
			}
		}

		// we always want to invoke atvi, regardless of ff optimization
		LuaCallbacks::call_vi();
		return;
	}

	// capturing, update screen and call readscreen, call avi/ffmpeg stuff
	updateScreen();
	LuaCallbacks::call_vi();
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
			VCRComp_finishFile();
			if (!avi_increment)
				endptr = avi_file_name + strlen(avi_file_name) - 4;
			//AVIIncrement
			sprintf(endptr, "%d.avi", ++avi_increment);
			VCRComp_startFile(avi_file_name, width, height, get_vis_per_second(ROM_HEADER.Country_code),
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
				show_modal_info("Audio frames became negative!", nullptr);
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
					show_modal_info(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?", nullptr);
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
					show_modal_info(
						"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?", nullptr);
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
				show_modal_info(
					"Video codec failure!\nA call to addVideoFrame() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?", nullptr);
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
					show_modal_info(
						"Audio output failure!\nA call to addAudioData() (AVIStreamWrite) failed.\nPerhaps you ran out of memory?", nullptr);
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
			get_vis_per_second(ROM_HEADER.Country_code);
	}
}

// calculates how long the audio data will last
float get_percent_of_frame(const int ai_len, const int audio_freq, const int audio_bitrate)
{
	const int limit = get_vis_per_second(ROM_HEADER.Country_code);
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
				int len3 = (int)(m_audio_freq / (long double)get_vis_per_second(ROM_HEADER.Country_code)) * (int)desync;
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

bool vcr_start_capture(const char* path, const bool show_codec_dialog)
{
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

	// If we are capturing internally, we get our dimensions from the window, otherwise from the GFX plugin
	long width = 0, height = 0;
	if (readScreen == vcrcomp_internal_read_screen)
	{
		// fill in window size at avi start, which can't change
		// scrap whatever was written there even if window didnt change, for safety
		vcrcomp_window_info = {0};
		get_window_info(mainHWND, vcrcomp_window_info);
		width = vcrcomp_window_info.width & ~3;
		height = vcrcomp_window_info.height & ~3;
	} else
	{
		void* dummy;
		readScreen(&dummy, &width, &height);
	}

	VCRComp_startFile(path, width, height, get_vis_per_second(ROM_HEADER.Country_code), show_codec_dialog);
	m_capture = 1;
	capture_with_f_fmpeg = false;
	strncpy(avi_file_name, path, MAX_PATH);

	Messenger::broadcast(Messenger::Message::CapturingChanged, true);

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
		s_info.width, s_info.height, get_vis_per_second(ROM_HEADER.Country_code), m_audio_freq,
		arguments + " " + output_name);

	auto err = capture_manager->initError;
	if (err != INIT_SUCCESS)
		capture_manager.reset();
	else
	{
		m_capture = 1;
		capture_with_f_fmpeg = 1;
		Messenger::broadcast(Messenger::Message::CapturingChanged, true);
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
	Messenger::broadcast(Messenger::Message::CapturingChanged, false);

	//this must be first in case object is being destroyed and other thread still sees m_capture=1
	capture_manager.reset();
	//apparently closing the pipes is enough to tell ffmpeg the movie ended.
#ifdef _DEBUG
	printf("[VCR] ffmpeg capture stopped\n");
#endif
}

int vcr_stop_capture()
{
	if (capture_with_f_fmpeg)
	{
		vcr_stop_f_fmpeg_capture();
		return 0;
	}

	m_capture = 0;
	write_sound(nullptr, 0, m_audio_freq, m_audio_freq * 2, TRUE);

	Messenger::broadcast(Messenger::Message::CapturingChanged, false);


	SetWindowPos(mainHWND, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	VCRComp_finishFile();
	avi_increment = 0;
	printf("[VCR]: Capture finished.\n");
	return 0;
}


void
vcr_core_stopped()
{
	switch (m_task)
	{
	case e_task::start_recording:
	case e_task::start_recording_from_snapshot:
	case e_task::recording:
		vcr_stop_record();
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
	std::string input_info = std::format("({}, {}) ", (int)b.Y_AXIS, (int)b.X_AXIS);
	if (b.START_BUTTON) input_info += "S";
	if (b.Z_TRIG) input_info += "Z";
	if (b.A_BUTTON) input_info += "A";
	if (b.B_BUTTON) input_info += "B";
	if (b.L_TRIG) input_info += "L";
	if (b.R_TRIG) input_info += "R";
	if (b.U_CBUTTON || b.D_CBUTTON || b.L_CBUTTON ||
		b.R_CBUTTON)
	{
		input_info += " C";
		if (b.U_CBUTTON) input_info += "^";
		if (b.D_CBUTTON) input_info += "v";
		if (b.L_CBUTTON) input_info += "<";
		if (b.R_CBUTTON) input_info += ">";
	}
	if (b.U_DPAD || b.D_DPAD || b.L_DPAD || b.
		R_DPAD)
	{
		input_info += " D";
		if (b.U_DPAD) input_info += "^";
		if (b.D_DPAD) input_info += "v";
		if (b.L_DPAD) input_info += "<";
		if (b.R_DPAD) input_info += ">";
	}

	auto index_adjustment = (Config.vcr_0_index ? 1 : 0);

	Statusbar::post(input_info, Statusbar::Section::Input);

	if (vcr_is_recording())
	{
		std::string vcr_info = std::format("{} ({}) ", m_current_vi - index_adjustment, m_current_sample - index_adjustment);
		Statusbar::post(vcr_info, Statusbar::Section::VCR);
	}

	if (vcr_is_playing())
	{
		std::string vcr_info = std::format("{} / {} ({} / {}) ", m_current_vi - index_adjustment, vcr_get_length_v_is(), m_current_sample - index_adjustment, vcr_get_length_samples());
		Statusbar::post(vcr_info);
	}
}

void vcr_on_vi()
{
	m_current_vi++;
	if (vcr_is_recording())
		vcr_set_length_v_is(m_current_vi);
	if (!vcr_is_playing())
		return;

	bool pausing_at_last = (Config.pause_at_last_frame && m_current_sample == m_header.length_samples);
	bool pausing_at_n = (Config.pause_at_frame != -1 && m_current_sample >= Config.pause_at_frame);

	if (pausing_at_last || pausing_at_n)
	{
		pause_emu();
	}

	if (pausing_at_last)
	{
		Config.pause_at_last_frame = 0;
	}

	if (pausing_at_n)
	{
		Config.pause_at_frame = -1;
	}
}

void VCR::init()
{
	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	Messenger::subscribe(Messenger::Message::ResetCompleted, [](std::any)
	{
		just_reset = true;
	});
	Messenger::subscribe(Messenger::Message::ResetRequested, [](std::any)
	{
		user_requested_reset = true;
	});
}

bool is_frame_skipped()
{
	if (VCR::is_seeking())
	{
		return true;
	}

	if (!fast_forward || vcr_is_capturing())
	{
		return false;
	}

	// skip every frame
	if (Config.frame_skip_frequency == 0)
	{
		return true;
	}

	// skip no frames
	if (Config.frame_skip_frequency == 1)
	{
		return false;
	}

	return screen_updates % Config.frame_skip_frequency != 0;
}
