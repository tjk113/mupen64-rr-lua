
#include "vcr.h"
#include <cassert>
#include <shared/Config.hpp>
#include <memory>
#include <filesystem>
#include <cerrno>
#include <malloc.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <mutex>

#include "r4300.h"
#include "Plugin.hpp"
#include "rom.h"
#include "../memory/savestates.h"
#include "../memory/pif.h"
#include <shared/LuaCallbacks.h>

#include <shared/guifuncs.h>
#include <shared/messenger.h>

// M64\0x1a
enum
{
	mup_magic = (0x1a34364d),
	mup_version = (3),
};

#define MUP_HEADER_SIZE (sizeof(t_movie_header))

const auto rawdata_warning_message = "Warning: One of the active controllers of your input plugin is set to accept \"Raw Data\".\nThis can cause issues when recording and playing movies. Proceed?";
const auto rom_name_warning_message = "The movie was recorded on the rom '{}', but is being played back on '{}'.\r\nPlayback might desynchronize. Are you sure you want to continue?";
const auto rom_country_warning_message = "The movie was recorded on a rom with country {}, but is being played back on {}.\r\nPlayback might desynchronize. Are you sure you want to continue?";
const auto rom_crc_warning_message = "The movie was recorded with a ROM that has CRC \"0x%X\",\nbut you are using a ROM with CRC \"0x%X\".\r\nPlayback might desynchronize. Are you sure you want to continue?";
const auto truncate_message = "Failed to truncate the movie file. The movie may be corrupted.";

volatile e_task m_task = e_task::idle;
std::filesystem::path movie_path;

// The frame to seek to during playback, or an empty option if no seek is being performed
std::optional<size_t> seek_to_frame;

t_movie_header g_header;
std::vector<BUTTONS> g_movie_inputs;

long m_current_sample = -1;
// should = length_samples when recording, and be < length_samples when playing
int m_current_vi = -1;

int title_length;


// Used for tracking VCR-invoked resets
bool just_reset;

// Used for tracking user-invoked resets
bool user_requested_reset;

std::recursive_mutex vcr_mutex;

// Writes the movie header + inputs to current movie_path
bool write_movie()
{
	std::filesystem::remove(movie_path);
	FILE* f = fopen(movie_path.string().c_str(), "wb");
	if (!f)
	{
		return false;
	}
	fwrite(&g_header, MUP_HEADER_SIZE, 1, f);
	fwrite(g_movie_inputs.data(), sizeof(BUTTONS), g_header.length_samples, f);
	fclose(f);
	return true;
}

bool is_task_playback(const e_task task)
{
	return task == e_task::start_playback || task == e_task::start_playback_from_snapshot || task == e_task::playback;
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

	strncpy(header->video_plugin_name, video_plugin->name().c_str(),
	        64);
	strncpy(header->input_plugin_name, input_plugin->name().c_str(),
	        64);
	strncpy(header->audio_plugin_name, audio_plugin->name().c_str(),
	        64);
	strncpy(header->rsp_plugin_name, rsp_plugin->name().c_str(), 64);
}

static int read_movie_header(std::vector<uint8_t> buf, t_movie_header* header)
{
	const auto old_header_size = 512;

	if (buf.size() < old_header_size)
		return WRONG_FORMAT;

	t_movie_header new_header = {};
	memcpy(&new_header, buf.data(), old_header_size);

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

	if (read_movie_header(buf, &new_header) != SUCCESS)
	{
		return Result::InvalidFormat;
	}

	*header = new_header;
	return Result::Ok;
}

bool
vcr_is_active()
{
	return m_task == e_task::recording || m_task == e_task::playback;
}

bool
vcr_is_idle()
{
	return m_task == e_task::idle;
}

bool
vcr_is_starting()
{
	return m_task == e_task::start_playback || m_task == e_task::start_playback_from_snapshot;
}

