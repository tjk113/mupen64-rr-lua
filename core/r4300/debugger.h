#pragma once

namespace Debugger
{
	/**
	 * \brief Gets whether execution is resumed
	 */
	bool get_resumed();

	/**
	 * \brief Sets execution resumed status
	 */
	void set_is_resumed(bool);

	/**
	 * \brief Gets whether DMA reads are allowed. If false, reads should return 0xFF
	 */
	bool get_dma_read_enabled();

	/**
	 * \brief Sets whether DMA reads are allowed
	 */
	void set_dma_read_enabled(bool);

	/**
	 * \brief Notifies the debugger of a processor cycle ending
	 * \param opcode The processor's opcode
	 * \param address The processor's address
	 */
	void on_late_cycle(unsigned long opcode, unsigned long address);
}
