Instructions for using MS Visual Studio to build stuff in sw_x:

You may want to define an environment variable BUILDROOT before starting Visual Studio.
Output will be placed at $(BUILDROOT)\build_msvc\$(SolutionName)\$(ConfigurationName)\$(ProjectName)\$(PlatformName)
If you do not define $(BUILDROOT), it will likely use "C:" as the BUILDROOT.

You will need to generate a bunch of *.pb.cc and *.pb.h files before CCDI Client will build.
You can use the "msvc_prebuild" makegen target; follow the steps at:
  http://www.ctbg.acer.com/wiki/index.php/Building_sw_x_for_Win32_%28Windows_buildhost%29

To avoid creating junk files (ipch, *.sdf, *.opensdf) in your source tree, 
you should make the following change in Visual Studio 2010 (or 2012):
* Tools > Options > Text Editor > C/C++ > Advanced
** Set "Always Use Fallback Location" to True
** Set "Do Not Warn If Fallback Location" to True
** Set "Fallback Location" to %TEMP%\VC++

To match our coding conventions, insert spaces instead of tabs:
* Tools > Options > Text Editor > C/C++ > Tabs
** Select "Insert spaces"

Notes for when adding new MS Visual Studio projects:
For Win32, currently, there's only one solution file, PersonalCloudAll.sln, but if we need to add more later,
note that all solutions (.sln files) should go under <SW_X>/devroot/projects/msvc
This is because the individual .props (formerly .vsprops) files use $(SolutionDir)/../../.. to locate the sw_x directory.
Using relative paths from each $(ProjectDir) makes more sense at first, but the problem is that we wouldn't be
able to reuse the same .props file in different projects unless all of those project files are at
the same folder depth in the source tree.  (Well that's not completely true; if we really want to make the
.props based on $(ProjectDir), we could do so by adding multiple variants of each include path, prefixed with different
amounts of "../" to make sure we handle all the cases, but that is even less fun to maintain).
