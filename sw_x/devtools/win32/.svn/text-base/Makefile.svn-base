include win32_env_defs.mk

win32_build_tools: \
				$(WIN32API_ROOT)/timestamp

$(WIN32_TOOLS_DOWNLOAD)/%:
	mkdir -p $(WIN32_TOOLS_DOWNLOAD)
	wget --progress=dot:mega --timestamping --directory-prefix=$(WIN32_TOOLS_DOWNLOAD) $(WIN32_PREBUILT_DOWNLOAD_URL)/$*

$(WIN32API_ROOT)/timestamp: $(WIN32_TOOLS_DOWNLOAD)/$(WIN32API)
	mkdir -p $(WIN32API_ROOT)
	rm -fr $(WIN32API_ROOT)/*
	tar -x --lzma -C $(WIN32API_ROOT) -f $(WIN32_TOOLS_DOWNLOAD)/$(WIN32API)
	date > $@
