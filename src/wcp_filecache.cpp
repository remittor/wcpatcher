#include "stdafx.h"
#include "wcp_filecache.h"

namespace wcp {

filecache::filecache() noexcept
{
  m_wcx_index = -1;
  m_fc_patch_status = psEnabled;
  m_type_index = -1;
  m_fc_patched = false;
  m_WcxItemPtr = NULL;
  m_WcxItem = NULL;
}

filecache::~filecache() noexcept
{
  unpatch_fc();
}

int filecache::init(const wcp::cfg & config, int wcx_index, PFileCollection * pWcxItemPtr) noexcept
{
  m_cfg = config;
  m_ftree.set_case_sensitive(m_cfg.get_case_sens());
  m_wcx_index = wcx_index;
  m_WcxItemPtr = pWcxItemPtr;
  m_WcxItem = NULL;
  return 0;
}

int filecache::reset() noexcept
{
  m_fc_patch_status = psEnabled;
  m_type_index = -1;
  m_ArcName.clear();
  m_CurDir.clear();
  m_item_count = SIZE_MAX;
  m_ftree.clear();
  m_proc_init = true;
  return 0;
}

int filecache::WcxProcessor(bool init, int type_index, LPCWSTR ArcName, LPWSTR CurDir) noexcept
{
  int hr = -1;
  if (*m_WcxItemPtr == &m_fcollection) {
    LOGe("%s: ERROR: pWcxItemPtr == fcollection", __func__);
    return -1;
  } else {
    m_WcxItem = *m_WcxItemPtr;      /* update ptr for file collection */
  }
  //FIN_IF(m_WcxItem == NULL, -1);
  m_fcol_gen.init();
  if (init) {
    reset();
    m_fc_patch_status = psEnabled;
    m_proc_init = true;
    m_type_index = type_index;
    m_ArcName.assign(ArcName);
    FIN_IF(!ArcName || ArcName[0] == 0, -2);
    if (StrStrW(m_ArcName.c_str(), patch_enabled_str)) {
      m_fc_patch_status = psForced;
    } else {
      if (StrStrW(m_ArcName.c_str(), patch_disable_str))
        m_fc_patch_status = psDisabled;
    }
  } else {
    m_proc_init = false;
    FIN_IF(m_ArcName.empty(), -10);
    FIN_IF(type_index != m_type_index, -13);
    if (m_cfg.get_case_sens()) {
      FIN_IF(StrCmpW(ArcName, m_ArcName.c_str()), -14);
    } else {
      FIN_IF(StrCmpIW(ArcName, m_ArcName.c_str()), -14);
    }
    //WLOGd(L"%S: change cur dir = '%s'", __func__, CurDir);
  }
  m_CurDir.assign(CurDir);
  //WLOGd_IF(init, L"%S: cache for '%s' inited!", __func__, m_ArcName.c_str());
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  if (hr) {
    reset();
  }
  return hr;
}

int filecache::TcCreateFileInfo(char mode, LPCWSTR fileName, BYTE fileAttr, INT64 fileSize, UINT16 tcAttr) noexcept
{
  int hr = -1;

  m_WcxItem = *m_WcxItemPtr;      /* update ptr for file collection */
  FIN_IF(m_WcxItem == NULL, -1);
  if (m_proc_init) {
    double dms = m_fcol_gen.get_diff_msec();
    LOGn("------- Generation LINEAR file collection completed -------- time: %.2f ms (item_count = %d)", dms, m_WcxItem->count);
  }
  m_dir_gen.init();
  if (m_fc_patch_status == psEnabled && m_WcxItem->count < (DWORD)m_cfg.get_act_limit()) {
    m_fc_patch_status = psDisabled;
  }
  FIN_IF(m_fc_patch_status == psDisabled, 0);
  FIN_IF(m_fc_patched, -2);
  //WLOGd(L"%S: %d 0x%04X 0x%04X %I64d '%s' \"%s\"", __func__, mode, fileAttr, tcAttr, fileSize, fileName, m_ArcName.c_str());

  //FIN_IF(mode != 1, 0);
  //FIN_IF(fileAttr != TcFileAttr::DIRECTORY, 0);
  //FIN_IF(tcAttr != 0, 0);
  //FIN_IF(fileSize != -1LL, 0);
  //FIN_IF(!fileName, 0);
  //FIN_IF(wcscmp(fileName, L".."), 0);

  FIN_IF(m_ArcName.empty(), -2);

  FIN_IF(m_fc_patch_status == psDisabled, 0);

  if (m_item_count == SIZE_MAX) {
    //m_dir_gen.init();
    size_t item_cnt = m_WcxItem->count;
    m_ftree.clear();
    //LOGd("%s: wcx_index = %d, item_count = %Id", __func__, m_wcx_index, item_cnt);
    FIN_IF(item_cnt == 0, 0);
    FIN_IF(item_cnt >= _I32_MAX, -5);
    for (size_t i = 0; i < item_cnt; i++) {
      PFileItem item = m_WcxItem->get_item(i);
      FIN_IF(!item, -6);
      //WLOGd(L"%S: [%02d] attr = %02X (%04X), size = %I64d, name = '%s'", __func__, item->index, item->attr, item->tcAttr, item->get_size(), item->name);
      //WLOGd(L"%S:     unk14 = %08X, str1 = '%s', unk8 = %d,%d ", __func__, item->unk14, item->str1, item->unk8, item->unk6);
      //SYSTEMTIME st;
      //FileTimeToSystemTime((FILETIME *)&item->time, &st);
      //WLOGd(L"%S:     %04d-%02d-%02d %02d:%02d:%02d", __func__, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
      m_ftree.add_file_item(item);
    }
    m_item_count = item_cnt;
    double dms = m_dir_gen.get_diff_msec();
    LOGn("======= Generation TREE-LIKE file collection completed ===== time: %.2f ms (elem_count = %Id)", dms, m_ftree.get_num_elem());
  }

  m_dir_gen.init();

  FileTreeEnum ftenum;
  bool x = m_ftree.find_directory(ftenum, m_CurDir.c_str());
  FIN_IF(!x, -10);
  //int ecnt = m_ftree.get_dir_num_item(ftenum);
  //FIN_IF(ecnt < 0, -11);
  //LOGd("%s: dir_num_item = %d", __func__, m_ftree.get_dir_num_item(ftenum));

  const size_t alloc_factor = 64*1024 / sizeof(PFileItem);
  if (m_fitem_array.empty()) {
    size_t sz = m_fitem_array.resize(alloc_factor * sizeof(PFileItem));
    FIN_IF(sz == bst::npos, -20);
  }
  PFileItem * fiarray = (PFileItem *)m_fitem_array.data();
  size_t cnt = 0;
  TTreeElem * elem;
  while (elem = m_ftree.get_next(ftenum)) {
    if (elem->data) {
      if ((cnt + 2) * sizeof(PFileItem) >= m_fitem_array.size()) {
        PBYTE p = m_fitem_array.expand(alloc_factor * sizeof(PFileItem));
        FIN_IF(!p, -21);
        fiarray = (PFileItem *)m_fitem_array.data();
      }
      fiarray[cnt++] = elem->data;
    }
  }
  fiarray[cnt] = NULL;

  double dms = m_dir_gen.get_diff_msec();
  LOGn("======= Search in TREE-LIKE file collection completed ====== time: %.2f ms (dir_content = %Id)", dms, cnt);
  m_dir_gen.init();

  m_fcollection.method = m_WcxItem->method;
  m_fcollection.count = (DWORD)cnt;
  m_fcollection.item = fiarray;
  patch_fc(__func__);
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

bool filecache::patch_fc(LPCSTR src_func) noexcept
{
  if (m_WcxItemPtr) {
    *m_WcxItemPtr = &m_fcollection;   /* set new file collection */
    LOGi("%s: file collection patched!  item_count = %d", src_func ? src_func : __func__, m_fcollection.count);
    m_fc_patched = true;
    return true;
  }
  return false;
}

bool filecache::unpatch_fc(LPCSTR src_func) noexcept
{
  if (m_fc_patched) {
    *m_WcxItemPtr = m_WcxItem;     /* restore original file collection */
    LOGi("%s: Restore orig file collection = %p", src_func ? src_func : __func__, m_WcxItem);
    m_fc_patched = false;
    return true;
  }
  //memset(&m_fcollection, 0, sizeof(m_fcollection));
  return false;
}

int filecache::WcxProcessor_loop_end() noexcept
{
  if (m_WcxItem) {
    if (m_fc_patched) {
      unpatch_fc(__func__);
    }
    double dms = m_dir_gen.get_diff_msec();
    LOGn("------- Search in LINEAR file collection completed --------- time: %.2f ms", dms);
  }
  return 0;
}

// =========================================================================================

filecachelist::filecachelist() noexcept
{
  m_inicfg = NULL;
  m_WcxItemList = NULL;
  memset(&m_list, 0, sizeof(filecachelist));
  m_current_index = -1;
}

filecachelist::~filecachelist() noexcept
{
  destroy();
}

void filecachelist::destroy() noexcept
{
  for (size_t i=0; i <= max_cache_num; i++) {
    filecache * fc = m_list[i];
    if (fc) {
      delete fc;
    }
    m_list[i] = NULL;
  }
}

int filecachelist::init(const wcp::cfg & config, LPCVOID g_WcxItemList) noexcept
{
  m_cfg = config;
  m_WcxItemList = (PFileCollection *)g_WcxItemList;
  return 0;
}

int filecachelist::update_cfg() noexcept
{
  if (m_inicfg) {
    if (m_inicfg->update() == 0) {
      m_inicfg->copy(m_cfg);
    }
  }
  return 0;     
}

int filecachelist::WcxProcessor(bool init, int type_index, LPCWSTR ArcName, LPWSTR CurDir, BYTE wcx_index) noexcept
{
  int hr = -1;

  m_current_index = wcx_index;
  FIN_IF((SSIZE_T)wcx_index < 0, -2);
  FIN_IF(wcx_index >= max_cache_num, -3);
  update_cfg();
  PFileCollection * pWcxItemPtr = &m_WcxItemList[wcx_index];
  //FIN_IF(*pWcxItemPtr == NULL, -3);
  filecache * fc = get_item(wcx_index);
  if (!fc) {
    fc = new(bst::nothrow) filecache();
    FIN_IF(!fc, -4);
    m_list[wcx_index] = fc;
    fc->init(m_cfg, wcx_index, pWcxItemPtr);
  }
  fc->set_cfg(m_cfg);
  fc->WcxProcessor(init, type_index, ArcName, CurDir);
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

int filecachelist::TcCreateFileInfo(char mode, LPCWSTR fileName, BYTE fileAttr, INT64 fileSize, UINT16 tcAttr) noexcept
{
  int hr = -1;

  FIN_IF(m_current_index < 0, -2);
  FIN_IF(m_current_index >= max_cache_num, -3);
  FIN_IF(m_list[m_current_index] == NULL, -4);
  m_list[m_current_index]->TcCreateFileInfo(mode, fileName, fileAttr, fileSize, tcAttr);
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

int filecachelist::WcxProcessor_loop_end() noexcept
{
  int hr = -1;

  FIN_IF(m_current_index < 0, -2);
  FIN_IF(m_current_index >= max_cache_num, -3);
  FIN_IF(m_list[m_current_index] == NULL, -4);
  m_list[m_current_index]->WcxProcessor_loop_end();
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

} /* namespace */
