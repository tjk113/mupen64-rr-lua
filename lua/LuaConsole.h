#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "luaDefine.h"


//識別子衝突対策
//本当はヘッダ分割すべきか
// ^ no
#include "../r4300/r4300.h"

#ifndef LUACONSOLE_H_NOINCLUDE_WINDOWS_H

#include <Windows.h>
#include <locale>
#include <iostream>
#include <string>
#include <sstream>
void NewLuaScript(void(*callback)());
void LuaWindowMessage(HWND, UINT, WPARAM, LPARAM);
#endif

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

//無理やりinline関数に
namespace LuaEngine {
	void PCBreak(void*, unsigned long);
	extern void* pcBreakMap_[0x800000 / 4];
}

inline void LuaPCBreakPure() {
	void* p = LuaEngine::pcBreakMap_[(interp_addr & 0x7FFFFF) >> 2];
	if (p)LuaEngine::PCBreak(p, interp_addr);
}
inline void LuaPCBreakInterp() {
	void* p = LuaEngine::pcBreakMap_[(PC->addr & 0x7FFFFF) >> 2];
	if (p)LuaEngine::PCBreak(p, PC->addr);
}

extern unsigned long lastInputLua[4];
extern unsigned long rewriteInputLua[4];
extern bool rewriteInputFlagLua[4];
extern bool enableTraceLog;
extern bool traceLogMode;
extern bool enablePCBreak;
extern bool maximumSpeedMode;
extern unsigned long gdiPlusToken;


// https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t
inline static std::wstring widen(const std::string& str) {
	std::wstring ws(str.size(), L' ');
	ws.resize(std::mbstowcs(&ws[0], str.c_str(), str.size()));
	return ws;
}

// goddamn core polluting with macros
#undef Index
// https://stackoverflow.com/a/7115547/14472122
#include <tuple>
namespace std {
	namespace {

		// Code from boost
		// Reciprocal of the golden ratio helps spread entropy
		//     and handles duplicates.
		// See Mike Seymour in magic-numbers-in-boosthash-combine:
		//     http://stackoverflow.com/questions/4948780

		template <class T>
		inline void hash_combine(std::size_t& seed, T const& v) {
			seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		// Recursive template code derived from Matthieu M.
		template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
		struct HashValueImpl {
			static void apply(size_t& seed, Tuple const& tuple) {
				HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
				hash_combine(seed, std::get<Index>(tuple));
			}
		};

		template <class Tuple>
		struct HashValueImpl<Tuple, 0> {
			static void apply(size_t& seed, Tuple const& tuple) {
				hash_combine(seed, std::get<0>(tuple));
			}
		};
	}

	template <typename ... TT>
	struct hash<std::tuple<TT...>> {
		size_t
			operator()(std::tuple<TT...> const& tt) const {
			size_t seed = 0;
			HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
			return seed;
		}

	};
}


#endif

//#endif