bool
vcr_is_playing()
{
	return m_task == e_task::playback;
}

bool
vcr_is_recording()
{
	return m_task == e_task::recording;
}

bool vcr_is_looping()
{
	return Config.is_movie_loop_enabled;
}

unsigned long vcr_get_length_v_is()
{
	return vcr_is_active() ? g_header.length_vis : 0;
}

unsigned long vcr_get_length_samples()
{
	return vcr_is_active() ? g_header.length_samples : 0;
}

void vcr_set_length_v_is(const unsigned long val)
{
	g_header.length_vis = val;
}

void vcr_set_length_samples(const unsigned long val)
{
	g_header.length_samples = val;
}

std::optional<t_movie_freeze> VCR::freeze()
{
	if (!vcr_is_active())
	{
		return std::nullopt;
	}

	t_movie_freeze freeze = {
		.size = sizeof(unsigned long) * 4 + sizeof(BUTTONS) * (g_header.length_samples + 1),
		.uid = g_header.uid,
		.current_sample = (unsigned long)m_current_sample,
		.current_vi = (unsigned long)m_current_vi,
		.length_samples = g_header.length_samples,
	};

	freeze.input_buffer.resize(sizeof(BUTTONS) * (g_header.length_samples + 1));
	memcpy(freeze.input_buffer.data(), g_movie_inputs.data(), sizeof(BUTTONS) * (g_header.length_samples + 1));

	return std::make_optional(std::move(freeze));
}

VCR::Result VCR::unfreeze(t_movie_freeze freeze)
{
	if (vcr_is_idle())
	{
		return Result::Idle;
	}

	if (freeze.size <
		sizeof(g_header.uid)
		+ sizeof(m_current_sample)
		+ sizeof(m_current_vi)
		+ sizeof(g_header.length_samples))
	{
		return Result::InvalidFormat;
	}

	const unsigned long space_needed = sizeof(BUTTONS) * (freeze.length_samples + 1);

	if (freeze.uid != g_header.uid)
		return Result::NotFromThisMovie;

	// This means playback desync in read-only mode, but in read-write mode it's fine, as the input buffer will be copied and grown from st.
	if (freeze.current_sample > freeze.length_samples && Config.vcr_readonly)
		return Result::InvalidFrame;

	if (space_needed > freeze.size)
		return Result::InvalidFormat;

	const e_task last_task = m_task;
	if (!Config.vcr_readonly)
	{
		// here, we are going to take the input data from the savestate
		// and make it the input data for the current movie, then continue
		// writing new input data at the currentFrame pointer
		m_task = e_task::recording;
		Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);

		// update header with new ROM info
		if (last_task == e_task::playback)
			set_rom_info(&g_header);

		m_current_sample = (long)freeze.current_sample;
		g_header.length_samples = freeze.current_sample;
		m_current_vi = (int)freeze.current_vi;

		g_header.rerecord_count++;
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);

		g_movie_inputs.resize(freeze.length_samples + 1);
		memcpy(g_movie_inputs.data(), freeze.input_buffer.data(), space_needed);
	} else
	{
		// here, we are going to keep the input data from the movie file
		// and simply rewind to the currentFrame pointer
		// this will cause a desync if the savestate is not in sync
		// with the on-disk recording data, but it's easily solved
		// by loading another savestate or playing the movie from the beginning

		// and older savestate might have a currentFrame pointer past
		// the end of the input data, so check for that here
		if (freeze.current_sample > g_header.length_samples)
			return Result::InvalidFrame;

		m_task = e_task::playback;
		Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);

		m_current_sample = (long)freeze.current_sample;
		m_current_vi = (int)freeze.current_vi;
	}

	return Result::Ok;
}

