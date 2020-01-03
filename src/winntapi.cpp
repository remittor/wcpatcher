#include "stdafx.h"
#include "winntapi.h"

#define WIN32_NO_STATUS
#include <ntstatus.h>

namespace nt {

BOOL GetFileInformationByHandleEx(HANDLE handle, FILE_INFO_BY_HANDLE_CLASS FileInfoClass, LPVOID info, size_t size)
{
  NTSTATUS status;
  IO_STATUS_BLOCK io;

  switch (FileInfoClass) {
    case FileBasicInfo:
      status = NtQueryInformationFile(handle, &io, info, (ULONG)size, FileBasicInformation);
      break;

    case FileStandardInfo:
      status = NtQueryInformationFile(handle, &io, info, (ULONG)size, FileStandardInformation);
      break;

    case FileNameInfo:
      status = NtQueryInformationFile(handle, &io, info, (ULONG)size, FileNameInformation);
      break;

    case FileIdBothDirectoryRestartInfo:
    case FileIdBothDirectoryInfo:
      status = NtQueryDirectoryFile(handle, NULL, NULL, NULL, &io, info, (ULONG)size,
        FileIdBothDirectoryInformation, FALSE, NULL,
        (FileInfoClass == FileIdBothDirectoryRestartInfo) );
      break;

    default:
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
  }

  if (status != NT_STATUS_SUCCESS) {
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }
  return TRUE;
}

INT64 GetFileCurrentPos(HANDLE handle)
{
  IO_STATUS_BLOCK io;
  LARGE_INTEGER pos;
  NTSTATUS status = NtQueryInformationFile(handle, &io, &pos, sizeof(pos), FilePositionInformation);
  if (status != NT_STATUS_SUCCESS) {
    SetLastError(RtlNtStatusToDosError(status));
    return -1LL;
  }
  return pos.QuadPart;
}

BOOL GetFileIdByHandle(HANDLE handle, UINT64 * fid)
{
  NTSTATUS status;
  IO_STATUS_BLOCK io;
  __declspec(align(8)) FILE_INTERNAL_INFORMATION fii;

  status = NtQueryInformationFile(handle, &io, &fii, sizeof(fii), FileInternalInformation);
  if (status != NT_STATUS_SUCCESS) {
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }
  *fid = (UINT64)fii.IndexNumber.QuadPart;
  return TRUE;
}

BOOL GetVolumeIdByHandle(HANDLE handle, UINT64 * vid)
{
  NTSTATUS status;
  IO_STATUS_BLOCK io;
  __declspec(align(8)) BYTE buf[sizeof(FILE_FS_VOLUME_INFORMATION) + 400];
  FILE_FS_VOLUME_INFORMATION * fvi = (FILE_FS_VOLUME_INFORMATION *)buf;

  status = NtQueryVolumeInformationFile(handle, &io, fvi, sizeof(buf), FileFsVolumeInformation);
  if (status != NT_STATUS_SUCCESS) {
    SetLastError(RtlNtStatusToDosError(status));
    return FALSE;
  }
  *vid = fvi->VolumeSerialNumber;
  return TRUE;
}

DWORD BaseSetLastNTError(NTSTATUS Status)
{
  DWORD dwErrCode;
  dwErrCode = RtlNtStatusToDosError(Status);
  SetLastError(dwErrCode);
  return dwErrCode;
}

/* Quick and dirty table for conversion */
static FILE_INFORMATION_CLASS ConvertToFileInfo[MaximumFileInfoByHandleClass] =
{
  FileBasicInformation, FileStandardInformation, FileNameInformation, FileRenameInformation,
  FileDispositionInformation, FileAllocationInformation, FileEndOfFileInformation, FileStreamInformation,
  FileCompressionInformation, FileAttributeTagInformation, FileIdBothDirectoryInformation, (FILE_INFORMATION_CLASS)-1,
  FileIoPriorityHintInformation
};

BOOL SetFileInformationByHandle(HANDLE handle, FILE_INFO_BY_HANDLE_CLASS FileInfoClass, LPCVOID info, size_t size)
{
  NTSTATUS Status;
  IO_STATUS_BLOCK IoStatusBlock;
  FILE_INFORMATION_CLASS ntFileInfoClass;

  ntFileInfoClass = (FILE_INFORMATION_CLASS)-1;
  if (FileInfoClass < MaximumFileInfoByHandleClass) {
    ntFileInfoClass = ConvertToFileInfo[FileInfoClass];
  }
  if (ntFileInfoClass == -1) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  Status = NtSetInformationFile(handle, &IoStatusBlock, (PVOID)info, (ULONG)size, ntFileInfoClass);
  if (!NT_SUCCESS(Status)) {
    nt::BaseSetLastNTError(Status);
    return FALSE;
  }
  return TRUE;
}

BOOL SetFileAttrByHandle(HANDLE handle, const FILE_BASIC_INFORMATION * fbi)
{
  return nt::SetFileInformationByHandle(handle, FileBasicInfo, fbi, sizeof(*fbi));
} 

BOOL SetFileAttrByHandle(HANDLE handle, const FILE_BASIC_INFO * fbi)
{
  return nt::SetFileInformationByHandle(handle, FileBasicInfo, fbi, sizeof(*fbi));
} 

BOOL DeleteFileByHandle(HANDLE handle)
{
  FILE_DISPOSITION_INFORMATION fdi;
  fdi.DeleteFile = TRUE;
  return nt::SetFileInformationByHandle(handle, FileDispositionInfo, &fdi, sizeof(fdi));
}


PIMAGE_SECTION_HEADER RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeader, PVOID BaseAddress, ULONG Rva)
{
  ULONG Count = NtHeader->FileHeader.NumberOfSections;
  PIMAGE_SECTION_HEADER Section = IMAGE_FIRST_SECTION(NtHeader);
  while (Count--) {
    if (Section->VirtualAddress <= Rva && Rva < Section->VirtualAddress + Section->SizeOfRawData)
      return Section;
    Section++;
  }
  return NULL;
}

