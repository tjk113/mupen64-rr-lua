#include "debugger.h"

bool resumed = true;

// TODO: Implement the rest

bool Debugger::is_resumed()
{
	return resumed;
}

void Debugger::set_is_resumed(bool value)
{
	resumed = value;
}

void Debugger::on_late_cycle(unsigned long opcode, unsigned long address)
{
}
