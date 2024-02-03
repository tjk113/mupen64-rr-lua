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
	};

	t_record_params show();
}
