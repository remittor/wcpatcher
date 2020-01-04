#include "wcp_plugin.h"


HANDLE g_hInstance = NULL;
wcp::plugin g_wcp;


void WINAPI ContentSetDefaultParams(ContentDefaultParamStruct * dps)
{
  #pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  //LOGt("%s: ver = %d.%d, ini = \"%s\" ", __func__, dps->PluginInterfaceVersionHi, dps->PluginInterfaceVersionLow, dps->DefaultIniName);
  g_wcp.patch();
}

int WINAPI ContentGetSupportedField(int FieldIndex, LPCSTR FieldName, LPCSTR Units, int maxlen)
{
  #pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  //LOGt("%s: FieldIndex = %d FieldName = '%s' ", __func__, FieldIndex, FieldName);
  return ft_nomorefields;
}

int WINAPI ContentGetValue(LPCSTR FileName, int FieldIndex, int UnitIndex, LPVOID FieldValue, int maxlen, int flags)
{
  #pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
  //LOGt("%s: FileName = '%s' FieldIndex = %d, UnitIndex = %d, flags = 0x%08X ", __func__, FileName, FieldIndex, UnitIndex);
  return ft_nosuchfield;
}

// ==================================================================================================

extern "C" BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved);

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpReserved)
{
  if (fdwReason == DLL_PROCESS_ATTACH) {
    volatile LPVOID ep = _DllMainCRTStartup;
    DWORD tid = GetCurrentThreadId();
    g_hInstance = hInstDLL;
    LOGn("WCP Plugin Loaded ===================== Thread ID = 0x%04X ========", tid);
    g_wcp.init(hInstDLL, tid);
  }
  if (fdwReason == DLL_PROCESS_DETACH) {
    LOGn("WCP Plugin unload! ------------------------------------------------");
  }
  return TRUE;
}

