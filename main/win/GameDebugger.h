#pragma once
#include <functional>

#ifdef _DEBUG

#define GAME_DEBUGGER 1

#endif

/// <summary>
/// Whether the game debugger is resuming the game
/// </summary>
extern int gameDebuggerIsResumed;

/// <summary>
/// Whether DMA read operations are performed
/// </summary>
extern int gameDebuggerIsDmaReadEnabled;

/// <summary>
/// Starts the game debugger on a separate thread
/// </summary>
/// <param name="getOpcode">A callback which returns the processor's current opcode</param>
/// <param name="getAddress">A callback which returns the processor's current operating address</param>
extern void GameDebuggerStart(std::function<unsigned long(void)> getOpcode, std::function<unsigned long(void)> getAddress);

/// <summary>
/// A function which shall be called by the r4300 core when a cycle ends
/// </summary>
extern void GameDebuggerOnLateCycle();
