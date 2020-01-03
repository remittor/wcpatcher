#pragma once

#include <windows.h>
#include "..\fake_ntdll\fake_ntdll.h"

namespace nt {

#define ntapi __declspec(nothrow) __cdecl 

DWORD ntapi BaseSetLastNTError(NTSTATUS Status);

BOOL ntapi GetFileInformationByHandleEx(HANDLE handle, FILE_INFO_BY_HANDLE_CLASS FileInfoClass, LPVOID info, size_t size);
INT64 ntapi GetFileCurrentPos(HANDLE handle);
BOOL ntapi GetFileIdByHandle(HANDLE handle, UINT64 * fid);
BOOL ntapi GetVolumeIdByHandle(HANDLE handle, UINT64 * vid);

BOOL ntapi SetFileInformationByHandle(HANDLE handle, FILE_INFO_BY_HANDLE_CLASS FileInfoClass, LPCVOID info, size_t size);
BOOL ntapi SetFileAttrByHandle(HANDLE handle, const FILE_BASIC_INFORMATION * fbi);
BOOL ntapi SetFileAttrByHandle(HANDLE handle, const FILE_BASIC_INFO * fbi);
BOOL ntapi DeleteFileByHandle(HANDLE handle);


PIMAGE_SECTION_HEADER ntapi RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeader, PVOID BaseAddress, ULONG Rva);
PVOID ntapi RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeader, PVOID BaseAddress, ULONG Rva, PIMAGE_SECTION_HEADER * SectionHeader);

PIMAGE_NT_HEADERS ntapi GetModuleNtHeader(PVOID BaseAddress);
LPVOID ntapi GetModuleResourseDir(PVOID BaseAddress, SIZE_T * resDirSize);
LPVOID ntapi GetModuleResourcePtr(PVOID BaseAddress, IMAGE_RESOURCE_DATA_ENTRY * entry);
VS_FIXEDFILEINFO * GetModuleFixedFileInfo(PVOID BaseAddress);


} /* namespace */
