# [BAD APPLE] Injected CPU Chart
An win32 console application using injection to make *CPU chart* of *taskmgr.exe* show animation *BAD APPLE*.

*Support: Win32*

*Encoding: GB2312*

## Features

* API hook
* Win32 message hook
* Presentation using SDL
* Simple edge detection
* IPC using pipe

## Usage

1. In *InjectDll/InjectDll.cpp*, set *g_ExecPath*, *g_strWndClass*, *g_strWndName* to whatever you want. Then set *g_strImagePathFormat* to a dictionary which contains your animation frames.
2. Start *InjectConsole.exe*, then wait until you see a dedicated application opening. When it's ready for playing, click(***middle click***) any window you want to play animation and watch!

## Prerequisite

1. VS2015(*C++11, STL*)
2. SDL/SDL_TTF 2.0 Runtime(*SDL2.lib, SDLmain.lib, libfreetype-6.dll, zlib1.dll, SDL2.dll, SDL2_TTF.dll*) & Include
3. mmsystem(*mmsystem.h, winmm.lib*)
4. psapi(*.h, .lib*)
5. pathcch(*.h, .lib*)