#pragma once

#include "disasm.h"

namespace tracelog
{

	/**
	 * \return Whether the trace logger is active
	 */
	bool active();

	/**
	 * \brief Starts trace logging to the specified file
	 * \param name The output path
	 * \param append Whether log output will be appended to the file
	 */
	void start(const char* name, bool append = false);

	/**
	 * \brief Stops trace logging
	 */
	void stop();

	/**
	 * \brief Logs a dynarec-generated instruction
	 */
	void log_interp_ops();

	/**
	 * \brief Logs a pure interp instruction
	 */
	void log_pure();
}
