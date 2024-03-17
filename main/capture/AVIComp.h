#pragma once
#include <Windows.h>

namespace AVIComp
{
	typedef struct VideoParams
	{
		/**
		 * \brief The video file's path
		 */
		std::filesystem::path path;
		/**
		 * \brief The video stream's width
		 */
		uint32_t width;
		/**
		 * \brief The video stream's height
		 */
		uint32_t height;
		/**
		 * \brief The video stream's FPS
		 */
		uint32_t fps;
	};

	enum class Result
	{
		// The operation completed successfully
		Ok,
		// The user cancelled the operation
		Cancelled,
		// The AVI preset is invalid or doesn't exist
		InvalidPreset,
	};

	/**
	 * \brief Starts encoding to a new file
	 * \param params The parameters for encoding
	 * \param show_codec_picker Whether the vfw codec picker should be shown. If not, the previous settings will be used. If no previous settings are available, the function will fail.
	 * \return The operation result
	 */
	Result start(VideoParams params, bool show_codec_picker);

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

void get_window_info(HWND hwnd, t_window_info& info);

/**
 * \brief Writes the emulator's current emulation front buffer into the destination buffer
 * \param dest The buffer holding video data of size <c>width * height</c>
 * \param width The buffer's width
 * \param height The buffer's height
 */
void __cdecl vcrcomp_internal_read_screen(void** dest, long* width, long* height);
