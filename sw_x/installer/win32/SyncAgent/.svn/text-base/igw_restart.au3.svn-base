AutoItSetOption("WinTitleMatchMode", 2)

Const $prog_name = "igw_restart"
Const $register_prog = "igw_register.exe"
Const $explorer_prog = "explorer.exe"

Func CloseShellWindow()
	Local $h = WinWait($prog_name, "", 1)
	If $h <> 0 Then
		WinSetState($prog_name, "", @SW_MINIMIZE)
	EndIf
EndFunc

Func RestartExplorer()
	While ProcessExists($register_prog) <> 0
		Sleep(500)
	WEnd
	
	ProcessClose($explorer_prog)
EndFunc

CloseShellWindow()
RestartExplorer()
