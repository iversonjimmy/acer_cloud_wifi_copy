Private Const MyCloud_lnk = "MyCloud.lnk"
Private oFSO
Private sDefaultUserStartupPath
Private sThisUserStartupPath
Private sThisUserProfilePath
Private sThisUserName
Private sStartupPathSuffix
Private sStartupPathPrefix

Private Sub Init()
  Set oFSO = CreateObject("Scripting.FileSystemObject")

  Set oShell = CreateObject("WScript.Shell")
  sDefaultUserStartupPath = oShell.SpecialFolders("AllUsersStartup")
  sThisUserStartupPath = oShell.SpecialFolders("Startup")
  sThisUserProfilePath = oShell.ExpandEnvironmentStrings("%USERPROFILE%")
  sThisUserName = oShell.ExpandEnvironmentStrings("%USERNAME%")

  sStartupPathSuffix = Right(sThisUserStartupPath, Len(sThisUserStartupPath)-Len(sThisUserProfilePath))
  sStartupPathPrefix = Left(sThisUserProfilePath, Len(sThisUserProfilePath)-Len(sThisUserName))
End Sub

Private Sub TryRemoveFile(sPath)
  If oFSO.FileExists(sPath) Then
    oFSO.DeleteFile(sPath)
  End If
End Sub

Private Sub TryRemoveFromDefaultUser()
  TryRemoveFile(oFSO.BuildPath(sDefaultUserStartupPath, MyCloud_lnk))
End Sub

Private Sub TryRemoveFromAllUsers()
  Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
  Set colItems = objWMIService.ExecQuery("Select * from Win32_UserAccount")
  For Each objItem in colItems
    TryRemoveFile(oFSO.BuildPath(sStartupPathPrefix & objItem.Name & sStartupPathSuffix, MyCloud_lnk))
  Next
End Sub

Sub RemoveStartupShortcuts()
  Init()
  TryRemoveFromDefaultUser()
  TryRemoveFromAllUsers()
End Sub


