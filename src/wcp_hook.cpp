#include "stdafx.h"
#include "wcp_hook.h"
#include "wcp_memory.h"
#include <intrin.h>


namespace wcp {

using namespace asmjit;

int fnprolog::init(LPCVOID address)
{
  int hr = -1;
  addr = NULL;
  size = 0;
  PBYTE p = (PBYTE)address;
#ifdef _WIN64
  DWORD header = *(PDWORD)p;
  FIN_IF(header != '\x55\x48\x89\xE5', -2);  /* push  rbp ; mov  rbp, rsp */
  p += 4;
  if ((*(PDWORD)p & 0xFFFFFF) == '\x48\x81\xEC') {    /* sub  rsp, imm32 */
    p += 3;
    esp_imm = *(PINT32)p;
    p += 4;
    FIN(0);
  }
#else
  DWORD header = *(PDWORD)p & 0xFFFFFF;
  FIN_IF(header != '\x55\x8B\xEC', -2);  /* push  ebp ; mov  ebp, esp */
  p += 3;
  if (*(PWCHAR)p == '\x81\xC4') {        /* add  esp, imm32 */
    p += 2;
    esp_imm = *(PINT32)p;
    p += 4;
    FIN(0);
  }
  if (*(PWCHAR)p == '\x83\xC4') {        /* add  esp, imm8 */
    p += 2;
    esp_imm = *(PINT8)p;
    p += 1;
    FIN(0);
  }
  if (*p >= 0x50 && *p <= 0x57 ) {         /* push eax ... edi */
    if (p[1] >= 0x50 && p[1] <= 0x57 ) {   /* push eax ... edi */
      p += 2;
      FIN(0);
    }
  }
  if (p[0] == 0x50 && p[1] == 0xB8) {      /* push eax ; mov eax, imm32 */
    p += 2 + 4;
    FIN(0);
  }
  if (p[0] == 0x53 && p[1] == 0x8B && p[2] == 0xD8) {   /* push ebx ; mov ebx, eax */
    p += 3;
    FIN(0);
  }
#endif
fin:
  if (hr == 0) {
    addr = (PBYTE)address;
    size = (int)(p - (PBYTE)addr);
    if (size < 5)
      return -3;
    memcpy(data, addr, size);
  }
  return hr;
}

func_hook::func_hook()
{
  m_type = ptFuncProlog;
  m_orig_patched = false;
  m_base_addr = NULL;
  m_fn_abs_addr = 0;
  m_fn_rel_addr = 0;
  m_obj = NULL;
  memset(&m_prolog, 0, sizeof(m_prolog));
  memset(&m_trampoline, 0, sizeof(m_trampoline));
}

func_hook::~func_hook()
{
  delete_patch();
  reset();
}

void func_hook::reset()
{
  m_offset_new_ret_addr = 0;
  m_pre_hook_addr = NULL;
  m_main_hook_addr = NULL;
  m_post_hook_addr = NULL;
  m_arg_reg_num = 0;
  m_arg_stk_num = 0;
  m_zsp_offset = 0;
  m_code.reset();
  m_trampoline.size = 0;
}

void func_hook::set_trampoline_memory(LPCVOID addr, size_t size)
{
  m_trampoline.addr = (PBYTE)addr;
  m_trampoline.capacity = size;
  m_trampoline.size = 0;
}

int func_hook::init(LPCSTR name, LPCVOID addr, int arg_reg_num, int arg_stk_num)
{
  int hr = 0;
  FIN_IF(m_base_addr, -1);
  reset();
  memset(m_name, 0, sizeof(m_name));
  strncpy(m_name, name, sizeof(m_name)-1);
  LOGd("%s: <0x%p> %s ...", __func__, addr, name);
  FIN_IF(!addr, -2);
  FIN_IF(!m_trampoline.addr, -2);
  FIN_IF(!m_trampoline.capacity, -2);
  FIN_IF((PBYTE)addr > m_trampoline.addr, -3);
  m_arg_reg_num = arg_reg_num;
  m_arg_stk_num = arg_stk_num;
  //LOGd("%s: Trampoline CODE = %p", __func__, m_trampoline.code);
  if (m_type == ptFuncProlog) {
    hr = m_prolog.init(addr);
    LOGe_IF(hr, "%s: ERROR init_prolog = %d", __func__, hr);
    FIN_IF(hr, -5);
    m_fn_rel_addr = 0;
    m_fn_abs_addr = (size_t)addr;
  }
  if (m_type == ptCalleeAddr) {
    PBYTE p = (PBYTE)addr;
    FIN_IF(*p != 0xE8, -6);    /* call rel32 */
    m_prolog.size = 5;
    memcpy(m_prolog.data, p, m_prolog.size);
    p++;
    m_fn_rel_addr = *(PINT32)p;
    m_fn_abs_addr = (SIZE_T)addr + 5 + (SSIZE_T)m_fn_rel_addr;
  }
  m_base_addr = (PBYTE)addr;
#ifdef _WIN64
  m_arch = ArchInfo::kTypeX64;
  m_arch_word_size = 8;
#else  
  m_arch = ArchInfo::kTypeX86;
  m_arch_word_size = 4;
#endif
  m_code.init(CodeInfo(m_arch, ArchInfo::kSubTypeNone, (INT64)m_trampoline.addr));
  m_code.attach(&m_asm);
  hr = 0;
fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

int func_hook::init_asm()
{
  X86Assembler & a = m_asm;
  zax = a.zax();
  zbx = a.zbx();
  zcx = a.zcx();
  zdx = a.zdx();
  zsp = a.zsp();
  zbp = a.zbp();
  zsi = a.zsi();
  zdi = a.zdi();
  return 0;
}

X86Mem func_hook::get_tramp_data_ptr(int field_offset, int size)
{
  int offset = m_zsp_offset - sizeof(tramp_data) + field_offset;
  return X86Mem(zsp, offset, size ? size : m_arch_word_size);  /* 4 = dword_ptr, 8 = qword_ptr */
}

X86Mem func_hook::get_reg_ptr(xreg::type reg)
{
  int offset = FIELD_OFFSET(tramp_data, regs) + (int)reg * sizeof(SIZE_T);
  int size = 0;
  //if (reg == xreg::cs) {
  //  size = 2;
  //}
  return get_tramp_data_ptr(offset, size);
}

X86Mem func_hook::get_ret_addr_ptr()
{
  return get_tramp_data_ptr(FIELD_OFFSET(tramp_data, ret_addr));
}

int func_hook::a_save_regs(bool save_esp)
{
  X86Assembler & a = m_asm;
  a.mov(get_reg_ptr(xreg::ax), zax);
  a.mov(get_reg_ptr(xreg::bx), zbx);
  a.mov(get_reg_ptr(xreg::cx), zcx);
  a.mov(get_reg_ptr(xreg::dx), zdx);
  a.mov(get_reg_ptr(xreg::bp), zbp);
  a.mov(get_reg_ptr(xreg::si), zsi);
  a.mov(get_reg_ptr(xreg::di), zdi);
#ifdef _WIN64
  a.mov(get_reg_ptr(xreg::r8), x86::r8);
  a.mov(get_reg_ptr(xreg::r9), x86::r9);
  a.mov(get_reg_ptr(xreg::r10), x86::r10);
  a.mov(get_reg_ptr(xreg::r11), x86::r11);
#endif
  if (save_esp && m_zsp_offset == 0) {
    a.mov(get_reg_ptr(xreg::sp), zsp);
  }
  return 0;
}

int func_hook::a_restore_regs()
{
  X86Assembler & a = m_asm;
  a.mov(zax, get_reg_ptr(xreg::ax));
  a.mov(zbx, get_reg_ptr(xreg::bx));
  a.mov(zcx, get_reg_ptr(xreg::cx));
  a.mov(zdx, get_reg_ptr(xreg::dx));
  a.mov(zbp, get_reg_ptr(xreg::bp));
  a.mov(zsi, get_reg_ptr(xreg::si));
  a.mov(zdi, get_reg_ptr(xreg::di));
#ifdef _WIN64
  a.mov(x86::r8, get_reg_ptr(xreg::r8));
  a.mov(x86::r9, get_reg_ptr(xreg::r9));
  a.mov(x86::r10, get_reg_ptr(xreg::r10));
  a.mov(x86::r11, get_reg_ptr(xreg::r11));
#endif
  return 0;
}

int func_hook::a_restore_zsp()
{
  X86Assembler & a = m_asm;
  a.mov(zsp, get_reg_ptr(xreg::sp));
  m_zsp_offset = 0;
  return 0;
}

int func_hook::a_init_trampoline()
{
  X86Assembler & a = m_asm;
  m_zsp_offset = 0;
  a_save_regs();                      /* save original reg values to tramp_data */
  a.mov(zax, x86::ptr(zsp));          /* EAX = original ret_addr */
  a.mov(get_ret_addr_ptr(), zax);     /* save original ret_addr */
  return 0;
}

int func_hook::a_replace_ret_addr()
{
  X86Assembler & a = m_asm;
  
  SSIZE_T cnt = get_arg_stk_size() / sizeof(SIZE_T);
  m_zsp_offset = sizeof(tramp_data) + get_arg_stk_size();
  a.mov(zcx, cnt);
  a.mov(zsi, zsp);
  a.sub(zsp, m_zsp_offset);
  a.mov(zdi, zsp);
  a.rep().movsd();

  a.pop(zax);                                /* remove top value on stack */
  m_offset_new_ret_addr = a.getOffset();     /* save code offset for patching */
  a.mov(zax, Imm((INT64)SIZE_MAX));          /* EAX = new ret_addr */
  a.push(zax);                               /* set new ret_addr on topstk */
  return 0;
}

int func_hook::a_call_hook(LPVOID func_addr)
{
  X86Assembler & a = m_asm;
  if (m_post_hook_addr) {
    int tramp_data_offset = get_arg_stk_size();
    a.mov(zdx, zsp);
    a.add(zdx, tramp_data_offset);           /* EDX = ptr of tramp_data */
  } else {
    m_zsp_offset = sizeof(tramp_data);
    a.sub(zsp, m_zsp_offset);
    a.mov(zdx, zsp);                         /* EDX = ptr of tramp_data */
  }
  a.mov(zcx, (UINT64)this);                  /* ECX = current object ptr */
#ifdef _WIN64
  size_t arg_size = 4*8 + 8;                 /* sizeof(RCX+RDX+R8+R9) + align16 because ret_addr will be added */
  a.sub(zsp, arg_size);                      /* reserve space for persistent args */
#endif
  if ((SIZE_T)func_addr - (SIZE_T)m_trampoline.addr > _I32_MAX - 32) {   /* actual for x64 only */
    a.mov(zax, (INT64)func_addr);
    a.call(zax);                             /* call FuncHook(ECX,EDX) */
  } else {
    a.call(Imm((INT64)func_addr));           /* call FuncHook(ECX,EDX) */
  }
#ifdef _WIN64
  a.add(zsp, arg_size);                      /* free persistent args space */
#endif
  return 0;
}

int func_hook::a_call_pre_hook()
{
  return a_call_hook((LPVOID)m_pre_hook_addr);
}

int func_hook::a_insert_prolog_and_jmp()
{
  int hr = -1;
  X86Assembler & a = m_asm;

  FIN_IF(m_trampoline.addr < get_base_addr(), -33);

  a_restore_regs();              /* restore original regs */
  if (!m_post_hook_addr) {
    a_restore_zsp();
  }
  a.embed(m_prolog.data, (int)m_prolog.size);      /* insert original prolog */
  a.jmp(Imm((INT64)get_base_addr() + m_prolog.size));    /* JMP rel32 */
  m_trampoline.size = a.getOffset();
  //INT64 jmp_imm = (INT64)(get_addr() + m_prolog.size) - (INT64)(m_trampoline.code + m_trampoline.size);
  //LOGd("hook(%p) + hook_end(%p) = %p", m_trampoline.code, m_trampoline.size, m_trampoline.code + m_trampoline.size);
  //LOGd("jmp_imm = 0x%08X  jmp_addr = 0x%08X", (DWORD)jmp_imm, (DWORD)(m_trampoline.code + m_trampoline.size) + (DWORD)jmp_imm);

  if (m_post_hook_addr) {
#ifdef _WIN64
    FIN(-77);
#endif
    int zsp_delta = sizeof(tramp_data) + get_arg_stk_size();
    a.add(zsp, zsp_delta);                            /* fix ESP */
    m_zsp_offset = 0L - get_arg_stk_size();
    a.mov(zdx, zax);                                  /* EDX = ret_value */
    a.mov(zcx, (UINT64)this);                         /* ECX = current object ptr */
    a.mov(zax, get_ret_addr_ptr());
    a.push(zax);                                      /* set ret_addr to topstk */
    a.jmp(Imm((INT64)m_post_hook_addr));              /* call FuncPostHook(ECX,EDX) */
    { /* patch new ret_addr */
      size_t cur_offset = a.getOffset();
      a.setOffset(m_offset_new_ret_addr);
      a.mov(zax, Imm((INT64)m_trampoline.addr + m_trampoline.size));  /* set new ret_addr */
      a.setOffset(cur_offset);
      m_trampoline.size = cur_offset;
    }
  }
  hr = 0;
fin:
  return hr;
}

int func_hook::create_trampoline()
{
  int hr = -1;

  FIN_IF(m_trampoline.addr < get_base_addr(), -3);
  FIN_IF(m_prolog.size <= 0, -4);
  FIN_IF(!m_pre_hook_addr && !m_main_hook_addr && !m_post_hook_addr, -5);

  init_asm();
  a_init_trampoline();
  if (m_main_hook_addr) {
    FIN_IF(m_pre_hook_addr || m_post_hook_addr, -6);
    a_call_hook(m_main_hook_addr);
  } else {
    if (m_post_hook_addr) {
      a_replace_ret_addr();
    }
    if (m_pre_hook_addr) {
      a_call_pre_hook();
    }
  }
  if (m_type == ptFuncProlog) {
    if (m_main_hook_addr) {
      X86Assembler & a = m_asm;
      a_restore_zsp();
      a.ret(m_arg_stk_num * m_arch_word_size);         /* remove args from stack and return to caller */
      m_trampoline.size = a.getOffset();
    } else {
      hr = a_insert_prolog_and_jmp();
      FIN_IF(hr, hr);
    }
  }
  if (m_type == ptCalleeAddr) {
    FIN_IF(m_main_hook_addr, -35);  // TODO
    FIN_IF(m_post_hook_addr, -36);  // TODO
    X86Assembler & a = m_asm;
    a_restore_regs();                                  /* restore original regs */
    a_restore_zsp();                                   /* restore original topstk */
    a.jmp(Imm((INT64)m_fn_abs_addr));                  /* jmp to original func */
    m_trampoline.size = a.getOffset();
  }

  size_t codeSize = m_code.getCodeSize();  /* strongly required !!! */
  FIN_IF(codeSize + 8 >= m_trampoline.capacity, -67);

  size_t relocSize = m_code.relocate(m_trampoline.addr);
  LOGd("tramp_addr = %p (%Id) [%Id]; this = %p, pre_hook = %p", m_trampoline.addr, relocSize, codeSize, this, m_pre_hook_addr);
  m_trampoline.size = relocSize;

  m_orig_patched = true;
  size_t tramp_addr_offset = (size_t)m_trampoline.addr - (size_t)get_base_addr() - 5;
  PBYTE p = get_base_addr();
  if (m_type == ptFuncProlog) {
    *p++ = 0xE9;    /* jmp rel32 */       // it doesn't look like to hot-patch!!! 
    *(PDWORD)p = (DWORD)tramp_addr_offset;
    p += 4;
    for (size_t i = 5; i < m_prolog.size; i++) {
      *p++ = 0x90;    /* nop */
    }
  }
  if (m_type == ptCalleeAddr) {
    p++;
    *(PDWORD)p = (DWORD)tramp_addr_offset;
  }
  //LOG_BYTECODE(LL_DEBUG, get_base_addr(), m_prolog.size);
  LOG_BYTECODE(LL_DEBUG, m_trampoline.addr, m_trampoline.size);
  hr = 0;

fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

int func_hook::delete_patch()
{
  int hr = -1;
  PBYTE p = get_base_addr();
  MEMORY_BASIC_INFORMATION mbi = {0};
  FIN_IF(!m_orig_patched, 0);
  SIZE_T dwSize = VirtualQuery(p, &mbi, sizeof(mbi));
  FIN_IF(!dwSize, -2);
  FIN_IF(!mbi.RegionSize, -3);
  FIN_IF((mbi.Protect & PAGE_EXECUTE_READWRITE) == 0, -4);
  if (m_type == ptFuncProlog) {
    memcpy(p, m_prolog.data, m_prolog.size);
  }
  if (m_type == ptCalleeAddr) {
    p++;    /* skip 0xE8 */
    *(PINT32)p = m_fn_rel_addr;
  }
  LOGi("%s: delete patch for %s", __func__, m_name);
fin:
  return 0;
}

} /* namespace */
