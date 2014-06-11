This Eclipse project enables the client_sw_x project's C/C++ indexer to find the Win32 header files
when using the "IndexMsvc" build configuration.
You can temporarily close this project to save memory when not using the "IndexMsvc" build configuration.

---------------------------
Suggested usage of this project file:
1. First, check out trunk/developer/eclipse/client/client_sw_x into your workspace.
2. File -> Import... -> Existing Projects Into Workspace
3. Click "Browse..." for "Select root directory:"
4. Choose <local folder where you checked out the client_sw_x project>/sw_x/devtools
5. Select this project (and any others) in the list of detected projects.
6. Uncheck "Copy projects into workspace".
7. Finish.

---------------------------
This Eclipse project expects the following path variables.
You can set them from Window->Preferences, General->Workspace->Linked Resources

WINDOWS_SDK
for example, C:\Program Files\Microsoft SDKs\Windows\v7.1

VISUAL_STUDIO_DIR
for example, C:\Program Files (x86)\Microsoft Visual Studio 9.0

---------------------------
