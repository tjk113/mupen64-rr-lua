#include <LuaConsole.h>

#include "../resource.h"
#include "../main/win/main_win.h"
#include "GameDebugger.h"
#include <stdio.h>
#include "vcr.h"
#include <Windows.h>
#include "../../r4300/r4300.h"
#include "../../memory/memory.h"
#include "disasm.h"

int gameDebuggerIsResumed = true;
int gameDebuggerIsDmaReadEnabled = true;
int gameDebuggerIsStepping = false;
UINT gameDebuggerTimerId;

DWORD (__cdecl*original_doRspCycles)(DWORD Cycles) = NULL;

// instead of implementing a dispatcher queue, we set this simple flag
BOOL gameDebuggerIsUpdateQueued = FALSE;
std::function<unsigned long(void)> gameDebuggerGetOpcode;
std::function<unsigned long(void)> gameDebuggerGetAddress;

struct
{
	unsigned long opcode;
	unsigned long address;
} typedef ProcessorState;

// TODO: make this a vector and push to it upon explicitly stepping over any cycles
ProcessorState gameDebuggerProcessorState;

void GameDebuggerUpdate(HWND hwnd)
{
	gameDebuggerProcessorState.opcode = gameDebuggerGetOpcode();
	gameDebuggerProcessorState.address = gameDebuggerGetAddress();

	char opcodeStr[255] = {0};
	sprintf(opcodeStr, "0x%lx", gameDebuggerProcessorState.opcode);

	char addressStr[255] = {0};
	sprintf(addressStr, "0x%lx", gameDebuggerProcessorState.address);

	char disassembledStr[255] = {0};
	DisassembleInstruction(disassembledStr, gameDebuggerProcessorState.opcode,
	                       gameDebuggerProcessorState.address);

	SetWindowText(GetDlgItem(hwnd, IDC_DEBUGGER_PRECOMPADDR), opcodeStr);
	SetWindowText(GetDlgItem(hwnd, IDC_DEBUGGER_PRECOMPOP), addressStr);
	SetWindowText(GetDlgItem(hwnd, IDC_DEBUGGER_DISASSEMBLED), disassembledStr);
	SetWindowText(GetDlgItem(hwnd, IDC_DEBUGGER_TOGGLEPAUSE),
	              gameDebuggerIsResumed ? "Pause" : "Resume");
}

VOID CALLBACK TimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                            DWORD dwTime)
{
	if (gameDebuggerIsUpdateQueued)
	{
		if (!IsDlgButtonChecked(hwnd, IDC_DEBUGGER_ALWAYS_UPDATE))
		{
			gameDebuggerIsUpdateQueued = false;
		}
		GameDebuggerUpdate(hwnd);
	}
}

BOOL CALLBACK GameDebuggerDialogProc(HWND hwnd, UINT Message, WPARAM wParam,
                                     LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hwnd, IDC_DEBUGGER_RSP_TOGGLE, 1);
		gameDebuggerIsUpdateQueued = true;
		gameDebuggerTimerId = SetTimer(hwnd, 1, 1000 / 60, TimerCallback);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case WM_DESTROY:
			KillTimer(hwnd, gameDebuggerTimerId);
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		case IDC_DEBUGGER_DMA_R_TOGGLE:
			gameDebuggerIsDmaReadEnabled = !IsDlgButtonChecked(
				hwnd, IDC_DEBUGGER_DMA_R_TOGGLE);
			break;
		case IDC_DEBUGGER_RSP_TOGGLE:

			// if real doRspCycles pointer hasnt been stored yet, cache it
			if (!original_doRspCycles)
			{
				original_doRspCycles = doRspCycles;
			}

		// if rsp is disabled, we swap out the real doRspCycles function for the dummy one, effectively disabling the rsp unit
			if (IsDlgButtonChecked(hwnd, IDC_DEBUGGER_RSP_TOGGLE))
				doRspCycles = original_doRspCycles;
			else
				doRspCycles = dummy_doRspCycles;

			break;
		case IDC_DEBUGGER_STEP:
			// set step flag and resume
			// the flag is cleared at the end of the current cycle
			gameDebuggerIsStepping = true;
			gameDebuggerIsResumed = true;
			break;
		case IDC_DEBUGGER_TOGGLEPAUSE:
			gameDebuggerIsResumed ^= true;
			gameDebuggerIsUpdateQueued = true;
			break;
		default:
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI GameDebuggerThread(LPVOID lpParam)
{
	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_GAMEDEBUGGERDIALOG), 0,
	          GameDebuggerDialogProc);
	ExitThread(0);
}


void GameDebuggerStart(std::function<unsigned long(void)> getOpcode,
                       std::function<unsigned long(void)> getAddress)
{
	gameDebuggerGetOpcode = getOpcode;
	gameDebuggerGetAddress = getAddress;
	DWORD gameDebuggerThreadId;
	CreateThread(NULL, 0, GameDebuggerThread, NULL, 0, &gameDebuggerThreadId);
}

void GameDebuggerOnLateCycle()
{
	if (gameDebuggerIsStepping)
	{
		gameDebuggerIsStepping = false;
		gameDebuggerIsResumed = false;
		gameDebuggerIsUpdateQueued = true;
	}
}
