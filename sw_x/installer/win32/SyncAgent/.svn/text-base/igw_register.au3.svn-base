AutoItSetOption("WinTitleMatchMode", 2)

Const $prog_name = "igw_register"
Const $counterMax = 30
Const $plugin = "shellext_win.dll"
Const $pluginDir = "\Program Files\Common Files\iGware\SyncAgent\"
Const $pluginPath = $pluginDir & $plugin
Const $plugin32Dir = "\Program Files (x86)\Common Files\iGware\SyncAgent\"
Const $plugin32Path = $plugin32Dir & $plugin

Func CloseShellWindow()
	Local $h = WinWait($prog_name, "", 1)
	If $h <> 0 Then
		WinSetState($prog_name, "", @SW_MINIMIZE)
	EndIf
EndFunc

Func RegisterPlugin()
	Local $done = 0
	Local $done32 = 0
	Local $counter = 0
	
	If Not FileExists($pluginPath) Then
		$done = 1
	EndIf

	If Not FileExists($plugin32Path) Then
		$done32 = 1
	EndIf

	Sleep(1000)  ; wait for igw_regstart.exe to catch up	
	While ($counter < $counterMax) and ($done = 0 Or $done32 = 0)
		If $done = 0 Then
			If RunWait("regsvr32 /s " & $plugin, $pluginDir, @SW_HIDE) = 0 Then
				If @error = 0 Then
					$done = 1
				EndIf
			EndIf
		EndIf

		If $done32 = 0 Then
			If RunWait("regsvr32 /s " & $plugin, $plugin32Dir, @SW_HIDE) = 0 Then
				If @error = 0 Then
					$done32 = 1
				EndIf
			EndIf
		EndIf

		$counter = $counter + 1

		Sleep(500)
	WEnd
	
EndFunc

CloseShellWindow()
RegisterPlugin()
