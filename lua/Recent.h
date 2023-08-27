#pragma once

#include <windows.h>
#ifdef __cplusplus
#include <string>
#endif

#define LUA_MAX_RECENT 5

void lua_recent_scripts_build();
void lua_recent_scripts_reset();
void lua_recent_scripts_add(std::string path);
int32_t lua_recent_scripts_run(uint16_t menu_item_id);
