#include "stdafx.h"
#include "wcp_plugin.h"
#include "wcp_filecache.h"


namespace wcp {

plugin::plugin()
{
  m_module = NULL;
  m_main_thread_id = 0;
  m_inited = false;
  m_patched = false;
  m_exever.dw = 0;
}

plugin::~plugin()
{
  destroy();
}

int plugin::init(HMODULE lib_addr, DWORD thread_id)	noexcept
{
  int hr = 0;
  m_module = lib_addr;
  m_main_thread_id = thread_id;
  PBYTE addr = find_str_in_memory(NULL, wcp_trampoline_region, 0, PAGE_EXECUTE_READWRITE, false);
  if (addr) {
    LOGe("######## WCPatcher already loaded. Please, restart TotalCmd. #########");
    m_patched = true;
    return -1;
  }
  hr = m_inicfg.init(m_module);
  if (hr == 0) {
    m_inicfg.copy(m_cfg);
  }
  m_inited = true;
  return 0;
}

int plugin::destroy()
{
  if (m_patched) {
    DWORD dwOldProt;
    BOOL x = VirtualProtect(m_codesec.base, m_codesec.size, PAGE_EXECUTE_READWRITE, &dwOldProt);
    if (x != FALSE) {
      m_fhlist.clear();
      VirtualProtect(m_codesec.base, m_codesec.size, dwOldProt, &dwOldProt);
    }
  }
  return 0;
}

static size_t StrLenEx(const char * s, DWORD terminator)
{
  DWORD trmn = (terminator & 0x000000FF) << 24 | (terminator & 0x0000FF00) << 8 |
               (terminator & 0x00FF0000) >> 8  | (terminator & 0xFF000000) >> 24;
  size_t len = 0;
  while (*(PDWORD)s != trmn) {
    s++;
    len++;
  }
  return len;
}

static size_t StrLenEOS(const char * s)
{
  return StrLenEx(s, '#EOS');
}

PBYTE plugin::find_pattern(PBYTE beg, PBYTE end, LPVOID pattern, size_t size)
{
  if (size >= 4 && beg + size + 4 < end) {
    end -= (size + 4);
    DWORD const prefix0 = *(PDWORD)pattern;
    DWORD const prefix4 = (size >= 8) ? *(PDWORD)((PBYTE)pattern + 4) : 0;
    DWORD const prefix8 = (size >= 12) ? *(PDWORD)((PBYTE)pattern + 8) : 0;
    for (PBYTE p = beg; p <= end; p++) {
      if (*(PDWORD)p != prefix0)
        continue;
      if (size >= 8 && *(PDWORD)(p + 4) != prefix4)
        continue;
      if (size >= 12 && *(PDWORD)(p + 8) != prefix8)
        continue;
      if (memcmp(p, pattern, size) == 0)
        return p;
    }
  }
  return NULL;
}

PBYTE plugin::find_func_enter_backward(PBYTE beg, PBYTE codebase)
{
  if (codebase + 8 < beg) {
    fnprolog fnp;
    for (PBYTE p = beg; p >= codebase; p--) {
      if (fnp.init(p) == 0)
        return p;
    }
  }
  return NULL;
}

// ============================================================================================

int plugin::load_img_obj(img_obj & obj, LPCSTR name, LPCWSTR sec, LPCWSTR ini)
{
  int hr = -1;
  bst::filename wname;
  bst::filename val;
  INT64 addr;

  strcpy(obj.name, name);
  wname.assign_fmt(L"%S", name);
  int vlen = (int)GetPrivateProfileStringW(sec, wname.c_str(), NULL, val.data(), (DWORD)val.capacity(), ini);
  FIN_IF(vlen <= 0, -2);
  BOOL x = StrToInt64ExW(val.c_str(), STIF_SUPPORT_HEX, &addr);
  FIN_IF(!x, -3);

  obj.addr = (SIZE_T)addr;
  hr = 0;

fin:
  return hr;
}

int plugin::load_image_cfg()
{
  int hr = -1;
  bst::filepath inifn;  /* ini filename */
  bst::filename sec;    /* section name */

  memset(&m_img, 0, sizeof(m_img));
  inifn.assign(m_inicfg.get_mod_path());
  FIN_IF(inifn.length() <= 3, -2);
  inifn.append(img_cfg_filename);
  FIN_IF(inifn.has_error(), -3);
  DWORD dw = GetFileAttributesW(inifn.c_str());
  FIN_IF(dw == INVALID_FILE_ATTRIBUTES, -12);
  FIN_IF(m_exever.dw == 0, -13);
  sec.assign_fmt(L"%d.%d.%d.%d", m_exever.major, m_exever.minor, m_exever.build, m_exever.revision);
#ifdef _WIN64
  sec.append(L"-x64");
#endif
  hr = load_img_obj(m_img.g_WcxItemList, "g_WcxItemList", sec.c_str(), inifn.c_str());
  FIN_IF(hr, -41);
  hr = load_img_obj(m_img.WcxProcessor, "WcxProcessor", sec.c_str(), inifn.c_str());
  FIN_IF(hr, -42);
  hr = load_img_obj(m_img.WcxProcessor_loop_end, "WcxProcessor_loop_end", sec.c_str(), inifn.c_str());
  FIN_IF(hr, -43);
  hr = load_img_obj(m_img.TcCreateFileInfo_dd, "TcCreateFileInfo_dd", sec.c_str(), inifn.c_str());
  FIN_IF(hr, -44);
  load_img_obj(m_img.tc_wcsicmp, "tc_wcsicmp", sec.c_str(), inifn.c_str());

  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

// ============================================================================================

func_hook * plugin::add_func_hook(img_obj & obj, int arg_reg_num, int arg_stk_num, func_hook::patch_type type)
{
  int hr = 0;

  func_hook * fh = new(bst::nothrow) func_hook();
  FIN_IF(!fh, -100);
  fh->set_type(type);
  fh->set_obj(&m_fcache);
  size_t tcap = 0;
  PBYTE taddr = m_trampolines.alloc(tcap);
  FIN_IF(!taddr, -101);
  fh->set_trampoline_memory(taddr, tcap);

  hr = fh->init(obj.name, (LPCVOID)obj.addr, arg_reg_num, arg_stk_num);
  FIN_IF(hr, -102);

  bool x = m_fhlist.add(fh);
  FIN_IF(!x, -103);
  hr = 0;

fin:
  if (hr) {
    if (fh)
      delete fh;
    LOGe("%s: ERROR = %d (%s)", __func__, hr, obj.name);
  }
  return hr ? NULL : fh;
}

static void set_main_func(func_hook * fh, FuncMainHook func_addr)
{
  if (fh)
    fh->set_main_func(func_addr);
}

static void set_post_func(func_hook * fh, FuncPostHook func_addr)
{
  if (fh)
    fh->set_post_func(func_addr);
}

int plugin::create_hook(func_hook * fh, FuncPreHook func_addr, FuncPostHook post_hook)
{
  int hr = 0;

  if (!fh)
    return -79;
  fh->set_pre_func(func_addr);
  if (post_hook)
    fh->set_post_func(post_hook);

  hr = fh->create_trampoline();
  FIN_IF(hr, -10000 - hr);

  LOGd("[[%s hooked!]]", fh->get_name());
  hr = 0;

fin:
  if (hr) {
    LOGe("%s: ERROR = %d (%s)", __func__, hr, fh->get_name());
    if (m_fhlist.del(fh) == false)
      delete fh;
  }
  return hr;
}

// ============================================================================================

//char __userpurge WcxProcessor@<al>(int type_index@<eax>, WCHAR * ArcName@<edx>, WCHAR * ArcCurDir@<ecx>, int a4@<ebx>, int a5@<edi>, WCHAR * ArcNameOrDir@<esi>, char argX, UCHAR a8, char a9, UCHAR wcx_index, WCHAR * a11)
//int64 __usercall WcxProcessor@<rax>(int a1@<eax>, const WCHAR *ArcName@<rdx>, unsigned int type_index@<ecx>, const WCHAR *ArcCurDir@<r8>, _WORD *a5@<r9>, unsigned __int8 wcx_index, unsigned __int8 a7, unsigned __int8 a8, char argX)
void WCP_HOOK_CALL WcxProcessor(const func_hook & fh, tramp_data & td)
{
  filecachelist * fcl = (filecachelist *)fh.get_obj();
#ifdef _WIN64
  int type_index = (int)td.get_reg(xreg::cx);
  LPCWSTR ArcName = (LPCWSTR)td.get_reg(xreg::dx);
  LPWSTR ArcCurDir = (LPWSTR)td.get_reg(xreg::r8);
  BYTE wcx_index = (BYTE)td.get_arg(4);
  BYTE argX = (BYTE)td.get_arg(7);
  //LOGd("%s: ret_addr = %p <%p>", __func__, td.ret_addr, td.get_arg(-1));
  //WLOGd(L"%S: www [0] = %p, [1] = %p, [2] = %p, [3] = %p", __func__, td.get_arg(0), td.get_arg(1), td.get_arg(2), td.get_arg(3));
  //WLOGd(L"%S: www [4] = %p, [5] = %p, [6] = %p, [7] = %p", __func__, td.get_arg(4), td.get_arg(5), td.get_arg(6), td.get_arg(7));
  //WLOGd(L"%S: www [8] = %p, [9] = %p, [A] = %p, [B] = %p", __func__, td.get_arg(8), td.get_arg(9), td.get_arg(10), td.get_arg(11));
#else
  int type_index = (int)td.get_reg(xreg::ax);
  LPCWSTR ArcName = (LPCWSTR)td.get_reg(xreg::dx);
  LPWSTR ArcCurDir = (LPWSTR)td.get_reg(xreg::cx);
  BYTE wcx_index = (BYTE)td.get_arg(3);
  BYTE argX = (BYTE)td.get_arg(0);
#endif
  bool const init = (argX != 0);
  WLOGd_IF(init,  L"%S: ___________ Type = %d, ArcId = %d, ArcName = '%s', Dir = '%s'", __func__, type_index, wcx_index, ArcName, ArcCurDir);
  WLOGd_IF(!init, L"%S: ........... type = %d, arcid = %d, arcname = '%s', dir = '%s'", __func__, type_index, wcx_index, ArcName, ArcCurDir);
  fcl->WcxProcessor(init, type_index, ArcName, ArcCurDir, wcx_index);
}

//size_t __fastcall WcxProcessor_post(const func_hook & fh, size_t ret_value) noexcept
//{
//  LOGd("%s: fh = %p, ret_value = %p, ret_addr = %p", __func__, &fh, ret_value, _ReturnAddress());
//  return ret_value;
//}

/* void TObject@Free(eax) / void TObject@Free(rcx) */
void WCP_HOOK_CALL WcxProcessor_loop_end(const func_hook & fh, tramp_data & td)
{
  filecachelist * fcl = (filecachelist *)fh.get_obj();
  //LOGd("%s: ret_addr = %p, obj = %p", __func__, td.ret_addr, fc);
  fcl->WcxProcessor_loop_end();
}

/* int TcCreateFileInfo(ptr_aDoubleDot, (int)&ptrTcCreateFileInfo, 1, 0, 0, 16, -1, -1, a11a, 0, 0, 0); */

//int  __userpurge TcCreateFileInfo(LPCWSTR hd_FileName@<ecx>, int method@<eax>, char mode@<dl>, int a4, __int16 tcFileAttr, char hd_FileAttr, int fileSize_LO, int FileSize_HI, int a9, int a10, int a11, int a12)
//int64 __fastcall TcCreateFileInfo(LPVOID method<rcx>, UINT64 mode<rdx>, WCHAR * hd_FileName<r8>, _BYTE *a4<r9>, int fileSizeLO, int fileSizeHI, __int64 ftime, __int64 index, char hd_FileAttr, __int16 tcFileAttr, int UNK_4144)
void WCP_HOOK_CALL TcCreateFileInfo_dd(const func_hook & fh, tramp_data & td)
{
  filecachelist * fcl = (filecachelist *)fh.get_obj();
#ifdef _WIN64
  //char mode = (char)td.get_reg(xreg::dx);
  //LPCWSTR fileName = (LPCWSTR)td.get_reg(xreg::r8);
  //UINT16 tcAttr = (UINT16)td.get_arg(9);
  //BYTE fileAttr = (BYTE)td.get_arg(8);
  //INT64 fileSize = ((UINT64)td.get_arg(5) << 32) | (td.get_arg(4) & _UI32_MAX);
#else
  //char mode = (char)td.get_reg(xreg::dx);
  //LPCWSTR fileName = (LPCWSTR)td.get_reg(xreg::cx);
  //UINT16 tcAttr = (UINT16)td.get_arg(1);
  //BYTE fileAttr = (BYTE)td.get_arg(2);
  //INT64 fileSize = ((UINT64)td.get_arg(4) << 32) | (td.get_arg(3) & _UI32_MAX);
  //WLOGd(L"%S: ooo mode = %d, attr = %02X (%04X), size = %I64d, fileName = '%s' ", __func__, mode, fileAttr, tcAttr, fileSize, fileName);
  //fc->TcCreateFileInfo(mode, fileName, fileAttr, fileSize, tcAttr);
#endif
  fcl->TcCreateFileInfo(0, NULL, 0, 0, 0);
}

//int   __usercall wcsicmp@<eax>(WCHAR *wstr1@<eax>, WCHAR *wstr2@<edx>)
//int64 __usercall wcsicmp@<rax>(WCHAR *wstr1@<rcx>, WCHAR *wstr2@<rdx>)
size_t WCP_HOOK_CALL tc_wcsicmp(const func_hook & fh, tramp_data & td) noexcept
{
#ifdef _WIN64
  LPCWSTR const wstr1 = (LPCWSTR)td.get_reg(xreg::cx);
  LPCWSTR const wstr2 = (LPCWSTR)td.get_reg(xreg::dx);
#else
  LPCWSTR const wstr1 = (LPCWSTR)td.get_reg(xreg::ax);
  LPCWSTR const wstr2 = (LPCWSTR)td.get_reg(xreg::dx);
#endif
  int const res = StrCmpIW(wstr1, wstr2);
  if (res < 0)
    return (size_t)(-1);
  if (res > 0)
    return 1;
  return 0;
}

// ============================================================================================

int plugin::patch()
{
  int hr = -1;
  try {
    hr = patch_internal();
  } 
  catch(...) {
    LOGf("%s: <<<<<< FATAL ERROR >>>>>>", __func__);
  }
  return hr;
}

int plugin::patch_internal()
{
  int hr = 0;
  MEMORY_BASIC_INFORMATION mbi = {0};
  DWORD dwOldProt;
  BOOL xprot = FALSE;

  if (m_patched)
    return 0;

  m_patched = true;
  PBYTE image_base = (PBYTE)GetModuleHandleW(NULL);
  SIZE_T dwSize = VirtualQuery(image_base, &mbi, sizeof(mbi));
  FIN_IF(!dwSize, -10);
  dwSize = VirtualQuery((PBYTE)mbi.BaseAddress + mbi.RegionSize, &mbi, sizeof(mbi));
  FIN_IF(!dwSize, -11);
  FIN_IF((PBYTE)mbi.BaseAddress <= image_base, -12);
  FIN_IF((mbi.Protect & PAGE_EXEC_MASK) == 0, -13);
  m_codesec.base = (PBYTE)mbi.BaseAddress;
  m_codesec.size = mbi.RegionSize; 
  LOGd("code section = 0x%p ... 0x%p", mbi.BaseAddress, (PBYTE)mbi.BaseAddress + mbi.RegionSize);
  
  xprot = VirtualProtect(m_codesec.base, m_codesec.size, PAGE_EXECUTE_READWRITE, &dwOldProt);
  FIN_IF(!xprot, -17);

  hr = m_trampolines.init();
  FIN_IF(hr, -21);

  hr = get_exe_ver();
  FIN_IF(hr, -22);

  //LOGd("sizeof(TFileItem) = 0x%02X", sizeof(TFileItem));
  //LOGd("TFileItem.szHI    = 0x%02X", FIELD_OFFSET(TFileItem, sizeHI));
  //LOGd("TFileItem.ptr1    = 0x%02X", FIELD_OFFSET(TFileItem, ptr1));
  //LOGd("TFileItem.index   = 0x%02X", FIELD_OFFSET(TFileItem, index));
  //LOGd("TFileItem.attr    = 0x%02X", FIELD_OFFSET(TFileItem, attr));
  //LOGd("TFileItem.tcAttr  = 0x%02X", FIELD_OFFSET(TFileItem, tcAttr));
  //LOGd("TFileItem.ptr3    = 0x%02X", FIELD_OFFSET(TFileItem, ptr3));
  //LOGd("TFileItem.ptr4    = 0x%02X", FIELD_OFFSET(TFileItem, ptr4));

  hr = load_image_cfg();
  FIN_IF(hr, -23);

  hr = m_fcache.init(m_cfg, (LPCVOID)m_img.g_WcxItemList.addr);
  FIN_IF(hr, -24);

  int arg_reg_num;
  int arg_stk_num;
  func_hook * fh = NULL;

  {
#ifdef _WIN64
    arg_reg_num = 5;
    arg_stk_num = 4;
#else
    arg_reg_num = 6;
    arg_stk_num = 5;
#endif
    /* WcxProcessor: search string aUc2Dir = ">>> UC2 (DIR) <<<" */
    fh = add_func_hook(m_img.WcxProcessor, arg_reg_num, arg_stk_num);
    //set_post_func(fh, WcxProcessor_post);  // TEST
    hr = create_hook(fh, WcxProcessor);
    FIN_IF(hr, -31);
  }
  {
    arg_reg_num = 1;
    arg_stk_num = 0;
    /* WcxProcessor_loop_end: call TObject@Free(eax) / call TObject@Free(rcx) */
    fh = add_func_hook(m_img.WcxProcessor_loop_end, arg_reg_num, arg_stk_num, func_hook::ptCalleeAddr);
    hr = create_hook(fh, WcxProcessor_loop_end);
    FIN_IF(hr, -41);
  }
  {
#ifdef _WIN64
    arg_reg_num = 4;
    arg_stk_num = 7;
#else
    arg_reg_num = 3;
    arg_stk_num = 9;
#endif
    /* WcxProcessor: call TcCreateFileInfo(ptr_aDoubleDot) */
    fh = add_func_hook(m_img.TcCreateFileInfo_dd, arg_reg_num, arg_stk_num, func_hook::ptCalleeAddr);
    hr = create_hook(fh, TcCreateFileInfo_dd);
    FIN_IF(hr, -51);
  }
  if (m_cfg.get_patch_wcsicmp() && m_img.tc_wcsicmp.addr) {
    arg_reg_num = 2;
    arg_stk_num = 0;
    fh = add_func_hook(m_img.tc_wcsicmp, arg_reg_num, arg_stk_num);
    set_main_func(fh, tc_wcsicmp);
    hr = create_hook(fh, NULL);
    FIN_IF(hr, -61);
  }

  hr = 0;
  LOGn("%s: All functions successfully hooked!", __func__);

fin:  
  if (hr) {
    LOGe("%s: ERROR = %d", __func__, hr);
    m_fhlist.clear();
  }
  if (xprot) {
    VirtualProtect(m_codesec.base, m_codesec.size, dwOldProt, &dwOldProt);
  }
  return hr;
}

int plugin::get_exe_ver(modver * ver)
{
  int hr = -1;

  VS_FIXEDFILEINFO * vffi = nt::GetModuleFixedFileInfo(NULL);
  FIN_IF(!vffi, -2);
  //LOGd("Signature = %p ", vffi->dwSignature);
  //LOGd("dwStrucVersion = %p ", vffi->dwStrucVersion);
  //LOGd("dwFileVersionMS    = %p/%p ", vffi->dwFileVersionMS, vffi->dwFileVersionLS);
  //LOGd("dwProductVersionMS = %p/%p ", vffi->dwProductVersionMS, vffi->dwProductVersionLS);
  m_exever.major = (BYTE)HIWORD(vffi->dwFileVersionMS);
  m_exever.minor = (BYTE)LOWORD(vffi->dwFileVersionMS);
  m_exever.build = (BYTE)HIWORD(vffi->dwFileVersionLS);
  m_exever.revision = (BYTE)LOWORD(vffi->dwFileVersionLS);
  LOGn("TotalCmd version = %d.%d.%d.%d", m_exever.major, m_exever.minor, m_exever.build, m_exever.revision);
  if (ver)
    *ver = m_exever;
  hr = 0;

fin:
  return hr;
}


} /* namespace */
