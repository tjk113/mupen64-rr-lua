#pragma once

#define MAX_RECENT 10

#ifdef __cplusplus
#include <string>
#endif

void lua_recent_scripts_build(int32_t reset = 0);
void lua_recent_scripts_add(const std::string& path);
int32_t lua_recent_scripts_run(uint16_t menu_item_id);
