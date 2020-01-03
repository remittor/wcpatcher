#pragma once

#include <windows.h>
#include "bst\string.hpp"

__forceinline
int GetTickBetween(DWORD prev, DWORD now) noexcept
{
  if (prev == now)
    return 0;
  if (prev < now)
    return (int)(now - prev);
  return (int)((0xFFFFFFFF - prev) + now); 
}


int get_full_filename(LPCWSTR path, LPCWSTR name, LPWSTR fullname, size_t fullname_max_len) noexcept;
int get_full_filename(LPCWSTR fn, LPWSTR fullname, size_t fullname_max_len) noexcept;
int get_full_filename(LPCWSTR fn, LPWSTR * fullname) noexcept;

int get_full_filename(LPCWSTR path, LPCWSTR name, bst::wstr & fullname) noexcept;
int get_full_filename(LPCWSTR fn, bst::wstr & fullname) noexcept;

int get_full_filename(LPCWSTR path, LPCWSTR name, bst::filepath & fullname) noexcept;
int get_full_filename(LPCWSTR fn, bst::filepath & fullname) noexcept;


class time_meter
{
public:
  time_meter() noexcept { m_freq.QuadPart = 1; m_init.QuadPart = 0; }

  ~time_meter() noexcept { }

  void init() noexcept
  {
    QueryPerformanceFrequency(&m_freq);
    QueryPerformanceCounter(&m_init);
  }

  INT64 get_diff() noexcept
  {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return now.QuadPart - m_init.QuadPart;
  }

  double get_diff_msec() noexcept { return ((double)get_diff() * 1000.0) / (double)m_freq.QuadPart; }

private:
  LARGE_INTEGER     m_freq;
  LARGE_INTEGER     m_init;
};

