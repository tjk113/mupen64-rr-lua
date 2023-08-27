#pragma once
#include <windows.h>
#define LUA_MAX_RECENT 5
//functions
void AddToRecentScripts(char* path);
void BuildRecentScriptsMenu(HWND);
void RunRecentScript(WORD);
void ClearRecent(BOOL);
void EnableRecentScriptsMenu(HMENU hMenu, BOOL flag);
