#pragma once
#include <Windows.h>

namespace AVIComp
{
	enum class Result
	{
		// The operation completed successfully
		Ok,
		// The user cancelled the operation
		Cancelled,
	};

	/**
	 * \brief Starts encoding to a new file
	 * \param path The file's path
	 * \param width The video stream's width
	 * \param height The video stream's height
	 * \param fps The video stream's FPS
	 * \param show_codec_picker Whether the vfw codec picker should be shown. If not, the previous settings will be used. If no previous settings are available, the function will fail.
	 * \return The operation result
	 */
	Result start(std::filesystem::path path, uint32_t width, uint32_t height, uint32_t fps, bool show_codec_picker);

	/**
	 * \brief Stops encoding to the current file and writes it to disk
	 * \return The operation result
	 */
	Result stop();

	/**
	 * \brief Adds one frame of video data
	 * \param data The video buffer
	 * \return Whether the operation succeeded
	 */
	bool add_video_data(uint8_t* data);

	/**
	 * \brief Adds samples of audio data
	 * \param data The audio buffer
	 * \param len The audio length
	 * \return Whether the operation succeeded
	 */
	bool add_audio_data(uint8_t* data, size_t len);
}
typedef struct
{
	long width;
	long height;
	long toolbar_height;
	long statusbar_height;
} t_window_info;

extern t_window_info vcrcomp_window_info;

unsigned int VCRComp_GetSize();

void get_window_info(HWND hwnd, t_window_info& info);

/**
 * \brief Writes the emulator's current emulation front buffer into the destination buffer
 * \param dest The buffer holding video data of size <c>width * height</c>
 * \param width The buffer's width
 * \param height The buffer's height
 */
void __cdecl vcrcomp_internal_read_screen(void** dest, long* width, long* height);
