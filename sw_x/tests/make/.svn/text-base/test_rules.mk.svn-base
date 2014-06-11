.PHONY local_run:
local_run:
ifeq ($(strip $(wildcard $(SRC_SWX)/tests/$(MY_TEST_NAME)/Makefile)),)
	$(error "can't locate sw_x/tests/$(MY_TEST_NAME)/Makefile - current directory is $(shell pwd)")
endif
	make cleanup
	make setup
	make runtests

.PHONY: prepare_account
prepare_account:
	@if [ -d $(IDT_TOOLS) ]; then \
		echo 'IDT tool detected'; \
	else \
		echo 'IDT tool does not exist, check out the $(IDT_TOOLS)'; \
		false; \
	fi 
	@date +"%Y/%m/%d %k:%M:%S.%N"
	-cd $(IDT_TOOLS) && \
		./deleteUser -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT)
	@date +"%Y/%m/%d %k:%M:%S.%N"
	cd $(IDT_TOOLS) && \
		./createUser -f -i ids.$(INTERNAL_LAB_DOMAIN_NAME) -u $(CCD_TEST_ACCOUNT) -p password
	@date +"%Y/%m/%d %k:%M:%S.%N"
ifdef CALL_EXTRA_PREPARE_ACCOUNT_TARGET
	$(MAKE) extra_prepare_account
endif

.PHONY: ping_devices
ping_devices:
# Ping each of MD_IP, CLIENT_IP, CLOUDPC_IP, and TARGET_MOBILE_DEVICE (if nonempty and not localhost) to confirm needed devices are online. Even one packet received is a pass.
	@for host in $(MD_IP) $(CLIENT_IP) $(CLOUDPC_IP) $(TARGET_MOBILE_DEVICE) ; do \
		if [[ $$host != "127.0.0.1" ]]; then \
			echo "Ping $$host"; \
			ping_result=`ping -q -c 5 $$host | grep received`; \
			echo "$$ping_result"; \
			ping_fail=" 0( packet)?s? received"; \
			if [[ $$ping_result =~ $$ping_fail ]] ; then \
	  			echo "*** Ping failed for $$host. Check host machine."; \
				echo "TC_RESULT=FAIL ;;; TC_NAME=PingHost_$$host"; \
				exit 1 ; \
			fi; \
		fi; \
	done
.PHONY: runtests
runtests: 
ifndef SKIP_REMOTE_AGENT_SETUP
	-$(MAKE) prepare_remote_agent
endif
	-$(MAKE) prepare_ccd_monitor_service
	-$(MAKE) $(MY_TEST_NAME)

.PHONY: runtests_and_collect_log
runtests_and_collect_log:
	-$(MAKE) local_run
	-$(MAKE) collect_logs 

.PHONY: setup
setup: ping_devices prepare_account prepare_test_data
	# TARGET_TESTROOT is the test root path of the test target
	# By default, the TARGET_MACHINE is localhost (i.e. on the same machine as test builder)
	@if [ "$(TARGET_MACHINE)" == "" ]; then \
	  echo '*** Must define "TARGET_MACHINE"'; \
	  false; \
	fi
	@if [ "$(TARGET_USER)" == "" ]; then \
	  echo '*** Must define "TARGET_USER"'; \
	  false; \
	fi
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)'

ifeq ($(TESTPLAT), cloudnode)
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DXSHELL_HOST_PACKAGE_PATH)/$(DXSHELL_HOST_PACKAGE) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(DXSHELL_HOST_PACKAGE)'
	-$(REMOTE_RUN) 'mkdir -p $(TARGET_TESTROOT)/cloudnode'
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DXSHELL_PACKAGE_PATH)/$(DXSHELL_PACKAGE) -P $(TARGET_TESTROOT)/cloudnode'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/cloudnode; $(UNZIP) $(DXSHELL_PACKAGE)'
else
	$(REMOTE_RUN) 'wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(DXSHELL_PACKAGE_PATH)/$(DXSHELL_PACKAGE) -P $(TARGET_TESTROOT)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); $(UNZIP) $(DXSHELL_PACKAGE)'
