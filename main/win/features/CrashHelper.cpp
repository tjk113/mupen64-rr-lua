#include "CrashHelper.h"
#include "../main_win.h"
#include <vcr.h>
#include <Psapi.h>
#include "../r4300/r4300.h"
#include "../Config.hpp"


int CrashHelper::find_module_name(char* error, void* addr, const int len)
{
	const HANDLE h_process = GetCurrentProcess();
	DWORD cb_needed;
	printf("addr: %p\n", addr);
	if (HMODULE h_mods[1024]; EnumProcessModules(h_process, h_mods, sizeof(h_mods), &cb_needed))
	{
		HMODULE maxbase = nullptr;
		for (int i = 0; i < (int)(cb_needed / sizeof(HMODULE)); i++)
		{
			//find closest addr
			if (h_mods[i] > maxbase && h_mods[i] < addr)
			{
				maxbase = h_mods[i];
				char modname[MAX_PATH];
				GetModuleBaseName(h_process, maxbase, modname,
				                  sizeof(modname) / sizeof(char));
				printf("%s: %p\n", modname, (void*)maxbase);
			}
		}
		// Get the full path to the module's file.
		char modname[MAX_PATH];
		if (GetModuleBaseName(h_process, maxbase, modname,
		                      sizeof(modname) / sizeof(char)))
			// write the address with module
			return sprintf(error + len, "Addr:0x%p (%s 0x%p)\n", addr, modname,
			               (void*)maxbase);
	}
	return 0; //what
}

void CrashHelper::get_exception_code_friendly_name(
	const _EXCEPTION_POINTERS* exception_pointers_ptr, char* exception_code_string_ptr)
{
	switch (exception_pointers_ptr->ExceptionRecord->ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		strcpy(exception_code_string_ptr, "Access violation");
		break;
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		strcpy(exception_code_string_ptr,
		       "Attempted to access out-of-bounds array index");
		break;
	case EXCEPTION_FLT_DENORMAL_OPERAND:
		strcpy(exception_code_string_ptr,
		       "An operand in floating point operation was denormal");
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		strcpy(exception_code_string_ptr, "Float division by zero");
		break;
	case EXCEPTION_STACK_OVERFLOW:
		strcpy(exception_code_string_ptr, "Stack overflow");
		break;
	default:
		sprintf(exception_code_string_ptr, "%d",
		        (int)exception_pointers_ptr->ExceptionRecord->ExceptionCode);
		break;
	}
}


void CrashHelper::generate_log(const _EXCEPTION_POINTERS* exception_pointers_ptr,
                               char* log_ptr)
{
	int len = 0;

	if (exception_pointers_ptr != nullptr)
	{
		void* addr = exception_pointers_ptr->ExceptionRecord->ExceptionAddress;
		len += find_module_name(log_ptr, addr, len); //appends to error as well

		//emu info
#ifdef _DEBUG
		len += sprintf(log_ptr + len, "Version:" MUPEN_VERSION " DEBUG\n");
#else
		len += sprintf(log_ptr + len, "Version:" MUPEN_VERSION "\n");
#endif
		char exception_code_friendly[1024]{};
		get_exception_code_friendly_name(exception_pointers_ptr,
		                             exception_code_friendly);
		len += sprintf(log_ptr + len, "Exception code: %s (0x%08x)\n",
		               exception_code_friendly,
		               (int)exception_pointers_ptr->ExceptionRecord->
		                                          ExceptionCode);
	} else
	{
		//emu info
#ifdef _DEBUG
		len += sprintf(log_ptr + len, "Version:" MUPEN_VERSION " DEBUG\n");
#else
		len += sprintf(log_ptr + len, "Version:" MUPEN_VERSION "\n");
#endif
		len += sprintf(log_ptr + len,
		               "Exception code: unknown (no exception thrown, was crash log called manually?)\n");
	}
	len += sprintf(log_ptr + len, "Gfx:%s\n",
	               Config.selected_video_plugin.c_str());
	len += sprintf(log_ptr + len, "Input:%s\n",
	               Config.selected_input_plugin.c_str());
	len += sprintf(log_ptr + len, "Audio:%s\n",
	               Config.selected_audio_plugin.c_str());
	len += sprintf(log_ptr + len, "rsp:%s\n",
	               Config.selected_rsp_plugin.c_str());
	//some flags
	len += sprintf(log_ptr + len, "m_task:%d\n", m_task);
	len += sprintf(log_ptr + len, "emu_launched:%d\n", emu_launched);
	len += sprintf(log_ptr + len, "is_capturing_avi:%d\n", vcr_is_capturing());

	strcpy(log_ptr, log_ptr); // ????
}
