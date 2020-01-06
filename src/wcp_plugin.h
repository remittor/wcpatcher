#pragma once

#include "stdafx.h"
#include "wdx_api.h"
#include <algorithm>
#include "wcp_memory.h"
#include "wcp_hook.h"
#include "wcp_filecache.h"
#include "wcp_cfg.h" 


namespace wcp {

#pragma pack(push, 1)
struct modver {
  union {
    DWORD   dw;
    struct {
      BYTE  revision;
      BYTE  build;
      BYTE  minor;
      BYTE  major;
    };
  };
};
#pragma pack(pop)

static const wchar_t  img_cfg_filename[] = L"imgcfg.ini";

struct img_obj {
  CHAR    name[48];
  SIZE_T  addr;
};

struct img_obj_list {
  img_obj g_WcxItemList;
  img_obj WcxProcessor;
  img_obj WcxProcessor_loop_end;
  img_obj TcCreateFileInfo_dd;
  img_obj tc_wcsicmp;
};

enum direction {
  dirForward  = 0,
  dirBackward = 1,
};

class plugin
{
public:
  plugin();
  ~plugin();

  int init(HMODULE lib_addr, DWORD thread_id) noexcept;
  int destroy();
  int load_image_cfg();
  int patch();
  
  UINT get_main_thread_id() { return m_main_thread_id; }
  LPCVOID find_pattern(LPCVOID lpBegin, LPCVOID lpEnd, LPCVOID pattern, size_t size);
  PBYTE find_func_enter_backward(PBYTE beg, PBYTE codebase);

  func_hook * add_func_hook(img_obj & obj, int arg_reg_num, int arg_stk_num, func_hook::patch_type type = func_hook::ptFuncProlog);
  int create_hook(func_hook * fh, FuncPreHook func_addr, FuncPostHook post_hook = NULL);

private:
  int patch_internal();
  int load_img_obj(img_obj & obj, LPCSTR name, LPCWSTR sec, LPCWSTR ini);
  int get_exe_ver();
  int find_exe_date(LPCVOID beg, LPCVOID end);

  DWORD         m_main_thread_id;
  HMODULE       m_module;
  bool          m_inited;
  wcp::cfg      m_cfg;
  wcp::inicfg   m_inicfg;
  img_obj_list  m_img;

  bool          m_patched;
  struct {
    PBYTE       base;
    size_t      size;
  } m_codesec;

  tramp_mm      m_trampolines;
  modver        m_exever;
  char          m_exedate[16];
  bst::list<func_hook> m_fhlist;
  filecachelist m_fcache;
};

} /* namespace */
