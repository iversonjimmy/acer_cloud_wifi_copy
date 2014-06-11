ifndef BUILDROOT
$(error Please define BUILDROOT first!)
endif

ifndef PRODUCT
$(error Please define product first!)
endif

ifeq ($(PRODUCT), winrt)
plat=Win32
else
plat=ARM
endif

MSBUILD_EXE="/cygdrive/c/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
FIXED_SOLUTION="$(shell cygpath -w $(CURDIR)/PersonalCloudAll.sln)"

# Note: having BUILDTYPE defined causes msbuild to always rebuild everything.
BUILD_MSVC_SOLUTION := \
	export BUILDROOT="$(shell cygpath -w $(BUILDROOT))" && \
	  export BUILDTYPE= && \
	  $(MSBUILD_EXE) $(FIXED_SOLUTION)

.PHONY: default
default: release debug

.PHONY: release
release:
	$(BUILD_MSVC_SOLUTION) /p:Configuration=Release /p:Platform=$(plat)
.PHONY: debug
debug:
	$(BUILD_MSVC_SOLUTION) /p:Configuration=Debug /p:Platform=$(plat)

.PHONY: clean
clean:
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Release /p:Platform=$(plat)
	-$(BUILD_MSVC_SOLUTION) /t:Clean /p:Configuration=Debug /p:Platform=$(plat)
