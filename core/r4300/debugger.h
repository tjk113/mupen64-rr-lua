#pragma once

namespace Debugger
{
	/**
	 * \brief Gets whether execution is resumed
	 */
	bool is_resumed();

	/**
	 * \brief Sets execution resumed status
	 */
	void set_is_resumed(bool);

	/**
	 * \brief Notifies the debugger of a processor cycle ending
	 * \param opcode The processor's opcode
	 * \param address The processor's address
	 */
	void on_late_cycle(unsigned long opcode, unsigned long address);
}
