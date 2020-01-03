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
  int patch();
  
  int get_exe_ver(modver * ver = NULL);
  UINT get_main_thread_id() { return m_main_thread_id; }
  PBYTE find_pattern(PBYTE beg, PBYTE end, LPVOID pattern, size_t size);
  PBYTE find_func_enter_backward(PBYTE beg, PBYTE codebase);

  func_hook * add_func_hook(LPCSTR name, int arg_reg_num, int arg_stk_num, SIZE_T addr,
                            func_hook::patch_type type = func_hook::ptFuncProlog);

  int create_hook(func_hook * fh, FuncPreHook func_addr, FuncPostHook post_hook = NULL);

private:
  int patch_internal();

  DWORD         m_main_thread_id;
  HMODULE       m_module;
  bool          m_inited;
  wcp::cfg      m_cfg;
  wcp::inicfg   m_inicfg;

  bool          m_patched;
  struct {
    PBYTE       base;
    size_t      size;
  } m_codesec;

  tramp_mm      m_trampolines;
  modver        m_exever;
  bst::list<func_hook> m_fhlist;
  filecachelist m_fcache;
};

} /* namespace */
