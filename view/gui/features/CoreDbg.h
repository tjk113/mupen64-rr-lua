#pragma once

namespace CoreDbg
{
	/**
	 * \brief Starts the core debugger
	 */
	void start();

	/**
	 * \brief Notifies the debugger of a processor cycle ending
	 * \param opcode The processor's opcode
	 * \param address The processor's address
	 */
	void on_late_cycle(unsigned long opcode, unsigned long address);
}