void vcr_on_controller_poll(int index, BUTTONS* input)
{
	// NOTE: When we call reset_rom from another thread, we only request a reset to happen in the future.
	// Until the reset, the emu thread keeps running and potentially generating many frames.
	// Those frames are invalid to us, because from the movie's perspective, it should be instantaneous.
	if (emu_resetting)
	{
		printf("[VCR] Omitting pre-reset frame!\n");
		return;
	}

	// NOTE: We mutate m_task and send task change messages in here, so we need to acquire the lock (what if playback start thread decides to beat us up midway through input poll? right...)
	std::scoped_lock lock(vcr_mutex);

	// When resetting during playback, we need to remind program of the rerecords
	if (m_task != e_task::idle && just_reset)
	{
		Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);
	}

	// if we aren't playing back a movie, our data source isn't movie
	if (!is_task_playback(m_task))
	{
		getKeys(index, input);
		LuaCallbacks::call_input(input, index);
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
			Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);
		} else
		{
			// We need to fully reset rom prior to actually pushing any samples to the buffer
			bool clear_eeprom = !(g_header.startFlags & MOVIE_START_FROM_EEPROM);
			std::thread([clear_eeprom]
			{
				vr_reset_rom(clear_eeprom, false);
			}).detach();
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
			Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);
		} else
		{
			bool clear_eeprom = !(g_header.startFlags & MOVIE_START_FROM_EEPROM);
			std::thread([clear_eeprom]
			{
				vr_reset_rom(clear_eeprom, false);
			}).detach();
		}
	}

	if (m_task == e_task::recording)
	{
		if (user_requested_reset)
		{
			*input = {
				.Reserved1 = 1,
				.Reserved2 = 1,
			};
		}

		g_movie_inputs.push_back(*input);
		g_header.length_samples++;
		m_current_sample++;

		if (user_requested_reset)
		{
			user_requested_reset = false;
			std::thread([]
			{
				vr_reset_rom(false, false);
			}).detach();
		}
		return;
	}

	// our input source is movie, input plugin is overriden
	if (m_task != e_task::playback)
		return;

	// This if previously also checked for if the VI is over the amount specified in the header,
	// but that can cause movies to end playback early on laggy plugins.
	if (m_current_sample >= (long)g_header.length_samples)
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

	if (!(g_header.controller_flags & CONTROLLER_X_PRESENT(index)))
	{
		// disconnected controls are forced to have no input during playback
		*input = {0};
		return;
	}

	// Use inputs from movie, also notify input plugin of override via setKeys
	*input = g_movie_inputs[m_current_sample];
	setKeys(index, *input);

	//no readable code because 120 star tas can't get this right >:(
	if (input->Value == 0xC000)
	{
		printf("[VCR] Resetting during playback...\n");
		std::thread([]
		{
			vr_reset_rom(false, false);
		}).detach();
	}

	LuaCallbacks::call_input(input, index);
	m_current_sample++;
}


