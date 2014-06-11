ifndef BUILDROOT
$(error Please define BUILDROOT first!)
endif

MSBUILD_EXE="/cygdrive/c/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
FIXED_SOLUTION="$(shell cygpath -w $(CURDIR)/PersonalCloudAll.sln)"

# Note: having BUILDTYPE defined causes msbuild to always rebuild everything.
BUILD_MSVC_SOLUTION := \
	export BUILDROOT="$(shell cygpath -w $(BUILDROOT))" && \
	  export BUILDTYPE= && \
	  $(MSBUILD_EXE) $(FIXED_SOLUTION)

.PHONY: default
default: win32_release win32_debug winx64_release winx64_debug

.PHONY: winx64_release
winx64_release:
	# "/m" enables a concurrent build process for each CPU (see Bug 17648).
	$(BUILD_MSVC_SOLUTION) /m /p:Configuration=Release /p:Platform=x64

.PHONY: winx64_debug
winx64_debug:
	# "/m" enables a concurrent build process for each CPU (see Bug 17648).
	$(BUILD_MSVC_SOLUTION) /m /p:Configuration=Debug /p:Platform=x64

.PHONY: win32_release
win32_release:
	# "/m" enables a concurrent build process for each CPU (see Bug 17648).
	$(BUILD_MSVC_SOLUTION) /m /p:Configuration=Release /p:Platform=Win32

.PHONY: win32_debug
win32_debug:
	# "/m" enables a concurrent build process for each CPU (see Bug 17648).
	$(BUILD_MSVC_SOLUTION) /m /p:Configuration=Debug /p:Platform=Win32

.PHONY: clean
clean:
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Release /p:Platform=Win32
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Release /p:Platform=x64
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Debug /p:Platform=Win32
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Debug /p:Platform=x64