endif

ifeq ($(TESTPLAT), winrt)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); rm -rf dx_remote_agent_winrt && wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/winrt/dx_remote_agent_winrt/dx_remote_agent_winrt.zip && $(UNZIP) dx_remote_agent_winrt.zip -d dx_remote_agent_winrt && rm -rf dx_remote_agent_winrt.zip && mv $(TARGET_TESTROOT)/dx_remote_agent_winrt/metro_app_utilities.exe $(TARGET_TESTROOT)/metro_app_utilities.exe'
endif

ifeq ($(TESTPLAT), android)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/AdbTool; rm -rf cc_service_for_dx-release.apk && wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/cc_service_for_dx-release/cc_service_for_dx-release.apk'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT)/AdbTool; rm -rf dx_remote_agent-release.apk && wget $(WGET_OPTIONS) http://$(STORE_HOST):$(HTTP_PORT)/$(STORE_PATH)/$(TEST_BRANCH)/$(PRODUCT)/dx_remote_agent-release/dx_remote_agent-release.apk'
endif

	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); chmod -R 755 *'

ifndef USE_ARCHIVED_BUILD
	@echo "sync test files to $(TARGET_TESTROOT) for TESTPLAT=$(TESTPLAT)"
	# Use local source python scripts
	rsync -p $(SRC_SWX)/tests/dxshell/tools/Remote_Agent_Launcher.py $(TARGET_USER)@$(TEST_MASTER_MACHINE):$(TARGET_TESTROOT)/Remote_Agent_Launcher.py
	rsync -p $(SRC_SWX)/tests/dxshell/tools/App_Launcher.py $(TARGET_USER)@$(TEST_MASTER_MACHINE):$(TARGET_TESTROOT)/App_Launcher.py

ifneq ($(TESTPLAT), linux)
ifneq ($(TESTPLAT), cloudnode)
	echo ">>>> WARNING : win32, winrt, android and ios do not build test binaries. This is for development purpose to force use local build"
	-rsync -p $(CCD_BINARY_PATH)/ccd$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/ccd$(EXE)
	-rsync -p $(DXSHELL_BINARY_PATH)/dxshell$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dxshell$(EXE)
	-rsync -p $(DXREMOTEAGENT_BINARY_PATH)/dx_remote_agent$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dx_remote_agent$(EXE)
endif
endif
	
ifeq ($(TESTPLAT), win32)    # win32 does not support building of dxshell binary 
	echo ">>>> WARNING : Windows 7/8 do not build test binaries. Force to use archived build"
endif
ifeq ($(TESTPLAT), winrt)    # winrt uses win32 for Client, CloudPC. Can it use local build for WinRT?
	echo ">>>> WARNING : WinRT does not build test binaries. Force to use archived build"
endif
ifeq ($(TESTPLAT), ios)    # ios always uses local build
	echo ">>>> WARNING : iOS uses archive build for win32 clients, local build for MD client."
endif
ifeq ($(TESTPLAT), linux)
	# Using local build CCD, dxshell, and dx_remote_agent for all clients
	rsync -p $(CCD_BINARY_PATH)/ccd $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/ccd
	rsync -p $(CCD_BINARY_PATH)/ccd.debug $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/ccd.debug
	rsync -p $(DXSHELL_BINARY_PATH)/dxshell $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dxshell
	rsync -p $(DXREMOTEAGENT_BINARY_PATH)/$(DXREMOTEAGENT_BINARY_NAME)$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dx_remote_agent$(EXE)
	rsync -p $(SRC_SWX)/tests/ccd/valgrind.supp $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/valgrind.supp
