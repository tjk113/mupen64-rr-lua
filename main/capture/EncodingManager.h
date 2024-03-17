#pragma once
#include <filesystem>

#include "Plugin.hpp"

namespace EncodingManager
{
	/**
	 * \brief Synchronization modes the encoding manager can abide by
	 */
	enum class Sync : int
	{
		/**
		 * \brief Video and Audio streams are not kept in sync
		 */
		None,

		/**
		 * \brief The audio stream dictates the video stream's rate
		 */
		Audio,

		/**
		 * \brief The video stream dictates the audio stream's rate
		 */
		Video,
	};

	/**
	 * \brief A data stream encoder type
	 */
	enum class EncoderType
	{
		/**
		 * \brief Microsoft's VFW
		 */
		VFW,

		/**
		 * \brief The FFmpeg library
		 */
		FFmpeg,
	};

	/**
	 * \brief Whether a capture is currently running
	 */
	bool is_capturing();

	/**
	 * \brief Starts capturing a video
	 * \param path The movie's path
	 * \param encoder_type The encoder to use for capturing
	 * \param ask_for_encoding_settings Whether the codec dialog should be shown. If false, the previously used codec or the default one will be used.
	 * \return Whether the operation was successful
	 */
	bool start_capture(std::filesystem::path path, EncoderType encoder_type, bool ask_for_encoding_settings = true);

	/**
	 * \brief Stops capturing a video
	 * \return Whether the operation was successful
	 */
	bool stop_capture();

	/**
	 * \brief Notifies the encoding manager of a new VI
	 */
	void at_vi();

	/**
	 * \brief Notifies the encoding manager of the audio changing
	 */
	void ai_len_changed();

	/**
	 * \brief Notifies the encoding manager of the DAC rate changing
	 * \param type The emulated system's typr
	 */
	void ai_dacrate_changed(system_type type);
}
