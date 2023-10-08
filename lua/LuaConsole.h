#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "luaDefine.h"

#include "include/lua.hpp"

#include <Windows.h>
#include <locale>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <cstdint>
#include <gdiplus.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <functional>
#include <queue>
#include <md5.h>
#include <assert.h>

void NewLuaScript(void (*callback)());
void LuaWindowMessage(HWND, UINT, WPARAM, LPARAM);

void LuaReload();
void LuaOpenAndRun(const char* path);
void CloseAllLuaScript();
void AtUpdateScreenLuaCallback();
void AtVILuaCallback();
void LuaProcessMessages();
void AtInputLuaCallback(int n);
void AtIntervalLuaCallback();
void AtPlayMovieLuaCallback();
void AtStopMovieLuaCallback();
void AtLoadStateLuaCallback();
void AtSaveStateLuaCallback();
void AtResetCallback();

void LuaBreakpointSyncPure();
void LuaBreakpointSyncInterp();
void lua_new_vi(int redraw);
void LuaTraceLoggingPure();
void LuaTraceLoggingInterpOps();
void LuaTraceLogState();

void instrStr1(unsigned long pc, unsigned long w, char* buffer);
void instrStr2(unsigned long pc, unsigned long w, char* buffer);

static uint32_t bitmap_color_mask = RGB(255, 0, 255);

enum class Renderer {
	None,
	GDIMixed,
	Direct2D
};

class LuaEnvironment {
private:
	bool stopping = false;
	Renderer renderer = Renderer::None;

public:
	HDC dc = nullptr;
	int dc_width, dc_height = 0;
	ID2D1Factory* d2d_factory = nullptr;
	ID2D1DCRenderTarget* d2d_render_target = nullptr;
	IDWriteFactory* dw_factory = nullptr;
	std::unordered_map<uint32_t, ID2D1SolidColorBrush*> d2d_brush_cache;
	std::unordered_map<std::string, ID2D1Bitmap*> d2d_bitmap_cache;

	LuaEnvironment(HWND wnd);
	~LuaEnvironment();

	Renderer get_renderer() { return renderer; }
	void create_renderer(Renderer renderer, int32_t override_identical_check = 0);
	void destroy_renderer();


	void draw();

	bool run(char* path);
	void stop();
	void setBrush(HBRUSH h);
	void selectBrush();
	void setPen(HPEN h);
	void selectPen();
	void setFont(HFONT h);
	void selectFont();
	void setTextColor(COLORREF c);
	void selectTextColor();
	void setBackgroundColor(COLORREF c, int mode = OPAQUE);
	void selectBackgroundColor();
	bool isrunning();
	//calls all functions that lua script has defined as callbacks, reads them from registry
	//returns true at fail
	bool invoke_callbacks_with_key(std::function<int(lua_State*)> function,
								   const char* key);
	HWND hwnd;
	HANDLE hMutex;

private:
	void newLuaState();
	void deleteLuaState();
	void registerAsPackage(lua_State* L, const char* name,
						   const luaL_Reg reg[]);
	void registerFunctions();
	bool runFile(char* path);
	void error();
	void setGDIObject(HGDIOBJ* save, HGDIOBJ newobj);
	void selectGDIObject(HGDIOBJ p);
	void deleteGDIObject(HGDIOBJ p, int stockobj);
	lua_State* L;
	// gdi objects are filled in on run(), then deleted on stop()
	HBRUSH brush;
	HPEN pen;
	HFONT font;
	COLORREF col, bkcol;
	int bkmode;
};

extern unsigned long lastInputLua[4];
extern unsigned long rewriteInputLua[4];
extern bool rewriteInputFlagLua[4];
extern bool enableTraceLog;
extern bool traceLogMode;
extern bool maximumSpeedMode;
extern unsigned long gdiPlusToken;

extern std::map<HWND, LuaEnvironment*> hwnd_lua_map;


// https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t
inline static std::wstring widen(const std::string& str) {
	std::wstring ws(str.size(), L' ');
	ws.resize(std::mbstowcs(&ws[0], str.c_str(), str.size()));
	return ws;
}


#endif
