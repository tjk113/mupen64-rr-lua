#pragma once
#ifdef _WIN32
#include <windows.h> //linux trolled
#endif
#define LUA_MAX_RECENT 5

//functions
void AddToRecentScripts(char* path);
void BuildRecentScriptsMenu(HWND);
void RunRecentScript(WORD);
void ClearRecent(HWND, BOOL);
