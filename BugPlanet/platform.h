#pragma once

//--------------------------------------------------------------------------------------
// Platform specific include files
//--------------------------------------------------------------------------------------
#ifdef _WIN32
#undef UNICODE

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <mmsystem.h>
#include <aclapi.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef _WIN32
#include <conio.h>
#include <process.h>
#endif


#ifdef __APPLE__
#include <mach/mach_types.h>
#include <mach/mach_time.h>
#endif

#define Yes	1
#define No	0

#ifndef BOOL
#define BOOL int
#define TRUE Yes
#define FALSE No
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef _WIN32
typedef struct _RECT
{
    long left;
    long top;
    long right;
    long bottom;
}RECT;

#define ZeroMemory(a,b) memset( a, 0, b )
#endif