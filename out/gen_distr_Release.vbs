set objFSO = CreateObject("Scripting.FileSystemObject")
scriptdir = objFSO.GetParentFolderName(WScript.ScriptFullName)
confname = "Release"
wcx_file_x86 = scriptdir & "\Win32_" & confname & "\wcpatcher.wdx"
wcx_file_x64 = scriptdir & "\x64_" & confname & "\wcpatcher.wdx64"
wcx_ver = objFSO.GetFileVersion(wcx_file_x86)
dotPos = InStr(wcx_ver, ".")
dotPos = InStr(dotPos+1, wcx_ver, ".")
wcx_ver = left(wcx_ver, dotPos-1)
target = scriptdir & "\wcpatcher_v" & wcx_ver & ".zip"

pos = InStrRev(scriptdir, "\")
rootdir = left(scriptdir, pos-1)

set zip = objFSO.OpenTextFile(target, 2, vbtrue)
zip.Write "PK" & Chr(5) & Chr(6) & String( 18, Chr(0) )
zip.Close
set zip = nothing

wscript.sleep 150

set objApp = CreateObject("Shell.Application")
objApp.NameSpace(target).CopyHere (wcx_file_x64)
wscript.sleep 150
objApp.NameSpace(target).CopyHere (wcx_file_x86)
wscript.sleep 150
objApp.NameSpace(target).CopyHere (scriptdir & "\wcpatcher.ini")
wscript.sleep 100
objApp.NameSpace(target).CopyHere (scriptdir & "\install_EN.txt")
wscript.sleep 100
objApp.NameSpace(target).CopyHere (scriptdir & "\install_RU.txt")
wscript.sleep 100
objApp.NameSpace(target).CopyHere (scriptdir & "\pluginst.inf")
wscript.sleep 100


set obj = nothing
set objApp = nothing
set objFSO = nothing
