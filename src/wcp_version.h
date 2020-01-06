#pragma once

#define WCP_VER_MAJOR     0
#define WCP_VER_MINOR     6

#define WCP_VER_GET_STR(num)  WCP_VER_GET_STR2(num)
#define WCP_VER_GET_STR2(num) #num

#define WCP_INTERNAL_NAME "wcpatcher"
#define WCP_DESCRIPTION   "Patcher for TotalCmd"
#define WCP_VERSION       WCP_VER_GET_STR(WCP_VER_MAJOR) "." WCP_VER_GET_STR(WCP_VER_MINOR)
#define WCP_RC_VERSION    WCP_VER_MAJOR, WCP_VER_MINOR, 0, 0

#define WCP_COPYRIGHT     "\xA9 2020 remittor"      // A9 for (c)
#define WCP_COMPANY_NAME  "https://github.com/remittor"
#define WCP_SOURCES       "https://github.com/remittor/wcpatcher"
#define WCP_LICENSE       "https://github.com/remittor/wcpatcher/blob/master/License.txt"
//#define WCP_LICENSE     "https://raw.githubusercontent.com/remittor/wcpatcher/master/License.txt"

#ifdef WCP_DEBUG
#define WCP_FILE_DESC     WCP_DESCRIPTION " (DEBUG)"
#else
#define WCP_FILE_DESC     WCP_DESCRIPTION
#endif

#ifdef _WIN64
#define WCP_ORIG_FILENAME WCP_INTERNAL_NAME ".wdx64"
#else
#define WCP_ORIG_FILENAME WCP_INTERNAL_NAME ".wdx"
#endif

