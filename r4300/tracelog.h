#pragma once

// TODO: Decouple from michaelsoft
#include <Windows.h>

#include "disasm.h"

namespace tracelog
{
	bool active();
	void TraceLogStart(const char* name, BOOL append = FALSE);
	void TraceLogStop();
	void LuaTraceLoggingInterpOps();
	void LuaTraceLoggingPure();
	void TraceLoggingBin(r4300word pc, r4300word w);
	void TraceLoggingBufFlush();
	void TraceLoggingWriteBuf();
	void TraceLogging(r4300word pc, r4300word w);
	void LuaTraceLogState();
}