int
vcr_start_record(const char* filename, const unsigned short flags,
                 const char* author_utf8, const char* description_utf8)
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock())
	{
		printf("[VCR] vcr_start_record busy!\n");
		return -1;
	}

	VCR::stop_all();
	movie_path = filename;

	for (auto& [Present, RawData, Plugin] : Controls)
	{
		if (Present && RawData)
		{
			bool proceed = show_ask_dialog(rawdata_warning_message, "VCR", true);
			if (!proceed)
			{
				return -1;
			}
			break;
		}
	}

	// FIXME: Do we want to reset this every time?
	Config.vcr_readonly = 0;
	Messenger::broadcast(Messenger::Message::ReadonlyChanged, (bool)Config.vcr_readonly);

	memset(&g_header, 0, MUP_HEADER_SIZE);

	g_header.magic = mup_magic;
	g_header.version = mup_version;
	g_header.uid = (unsigned long)time(nullptr);
	g_header.length_vis = 0;
	g_header.length_samples = 0;

	g_header.rerecord_count = 0;
	g_header.startFlags = flags;

	if (flags & MOVIE_START_FROM_SNAPSHOT)
	{
		// save state
		printf("[VCR]: Saving state...\n");
		savestates_do_file(std::filesystem::path(movie_path).replace_extension(".st"), e_st_job::save);
		m_task = e_task::start_recording_from_snapshot;
	} else if (flags & MOVIE_START_FROM_EXISTING_SNAPSHOT)
	{
		printf("[VCR]: Loading state...\n");
		savestates_do_file(std::filesystem::path(movie_path).replace_extension(".st"), e_st_job::load);
		// set this to the normal snapshot flag to maintain compatibility
		g_header.startFlags = MOVIE_START_FROM_SNAPSHOT;
		m_task = e_task::start_recording_from_existing_snapshot;
	} else
	{
		m_task = e_task::start_recording;
	}

	set_rom_info(&g_header);

	// utf8 strings are also null-terminated so this method still works
	if (author_utf8)
		strncpy(g_header.author, author_utf8, MOVIE_AUTHOR_DATA_SIZE);
	g_header.author[MOVIE_AUTHOR_DATA_SIZE - 1] = '\0';
	if (description_utf8)
		strncpy(g_header.description, description_utf8,
		        MOVIE_DESCRIPTION_DATA_SIZE);
	g_header.description[MOVIE_DESCRIPTION_DATA_SIZE - 1] = '\0';

	m_current_sample = 0;
	m_current_vi = 0;

	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);
	return 0;
}


int
vcr_stop_record()
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock())
	{
		printf("[VCR] vcr_stop_record busy!\n");
		return -1;
	}

	if (!task_is_recording(m_task))
	{
		return -1;
	}

	if (m_task == e_task::start_recording)
	{
		m_task = e_task::idle;
		printf("[VCR]: Removing files (nothing recorded)\n");
		_unlink(std::filesystem::path(movie_path).replace_extension(".m64").string().c_str());
		_unlink(std::filesystem::path(movie_path).replace_extension(".st").string().c_str());
	}

	if (m_task == e_task::recording)
	{
		m_task = e_task::idle;

		write_movie();

		printf("[VCR]: Record stopped. Recorded %ld input samples\n",
			   g_header.length_samples);
	}

	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
}

std::filesystem::path find_savestate_for_movie(std::filesystem::path path)
{
	// To allow multiple m64s to reference on st, we construct the st name from the m64 name up to the first point only
	// movie.a.m64 => movie.st
	// movie.m64   => movie.st
	char drive[260] = {0};
	char dir[260] = {0};
	char filename[260] = {0};
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


int check_warn_controllers(char* warning_str)
{
	for (int i = 0; i < 4; ++i)
	{
		if (!Controls[i].Present && (g_header.controller_flags &
			CONTROLLER_X_PRESENT(i)))
		{
			sprintf(warning_str,
			        "Error: You have controller %d disabled, but it is enabled in the movie file.\nIt cannot play back correctly unless you fix this first (in your input settings).\n",
			        (i + 1));
			return 0;
		}
		if (Controls[i].Present && !(g_header.controller_flags &
			CONTROLLER_X_PRESENT(i)))
			sprintf(warning_str,
			        "Warning: You have controller %d enabled, but it is disabled in the movie file.\nIt might not play back correctly unless you change this first (in your input settings).\n",
			        (i + 1));
		else
		{
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::mempak) && (g_header.
				controller_flags & CONTROLLER_X_MEMPAK(i)))
				sprintf(warning_str,
				        "Warning: Controller %d has a rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n",
				        (i + 1));
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::rumblepak) && (g_header.
				controller_flags & CONTROLLER_X_RUMBLE(i)))
				sprintf(warning_str,
				        "Warning: Controller %d has a memory pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n",
				        (i + 1));
			if (Controls[i].Present && (Controls[i].Plugin !=
				controller_extension::none) && !(g_header.
				controller_flags & (CONTROLLER_X_MEMPAK(i) |
					CONTROLLER_X_RUMBLE(i))))
				sprintf(warning_str,
				        "Warning: Controller %d does not have a mempak or rumble pack in the movie.\nYou may need to change your input plugin settings accordingly for this movie to play back correctly.\n",
				        (i + 1));
		}
	}
	return 1;
}

