# Mupen64 Lua
[![Release](https://img.shields.io/github/v/release/mkdasher/mupen64-rr-lua-?label=Release)](https://github.com/mkdasher/mupen64-rr-lua-/releases)
[<img src="https://img.shields.io/github/downloads/mkdasher/mupen64-rr-lua-/total?label=Downloads">]()

[comment]: <> (Second image has to be inline so another approach is used)


This repository contains source code for continued development of Mupen64 - Nintendo 64 emulator with TAS support, and TASinput plugin. 

This version includes avisplit, reset recording and Wii VC options all in one.

# Building
(This was written having Windows in mind, but it should be possible on Linux as well)
If you want to build mupen/tasinput on your own, you will need Visual Studio (2017 or 2019 recommended) with C++ packages installed. In repository you will find two solutions files `winproject\mupen64\mupen_2017.sln` and `tasinput_plugin\src\DefDI.sln` used to compile mupen or tasinput respectively. 

Open either mupen or tasinput solution, on the top you can select Release or Debug build target, press F5 to compile and run with debugger attached. Binaries are placed inside `bin` folder (mupen and tasinput have separate folders)

# Debugging
You can debug mupen straight away by running it and placing breakpoints in Visual Studio, but since tasinput is a dll it needs a host application. The .sln is setup in a way that automatically starts mupen (which you need to compile), however you must move the newly compiled tasinput binary **and .pdb file located in the same directory** to plugins folder (this might be automated in the future)

# Project structure
Work in progress

# Links
Discord: https://discord.gg/eZXbmguKEq
