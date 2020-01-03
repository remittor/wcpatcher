#pragma once

#include "stdafx.h"


namespace wcp {

static const char wcp_trampoline_region[] = "WCP_TRAMPOLINE_REGION";

#define ALIGN_UP_BY(Address, Align) (((SIZE_T)(Address) + (Align) - 1) & ~((Align) - 1))

#define PAGE_EXEC_MASK  (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

PBYTE find_str_in_memory(LPCVOID start_addr, LPCVOID str, SIZE_T strsize, DWORD protect_mask, bool skip_first = true) noexcept;
PBYTE find_memory_for_alloc(LPCVOID start_addr, size_t size, bool skip_first = true) noexcept;

void LOG_BYTECODE(int log_level, LPVOID addr, size_t size) noexcept;


class tramp_mm
{
public:
  static const size_t len_trampoline_header = 32;
  static const size_t max_trampoline_num = 7;
  static const size_t max_trampoline_size = 0x200;  /* 512 bytes */

  tramp_mm() noexcept : m_base_addr(0), m_count(0) { }
  ~tramp_mm() noexcept { }

  int init(LPCVOID start = NULL) noexcept;
  PBYTE alloc(size_t & size) noexcept;
  PBYTE get_base_addr() { return m_base_addr; }
  size_t count() { return m_count; }

private:
  PBYTE    m_base_addr;
  size_t   m_count;
};


} /* namespace */
