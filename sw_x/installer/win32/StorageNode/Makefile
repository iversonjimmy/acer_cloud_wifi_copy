ifndef VERSION
$(error VERSION must be defined)
endif
ifndef PBROOT
$(error PBROOT must be defined)
endif
ifndef SRCROOT
$(error SRCROOT must be defined)
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

STORAGENODE_EXE_PATH := $(PBROOT)/storageNode/storageNode.exe

EXE_DLL_LIST := $(STORAGENODE_EXE_PATH)

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

%.wixobj: %.wxs
	$(CANDLE) $< -ext WixUtilExtension -out $@

.PHONY: default all
default all: $(foreach lang,$(LOCALIZATIONS),$(lang)/StorageNode.msi) $(foreach lang,$(LOCALIZATIONS),$(lang)/StorageNode.msi$(SIGNED_SUFFIX))

%/StorageNode.msi: StorageNode.wixobj %.wxl $(foreach exe_dll,$(EXE_DLL_LIST),$(exe_dll)$(SIGNED_SUFFIX))
	mkdir -p $(@D)
	$(LIGHT) $< -dWixUILicenseRtf=License-$*.rtf -cultures:$* -loc $*.wxl -ext WixUIExtension -ext WixUtilExtension -out $@
