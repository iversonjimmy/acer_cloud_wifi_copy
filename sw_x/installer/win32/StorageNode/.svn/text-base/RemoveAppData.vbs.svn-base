Private Const StorageNodeFolder = "iGware\StorageNode"
Private oFSO
Private sThisUserLocalAppDataPath
Private sThisUserProfilePath
Private sThisUserName
Private sLocalAppDataPathSuffix
Private sLocalAppDataPathPrefix

Private Sub Init()
  Set oFSO = CreateObject("Scripting.FileSystemObject")

  Set oShell = CreateObject("WScript.Shell")
  sThisUserLocalAppDataPath = oShell.ExpandEnvironmentStrings("%LOCALAPPDATA%")
  sThisUserProfilePath = oShell.ExpandEnvironmentStrings("%USERPROFILE%")
  sThisUserName = oShell.ExpandEnvironmentStrings("%USERNAME%")

  sLocalAppDataPathSuffix = Right(sThisUserLocalAppDataPath, Len(sThisUserLocalAppDataPath)-Len(sThisUserProfilePath))
  sLocalAppDataPathPrefix = Left(sThisUserProfilePath, Len(sThisUserProfilePath)-Len(sThisUserName))
End Sub

Private Sub TryRemoveFolder(sPath)
  On Error Goto 0
  If oFSO.FolderExists(sPath) Then
    oFSO.DeleteFolder(sPath)
  End If
End Sub

Private Sub TryRemoveFromAllUsers()
  Set objWMIService = GetObject("winmgmts:\\.\root\cimv2")
  Set colItems = objWMIService.ExecQuery("Select * from Win32_UserAccount")
  For Each objItem in colItems
    TryRemoveFolder(oFSO.BuildPath(sLocalAppDataPathPrefix & objItem.Name & sLocalAppDataPathSuffix, StorageNodeFolder))
  Next
End Sub

Sub RemoveAppData()
  Init()
  TryRemoveFromAllUsers()
End Sub
