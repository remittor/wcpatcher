// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define NOMINMAX

#include "targetver.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>

#ifndef RESOURCE_ENUM_VALIDATE
#error "Please, update Microsoft SDKs to 6.1 or later"
#endif

#ifndef _STDINT
#include "win32common\stdint.h"
#define _STDINT
#endif

#include "winntapi.h"
#include "log.h"
#include "wdx_api.h"
#include "bst\string.hpp"
#include "bst\list.hpp"
#include "utils.h"

#pragma comment (lib, "fake_ntdll.lib")
#pragma comment (lib, "shlwapi.lib")

#define FIN_IF(_cond_,_code_) do { if ((_cond_)) { hr = _code_; goto fin; } } while(0)
#define FIN(_code_)           do { hr = _code_; goto fin; } while(0)

#ifndef FILE_SHARE_VALID_FLAGS
#define FILE_SHARE_VALID_FLAGS  (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
#endif


