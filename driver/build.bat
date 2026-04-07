@echo off
cd /d "%~dp0"

: Path of windows kits
set WINKITPATH=F:\Windows Kits\10

cl /kernel /Gz /W3 /WX- /O2 /GS- /Zi /D NDEBUG /D _WIN64 /I "%WINKITPATH%\Include\10.0.26100.0\km" /I "%WINKITPATH%\Include\10.0.26100.0\shared" /I "%WINKITPATH%\Include\10.0.26100.0\km\crt" /LIBPATH:"%WINKITPATH%\Lib\10.0.26100.0\km\x64" /Fo FolderGuardDriver.obj /PDB:NO FolderGuardDriver.c /link /SUBSYSTEM:NATIVE /ENTRY:DriverEntry /DRIVER /MACHINE:X64 /LIBPATH:"%WINKITPATH%\Lib\10.0.26100.0\km\x64" fltmgr.lib ntoskrnl.lib /OUT:.\FolderGuardDriver.sys
powershell -ExecutionPolicy Bypass -File "sign.ps1"
pause