VCR::Result VCR::start_playback(std::filesystem::path path)
{
	std::unique_lock lock(vcr_mutex, std::try_to_lock);
	if (!lock.owns_lock())
	{
		printf("[VCR] vcr_start_playback busy!\n");
		return Result::Busy;
	}

	// Nope, doesnt work since some random user32 call in core somewhere converts it to "GUI" thread.
	// assert(!IsGUIThread(false));

	if (!emu_launched && vr_start_rom(path) != Core::Result::Ok)
	{
		return Result::NoMatchingRom;
	}

	auto movie_buf = read_file_buffer(path);

	if (movie_buf.empty())
	{
		return Result::BadFile;
	}

	// We can't call this after opening m_file, since it will potentially nuke it
	VCR::stop_all();

	const int code = read_movie_header(movie_buf, &g_header);
	if (code != SUCCESS)
	{
		// FIXME: The results don't collide, but use typed errors anyway!!!
		return Result::InvalidFormat;
	}

	g_movie_inputs = {};
	g_movie_inputs.resize(g_header.length_samples);
	memcpy(g_movie_inputs.data(), movie_buf.data() + MUP_HEADER_SIZE, sizeof(BUTTONS) * g_header.length_samples);

	for (auto& [Present, RawData, Plugin] : Controls)
	{
		if (!Present || !RawData)
			continue;

		bool proceed = show_ask_dialog(rawdata_warning_message, "VCR", true);
		if (!proceed)
		{
			return Result::Cancelled;
		}

		break;
	}

	char dummy[1024] = {0};
	if (!check_warn_controllers(dummy))
	{
		return Result::InvalidControllers;
	}

	if (strlen(dummy) > 0)
	{
		show_warning(dummy, "VCR");
	}

	if (_stricmp(g_header.rom_name,
	             (const char*)ROM_HEADER.nom) != 0)
	{
		bool proceed = show_ask_dialog(std::format(rom_name_warning_message, g_header.rom_name, (const char*)ROM_HEADER.nom).c_str(), "VCR", true);

		if (!proceed)
		{
			return Result::Cancelled;
		}
	} else
	{
		if (g_header.rom_country != ROM_HEADER.
			Country_code)
		{
			bool proceed = show_ask_dialog(std::format(rom_country_warning_message, g_header.rom_country, ROM_HEADER.Country_code).c_str(), "VCR", true);
			if (!proceed)
			{
				return Result::Cancelled;
			}
		} else if (g_header.rom_crc1 != ROM_HEADER.
			CRC1)
		{
			char str[512] = {0};
			sprintf(
				str,
				rom_crc_warning_message,
				(unsigned int)g_header.rom_crc1,
				(unsigned int)ROM_HEADER.CRC1);

			bool proceed = show_ask_dialog(str, "VCR", true);
			if (!proceed)
			{
				return Result::Cancelled;
			}
		}
	}

	// Reset VCR-related state
	m_current_sample = 0;
	m_current_vi = 0;
	movie_path = path;

	if (g_header.startFlags & MOVIE_START_FROM_SNAPSHOT)
	{
		printf("[VCR]: Loading state...\n");

		// Load appropriate state for movie
		auto st_path = find_savestate_for_movie(movie_path);

		if (st_path.empty())
		{
			return Result::InvalidSavestate;
		}

		savestates_do_file(st_path, e_st_job::load);
		m_task = e_task::start_playback_from_snapshot;
	} else
	{
		m_task = e_task::start_playback;
	}

	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	Messenger::broadcast(Messenger::Message::RerecordsChanged, (uint64_t)g_header.rerecord_count);
	LuaCallbacks::call_play_movie();
	return Result::Ok;
}

