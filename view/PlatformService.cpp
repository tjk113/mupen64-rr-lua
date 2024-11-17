#include <shared/services/PlatformService.h>
#include <core/r4300/Plugin.hpp>
#include <Windows.h>
#include <DbgHelp.h>

void (* PlatformService::get_free_function_in_module(void* module))(void*)
{
    auto dll_crt_free = (DLLCRTFREE)GetProcAddress((HMODULE)module, "DllCrtFree");
    if (dll_crt_free != nullptr) return dll_crt_free;

    ULONG size;
    auto import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)
        ImageDirectoryEntryToDataEx(module, true, IMAGE_DIRECTORY_ENTRY_IMPORT,
                                    &size, nullptr);
    if (import_descriptor != nullptr)
    {
        while (import_descriptor->Characteristics && import_descriptor->Name)
        {
            auto importDllName = (LPCSTR)((PBYTE)module + import_descriptor->
                Name);
            auto importDllHandle = GetModuleHandleA(importDllName);
            if (importDllHandle != nullptr)
            {
                dll_crt_free = (DLLCRTFREE)GetProcAddress(
                    importDllHandle, "free");
                if (dll_crt_free != nullptr) return dll_crt_free;
            }

            import_descriptor++;
        }
    }


    // this is probably always wrong
    return free;
}
