#pragma once

#include <windows.h>

namespace wcp {

namespace tfa {      /* TotalCmd File Attr */
  enum Type : UCHAR {
    READONLY    = 0x01,
    HIDDEN      = 0x02,
    SYSTEM      = 0x04,
    DEVICE      = 0x08,
    DIRECTORY   = 0x10,
    ARCHIVE     = 0x20,
  };
};

#ifdef _WIN64
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif

struct TFileItem {                   /* see TcCreateFileInfo */
/* x32 x64*/ LPVOID     method_get_nameA; /* addr 0x70915C -> 0x70A0EC */
  /*04 08*/  DWORD      sizeLO;
  /*08 0C*/  DWORD      timeLO;   /* MS FILETIME */
  /*0C 10*/  DWORD      timeHI;   /* MS FILETIME */
  /*10 14*/  DWORD      sizeHI;
  /*14 18*/  int        unk6;     /* -1 */
  /*18 1C*/  DWORD      unk7;     /* 0 */
  /*1C 20*/  int        unk8;     /* -1 */   /* elem[28]  elem[7*4] */
  /*20 28*/  LPVOID     ptr1;     /* 0 */
  /*24 30*/  LPVOID     ptr2;     /* 0 */
  /*28 38*/  LPSTR      str1;
  /*2C 40*/  SIZE_T     index;    /* may be UINT64 ??? */
#ifndef _WIN64
  /*30   */  DWORD      unk12;
#endif
  /*34 48*/  tfa::Type  attr;
  /*35 49*/  BYTE       unkVV;    /* 0 */
  /*36 4A*/  UINT16     auxAttr;
  /*38 4C*/  DWORD      unk14;    /* elem[4144] */
  /*3C 50*/  UINT16     unk15;    /* 0 */
  /*40 58*/  LPVOID     ptr3;     /* 0 */
  /*44 60*/  LPCWSTR    name;     /* wchar_t[1024] */
  /*48 68*/  LPVOID     ptr4;     /* 0 */

  INT64 get_size() { return ((INT64)sizeHI << 32) | sizeLO; }
  INT64 get_time() { return ((INT64)timeHI << 32) | timeLO; }
};

typedef  TFileItem * PFileItem;


struct TFileCollection {
  LPVOID      method;    /* ctor ??? */
  PFileItem * item;
  DWORD       count;

  PFileItem get_item(size_t index) { return (index >= 0 && index < (size_t)count) ? item[index] : NULL; }
  size_t get_count() { return (size_t)count; }
};

typedef  TFileCollection * PFileCollection;

#pragma pack(pop)

} /* namespace */
