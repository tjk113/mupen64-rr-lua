#pragma once

#include <filesystem>

namespace MovieDialog
{
	struct t_record_params
	{
		std::filesystem::path path;
		unsigned short start_flag;
		std::string author;
		std::string description;
		int32_t pause_at;
		int32_t pause_at_last;
	};

	/**
	 * \brief Shows a movie inspector dialog
	 * \param readonly Whether movie properties can't be edited
	 * \return The user-chosen parameters
	 */
	t_record_params show(bool readonly);
}
