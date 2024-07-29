// header.h: 标准系统包含文件的包含文件，
// 或特定于项目的包含文件
//

#pragma once
#include "resource.h"

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// Direct2D需要的头文件
#include <d2d1.h>
#include <d2d1_1helper.h>
#include <wincodec.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <cassert>
#include <dwrite_2.h>

#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D2D1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DWrite.lib")
//透明窗口需要的头文件
#include <versionhelpers.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi")

//窗口标题需要的头文件
#include <sstream>

//托盘图标用的头文件
#include <shellapi.h>

//不知道干什么用的的头文件
/*
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cwchar>
#include <cstddef>
#include <atomic>
#include <thread>
#include <new>
#include <string>
#include <sstream>
*/


// 托盘
#define MYWM_NOTIFYICON 0x00101   //托盘菜单消息号
//托盘菜单
#define ID_SHOW       2001
#define ID_SOUND      2002
#define ID_TOP        2003
#define ID_PENETARTE  2004
#define ID_ABOUT      2005
#define ID_QUIT       2006

//playsound用的头文件
#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")
using namespace std;

//音效用到的头文件
#include <map>

//创建线程的头文件
#include <thread>

//时间用到的头文件
#include <iomanip>


//定义数据字符0~9
#define   VK_0         0x30 
#define   VK_1         0x31 
#define   VK_2         0x32 
#define   VK_3         0x33 
#define   VK_4         0x34 
#define   VK_5         0x35 
#define   VK_6         0x36 
#define   VK_7         0x37 
#define   VK_8         0x38 
#define   VK_9         0x39

//定义数据字符A~Z
#define   VK_A	0x41 
#define   VK_B	0x42 
#define   VK_C	0x43 
#define   VK_D	0x44 
#define   VK_E	0x45 
#define   VK_F	0x46 
#define   VK_G	0x47 
#define   VK_H	0x48 
#define   VK_I	0x49 
#define   VK_J	0x4A 
#define   VK_K	0x4B 
#define   VK_L	0x4C 
#define   VK_M	0x4D 
#define   VK_N	0x4E 
#define   VK_O	0x4F 
#define   VK_P	0x50 
#define   VK_Q	0x51 
#define   VK_R	0x52 
#define   VK_S	0x53 
#define   VK_T	0x54 
#define   VK_U	0x55 
#define   VK_V	0x56 
#define   VK_W	0x57 
#define   VK_X	0x58 
#define   VK_Y	0x59 
#define   VK_Z	0x5A 