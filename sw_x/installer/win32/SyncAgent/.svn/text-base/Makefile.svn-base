ifndef VERSION
$(error VERSION must be defined)
endif
ifndef PBROOT
$(error PBROOT must be defined)
endif
ifndef SRCROOT
$(error SRCROOT must be defined)
endif
ifndef TITLE_VERSION
$(error TITLE_VERSION must be defined)
endif

CANDLE := "C:/Program Files (x86)/Windows Installer XML v3.5/bin/candle.exe"
LIGHT  := "C:/Program Files (x86)/Windows Installer XML v3.5/bin/light.exe"

ifdef FOKUSHI_W7
SIGNTOOL    := "C:/Program Files/Microsoft SDKs/Windows/v7.1/Bin/signtool.exe"
SIGNKEYPATH := "C:/Users/fokushi/verisign-code-signing.pfx"
else
SIGNTOOL    := "C:/Program Files/Microsoft SDKs/Windows/v6.0A/Bin/signtool.exe"
SIGNKEYPATH := $(shell cygpath -m $(SRCROOT)/tools/license/verisign-code-signing.pfx)
endif

CCD_EXE_PATH     := $(PBROOT)/PersonalCloudAll/Release/ccd/Win32/ccd.exe
ACPANEL_EXE_PATH := $(PBROOT)/TS_ControlPanel/Win32/Release/acpanel_win.exe
ACTOOL_EXE_PATH  := $(PBROOT)/PersonalCloudAll/Release/actool_win/Win32/actool_win.exe
INTEROP_DLL_PATH := $(PBROOT)/TS_ControlPanel/Win32/Release/Interop.IWshRuntimeLibrary.1.0.dll

EXE_DLL_LIST := $(CCD_EXE_PATH) $(ACPANEL_EXE_PATH) $(ACTOOL_EXE_PATH) $(INTEROP_DLL_PATH)

export IMAGEFILES_A := $(shell bash $(SRCROOT)/installer/win32/SyncAgent/getImageFiles.sh gvm_apps/acer/TS_ControlPanel/platform_win/images)
export IMAGEFILES_A_CP := $(shell bash $(SRCROOT)/installer/win32/SyncAgent/getImageFiles.sh gvm_apps/acer/TS_ControlPanel/platform_win/images/ControlPanel)
export IMAGEFILES_A_SU := $(shell bash $(SRCROOT)/installer/win32/SyncAgent/getImageFiles.sh gvm_apps/acer/TS_ControlPanel/platform_win/images/SignUp)

PBROOT_CYGPATH  := $(PBROOT)
SRCROOT_CYGPATH := $(SRCROOT)

# WiX tools need the paths in Windows format.
# cygpath "mixed" format is sufficient, so that's what we'll use
export PBROOT  := $(shell cygpath -m $(PBROOT_CYGPATH))
export SRCROOT := $(shell cygpath -m $(SRCROOT_CYGPATH))

ifdef SIGN
SIGNED_SUFFIX = .signed
else
SIGNED_SUFFIX =
endif

define sign_file
	$(SIGNTOOL) sign /f $(SIGNKEYPATH) /p route2me /t http://timestamp.verisign.com/scripts/timestamp.dll /v $(shell cygpath -m $<)
	touch $@
endef

ifndef LOCALIZATIONS
LOCALIZATIONS = en-us zh-tw
endif

$(EXE_DLL_LIST):
	@echo ERROR MISSING FILE: $@
	@test -f $@

%.exe.signed: %.exe
	$(sign_file)

%.dll.signed: %.dll
	$(sign_file)

%.msi.signed: %.msi
	$(sign_file)

SyncAgent-x64.wixobj: SyncAgent.wxs
	$(CANDLE) $< -dIsX64=yes -ext WixUtilExtension -out $@

SyncAgent-x86.wixobj: SyncAgent.wxs
	$(CANDLE) $< -dIsX64=no -ext WixUtilExtension -out $@

.PHONY: default all
default all: $(foreach lang,$(LOCALIZATIONS),$(lang)/SyncAgent-x86.msi) $(foreach lang,$(LOCALIZATIONS),$(lang)/SyncAgent-x86.msi$(SIGNED_SUFFIX))

%/SyncAgent-x64.msi: SyncAgent-x64.wixobj %.wxl $(foreach exe_dll,$(EXE_DLL_LIST),$(exe_dll)$(SIGNED_SUFFIX))
	mkdir -p $(@D)
	$(LIGHT) $< -dIsX64=yes -dWixUILicenseRtf=License-$*.rtf -cultures:$* -loc $*.wxl -ext WixUIExtension -ext WixUtilExtension -out $@

%/SyncAgent-x86.msi: SyncAgent-x86.wixobj %.wxl $(foreach exe_dll,$(EXE_DLL_LIST),$(exe_dll)$(SIGNED_SUFFIX))
	mkdir -p $(@D)
	$(LIGHT) $< -dIsX64=no -dWixUILicenseRtf=License-$*.rtf -cultures:$* -loc $*.wxl -ext WixUIExtension -ext WixUtilExtension -out $@
