#pragma once
#include <include/lua.h>
#include <Windows.h>

namespace LuaCore::Memory
{
	static DWORD LuaCheckIntegerU(lua_State* L, int i = -1)
	{
		return static_cast<DWORD>(luaL_checknumber(L, i));
	}

	static ULONGLONG LuaCheckQWord(lua_State* L, int i)
	{
		lua_pushinteger(L, 1);
		lua_gettable(L, i);
		ULONGLONG n = static_cast<ULONGLONG>(LuaCheckIntegerU(L)) << 32;
		lua_pop(L, 1);
		lua_pushinteger(L, 2);
		lua_gettable(L, i);
		n |= LuaCheckIntegerU(L);
		return n;
	}

	static void LuaPushQword(lua_State* L, ULONGLONG x)
	{
		lua_newtable(L);
		lua_pushinteger(L, 1);
		lua_pushinteger(L, x >> 32);
		lua_settable(L, -3);
		lua_pushinteger(L, 2);
		lua_pushinteger(L, x & 0xFFFFFFFF);
		lua_settable(L, -3);
	}

	//memory
	unsigned char* const rdramb = reinterpret_cast<unsigned char*>(rdram);
	constexpr unsigned long AddrMask = 0x7FFFFF;

	template <typename T>
	ULONG ToAddr(ULONG addr)
	{
		return sizeof(T) == 4
			       ? addr
			       : sizeof(T) == 2
			       ? addr ^ S16
			       : sizeof(T) == 1
			       ? addr ^ S8
			       : throw"ToAddr: sizeof(T)";
	}

	template <typename T>
	T LoadRDRAMSafe(unsigned long addr)
	{
		return *reinterpret_cast<T*>(rdramb + ((ToAddr<T>(addr) & AddrMask)));
	}

	template <typename T>
	void StoreRDRAMSafe(unsigned long addr, T value)
	{
		*reinterpret_cast<T*>(rdramb + ((ToAddr<T>(addr) & AddrMask))) = value;
	}

	// Read functions

