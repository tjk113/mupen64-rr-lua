#pragma once

/*
 *	Interface for calls originating from core or shared layer to the view layer
 *
 *	Must be implemented in the view layer.
 */

#include <shared/Config.hpp>

// TODO: Move this into a namespace

/**
 * \brief Demands user confirmation for an exit action
 * \return Whether the action is allowed
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool confirm_user_exit();

/**
 * \brief Asks the user a yes/no question
 * \param str The dialog content
 * \param title The dialog title
 * \param warning Whether the tone of the message is perceived as a warning
 * \param hwnd The parent window
 * \return Whether the user answered yes
 * \remarks If the user has chosen to not use modals, this function will return true by default
 */
bool show_ask_dialog(const char* str, const char* title = nullptr, bool warning = false, void* hwnd = nullptr);

/**
 * \brief Shows the user a warning dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_warning(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Shows the user an error dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_error(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Shows the user an information dialog
 * \param str The dialog content
 * \param title The dialog title
 * \param hwnd The parent window
 */
void show_information(const char* str, const char* title = nullptr, void* hwnd = nullptr);

/**
 * \brief Whether the current execution is on the UI thread
 */
bool is_on_gui_thread();

/**
 * \brief Gets the path to the directory containing the mupen64 executable
 */
std::filesystem::path get_app_path();

/**
 * \brief Fills out the default hotkey keys in the config structure
 * \param config The config structure to modify
 * \remarks Because the shared layer is unaware of keycodes, the view must do this.
 * FIXME: While this works for now, storing platform-dependent keycodes in the config will make config files incompatible across platforms.
 */
void set_default_hotkey_keys(CONFIG* config);

/**
 * \brief Gets the native handle of the app instance
 */
void* get_app_instance_handle();

/**
 * \brief Gets the native handle of the main emulator window
 */
void* get_main_window_handle();

/**
 * \brief Gets the native handle of the statusbar. Can be null.
 */
void* get_statusbar_handle();

/**
 * \brief Gets the native handle of the parent window of the plugin configuration dialog
 */
void* get_plugin_config_parent_handle();

/**
 * \brief Gets whether the view prefers the core to not skip rendering any frames
 */
bool get_prefers_no_render_skip();

/**
 * \brief Updates the screen with the current visual information
 */
void update_screen();

/**
 * \brief Notifies the frontend of a new VI
 */
void at_vi();
