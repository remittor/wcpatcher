@echo off

set WCP_VER_MAJOR=x
set WCP_VER_MINOR=x
set WCP_VERSION=x.x

SETLOCAL ENABLEEXTENSIONS EnableDelayedExpansion

set VER_FILE=%APPVEYOR_BUILD_FOLDER%\src\wcp_version.h

set "VER_PRODUCT=WCP_VER_MAJOR"
set VER_RESULT=
for /F "tokens=*" %%A in (%VER_FILE%) do (
  set str=%%A
  Echo.!str! | findstr /C:"!VER_PRODUCT!">nul
  if !errorlevel!==0 set VER_RESULT=!str! & goto :MajorDone
)
echo ***ERROR on detect ver major***
exit /b 1

:MajorDone
for %%A in (%VER_RESULT%) do set "VER_MAJOR=%%A"

set "VER_PRODUCT=WCP_VER_MINOR"
set VER_RESULT=
for /F "tokens=*" %%A in (%VER_FILE%) do (
  set str=%%A
  Echo.!str! | findstr /C:"!VER_PRODUCT!">nul
  if !errorlevel!==0 set VER_RESULT=!str! & goto :MinorDone
)
echo ***ERROR on detect ver minor***
exit /b 1

:MinorDone
for %%A in (%VER_RESULT%) do set "VER_MINOR=%%A"

ENDLOCAL & set "WCP_VER_MAJOR=%VER_MAJOR%" & set "WCP_VER_MINOR=%VER_MINOR%"

set "WCP_VERSION=%WCP_VER_MAJOR%.%WCP_VER_MINOR%"
echo WCP_VERSION: "%WCP_VERSION%"

