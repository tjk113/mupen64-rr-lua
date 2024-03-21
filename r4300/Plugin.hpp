#pragma once
/**
 * Mupen64 - Plugin.hpp
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <functional>
#include <helpers/io_helpers.h>

enum plugin_type
{
	video = 2,
	audio = 3,
	input = 4,
	rsp = 1,
};

enum controller_extension
{
	none = 1,
	mempak = 2,
	rumblepak = 3,
	transferpak = 4,
	raw = 5
};

enum system_type
{
	ntsc,
	pal,
	mpal
};

// ReSharper disable CppInconsistentNaming

typedef struct
{
	unsigned short Version;
	unsigned short Type;
	char Name[100];

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	int NormalMemory; /* a normal unsigned char array */
	int MemoryBswaped; /* a normal unsigned char array where the memory has been pre
							 bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef struct
{
	void* hInst;
	// Whether the memory has been pre-byteswapped on a unsigned long boundary
	int MemoryBswaped;
	unsigned char* RDRAM;
	unsigned char* DMEM;
	unsigned char* IMEM;

	unsigned long* MI_INTR_REG;

	unsigned long* SP_MEM_ADDR_REG;
	unsigned long* SP_DRAM_ADDR_REG;
	unsigned long* SP_RD_LEN_REG;
	unsigned long* SP_WR_LEN_REG;
	unsigned long* SP_STATUS_REG;
	unsigned long* SP_DMA_FULL_REG;
	unsigned long* SP_DMA_BUSY_REG;
	unsigned long* SP_PC_REG;
	unsigned long* SP_SEMAPHORE_REG;

	unsigned long* DPC_START_REG;
	unsigned long* DPC_END_REG;
	unsigned long* DPC_CURRENT_REG;
	unsigned long* DPC_STATUS_REG;
	unsigned long* DPC_CLOCK_REG;
	unsigned long* DPC_BUFBUSY_REG;
	unsigned long* DPC_PIPEBUSY_REG;
	unsigned long* DPC_TMEM_REG;

	void (__cdecl*CheckInterrupts)(void);
	void (__cdecl*ProcessDlistList)(void);
	void (__cdecl*ProcessAlistList)(void);
	void (__cdecl*ProcessRdpList)(void);
	void (__cdecl*ShowCFB)(void);
} RSP_INFO;

typedef struct
{
	void* hWnd; /* Render window */
	void* hStatusBar;
	/* if render window does not have a status bar then this is NULL */

	int MemoryBswaped; // If this is set to TRUE, then the memory has been pre
	//   bswap on a dword (32 bits) boundry
	//	eg. the first 8 bytes are stored like this:
	//        4 3 2 1   8 7 6 5

	unsigned char* HEADER; // This is the rom header (first 40h bytes of the rom
	// This will be in the same memory format as the rest of the memory.
	unsigned char* RDRAM;
	unsigned char* DMEM;
	unsigned char* IMEM;

	unsigned long* MI_INTR_REG;

	unsigned long* DPC_START_REG;
	unsigned long* DPC_END_REG;
	unsigned long* DPC_CURRENT_REG;
	unsigned long* DPC_STATUS_REG;
	unsigned long* DPC_CLOCK_REG;
	unsigned long* DPC_BUFBUSY_REG;
	unsigned long* DPC_PIPEBUSY_REG;
	unsigned long* DPC_TMEM_REG;

	unsigned long* VI_STATUS_REG;
	unsigned long* VI_ORIGIN_REG;
	unsigned long* VI_WIDTH_REG;
	unsigned long* VI_INTR_REG;
	unsigned long* VI_V_CURRENT_LINE_REG;
	unsigned long* VI_TIMING_REG;
	unsigned long* VI_V_SYNC_REG;
	unsigned long* VI_H_SYNC_REG;
	unsigned long* VI_LEAP_REG;
	unsigned long* VI_H_START_REG;
	unsigned long* VI_V_START_REG;
	unsigned long* VI_V_BURST_REG;
	unsigned long* VI_X_SCALE_REG;
	unsigned long* VI_Y_SCALE_REG;

	void (__cdecl*CheckInterrupts)(void);
} GFX_INFO;

typedef struct
{
	void* hwnd;
	void* hinst;

	int MemoryBswaped; // If this is set to TRUE, then the memory has been pre
	//   bswap on a dword (32 bits) boundry
	//	eg. the first 8 bytes are stored like this:
	//        4 3 2 1   8 7 6 5
	unsigned char* HEADER; // This is the rom header (first 40h bytes of the rom
	// This will be in the same memory format as the rest of the memory.
	unsigned char* RDRAM;
	unsigned char* DMEM;
	unsigned char* IMEM;

	unsigned long* MI_INTR_REG;

	unsigned long* AI_DRAM_ADDR_REG;
	unsigned long* AI_LEN_REG;
	unsigned long* AI_CONTROL_REG;
	unsigned long* AI_STATUS_REG;
	unsigned long* AI_DACRATE_REG;
	unsigned long* AI_BITRATE_REG;

	void (__cdecl*CheckInterrupts)(void);
} AUDIO_INFO;

