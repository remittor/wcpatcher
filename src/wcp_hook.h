#pragma once

#include "stdafx.h"

//#include <stdint.h>
#define ASMJIT_STATIC
#define ASMJIT_SUPPRESS_STD_TYPES
#include "asmjit/asmjit.h"


namespace wcp {

using namespace asmjit;

namespace xreg {
  enum type {
    ax = 0,
    cx,
    dx,
    bx,
    sp,
    bp,
    si,
    di,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
  };
};  

#pragma pack(push, 1)
struct tramp_data
{
  SIZE_T   regs[17];
  SIZE_T   ret_addr;
  SIZE_T   reserved[2];

  SIZE_T get_reg(xreg::type reg) { return regs[(size_t)reg]; }
  SIZE_T get_arg(int pos) { return ((PSIZE_T)regs[(size_t)xreg::sp])[pos + 1]; }  /* stdcall, fastcall, etc */
  //SIZE_T get_arg_cd(int pos) { return ((PSIZE_T)regs[(size_t)xreg::sp])[arg_stk_num - pos + 1]; }  /* cdecl */
};
#pragma pack(pop)

static_assert(sizeof(tramp_data) % 16 == 0, "error_tramp_data");

class func_hook;   /* forward declaration */

#define WCP_HOOK_CALL __declspec(nothrow) __fastcall

typedef void   (__fastcall * FuncPreHook)  (const func_hook & fh, tramp_data & td);
typedef size_t (__fastcall * FuncMainHook) (const func_hook & fh, tramp_data & td);
typedef size_t (__fastcall * FuncPostHook) (const func_hook & fh, size_t ret_value);

struct fnprolog {
  PBYTE        addr;
  BYTE         data[32];
  size_t       size;
  int          esp_imm;
  
  int init(LPCVOID addr);
};

class func_hook
{
public:
  func_hook();
  ~func_hook();

  enum patch_type {
    ptFuncProlog,
    ptCalleeAddr,
  };

  void reset();
  void set_type(patch_type ptype) { m_type = ptype; }
  void set_obj(LPCVOID obj) { m_obj = obj; }
  void set_trampoline_memory(LPCVOID addr, size_t size);
  void set_pre_func(FuncPreHook func_addr)   { m_pre_hook_addr = func_addr; }
  void set_main_func(FuncMainHook func_addr) { m_main_hook_addr = func_addr; }
  void set_post_func(FuncPostHook func_addr) { m_post_hook_addr = func_addr; }

  int init(LPCSTR name, LPCVOID addr, int arg_reg_num, int arg_stk_num);
  int create_trampoline();

  CodeHolder & get_code_holder() { return m_code; }
  PBYTE  get_base_addr() const { return m_base_addr; }
  size_t get_prolog_size() const { return m_prolog.size; }
  PBYTE  get_trampoline_addr() const { return m_trampoline.addr; }
  LPCSTR get_name() const { return m_name; }
  patch_type get_type() const { return m_type; } 
  int get_arg_reg_num() const { return m_arg_reg_num; }
  int get_arg_stk_num() const { return m_arg_stk_num; }
  int get_arg_num() const { return m_arg_reg_num + m_arg_stk_num; }
  LPVOID get_obj() const { return (LPVOID)m_obj; }

private:  
  int init_asm();
  X86Mem get_tramp_data_ptr(int field_offset, int size = 0);
  X86Mem get_reg_ptr(xreg::type reg);
  X86Mem get_ret_addr_ptr();

  int a_save_regs(bool save_esp = true);
  int a_restore_regs();
  int a_restore_zsp();
  int a_init_trampoline();
  int a_replace_ret_addr();
  int a_call_hook(LPVOID func_addr);
  int a_call_pre_hook();
  int a_insert_prolog_and_jmp();

  int delete_patch();

  int get_arg_stk_size(bool with_ret_addr = true) { return (m_arg_stk_num + (int)with_ret_addr) * sizeof(SIZE_T); }

  CodeHolder     m_code;            /* code generator */
  X86Assembler   m_asm;
  ArchInfo::Type m_arch;
  int            m_arch_word_size;
  
  patch_type     m_type;
  PBYTE          m_base_addr;
  size_t         m_fn_abs_addr;
  INT32          m_fn_rel_addr;     /* offset for original call addr (only for ptCalleeAddr) */
  int            m_arg_reg_num;
  int            m_arg_stk_num;
  char           m_name[96];        /* func name */
  
  bool           m_orig_patched;    /* true if prolog original func is patched */
  fnprolog       m_prolog;
  struct {
    PBYTE        addr;
    size_t       capacity;
    size_t       size;
  } m_trampoline;
  int            m_zsp_offset;      /* correction for ESP */
  LPCVOID        m_obj;

  FuncPreHook    m_pre_hook_addr;
  FuncMainHook   m_main_hook_addr;
  FuncPostHook   m_post_hook_addr;

  size_t         m_offset_new_ret_addr;
  
  X86Gp zax;
  X86Gp zbx;
  X86Gp zcx;
  X86Gp zdx;
  X86Gp zsp;
  X86Gp zbp;
  X86Gp zsi;
  X86Gp zdi;
};

} /* namespace */