bool VCR::can_seek_to(int32_t frame, bool relative)
{
	frame += relative ? m_current_sample : 0;
	return frame < g_header.length_samples && frame > 0 && frame != m_current_sample;
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
	if (!lock.owns_lock())
	{
		printf("[VCR] vcr_stop_playback busy!\n");
		return -1;
	}

	if (!is_task_playback(m_task))
		return -1;

	m_task = e_task::idle;
	Messenger::broadcast(Messenger::Message::TaskChanged, m_task);
	LuaCallbacks::call_stop_movie();
	return 0;
}


bool task_is_playback(e_task task)
{
	return task == e_task::playback || task == e_task::start_playback || task ==
		e_task::start_playback_from_snapshot;
}

bool task_is_recording(e_task task)
{
	return task == e_task::recording || task == e_task::start_recording || task == e_task::start_recording_from_existing_snapshot || task ==
		e_task::start_recording_from_snapshot;
}

void vcr_update_screen()
{
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

int VCR::stop_all()
{
	switch (m_task)
	{
	case e_task::start_recording:
	case e_task::start_recording_from_snapshot:
	case e_task::recording:
		return vcr_stop_record();
	case e_task::start_playback:
	case e_task::start_playback_from_snapshot:
	case e_task::playback:
		return vcr_stop_playback();
	default: return 0;
	}
}

const char* VCR::get_input_text()
{
	static char text[1024]{};
	memset(text, 0, sizeof(text));

	BUTTONS b = LuaCallbacks::get_last_controller_data(0);
	sprintf(text, "(%d, %d) ", b.Y_AXIS, b.X_AXIS);
	if (b.START_BUTTON) strcat(text, "S");
	if (b.Z_TRIG) strcat(text, "Z");
	if (b.A_BUTTON) strcat(text, "A");
	if (b.B_BUTTON) strcat(text, "B");
	if (b.L_TRIG) strcat(text, "L");
	if (b.R_TRIG) strcat(text, "R");
	if (b.U_CBUTTON || b.D_CBUTTON || b.L_CBUTTON ||
		b.R_CBUTTON)
	{
		strcat(text, " C");
		if (b.U_CBUTTON) strcat(text, "^");
		if (b.D_CBUTTON) strcat(text, "v");
		if (b.L_CBUTTON) strcat(text, "<");
		if (b.R_CBUTTON) strcat(text, ">");
	}
	if (b.U_DPAD || b.D_DPAD || b.L_DPAD || b.
		R_DPAD)
	{
		strcat(text, "D");
		if (b.U_DPAD) strcat(text, "^");
		if (b.D_DPAD) strcat(text, "v");
		if (b.L_DPAD) strcat(text, "<");
		if (b.R_DPAD) strcat(text, ">");
	}
	return text;
}

const char* VCR::get_status_text()
{
	static char text[1024]{};
	memset(text, 0, sizeof(text));

	auto index_adjustment = (Config.vcr_0_index ? 1 : 0);

	if (vcr_is_recording())
	{
		sprintf(text, "%d (%d) ", m_current_vi - index_adjustment, m_current_sample - index_adjustment);
	}

	if (vcr_is_playing())
	{
		sprintf(text, "%d / %d (%d / %d) ",
			m_current_vi - index_adjustment,
			vcr_get_length_v_is(),
			m_current_sample - index_adjustment,
			vcr_get_length_samples());
	}

	return text;
}

void vcr_on_vi()
{
	m_current_vi++;
	if (vcr_is_recording())
		vcr_set_length_v_is(m_current_vi);
	if (!vcr_is_playing())
		return;

	bool pausing_at_last = (Config.pause_at_last_frame && m_current_sample == g_header.length_samples);
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

	if (!fast_forward)
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

	return g_total_frames % Config.frame_skip_frequency != 0;
}