typedef struct
{
	int Present;
	int RawData;
	int Plugin;
} CONTROL;

typedef union
{
	unsigned long Value;

	struct
	{
		unsigned R_DPAD : 1;
		unsigned L_DPAD : 1;
		unsigned D_DPAD : 1;
		unsigned U_DPAD : 1;
		unsigned START_BUTTON : 1;
		unsigned Z_TRIG : 1;
		unsigned B_BUTTON : 1;
		unsigned A_BUTTON : 1;

		unsigned R_CBUTTON : 1;
		unsigned L_CBUTTON : 1;
		unsigned D_CBUTTON : 1;
		unsigned U_CBUTTON : 1;
		unsigned R_TRIG : 1;
		unsigned L_TRIG : 1;
		unsigned Reserved1 : 1;
		unsigned Reserved2 : 1;

		signed Y_AXIS : 8;

		signed X_AXIS : 8;
	};
} BUTTONS;

typedef struct
{
	void* hMainWindow;
	void* hinst;

	int MemoryBswaped; // If this is set to TRUE, then the memory has been pre
	//   bswap on a dword (32 bits) boundry, only effects header.
	//	eg. the first 8 bytes are stored like this:
	//        4 3 2 1   8 7 6 5
	unsigned char* HEADER; // This is the rom header (first 40h bytes of the rom)
	CONTROL* Controls; // A pointer to an array of 4 controllers .. eg:
	// CONTROL Controls[4];
} CONTROL_INFO;

static unsigned long __cdecl dummy_doRspCycles(unsigned long Cycles) { return Cycles; };

extern CONTROL Controls[4];


#pragma region Base Functions

typedef void (__cdecl*GETDLLINFO)(PLUGIN_INFO*);
typedef void (__cdecl*DLLCONFIG)(void*);
typedef void (__cdecl*DLLTEST)(void*);
typedef void (__cdecl*DLLABOUT)(void*);
extern GETDLLINFO getDllInfo;
extern DLLCONFIG dllConfig;
extern DLLTEST dllTest;
extern DLLABOUT dllAbout;

#pragma endregion

#pragma region Video Functions

typedef void (__cdecl*CHANGEWINDOW)();
typedef void (__cdecl*CLOSEDLL_GFX)();
typedef int  (__cdecl*INITIATEGFX)(GFX_INFO);
typedef void (__cdecl*PROCESSDLIST)();
typedef void (__cdecl*PROCESSRDPLIST)();
typedef void (__cdecl*ROMCLOSED_GFX)();
typedef void (__cdecl*ROMOPEN_GFX)();
typedef void (__cdecl*SHOWCFB)();
typedef void (__cdecl*UPDATESCREEN)();
typedef void (__cdecl*VISTATUSCHANGED)();
typedef void (__cdecl*VIWIDTHCHANGED)();
typedef void (__cdecl*READSCREEN)(void**, long*, long*);
typedef void (__cdecl*DLLCRTFREE)(void*);
typedef void (__cdecl*MOVESCREEN)(int, int);
typedef void (__cdecl*CAPTURESCREEN)(char*);
typedef void (__cdecl*GETVIDEOSIZE)(long*, long*);
typedef void (__cdecl*READVIDEO)(void**);
typedef void (__cdecl*FBREAD)(unsigned long);
typedef void (__cdecl*FBWRITE)(unsigned long addr, unsigned long size);
typedef void (__cdecl*FBGETFRAMEBUFFERINFO)(void*);
extern CHANGEWINDOW changeWindow;
extern CLOSEDLL_GFX closeDLL_gfx;
extern INITIATEGFX initiateGFX;
extern PROCESSDLIST processDList;
extern PROCESSRDPLIST processRDPList;
extern ROMCLOSED_GFX romClosed_gfx;
extern ROMOPEN_GFX romOpen_gfx;
extern SHOWCFB showCFB;
extern UPDATESCREEN updateScreen;
extern VISTATUSCHANGED viStatusChanged;
extern VIWIDTHCHANGED viWidthChanged;
extern READSCREEN readScreen;
extern DLLCRTFREE DllCrtFree;
extern MOVESCREEN moveScreen;
extern CAPTURESCREEN CaptureScreen;
// Mupen Graphics Extension, currently only implemented by bettergln64/ngl64
extern GETVIDEOSIZE get_video_size;
extern READVIDEO read_video;
// Framebuffer Extension
extern FBREAD fBRead;
extern FBWRITE fBWrite;
extern FBGETFRAMEBUFFERINFO fBGetFrameBufferInfo;

#pragma endregion

#pragma region Audio Functions