	static int LuaReadByteUnsigned(lua_State* L)
	{
		UCHAR value = LoadRDRAMSafe<UCHAR>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadByteSigned(lua_State* L)
	{
		CHAR value = LoadRDRAMSafe<CHAR>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadWordUnsigned(lua_State* L)
	{
		USHORT value = LoadRDRAMSafe<USHORT>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadWordSigned(lua_State* L)
	{
		SHORT value = LoadRDRAMSafe<SHORT>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadDWorldUnsigned(lua_State* L)
	{
		ULONG value = LoadRDRAMSafe<ULONG>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadDWordSigned(lua_State* L)
	{
		LONG value = LoadRDRAMSafe<LONG>(luaL_checkinteger(L, 1));
		lua_pushinteger(L, value);
		return 1;
	}

	static int LuaReadQWordUnsigned(lua_State* L)
	{
		ULONGLONG value = LoadRDRAMSafe<ULONGLONG>(luaL_checkinteger(L, 1));
		LuaPushQword(L, value);
		return 1;
	}

	static int LuaReadQWordSigned(lua_State* L)
	{
		LONGLONG value = LoadRDRAMSafe<LONGLONG>(luaL_checkinteger(L, 1));
		LuaPushQword(L, value);
		return 1;
	}

	static int LuaReadFloat(lua_State* L)
	{
		ULONG value = LoadRDRAMSafe<ULONG>(luaL_checkinteger(L, 1));
		lua_pushnumber(L, *(FLOAT*)&value);
		return 1;
	}

	static int LuaReadDouble(lua_State* L)
	{
		ULONGLONG value = LoadRDRAMSafe<ULONGLONG>(luaL_checkinteger(L, 1));
		lua_pushnumber(L, *(DOUBLE*)value);
		return 1;
	}

	// Write functions

	static int LuaWriteByteUnsigned(lua_State* L)
	{
		StoreRDRAMSafe<UCHAR>(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
		return 0;
	}

	static int LuaWriteWordUnsigned(lua_State* L)
	{
		StoreRDRAMSafe<USHORT>(luaL_checkinteger(L, 1),
		                       luaL_checkinteger(L, 2));
		return 0;
	}

	static int LuaWriteDWordUnsigned(lua_State* L)
	{
		StoreRDRAMSafe<ULONG>(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
		return 0;
	}

	static int LuaWriteQWordUnsigned(lua_State* L)
	{
		StoreRDRAMSafe<ULONGLONG>(luaL_checkinteger(L, 1), LuaCheckQWord(L, 2));
		return 0;
	}

	static int LuaWriteFloatUnsigned(lua_State* L)
	{
		FLOAT f = luaL_checknumber(L, -1);
		StoreRDRAMSafe<ULONG>(luaL_checkinteger(L, 1), *(ULONG*)&f);
		return 0;
	}

	static int LuaWriteDoubleUnsigned(lua_State* L)
	{
		DOUBLE f = luaL_checknumber(L, -1);
		StoreRDRAMSafe<ULONGLONG>(luaL_checkinteger(L, 1), *(ULONGLONG*)&f);
		return 0;
	}

	static int LuaReadSize(lua_State* L)
	{
		const ULONG addr = luaL_checkinteger(L, 1);
		switch (luaL_checkinteger(L, 2))
		{
		// unsigned
		case 1: lua_pushinteger(L, LoadRDRAMSafe<UCHAR>(addr));
			break;
		case 2: lua_pushinteger(L, LoadRDRAMSafe<USHORT>(addr));
			break;
		case 4: lua_pushinteger(L, LoadRDRAMSafe<ULONG>(addr));
			break;
		case 8: LuaPushQword(L, LoadRDRAMSafe<ULONGLONG>(addr));
			break;
		// signed
		case -1: lua_pushinteger(L, LoadRDRAMSafe<CHAR>(addr));
			break;
		case -2: lua_pushinteger(L, LoadRDRAMSafe<SHORT>(addr));
			break;
		case -4: lua_pushinteger(L, LoadRDRAMSafe<LONG>(addr));
			break;
		case -8: LuaPushQword(L, LoadRDRAMSafe<LONGLONG>(addr));
			break;
		default: luaL_error(L, "size must be 1, 2, 4, 8, -1, -2, -4, -8");
		}
		return 1;
	}

	static int LuaWriteSize(lua_State* L)
	{
		ULONG addr = luaL_checkinteger(L, 1);
		switch (luaL_checkinteger(L, 2))
		{
		case 1: StoreRDRAMSafe<UCHAR>(addr, luaL_checkinteger(L, 3));
			break;
		case 2: StoreRDRAMSafe<USHORT>(addr, luaL_checkinteger(L, 3));
			break;
		case 4: StoreRDRAMSafe<ULONG>(addr, luaL_checkinteger(L, 3));
			break;
		case 8: StoreRDRAMSafe<ULONGLONG>(addr, LuaCheckQWord(L, 3));
			break;
		case -1: StoreRDRAMSafe<CHAR>(addr, luaL_checkinteger(L, 3));
			break;
		case -2: StoreRDRAMSafe<SHORT>(addr, luaL_checkinteger(L, 3));
			break;
		case -4: StoreRDRAMSafe<LONG>(addr, luaL_checkinteger(L, 3));
			break;
		case -8: StoreRDRAMSafe<LONGLONG>(addr, LuaCheckQWord(L, 3));
			break;
		default: luaL_error(L, "size must be 1, 2, 4, 8, -1, -2, -4, -8");
		}
		return 0;
	}
	static int LuaIntToFloat(lua_State* L)
	{
		ULONG n = luaL_checknumber(L, 1);
		lua_pushnumber(L, *(FLOAT*)&n);
		return 1;
	}

	static int LuaIntToDouble(lua_State* L)
	{
		ULONGLONG n = LuaCheckQWord(L, 1);
		lua_pushnumber(L, *(DOUBLE*)&n);
		return 1;
	}

	static int LuaFloatToInt(lua_State* L)
	{
		FLOAT n = luaL_checknumber(L, 1);
		lua_pushinteger(L, *(ULONG*)&n);
		return 1;
	}

	static int LuaDoubleToInt(lua_State* L)
	{
		DOUBLE n = luaL_checknumber(L, 1);
		LuaPushQword(L, *(ULONGLONG*)&n);
		return 1;
	}

	static int LuaQWordToNumber(lua_State* L)
	{
		ULONGLONG n = LuaCheckQWord(L, 1);
		lua_pushnumber(L, n);
		return 1;
	}
	template <typename T>
	void PushT(lua_State* L, T value)
	{
		LuaPushIntU(L, value);
	}

	template <>
	inline void PushT<ULONGLONG>(lua_State* L, ULONGLONG value)
	{
		LuaPushQword(L, value);
	}

	template <typename T, void(**readmem_func)()>
	static int ReadMemT(lua_State* L)
	{
		ULONGLONG *rdword_s = rdword, tmp, address_s = address;
		address = LuaCheckIntegerU(L, 1);
		rdword = &tmp;
		readmem_func[address >> 16]();
		PushT<T>(L, tmp);
		rdword = rdword_s;
		address = address_s;
		return 1;
	}

	template <typename T>
	T CheckT(lua_State* L, int i)
	{
		return LuaCheckIntegerU(L, i);
	}

	template <>
	inline ULONGLONG CheckT<ULONGLONG>(lua_State* L, int i)
	{
		return LuaCheckQWord(L, i);
	}

	template <typename T, void(**writemem_func)(), T& g_T>
	static int WriteMemT(lua_State* L)
	{
		ULONGLONG *rdword_s = rdword, address_s = address;
		T g_T_s = g_T;
		address = LuaCheckIntegerU(L, 1);
		g_T = CheckT<T>(L, 2);
		writemem_func[address >> 16]();
		address = address_s;
		g_T = g_T_s;
		return 0;
	}
}
