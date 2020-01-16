#include "stdafx.h"
#include "wcp_memory.h"

namespace wcp {

PBYTE find_str_in_memory(LPCVOID start_addr, LPCVOID str, SIZE_T strsize, DWORD protect_mask, bool skip_first) noexcept
{
  int hr = 0;
  size_t rc = 0;
  SYSTEM_INFO sysinfo;
  MEMORY_BASIC_INFORMATION mbi;
  if (start_addr == NULL) {
    start_addr = (LPCVOID)GetModuleHandleW(NULL);
    FIN_IF(!start_addr, -2);
  }
  GetSystemInfo(&sysinfo);
  strsize = (strsize == 0) ? strlen((LPCSTR)str) : strsize;
  PBYTE addr = (PBYTE)start_addr;
  while (1) {
    SIZE_T dwSize = VirtualQuery(addr, &mbi, sizeof(mbi));
    FIN_IF(!dwSize, -3);
    FIN_IF(mbi.RegionSize < sysinfo.dwPageSize, -4);  /* PAGE_SIZE */
    addr += mbi.RegionSize;
    rc++;
    if (rc == 1 && skip_first)
      continue;
    if (mbi.AllocationBase != mbi.BaseAddress) continue;
    if ((mbi.State & MEM_COMMIT) == 0) continue;
    if ((mbi.Type & MEM_PRIVATE) == 0) continue;
    if ((mbi.Protect & protect_mask) == 0) continue;
    if (mbi.Protect & PAGE_NOACCESS) continue;
    if (memcmp(mbi.BaseAddress, str, strsize) != 0) continue;
    return (PBYTE)mbi.BaseAddress;
  }
fin:
  return 0;
}

PBYTE find_memory_for_alloc(LPCVOID start_addr, size_t size, bool skip_first) noexcept
{
  int hr = 0;
  size_t rc = 0;
  SYSTEM_INFO sysinfo;
  MEMORY_BASIC_INFORMATION mbi;
  GetSystemInfo(&sysinfo);
  size = ALIGN_UP_BY(size, (size_t)sysinfo.dwPageSize);
  //LOGd("%s: AllocationGranularity = 0x%08X  size = 0x%08X", __func__, sysinfo.dwAllocationGranularity, (DWORD)size);
  PBYTE addr = (PBYTE)start_addr;
  while (1) {
    SIZE_T dwSize = VirtualQuery(addr, &mbi, sizeof(mbi));
    FIN_IF(!dwSize, -3);
    FIN_IF(mbi.RegionSize < sysinfo.dwPageSize, -4);  /* PAGE_SIZE */
    addr += mbi.RegionSize;
    rc++;
    if (rc == 1 && skip_first)
      continue;
    //if ((SIZE_T)mbi.BaseAddress < 0x2400000)
    //  LOGd("[%p] addr = %p  size = 0x%08X  state = 0x%08X  type = 0x%08X  prot = 0x%04X",
    //    mbi.AllocationBase, mbi.BaseAddress, (DWORD)mbi.RegionSize, mbi.State, mbi.Type, mbi.Protect);
    if (mbi.State != MEM_FREE) continue;
    if (mbi.RegionSize < size) continue;
    PBYTE mem = (PBYTE)ALIGN_UP_BY(mbi.BaseAddress, (size_t)sysinfo.dwAllocationGranularity);
    if (mem > (PBYTE)mbi.BaseAddress && mem < addr) {
      if ((size_t)(addr - mem) >= size)
        return mem;
    }
    return (PBYTE)mbi.BaseAddress;
  }
fin:
  return 0;
}

void LOG_BYTECODE(int log_level, LPVOID addr, size_t size) noexcept
{
  char buf[2049];
  PBYTE pb = (PBYTE)addr;
  if (size == 0) {
    size_t sz = 0;
    int k = 0;
    while (k < 5) {
      if (*pb++ == 0)
        k++;
      sz++;
    }
    if (k < 5) {
      LOGX(log_level, "ByteCode[%p] = <unknown size>", addr);
      return;
    }
    size = sz - 5;
  }
  memset(buf, 0, sizeof(buf));
  pb = (PBYTE)addr;  
  char * str = buf;
  for (size_t i = 0; i <= size; i++) {
    int x = sprintf(str, "%02X ", pb[i]);
    str += x;
    if (str + 8 >= buf + sizeof(buf))
      break;
  }
  LOGX(log_level, "ByteCode[%p] = %s", addr, buf);
}

int tramp_mm::init(LPCVOID start) noexcept
{
  int hr = -11;
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  FIN_IF(max_trampoline_size < 300, -11);
  m_base_addr = NULL;
  size_t size = len_trampoline_header + max_trampoline_size * max_trampoline_num;
  size = ALIGN_UP_BY(size, (size_t)sysinfo.dwPageSize);
  if (start == NULL) {
    start = (LPCVOID)GetModuleHandleW(NULL);
    FIN_IF(!start, -2);
  }
  PBYTE addr = (PBYTE)start;
  while (1) {
    addr = find_memory_for_alloc(addr, size);
    FIN_IF(!addr, -12);
    //FIN_IF((SSIZE_T)addr <= 0, (int)(SSIZE_T)addr);
    LPVOID mem = VirtualAlloc(addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    //LOGe_IF(!mem, "%s: ERROR = %d (0x%p)", __func__, GetLastError(), addr);
    if (mem) {
      FIN_IF((PBYTE)mem != addr, -13);
      m_base_addr = addr;
      break;
    }
  }
  //FIN_IF(!m_base_addr, -14);
  FIN_IF((SIZE_T)m_base_addr - (SIZE_T)start > 0x50000000, -15);
  memcpy(m_base_addr, wcp_trampoline_region, sizeof(wcp_trampoline_region));
  m_count = 0;  
  hr = 0;
  LOGd("%s: Trampoline base addr = %p", __func__, m_base_addr);
fin:
  LOGe_IF(hr, "%s: ERROR = %d", __func__, hr);
  return hr;
}

PBYTE tramp_mm::alloc(size_t & size) noexcept
{
  if (m_count >= max_trampoline_num)
    return NULL;
  size = max_trampoline_size;
  return m_base_addr + len_trampoline_header + max_trampoline_size * (m_count++);
}

} /* namespace */
