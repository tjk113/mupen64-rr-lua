<p align="center">
  <img width="128" align="center" src="https://github.com/mkdasher/mupen64-rr-lua-/assets/48759429/45351707-be77-4daf-987c-0bdb712891ab">
</p>



<h1 align="center">
  Mupen64-rr-lua
</h1>


<p align="center">
  N64 TASing emulator with Lua scripting support
</p>

<p align="center">
  <img src="https://img.shields.io/github/v/release/mkdasher/mupen64-rr-lua-?style=for-the-badge"/>  
  <img src="https://img.shields.io/github/downloads/mkdasher/mupen64-rr-lua-/total?style=for-the-badge"/>  
  <img src="https://img.shields.io/discord/723573549607944272?style=for-the-badge"/>  
</p>

# Quickstart
The latest release is available on the [releases page](https://github.com/mkdasher/mupen64-rr-lua-/releases/latest/)

If any issues arise or you need help, join the [discord server](https://discord.gg/eZXbmguKEq)

Cutting-edge features with potential instability are available for download as a zipped binary under the latest commit's artifact. 

# Compiling
Open the [solution](https://github.com/mkdasher/mupen64-rr-lua-/blob/dev/winproject/mupen64/mupen64_2017.sln) with your IDE of choice (VS20XX and Rider are recommended) and build the solution.

# Structure

The project is divided into 3 layers: Shared, Core, and View.

## Shared

The shared layer contains code with no dependencies other than the STL and project libraries (e.g.: libdeflate). 

This layer contains shared code, such as helpers, contracts, core types, and the config.
All other layers can reference the shared layer and utilize its code, hence the name.

## Core

The core layer contains the emulation core data and code, with access to the shared layer, STL, and project libraries.

This layer must be driven by the view layer, and is allowed to call into contract functions defined in the shared layer and implemented in the view layer.

## View

The view layer contains references to the other layers, implements the required contracts, and drives the view layer.

Platform-specific references (e.g.: Windows.h) are only permitted in the view layer.
