// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料
// Windows 头文件: 
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <sstream>
#include <assert.h>

// TODO:  在此处引用程序需要的其他头文件
#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#include <Pathcch.h>
#pragma comment(lib,"Pathcch.lib")
#include "SDL/include/SDL.h"
#include "SDL/include/SDL_ttf.h"
#pragma comment(lib,"SDL/lib/x86/SDL2.lib")
#pragma comment(lib,"SDL/lib/x86/SDL2main.lib")
#pragma comment(lib,"SDL/lib/x86/SDL2_ttf.lib")