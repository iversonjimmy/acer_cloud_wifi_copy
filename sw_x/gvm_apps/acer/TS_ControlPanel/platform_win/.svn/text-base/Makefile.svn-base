ifndef BUILDROOT
$(error Please define BUILDROOT first!)
endif

BUILD_MSVC_SOLUTION := $(CURDIR)/build_solution.bat

export BUILDROOT

ifeq (BUILDTYPE,debug)
MSVC_BUILDTYPE := Debug
else
MSVC_BUILDTYPE := Release
endif

.PHONY: default
default:
	chmod +x $(BUILD_MSVC_SOLUTION)
	$(BUILD_MSVC_SOLUTION) $(CURDIR)/acpanel_win.sln /Build $(MSVC_BUILDTYPE) Win32
	
.PHONY: clean
clean:
	chmod +x $(BUILD_MSVC_SOLUTION)
	$(BUILD_MSVC_SOLUTION) $(CURDIR)/acpanel_win.sln /Clean $(MSVC_BUILDTYPE) Win32