PVOID RtlImageRvaToVa(PIMAGE_NT_HEADERS NtHeader, PVOID BaseAddress, ULONG Rva, PIMAGE_SECTION_HEADER * SectionHeader)
{
  PIMAGE_SECTION_HEADER Section = SectionHeader ? *SectionHeader : NULL;

  if (Section == NULL || Rva < Section->VirtualAddress || Rva >= Section->VirtualAddress + Section->SizeOfRawData) {
    Section = RtlImageRvaToSection(NtHeader, BaseAddress, Rva);
    if (Section == NULL)
      return NULL;

    if (SectionHeader)
      *SectionHeader = Section;
  }
  return (PVOID)((SIZE_T)BaseAddress + Rva + (SIZE_T)Section->PointerToRawData - (SIZE_T)Section->VirtualAddress);
}

PIMAGE_NT_HEADERS GetModuleNtHeader(PVOID BaseAddress)
{
  PIMAGE_NT_HEADERS NtHeader;
  PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
  if (DosHeader && DosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
    NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)BaseAddress + DosHeader->e_lfanew);
    if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
      return NtHeader;
  }
  return NULL;
}

LPVOID GetModuleResourseDir(PVOID BaseAddress, SIZE_T * resDirSize)
{
  PIMAGE_NT_HEADERS pImg = GetModuleNtHeader(BaseAddress);
  if (pImg) {
    IMAGE_DATA_DIRECTORY * dir = &pImg->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
    if (resDirSize)
      *resDirSize = (SIZE_T)dir->Size;
    return (LPVOID)((SIZE_T)BaseAddress + dir->VirtualAddress);
  }
  return NULL;
}

LPVOID GetModuleResourcePtr(PVOID BaseAddress, IMAGE_RESOURCE_DATA_ENTRY * entry)
{
  SIZE_T ResDirSize;
  PVOID ResDirAddr = GetModuleResourseDir(BaseAddress, &ResDirSize);
  if (ResDirAddr && ResDirSize >= 256 && ResDirAddr > BaseAddress) {
    if ((SIZE_T)BaseAddress & 1) {  /* module handle is for a LOAD_LIBRARY_AS_DATAFILE module */
      PVOID mod = (PVOID)((ULONG_PTR)BaseAddress & ~1);
      PIMAGE_NT_HEADERS nthdr = GetModuleNtHeader(mod);
      if (nthdr)
        return RtlImageRvaToVa(nthdr, mod, entry->OffsetToData, NULL);
    } else {
      return (LPVOID)((PBYTE)BaseAddress + entry->OffsetToData);
    }
  }
  return NULL;
}

#define NT_DWORD_ALIGN( base, ptr )   ( (LPBYTE)(base) + ((((LPBYTE)(ptr) - (LPBYTE)(base)) + 3) & ~3) )     
#define VersionInfo32_Value( ver )    NT_DWORD_ALIGN( (ver), (ver)->szKey + wcslen((ver)->szKey) + 1 ) 

VS_FIXEDFILEINFO * GetModuleFixedFileInfo(PVOID BaseAddress)
{
  LPVOID image_base = BaseAddress;
  if (!image_base) {
    image_base = GetModuleHandleW(NULL);
    if (!image_base)
      return NULL;
  }
  LANGID english = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
  HRSRC hRsrc = FindResourceExW((HMODULE)image_base, MAKEINTRESOURCEW(VS_VERSION_INFO), (LPWSTR)VS_FILE_INFO, english);
  if (!hRsrc)
    hRsrc = FindResourceW((HMODULE)image_base, MAKEINTRESOURCEW(VS_VERSION_INFO), (LPWSTR)VS_FILE_INFO);

  if (hRsrc) {
    //PVS_VERSION_INFO_STRUCT32 vvis = LoadResource((HMODULE)image_base, hRsrc); 
    PVS_VERSION_INFO_STRUCT32 vvis = (PVS_VERSION_INFO_STRUCT32)GetModuleResourcePtr(image_base, (PIMAGE_RESOURCE_DATA_ENTRY)hRsrc);
    if (vvis)
      return (VS_FIXEDFILEINFO *)VersionInfo32_Value(vvis);
  }
  return NULL;
}

} /* namespace */
