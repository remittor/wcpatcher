#pragma once

#include "stdafx.h"
#include "wcp_filetree.h"
#include "wcp_cfg.h"

namespace wcp {

const WCHAR patch_disable_str[] = L"TURN+OFF+WCP";
const WCHAR patch_enabled_str[] = L"TURN+ON+WCP";

class filecache
{
public:
  enum patch_status {
    psDisabled = 0,
    psEnabled,
    psForced,
  };

  filecache() noexcept;
  ~filecache() noexcept;
  
  int reset() noexcept;
  int init(const wcp::cfg & config, int wcx_index, PFileCollection * pWcxItemPtr) noexcept;
  void set_cfg(const wcp::cfg & config) noexcept { m_cfg = config; }

  int WcxProcessor(bool init, int type_index, LPCWSTR ArcName, LPWSTR ArcCurDir) noexcept;
  int TcCreateFileInfo(char mode, LPCWSTR fileName, BYTE fileAttr, INT64 fileSize, UINT16 tcAttr) noexcept;
  int WcxProcessor_loop_end() noexcept;

private:
  bool patch_fc(LPCSTR src_func = NULL) noexcept;
  bool unpatch_fc(LPCSTR src_func = NULL) noexcept;

  wcp::cfg          m_cfg;
  int               m_wcx_index;
  PFileCollection * m_WcxItemPtr;
  PFileCollection   m_WcxItem;

  int               m_type_index;
  bst::filepath     m_ArcName;
  
  bst::filepath     m_CurDir;
  bool              m_proc_init;

  time_meter        m_fcol_gen;
  time_meter        m_dir_gen;

  FileTree          m_ftree;

  patch_status      m_fc_patch_status;
  bool              m_fc_patched;
  TFileCollection   m_fcollection;   /* custom file collection for spoofing */
  bst::buf          m_fitem_array;
  size_t            m_item_count;
};


class filecachelist
{
public:
  static const size_t max_cache_num = 127;
  
  filecachelist() noexcept;
  ~filecachelist() noexcept;

  void destroy() noexcept;
  int init(const wcp::cfg & config, LPCVOID g_WcxItemList) noexcept;
  void set_inicfg(wcp::inicfg * ini_cfg) noexcept { m_inicfg = ini_cfg; }

  int WcxProcessor(bool init, int type_index, LPCWSTR ArcName, LPWSTR ArcCurDir, BYTE wcx_index) noexcept;
  int TcCreateFileInfo(char mode, LPCWSTR fileName, BYTE fileAttr, INT64 fileSize, UINT16 tcAttr) noexcept;
  int WcxProcessor_loop_end() noexcept;

private:
  int update_cfg() noexcept;
  filecache * get_item(size_t index) { return (index < max_cache_num) ? m_list[index] : NULL; }

  wcp::cfg          m_cfg;
  wcp::inicfg     * m_inicfg;
  PFileCollection * m_WcxItemList;

  filecache       * m_list[max_cache_num + 1];
  int               m_current_index;
};

} /* namespace */
