#pragma once

#include <windows.h>
#include "bst\string.hpp"
#include "wcp_version.h"


namespace wcp {

namespace ini {
  static const char  filename[] = WCP_INTERNAL_NAME ".ini";
  static const WCHAR settings[] = L"settings";
};

class cfg
{
protected:  
  int        m_debug_level;
  int        m_act_limit;
  bool       m_patch_wcsicmp;
  bool       m_case_sens;

  void set_defaults() noexcept
  {
    m_debug_level = LL_INFO;
    m_patch_wcsicmp = true;
    m_act_limit = 3000;
    m_case_sens = false;
  }

public:
  cfg() noexcept { set_defaults(); }
  ~cfg() {  }
#if defined(_MSC_VER) && (_MSC_VER < 1600)
  /* msvc 2008 will independently create the necessary copy constructor */ 
#else
  cfg(const cfg & config) noexcept = default;
  cfg & operator=(const cfg & config) noexcept = default;
#endif
  bool assign(const cfg & config) noexcept { *this = config; return true; }

  int        get_debug_level()    noexcept { return m_debug_level; }
  int        get_act_limit()      noexcept { return m_act_limit; }
  bool       get_patch_wcsicmp()  noexcept { return m_patch_wcsicmp; }
  bool       get_case_sens()      noexcept { return m_case_sens; }
  
  bool set_debug_level(int dbg_level) noexcept;
  bool set_act_limit(int act_limit) noexcept { m_act_limit = (act_limit < 0) ? 0 : act_limit; return true; }
  bool set_patch_wcsicmp(bool patch_wcsicmp) noexcept { m_patch_wcsicmp = patch_wcsicmp; return true; }
  bool set_case_sens(bool case_sens) noexcept { m_case_sens = case_sens; return true; }
};

class inicfg
{
public:
  friend class cfg;
  
  inicfg() {  }
  ~inicfg() {  }
  bool copy(wcp::cfg & config) noexcept;
  wcp::cfg get_cfg() noexcept;

  int init(HMODULE mod_addr) noexcept;
  int update() noexcept { return load_from_ini(false); }

  int save_to_ini() noexcept;
  LPCWSTR get_mod_path() { return m_mod_path.c_str(); }

protected:
  int get_param_int(LPCWSTR param_name, int def_value) noexcept;
  int load_from_ini(bool forced) noexcept;
  bool save_param_str(LPCWSTR param_name, LPCWSTR value) noexcept;
  int save_to_ini(wcp::cfg & cfg) noexcept;

  wcp::cfg       m_cfg;
  bst::srw_mutex m_mutex;    // SRW lock for m_cfg;
  DWORD          m_last_load_time;
  bst::filepath  m_mod_path;
  bst::filepath  m_ini_file;
};


BST_INLINE
bool inicfg::copy(wcp::cfg & config) noexcept
{
  //bst::scoped_read_lock lock(m_mutex);
  config = m_cfg;
  return true;
}

BST_INLINE
wcp::cfg inicfg::get_cfg() noexcept
{
  wcp::cfg res;
  //bst::scoped_read_lock lock(m_mutex);
  res.assign(m_cfg);
  return res;
}

} /* namespace */

