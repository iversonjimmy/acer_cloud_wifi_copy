::set SIGNTOOL="C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\signtool.exe"
set SIGNTOOL="C:\Program Files\Microsoft SDKs\Windows\v6.0A\Bin\signtool.exe"
::set SIGNKEYPATH="C:\Users\fokushi\Documents\verisign-code-signing.pfx"
set SIGNKEYPATH="C:\cygwin\home\build\buildslaves\b3-05-01\builder_win32_installer\sw_x\tools\license\verisign-code-signing.pfx"

%SIGNTOOL% sign /f %SIGNKEYPATH% /p route2me /t http://timestamp.verisign.com/scripts/timestamp.dll /v %MSIPATH%

%SIGNTOOL% verify /pa /v %MSIPATH%