ifdef TEST_TARGET_IS_CLOUDNODE
	rsync -p $(CCD_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/$(CCD_ARCHIVE_BINARY_NAME)
	rsync -p $(DXSHELL_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/$(DXSHELL_ARCHIVE_BINARY_NAME)
endif
endif
ifeq ($(TESTPLAT), cloudnode)
	# Using local linux and cloudnode binaries
	rsync -p $(CCD_BINARY_PATH)/$(CCD_BINARY_NAME)$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/ccd$(EXE)
	rsync -p $(DXSHELL_BINARY_PATH)/$(DXSHELL_BINARY_NAME)$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dxshell$(EXE)
	rsync -p $(DXREMOTEAGENT_BINARY_PATH)/$(DXREMOTEAGENT_BINARY_NAME)$(EXE) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/dx_remote_agent$(EXE)
	rsync -p $(CCD_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/cloudnode/$(CCD_ARCHIVE_BINARY_NAME)
	rsync -p $(DXSHELL_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/cloudnode/$(DXSHELL_ARCHIVE_BINARY_NAME)
	rsync -p $(DXREMOTEAGENT_BUILDROOT_BINARY) $(TARGET_USER)@$(TARGET_MACHINE):$(TARGET_TESTROOT)/cloudnode/$(DXREMOTEAGENT_ARCHIVE_BINARY_NAME)
endif 
ifeq ($(TESTPLAT), android)
	# Using archived win32 binaries, local android binaries.
	rsync -p $(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_apps/dx_remote_agent/platform_android/dx_remote_agent_apk/bin/dx_remote_agent_apk-release.apk $(TARGET_USER)@$(TEST_MASTER_MACHINE):$(TARGET_TESTROOT)/AdbTool/dx_remote_agent-release.apk
	rsync -p  $(BUILDROOT)/$(BUILDTYPE)/$(PRODUCT)/gvm_apps/android_cc_sdk/example_service_only/example_service_only_apk/bin/example_service_only_apk-release.apk $(TARGET_USER)@$(TEST_MASTER_MACHINE):$(TARGET_TESTROOT)/AdbTool/cc_service_for_dx-release.apk
endif # ($(TESTPLAT), android)
endif # not USE_ARCHIVED_BUILD

ifdef TEST_TARGET_IS_CLOUDNODE  # This is defined in Makefile.cloudnode.linux.test or TESTPLAT=cloudnode
ifeq ($(TESTPLAT), linux)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mkdir client; mv ccd client/'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv $(CCD_ARCHIVE_BINARY_NAME) ccd'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv $(DXSHELL_ARCHIVE_BINARY_NAME) dxshell'
endif
ifeq ($(TESTPLAT), cloudnode)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); mv $(DXSHELL_ARCHIVE_BINARY_NAME) dxshell'
endif
endif

.PHONY: prepare_remote_agent
prepare_remote_agent:
ifdef CLOUDPC_ALIAS
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AddDevice $(CLOUDPC_ALIAS) $(CLOUDPC_OS) $(CLOUDPC_NAME) $(CLOUDPC_USER) $(CLOUDPC_IP)' 
endif
ifdef MD_ALIAS
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AddDevice $(MD_ALIAS) $(MD_OS) $(MD_NAME) $(MD_USER) $(MD_IP) $(TARGET_MOBILE_DEVICE)' 
endif
ifdef CLIENT_ALIAS
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) AddDevice $(CLIENT_ALIAS) $(CLIENT_OS) $(CLIENT_NAME) $(CLIENT_USER) $(CLIENT_IP)' 
endif
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) DeployRemoteAgent $(CLOUDPC_ALIAS) $(MD_ALIAS) $(CLIENT_ALIAS)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) StartRemoteAgent $(CLOUDPC_ALIAS) $(MD_ALIAS) $(CLIENT_ALIAS)'
ifdef CLOUDPC_ALIAS
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Reset'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set infraDomain $(LAB_DOMAIN_NAME)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLOUDPC_ALIAS) CCDConfig Set maxTotalLogSize $(MAX_TOTAL_LOG_SIZE)'
	$(MAKE) extra_cloudpc_config
endif #CLOUDPC_ALIAS
ifdef MD_ALIAS
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Reset'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set infraDomain $(LAB_DOMAIN_NAME)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(MD_ALIAS) CCDConfig Set maxTotalLogSize $(MAX_TOTAL_LOG_SIZE)'
	$(MAKE) extra_md_config
endif #MD_ALIAS
ifdef CLIENT_ALIAS
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Reset'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set infraDomain $(LAB_DOMAIN_NAME)'
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) -m $(CLIENT_ALIAS) CCDConfig Set maxTotalLogSize $(MAX_TOTAL_LOG_SIZE)'
	$(MAKE) extra_client_config
