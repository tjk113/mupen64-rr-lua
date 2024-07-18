#include "debugger.h"

bool resumed = true;
bool dma_read_enabled = true;

// TODO: Implement the rest

bool Debugger::get_resumed()
{
	return resumed;
}

void Debugger::set_is_resumed(bool value)
{
	resumed = value;
}

bool Debugger::get_dma_read_enabled()
{
	return dma_read_enabled;
}

void Debugger::set_dma_read_enabled(bool value)
{
	dma_read_enabled = value;
}

void Debugger::on_late_cycle(unsigned long opcode, unsigned long address)
{
}
