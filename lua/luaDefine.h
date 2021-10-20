#ifndef LUA_DEFINE_H
#define LUA_DEFINE_H

// Comment this define to exclude LUA extension from compile
#define LUA_MODULEIMPL

#ifdef WIN32

#ifdef LUA_MODULEIMPL

#define LUA_TRACEINTERP
#define LUA_GUI
#define LUA_SPEEDMODE
#define LUA_JOYPAD

#define LUA_BREAKPOINTSYNC_PURE
#define LUA_BREAKPOINTSYNC_INTERP
#define LUA_PCBREAK_PURE
#define LUA_PCBREAK_INTERP
#define LUA_TRACELOG
#define LUA_TRACEPURE

#define LUA_EMUPAUSED_WORK
#define LUA_WINDOWMESSAGE

#define LUA_LIB


#endif

#endif

#endif
