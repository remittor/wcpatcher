#include "stdafx.h"
#include "wcp_cfg.h"


namespace wcp {

bool cfg::set_debug_level(int dbg_level) noexcept
{
  m_debug_level = dbg_level;
  if (dbg_level < LL_CRIT_ERROR)
    m_debug_level = LL_CRIT_ERROR;
  if (dbg_level > LL_TRACE)
    m_debug_level = LL_TRACE;
  return true;
}

// =====================================================================================

int inicfg::init(HMODULE mod_addr) noexcept
{
  int hr = 0;

  m_mod_path.clear();
  DWORD nlen = GetModuleFileNameW(mod_addr, m_mod_path.data(), (DWORD)m_mod_path.capacity());
  FIN_IF(nlen < 4 || nlen >= m_mod_path.capacity(), -2);
  FIN_IF(GetLastError() != ERROR_SUCCESS, -3);  
  m_mod_path.fix_length();
  //WLOGd(L"mod path = '%s'", m_mod_path.c_str());
  size_t pos = m_mod_path.rfind(L'\\');
  m_mod_path.resize(pos + 1);
  
  m_ini_file.assign(m_mod_path);
  m_ini_file.append_fmt(L"%S", ini::filename);
  WLOGd(L"INI = \"%s\" ", m_ini_file.c_str());

  hr = load_from_ini(true);

fin:
  LOGe_IF(hr, "%s: ERROR = %d ", __func__, hr);
  return hr;  
}

int inicfg::get_param_int(LPCWSTR param_name, int def_value) noexcept
{
  return (int)GetPrivateProfileIntW(ini::settings, param_name, def_value, m_ini_file.c_str());
}

int inicfg::load_from_ini(bool forced) noexcept
{
  int hr = 0;
  int val;
  wcp::cfg config;
  
  if (!forced) {
    DWORD now = GetTickCount();
    if (GetTickBetween(m_last_load_time, now) < 60000)
      return 0;
  }
  m_last_load_time = GetTickCount();  

  FIN_IF(m_ini_file.empty(), -11);
  DWORD dw = GetFileAttributesW(m_ini_file.c_str());
  FIN_IF(dw == INVALID_FILE_ATTRIBUTES, -12);

  val = get_param_int(L"DebugLevel", -1);
  if (val >= 0) {
    config.set_debug_level(val);
    LOGn("%s: debug level = %d [%c]", __func__, val, val <= LL_TRACE ? WCP_LL_STRING[val] : 't');
    WcpSetLogLevel(val);
  }
  val = get_param_int(L"PatchWcsICmp", -1);
  if (val >= 0) {
    config.set_patch_wcsicmp(val ? true : false);
    LOGd("%s: patch tc_wcsicmp = %d ", __func__, val);
  }
  val = get_param_int(L"ActivateWhenExceed", -1);
  if (val >= 0) {
    config.set_act_limit(val);
    LOGd("%s: act_limit = %d ", __func__, val);
  }
  val = get_param_int(L"CaseSensitive", -1);
  if (val >= 0) {
    config.set_case_sens(val ? true : false);
    LOGd("%s: case-sensitive = %d ", __func__, val);
  }
  {
    bst::scoped_write_lock lock(m_mutex);
    m_cfg.assign(config);
  }
  hr = 0;

fin:
  return hr;
}

bool inicfg::save_param_str(LPCWSTR param_name, LPCWSTR value) noexcept
{
  BOOL x = WritePrivateProfileStringW(ini::settings, param_name, value, m_ini_file.c_str());
  return (x == FALSE) ? false : true;
}

int inicfg::save_to_ini(wcp::cfg & cfg) noexcept
{
  int hr = 0;
  bst::filename str;

  FIN_IF(m_ini_file.empty(), -21);
  DWORD dw = GetFileAttributesW(m_ini_file.c_str());
  FIN_IF(dw == INVALID_FILE_ATTRIBUTES, -22);

  str.assign_fmt(L"%d", cfg.get_debug_level());
  save_param_str(L"DebugLevel", str.c_str());

  str.assign_fmt(L"%d", (int)cfg.get_patch_wcsicmp());
  save_param_str(L"PatchWcsICmp", str.c_str());

  str.assign_fmt(L"%d", cfg.get_act_limit());
  save_param_str(L"ActivateWhenExceed", str.c_str());

  str.assign_fmt(L"%d", (int)cfg.get_case_sens());
  save_param_str(L"CaseSensitive", str.c_str());

  LOGd("%s: INI saved! ", __func__);
  hr = 0;

fin:  
  return hr;
}

int inicfg::save_to_ini() noexcept
{
  wcp::cfg cfg;
  {
    bst::scoped_read_lock lock(m_mutex);
    cfg = m_cfg;
  }
  return save_to_ini(cfg);
}

} /* namespace */

