#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

typedef struct t_hotkey
{
	std::string identifier;
	int32_t key;
	int32_t ctrl;
	int32_t shift;
	int32_t alt;
	int32_t down_cmd;
	int32_t up_cmd;
} t_hotkey;

#pragma pack(push, 1)
typedef struct _CONFIG
{
#pragma region Hotkeys
	t_hotkey fast_forward_hotkey;
	t_hotkey speed_down_hotkey;
	t_hotkey speed_up_hotkey;
	t_hotkey frame_advance_hotkey;
	t_hotkey pause_hotkey;
	t_hotkey toggle_read_only_hotkey;
	t_hotkey toggle_movie_loop_hotkey;
	t_hotkey start_movie_playback_hotkey;
	t_hotkey start_movie_recording_hotkey;
	t_hotkey stop_movie_hotkey;
	t_hotkey take_screenshot_hotkey;
	t_hotkey play_latest_movie_hotkey;
	t_hotkey load_latest_script_hotkey;
	t_hotkey new_lua_hotkey;
	t_hotkey close_all_lua_hotkey;
	t_hotkey load_rom_hotkey;
	t_hotkey close_rom_hotkey;
	t_hotkey reset_rom_hotkey;
	t_hotkey load_latest_rom_hotkey;
	t_hotkey fullscreen_hotkey;
	t_hotkey settings_hotkey;
	t_hotkey toggle_statusbar_hotkey;
	t_hotkey refresh_rombrowser_hotkey;
	t_hotkey seek_to_frame_hotkey;
	t_hotkey run_hotkey;
	t_hotkey save_current_hotkey;
	t_hotkey load_current_hotkey;
	t_hotkey save_as_hotkey;
	t_hotkey load_as_hotkey;
	t_hotkey save_to_slot_1_hotkey;
	t_hotkey save_to_slot_2_hotkey;
	t_hotkey save_to_slot_3_hotkey;
	t_hotkey save_to_slot_4_hotkey;
	t_hotkey save_to_slot_5_hotkey;
	t_hotkey save_to_slot_6_hotkey;
	t_hotkey save_to_slot_7_hotkey;
	t_hotkey save_to_slot_8_hotkey;
	t_hotkey save_to_slot_9_hotkey;
	t_hotkey save_to_slot_10_hotkey;
	t_hotkey load_from_slot_1_hotkey;
	t_hotkey load_from_slot_2_hotkey;
	t_hotkey load_from_slot_3_hotkey;
	t_hotkey load_from_slot_4_hotkey;
	t_hotkey load_from_slot_5_hotkey;
	t_hotkey load_from_slot_6_hotkey;
	t_hotkey load_from_slot_7_hotkey;
	t_hotkey load_from_slot_8_hotkey;
	t_hotkey load_from_slot_9_hotkey;
	t_hotkey load_from_slot_10_hotkey;
	t_hotkey select_slot_1_hotkey;
	t_hotkey select_slot_2_hotkey;
	t_hotkey select_slot_3_hotkey;
	t_hotkey select_slot_4_hotkey;
	t_hotkey select_slot_5_hotkey;
	t_hotkey select_slot_6_hotkey;
	t_hotkey select_slot_7_hotkey;
	t_hotkey select_slot_8_hotkey;
	t_hotkey select_slot_9_hotkey;
	t_hotkey select_slot_10_hotkey;

#pragma endregion

	/// <summary>
	/// The config protocol version
	/// </summary>
	int32_t version = 5;

	/// <summary>
	/// The currently selected core type
	/// <para/>
	/// 0 - Cached Interpreter
	/// 1 - Dynamic Recompiler
	/// 2 - Pure Interpreter
	/// </summary>
	int32_t core_type = 1;

	/// <summary>
	/// The target FPS modifier as a percentage
	/// <para/>
	/// (100 = default)
	/// </summary>
	int32_t fps_modifier = 100;

	/// <summary>
	/// The frequency at which frames are skipped during fast-forward
	/// <para/>
	/// 0 = skip all frames
	/// 1 = skip no frames
	/// >0 = every nth frame is skipped
	/// </summary>
	int32_t frame_skip_frequency = 1;

	/// <summary>
	/// Whether a playing movie will loop upon ending
	/// </summary>
	int32_t is_movie_loop_enabled;

	/// <summary>
	/// The emulated CPU's clock speed multiplier
	/// <para/>
	/// 1 = original clock speed
	/// high number = approach native clock speed
	/// </summary>
	int32_t cpu_clock_speed_multiplier = 1;

	/// <summary>
	/// Whether emulation will pause when the main window loses focus
	/// </summary>
	int32_t is_unfocused_pause_enabled;

	/// <summary>
	/// Whether the statusbar is enabled
	/// </summary>
	int32_t is_statusbar_enabled = 1;

	/// <summary>
	/// Whether the default plugins directory will be used (otherwise, falls back to <see cref="plugins_directory"/>)
	/// </summary>
	int32_t is_default_plugins_directory_used = 1;

	/// <summary>
	/// Whether the default save directory will be used (otherwise, falls back to <see cref="saves_directory"/>)
	/// </summary>
	int32_t is_default_saves_directory_used = 1;

	/// <summary>
	/// Whether the default screenshot directory will be used (otherwise, falls back to <see cref="screenshots_directory"/>)
	/// </summary>
	int32_t is_default_screenshots_directory_used = 1;

	/// <summary>
	/// The plugin directory
	/// </summary>
	std::string plugins_directory;

	/// <summary>
	/// The save directory
	/// </summary>
	std::string saves_directory;

	/// <summary>
	/// The screenshot directory
	/// </summary>
	std::string screenshots_directory;

	/// <summary>
	/// The savestate directory
	/// </summary>
	std::string states_path;

	/// <summary>
	/// The recently opened roms' paths
	/// </summary>
	std::vector<std::string> recent_rom_paths;

	/// <summary>
	/// Whether recently opened rom path collection is paused
	/// </summary>
	int32_t is_recent_rom_paths_frozen;

	/// <summary>
	/// The recently opened movies' paths
	/// </summary>
	std::vector<std::string> recent_movie_paths;

	/// <summary>
	/// Whether recently opened movie path collection is paused
	/// </summary>
	int32_t is_recent_movie_paths_frozen;

	/// <summary>
	/// Whether the rom browser will recursively search for roms beginning in the specified directories
	/// </summary>
	int32_t is_rombrowser_recursion_enabled;

	/// <summary>
	/// The paths to directories which are searched for roms
	/// </summary>
	std::vector<std::string> rombrowser_rom_paths;

	/// <summary>
	/// Whether rom resets are not recorded in movies
	/// </summary>
	int32_t is_reset_recording_enabled;

	/// <summary>
	/// The strategy to use when capturing video
	/// <para/>
	/// 0 = Use the video plugin's readScreen or read_video (MGE)
	/// 1 = Internal capture of window
	/// 2 = Internal capture of screen cropped to window
	/// </summary>
	int32_t capture_mode = 2;

	/// <summary>
	/// The delay (in milliseconds) before capturing the window
	/// <para/>
	/// May be useful when capturing other windows alongside mupen
	/// </summary>
	int32_t capture_delay;

	/// <summary>
	/// The audio-video synchronization mode
	/// <para/>
	/// 0 - No Sync
	/// 1 - Audio Sync
	/// 2 - Video Sync
	/// </summary>
	int32_t synchronization_mode = 1;

	/// <summary>
	/// The lua script path
	/// </summary>
	std::string lua_script_path;

	/// <summary>
	/// The recently opened lua scripts' paths
	/// </summary>
	std::vector<std::string> recent_lua_script_paths;

	/// <summary>
	/// Whether recently opened lua script path collection is paused
	/// </summary>
	int32_t is_recent_scripts_frozen;

	/// <summary>
	/// SD card emulation
	/// </summary>
	int32_t use_summercart;

	/// <summary>
	/// Whether floats will be truncated when rounding
	/// </summary>
	int32_t is_round_towards_zero_enabled;

	/// <summary>
	/// Whether floating point exceptions will propagate and crash the emulator
	/// </summary>
	int32_t is_float_exception_propagation_enabled;

	/// <summary>
	/// Whether audio interrupts will be delayed
	/// </summary>
	int32_t is_audio_delay_enabled = 1;

	/// <summary>
	/// Whether jmp instructions will cause the following code to be JIT'd by dynarec
	/// </summary>
	int32_t is_compiled_jump_enabled = 1;

	/// <summary>
	/// The path of the currently selected video plugin
	/// </summary>
	std::string selected_video_plugin;

	/// <summary>
	/// The path of the currently selected audio plugin
	/// </summary>
	std::string selected_audio_plugin;

	/// <summary>
	/// The path of the currently selected input plugin
	/// </summary>
	std::string selected_input_plugin;

	/// <summary>
	/// The path of the currently selected RSP plugin
	/// </summary>
	std::string selected_rsp_plugin;

	/// <summary>
	/// The last known value of the record movie dialog's "start type" field
	/// </summary>
	int32_t last_movie_type;

	/// <summary>
	/// The last known value of the record movie dialog's "author" field
	/// </summary>
	std::string last_movie_author;

	/// <summary>
	/// The main window's X position
	/// </summary>
	int32_t window_x = ((int)0x80000000);

	/// <summary>
	/// The main window's Y position
	/// </summary>
	int32_t window_y = ((int)0x80000000);

	/// <summary>
	/// The main window's width
	/// </summary>
	int32_t window_width = 640;

	/// <summary>
	/// The main window's height
	/// </summary>
	int32_t window_height = 480;

	/// <summary>
	/// The width of rombrowser columns by index
	/// </summary>
	std::vector<std::int32_t> rombrowser_column_widths = {24, 240, 240, 120};

	/// <summary>
	/// The index of the currently sorted column, or -1 if none is sorted
	/// </summary>
	int32_t rombrowser_sorted_column;

	/// <summary>
	/// Whether the selected column is sorted in an ascending order
	/// </summary>
	int32_t rombrowser_sort_ascending = 1;

	/// <summary>
	/// A map of persistent path dialog IDs and the respective value
	/// </summary>
	std::map<std::string, std::wstring> persistent_folder_paths;

	/// <summary>
	/// Resets the emulator faster, but may cause issues
	/// </summary>
	int32_t fast_reset;

	/// <summary>
	/// Whether VCR displays frame information relative to frame 0, not 1
	/// </summary>
	int32_t vcr_0_index;

	/// <summary>
	/// Increments the current slot when saving savestate to slot
	/// </summary>
	int32_t increment_slot;

	/// <summary>
	/// The movie frame to automatically pause at
	/// -1 none
	/// </summary>
	int32_t pause_at_frame = -1;

	/// <summary>
	/// Whether to pause at the last movie frame
	/// </summary>
	int32_t pause_at_last_frame;

	/// <summary>
	/// Whether VCR operates in read-only mode
	/// </summary>
	int32_t vcr_readonly;

	/// <summary>
	/// Whether mupen will display a dialog upon crashing
	/// </summary>
	int32_t crash_dialog = 1;

	/// <summary>
	/// The maximum amount of VIs allowed to be generated since the last input poll before a warning dialog is shown
	/// 0 - no warning
	/// </summary>
	int32_t max_lag = 480;

	/// <summary>
	/// The current seeker input value
	/// </summary>
	std::string seeker_value;
} CONFIG;
#pragma pack(pop)

extern "C" CONFIG Config;
extern std::vector<t_hotkey*> hotkeys;

/**
 * \brief Gets the string representation of a hotkey
 * \param hotkey The hotkey to convert
 * \return The hotkey as a string
 */
std::string hotkey_to_string(t_hotkey* hotkey);

/**
 * \brief Saves the current config state to the config file
 */
void save_config();

/**
 * \brief Restores the config state from the config file
 */
void load_config();

/// <summary>
/// Waits until the user inputs a valid key sequence, then fills out the hotkey
/// </summary>
/// <returns>
/// Whether a hotkey has successfully been picked
/// </returns>
int32_t get_user_hotkey(t_hotkey* hotkey);

/**
 * \brief Updates all menu hotkey accelerator labels to the appropriate config values
 */
void update_menu_hotkey_labels();
