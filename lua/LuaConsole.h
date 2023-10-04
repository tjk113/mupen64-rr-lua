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

static uint32_t d2d_color_mask = RGB(1, 0, 1);


enum class Renderer {
	None,
	GDIMixed,
	Direct2D
};

void PCBreak(void*, unsigned long);
extern void* pcBreakMap_[0x800000 / 4];


class LuaEnvironment {
private:
	bool stopping = false;
	Renderer renderer = Renderer::None;

public:
	uint64_t draw_call_count = 0;
	HDC dc = NULL;
	int dc_width, dc_height = NULL;
	ID2D1Factory* d2d_factory = NULL;
	ID2D1DCRenderTarget* d2d_render_target = NULL;
	IDWriteFactory* dw_factory = NULL;
	std::unordered_map<uint32_t, ID2D1SolidColorBrush*> d2d_brush_cache;
	std::unordered_map<std::string, ID2D1Bitmap*> d2d_bitmap_cache;
	Renderer get_renderer() { return renderer; };
	void create_renderer(Renderer renderer);
	void destroy_renderer();
	void early_draw();
	void draw();
	LuaEnvironment(HWND wnd);
	~LuaEnvironment();
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
	bool errorPCall(int a, int r);
	HWND ownWnd;
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
	template <typename T>
	void correctBreakMap(T& breakMap,
						 typename T::iterator(*removeFunc)(
							 typename T::iterator));
	void correctPCBreak();
	void correctData();
	lua_State* L;
		// gdi objects are filled in on run(), then deleted on stop()
	HBRUSH brush;
	HPEN pen;
	HFONT font;
	COLORREF col, bkcol;
	int bkmode;
};

void LuaPCBreakPure();
void LuaPCBreakInterp();
extern unsigned long lastInputLua[4];
extern unsigned long rewriteInputLua[4];
extern bool rewriteInputFlagLua[4];
extern bool enableTraceLog;
extern bool traceLogMode;
extern bool enablePCBreak;
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
