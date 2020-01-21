#define _CRT_SECURE_NO_WARNINGS

#include "log.h"
#include <stdio.h>
#include <vadefs.h>

static int  g_log_level = LL_TRACE;
static const char g_log_level_str[] = WCP_LL_STRING "--------------------------";

void WcpSetLogLevel(int level) noexcept
{
  if (level < 0) {
    g_log_level = -1;
  } else if (level > LL_TRACE) {
    g_log_level = LL_TRACE;
  } else {
    g_log_level = level;
  }
}

void WcpPrintMsgA(int level, const char * fmt, ...) noexcept
{
  char buf[4096];
  if (level <= g_log_level && level > 0) {
    va_list argptr;
    va_start(argptr, fmt);
    const size_t prefix_len = 8;
    memcpy(buf, "[WCP-X]", prefix_len - 1);
    buf[prefix_len - 3] = g_log_level_str[level];
    buf[prefix_len - 1] = 0x20;
    int len = _vsnprintf(buf + prefix_len, _countof(buf)-2, fmt, argptr);
    va_end(argptr);
    if (len < 0) {
      strcat(buf, "<INCORRECT-INPUT-DATA> ");
      strcat(buf, fmt);
    } else {
      len += prefix_len;
      buf[len] = 0;
    }
    OutputDebugStringA(buf);
  }
}

void WcpPrintMsgW(int level, const wchar_t * fmt, ...) noexcept
{
  union {
    struct {
      wchar_t reserved[4];
      wchar_t wbuf[4096];
    };
    char buf[4096 * 2];
  } u;
  if (level <= g_log_level && level > 0) {
    va_list argptr;
    va_start(argptr, fmt);
    const size_t prefix_len = 8;
    memcpy(u.wbuf, L"[WCP-X]", (prefix_len - 1) * sizeof(wchar_t));
    u.wbuf[prefix_len - 3] = (wchar_t)g_log_level_str[level];
    u.wbuf[prefix_len - 1] = 0x20;
    int len = _vsnwprintf(u.wbuf + prefix_len, _countof(u.wbuf)-2, fmt, argptr);
    va_end(argptr);
    if (len < 0) {
      wcscat(u.wbuf, L"<INCORRECT-INPUT-DATA> ");
      wcscat(u.wbuf, fmt);
      len = (int)wcslen(u.wbuf);
    } else {
      len += prefix_len;
    }
    len = WideCharToMultiByte(CP_ACP, 0, u.wbuf, len, (LPSTR)u.buf, sizeof(u.buf)-2, NULL, NULL);
    u.buf[(len > 0) ? len : 0] = 0;
    OutputDebugStringA(u.buf);
  }
}