typedef void 		  (__cdecl*AIDACRATECHANGED)(int system_type);
typedef void 		  (__cdecl*AILENCHANGED)();
typedef unsigned long (__cdecl*AIREADLENGTH)();
typedef void 		  (__cdecl*CLOSEDLL_AUDIO)();
typedef int 		  (__cdecl*INITIATEAUDIO)(AUDIO_INFO);
typedef void 		  (__cdecl*PROCESSALIST)();
typedef void 		  (__cdecl*ROMCLOSED_AUDIO)();
typedef void 		  (__cdecl*ROMOPEN_AUDIO)();
typedef void 		  (__cdecl*AIUPDATE)(int wait);
extern AIDACRATECHANGED aiDacrateChanged;
extern AILENCHANGED aiLenChanged;
extern AIREADLENGTH aiReadLength;
extern CLOSEDLL_AUDIO closeDLL_audio;
extern INITIATEAUDIO initiateAudio;
extern PROCESSALIST processAList;
extern ROMCLOSED_AUDIO romClosed_audio;
extern ROMOPEN_AUDIO romOpen_audio;
extern AIUPDATE aiUpdate;

#pragma endregion

#pragma region Input Functions

typedef void  (__cdecl*CLOSEDLL_INPUT)();
typedef void  (__cdecl*CONTROLLERCOMMAND)(int controller, unsigned char* command);
typedef void  (__cdecl*GETKEYS)(int controller, BUTTONS* keys);
typedef void  (__cdecl*SETKEYS)(int controller, BUTTONS keys);
typedef void  (__cdecl*OLD_INITIATECONTROLLERS)(void* hwnd, CONTROL controls[4]);
typedef void  (__cdecl*INITIATECONTROLLERS)(CONTROL_INFO control_info);
typedef void  (__cdecl*READCONTROLLER)(int controller, unsigned char* command);
typedef void  (__cdecl*ROMCLOSED_INPUT)();
typedef void  (__cdecl*ROMOPEN_INPUT)();
typedef void  (__cdecl*KEYDOWN)(unsigned int wParam, long lParam);
typedef void  (__cdecl*KEYUP)(unsigned int wParam, long lParam);
extern CLOSEDLL_INPUT closeDLL_input;
extern CONTROLLERCOMMAND controllerCommand;
extern GETKEYS getKeys;
extern SETKEYS setKeys;
extern OLD_INITIATECONTROLLERS old_initiateControllers;
extern INITIATECONTROLLERS initiateControllers;
extern READCONTROLLER readController;
extern ROMCLOSED_INPUT romClosed_input;
extern ROMOPEN_INPUT romOpen_input;
extern KEYDOWN keyDown;
extern KEYUP keyUp;

#pragma endregion

#pragma region RSP Functions

typedef void		   (__cdecl*CLOSEDLL_RSP)();
typedef unsigned long  (__cdecl*DORSPCYCLES)(unsigned long);
typedef void		   (__cdecl*INITIATERSP)(RSP_INFO rsp_info, unsigned long* cycles);
typedef void		   (__cdecl*ROMCLOSED_RSP)();
extern CLOSEDLL_RSP closeDLL_RSP;
extern DORSPCYCLES doRspCycles;
extern INITIATERSP initiateRSP;
extern ROMCLOSED_RSP romClosed_RSP;

#pragma endregion


// ReSharper restore CppInconsistentNaming

// frame buffer plugin spec extension
typedef struct
{
	unsigned long addr;
	unsigned long size;
	unsigned long width;
	unsigned long height;
} FrameBufferInfo;

class Plugin
{
public:
	/**
	 * \brief Tries to create a plugin from the given path
	 * \param path The path to a plugin
	 * \return A pointer to the plugin, or nothing if the plugin couldn't be created
	 */
	static std::optional<std::unique_ptr<Plugin>> create(std::filesystem::path path);

	Plugin() = default;
	~Plugin();

	/**
	 * \brief Opens the plugin configuration dialog
	 */
	void config();

	/**
	 * \brief Opens the plugin test dialog
	 */
	void test();

	/**
	 * \brief Opens the plugin about dialog
	 */
	void about();

	/**
	 * \brief Loads the plugin's exported functions into the globals
	 */
	void load_into_globals();

	/**
	 * \brief Gets the plugin's path
	 */
	auto path() const
	{
		return m_path;
	}

	/**
	 * \brief Gets the plugin's name
	 */
	auto name() const
	{
		return m_name;
	}

	/**
	 * \brief Gets the plugin's type
	 */
	auto type() const
	{
		return m_type;
	}

	/**
	 * \brief Gets the plugin's version
	 */
	auto version() const
	{
		return m_version;
	}

private:
	std::filesystem::path m_path;
	std::string m_name;
	plugin_type m_type;
	uint16_t m_version;
	void* m_module;
};

/**
 * \brief Gets all available plugins
 */
std::vector<std::unique_ptr<Plugin>> get_available_plugins();


/// <summary>
/// Initializes dummy info used by per-plugin functions
/// </summary>
void setup_dummy_info();