endif #CLIENT_ALIAS

## adb tools may get stuck via ssh, so we use orginal method to clean logs on android device ##	
ifeq ($(TESTPLAT), android)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) CleanCCLog $(MD_ALIAS)'
endif

ifdef USE_MOBILE_DEVICE
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) RestartRemoteAgentApp $(MD_ALIAS)'
endif

.PHONY: prepare_ccd_monitor_service
prepare_ccd_monitor_service:
ifneq ($(TESTPLAT), cloudnode)
ifneq ($(TESTPLAT), linux)
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./CCDMonitorService.exe -k' # (kill)
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./CCDMonitorService.exe -u' # (uninstall)
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./CCDMonitorService.exe -i' # (install)
	-$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./CCDMonitorService.exe -s' # (start)
endif
endif

.PHONY: collect_logs
collect_logs:
	-mkdir -p $(LOGDIR)
# Collect the dxshellFailureDir from the testroot (for all platforms).
# rsync will only create the destination directory when needed; the "<host>:<source_path>" syntax and trailing slash on the destination directory name are important for this behavior.
	-rsync -r $(TARGET_LOG):$(TARGET_TESTROOT)/dxshellFailureDir/* $(LOGDIR)/dxshellFailureDir/

ifneq ($(TESTPLAT), cloudnode)
ifneq ($(TESTPLAT), linux)
	-mkdir -p $(LOGDIR)/$(WIN_DESK_CCDMSRV_LOG)
	-rsync $(TARGET_LOG):$(WIN_DESK_CCDMSRV_PATH)/logs/CCDMonitorService/CCDMonitorService*.log $(LOGDIR)/$(WIN_DESK_CCDMSRV_LOG)/

# Collect any Windows crash dumps.
# rsync will only create the destination directory when needed; the "<host>:<source_path>" syntax and trailing slash on the destination directory name are important for this behavior.
	-rsync $(TARGET_LOG):$(WIN_DESK_CORE_DUMP_PATH)/dx_remote_agent.exe*.dmp $(LOGDIR)/$(WIN_DESK_CORE_DUMP_LOC)/
	-rsync $(TARGET_LOG):$(WIN_DESK_CORE_DUMP_PATH_FOR_SYSTEM_USER)/ccd.exe*.dmp $(LOGDIR)/$(WIN_DESK_CORE_DUMP_LOC)/
endif
endif

ifdef CLOUDPC_ALIAS
	-mkdir -p $(LOGDIR)/$(CLOUDPC_ALIAS)
ifeq ($(TESTPLAT), cloudnode)
	-$(CLOUDPC_REMOTE_RUN) 'cd $(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS); ./imltool $(CCD_APPDATA_PATH_CLOUDPC)'
	-rsync $(CLOUDPC_USER)@$(CLOUDPC_IP):$(CCD_APPDATA_PATH_CLOUDPC)/logs/imltool/*.log $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(CLOUDPC_USER)@$(CLOUDPC_IP):$(CCD_APPDATA_PATH_CLOUDPC)/logs/ccd/*.log $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(CLOUDPC_USER)@$(CLOUDPC_IP):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/$(CLOUDPC_ALIAS)_remote_agent.log $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(CLOUDPC_USER)@$(CLOUDPC_IP):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/core* $(LOGDIR)/$(CLOUDPC_ALIAS)
else
ifdef TEST_TARGET_IS_CLOUDNODE
	-$(REMOTE_RUN) 'cd $(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS); ./imltool $(CCD_APPDATA_PATH_CLOUDPC)'
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_CLOUDPC)/logs/imltool/*.log $(LOGDIR)/$(CLOUDPC_ALIAS)
endif
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_CLOUDPC)/logs/ccd/*.log $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/$(CLOUDPC_ALIAS)_remote_agent.log $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/core* $(LOGDIR)/$(CLOUDPC_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/valgrind.log $(LOGDIR)/$(CLOUDPC_ALIAS)
endif

ifneq ($(TESTPLAT), cloudnode)
ifneq ($(TESTPLAT), linux)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLOUDPC_ALIAS)/logs/dx_remote_agent/*.log $(LOGDIR)/$(CLOUDPC_ALIAS)
endif
endif
endif #CLOUDPC_ALIAS

ifdef MD_ALIAS
	-mkdir -p $(LOGDIR)/$(MD_ALIAS)
	$(MAKE) md_log_collection_$(TESTPLAT)
endif #MD_ALIAS

ifdef CLIENT_ALIAS
	-mkdir -p $(LOGDIR)/$(CLIENT_ALIAS)
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_CLIENT)/logs/ccd/*.log $(LOGDIR)/$(CLIENT_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLIENT_ALIAS)/$(CLIENT_ALIAS)_remote_agent.log $(LOGDIR)/$(CLIENT_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLIENT_ALIAS)/core* $(LOGDIR)/$(CLIENT_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLIENT_ALIAS)/valgrind.log $(LOGDIR)/$(CLIENT_ALIAS)
ifneq ($(TESTPLAT), cloudnode)
ifneq ($(TESTPLAT), linux)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(CLIENT_ALIAS)/logs/dx_remote_agent/*.log $(LOGDIR)/$(CLIENT_ALIAS)
endif
endif
endif #CLIENT_ALIAS

.PHONY: md_log_collection_win32
md_log_collection_win32:
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_MD)/logs/ccd/*.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/logs/dx_remote_agent/*.log $(LOGDIR)/$(MD_ALIAS)

.PHONY: md_log_collection_linux
md_log_collection_linux:
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_MD)/logs/ccd/*.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/$(MD_ALIAS)_remote_agent.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/core* $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/valgrind.log $(LOGDIR)/$(MD_ALIAS)

.PHONY: md_log_collection_cloudnode
md_log_collection_cloudnode:
	-rsync $(TARGET_LOG):$(CCD_APPDATA_PATH_MD)/logs/ccd/*.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/$(MD_ALIAS)_remote_agent.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/core* $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/valgrind.log $(LOGDIR)/$(MD_ALIAS)

.PHONY: md_log_collection_ios
md_log_collection_ios:
	-$(MD_REMOTE_RUN) 'security unlock-keychain -p notsosecret /Users/$(TARGET_MAC_USER)/Library/Keychains/act-mve.keychain'
	-$(MD_REMOTE_RUN) 'osascript $(TARGET_SCRIPTROOT)/CloseProject.scpt $(TARGET_IOS_TESTROOT) dx_remote_agent dx_remote_agent'
	-$(MD_REMOTE_RUN) 'osascript $(TARGET_SCRIPTROOT)/BackupAndDeleteApp.scpt Backup dx_remote_agent 1'
	-$(MD_REMOTE_RUN) 'chmod 777 $(TARGET_SCRIPTROOT)/WaitForAppData.sh'
	-$(MD_REMOTE_RUN) '$(TARGET_SCRIPTROOT)/WaitForAppData.sh com.acer.dx-remote-agent*.xcappdata'
	-rsync -av $(TARGET_MD_LOG):$(APP_DATA_DIR)/com.acer.dx-remote-agent*.xcappdata $(LOGDIR)/$(MD_ALIAS)

.PHONY: md_log_collection_winrt
md_log_collection_winrt:
	-rsync $(TARGET_MD_LOG):$(CCD_APPDATA_PATH_MD)/logs/dx_remote_agent/*.log $(LOGDIR)/$(MD_ALIAS)
	-rsync $(TARGET_MD_LOG):$(WIN_METRO_CORE_DUMP_PATH)/dx_remote_agent.exe*.dmp $(LOGDIR)/$(MD_ALIAS)

.PHONY: md_log_collection_android
md_log_collection_android:
	-rsync $(TARGET_LOG):$(TARGET_DXREMOTE)_$(MD_ALIAS)/logs/dx_remote_agent/*.log $(LOGDIR)/$(MD_ALIAS)
	$(REMOTE_RUN) 'cd $(TARGET_TESTROOT); ./dxshell$(EXE) CollectCCLog $(MD_ALIAS)'
	-rsync $(TARGET_LOG):$(TARGET_TESTROOT)/$(MD_CCD_Log) $(LOGDIR)/$(MD_ALIAS)
	-cd $(LOGDIR)/$(MD_ALIAS); tar xvzf $(LOGDIR)/$(MD_ALIAS)/$(MD_CCD_Log)
	-cd $(LOGDIR)/$(MD_ALIAS); rm $(LOGDIR)/$(MD_ALIAS)/$(MD_CCD_Log)

	$(REMOTE_RUN) '$(TARGET_DXREMOTE)_$(MD_ALIAS)/AdbTool/adb.exe logcat -v time -d > android_dump.txt'
	-rsync $(TARGET_LOG):android_dump.txt $(LOGDIR)/$(MD_ALIAS)

.PHONY: post_test_run
post_test_run:
# Attempt to create dumps for potentially deadlocked processes.
# We do this here (instead of in post_suite_timeout) since dxshell includes some of its own timeout
# logic, and it is therefore possible for a suite to complete without timing out even though
# CCD is actually deadlocked.
ifeq ($(TESTPLAT), linux)
# Caution: pkill under Cygwin seems to hang under buildbot, so only do this under Linux.
# See https://bugs.ctbg.acer.com/show_bug.cgi?id=16132#c3.
	# Attempt to create core dumps for potentially deadlocked processes by sending SIGQUIT (signal 3).
	-$(REMOTE_RUN) 'pkill -3 ccd$(EXE)'
	-$(REMOTE_RUN) 'pkill -3 dxshell$(EXE)'
	-$(REMOTE_RUN) 'kill `/sbin/pidof valgrind`'
endif
	-$(MAKE) collect_logs

.PHONY: post_suite_timeout
post_suite_timeout:

.PHONY: cleanup
cleanup:
	-$(MAKE) clean

.PHONY: clean
clean:
	-rm -fr $(LOGDIR)/*
	-$(REMOTE_RUN) '$(KILL) ccd$(EXE)'
	-$(REMOTE_RUN) '$(KILL) dxshell$(EXE)'
	-$(REMOTE_RUN) '$(KILL) dx_remote_agent$(EXE)'
ifneq ($(TESTPLAT), cloudnode)
ifneq ($(TESTPLAT), linux)
	-$(REMOTE_RUN) '$(KILL) CCDMonitorService$(EXE)'
	-$(REMOTE_RUN) 'rm -fr $(WIN_DESK_CORE_DUMP_PATH)/*.dmp'
	-$(REMOTE_RUN) 'rm -fr $(WIN_DESK_CORE_DUMP_PATH_FOR_SYSTEM_USER)/*.dmp'
	-$(REMOTE_RUN) 'rm -fr $(WIN_DESK_CCDMSRV_PATH)'
endif
endif

ifdef CLOUDPC_ALIAS
	-$(REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH_CLOUDPC)/*'
endif

ifdef MD_ALIAS
	-$(REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH_MD)/*'
endif

ifdef CLIENT_ALIAS
	-$(REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH_CLIENT)/*'
endif
	-$(REMOTE_RUN) 'rm -fr $(DEFAULT_CCD_APPDATA_PATH)/*'
	-$(REMOTE_RUN) 'rm -fr $(TARGET_TESTROOT)/*'
	-$(MAKE) clean_$(TESTPLAT)
	-$(MAKE) clean_test_data

.PHONY: clean_win32
clean_win32:

.PHONY: clean_linux
clean_linux:
	-$(REMOTE_RUN) 'ps aux|egrep "ccd$(EXE)|dx_remote_agent$(EXE)"'

.PHONY: clean_cloudnode
clean_cloudnode:
	-$(REMOTE_RUN) 'ps aux|egrep "ccd$(EXE)|dx_remote_agent$(EXE)"'
	-$(CLOUDPC_REMOTE_RUN) 'rm -fr $(CCD_APPDATA_PATH_CLOUDPC)/*'
	-$(CLOUDPC_REMOTE_RUN) 'rm -fr $(DEFAULT_CCD_APPDATA_PATH)/*'

.PHONY: clean_ios
clean_ios:
	-$(MD_REMOTE_RUN) 'rm -fr $(APP_DATA_DIR)/*.xcappdata'

.PHONY: clean_winrt
clean_winrt:
	-$(MD_REMOTE_RUN) 'cd $(TARGET_DXREMOTE)_$(MD_ALIAS); ./metro_app_utilities.exe remove 43d863b7-f317-4bf4-a669-9ba42a052c53_1.0.0.0_x86__bzrwd8f9mzhby'
	-$(MD_REMOTE_RUN) 'rm -fr $(WIN_METRO_CORE_DUMP_PATH)/*.dmp'

.PHONY: clean_android
clean_android:
	#-$(REMOTE_RUN) '$(TARGET_DXREMOTE)_$(MD_ALIAS)/AdbTool/adb.exe shell rm /sdcard/AcerCloud/logs/cc/*.log'
	-$(REMOTE_RUN) '$(KILL) adb$(EXE)'
	-$(REMOTE_RUN) 'rm -fr cc'
	-$(REMOTE_RUN) 'rm -f android_dump.txt'

.PHONY: prepare_test_doc_data
prepare_test_doc_data:
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_DOC_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_DOC_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_DOC_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_DOC_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_DOC_DEPLOYED) ] && mkdir -p $(GOLDEN_TEST_DATA_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_DOC_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && wget http://$(STORE_HOST):$(HTTP_PORT)/$(ARCHIVED_TESTDATADIR)/$(CURRENT_DOC_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_DOC_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && tar -zxvf $(CURRENT_DOC_DATA_FILE) && date > $(GOLDEN_TEST_DATA_DOC_DEPLOYED)'
	-$(REMOTE_RUN) 'rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_DOC_DATA_FILE)'

.PHONY: prepare_test_photo_data
prepare_test_photo_data:
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_PHOTO_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_PHOTO_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED) ] && mkdir -p $(GOLDEN_TEST_DATA_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && wget http://$(STORE_HOST):$(HTTP_PORT)/$(ARCHIVED_TESTDATADIR)/$(CURRENT_PHOTO_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && tar -zxvf $(CURRENT_PHOTO_DATA_FILE) && date > $(GOLDEN_TEST_DATA_PHOTO_DEPLOYED)'
	-$(REMOTE_RUN) 'rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_PHOTO_DATA_FILE)'

.PHONY: prepare_test_music_data
prepare_test_music_data:
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_MUSIC_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED) ] && rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_MUSIC_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED) ] && mkdir -p $(GOLDEN_TEST_DATA_DIR)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && wget http://$(STORE_HOST):$(HTTP_PORT)/$(ARCHIVED_TESTDATADIR)/$(CURRENT_MUSIC_DATA_FILE)'
	-$(REMOTE_RUN) '[ ! -f $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED) ] && cd $(GOLDEN_TEST_DATA_DIR) && tar -zxvf $(CURRENT_MUSIC_DATA_FILE) && date > $(GOLDEN_TEST_DATA_MUSIC_DEPLOYED)'
	-$(REMOTE_RUN) 'rm -rf $(GOLDEN_TEST_DATA_DIR)/$(CURRENT_MUSIC_DATA_FILE)'

