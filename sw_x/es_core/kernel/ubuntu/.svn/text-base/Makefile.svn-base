ARCHIVES_TO_EXTRACT := \
	linux-headers-2.6.32-21.tar.gz \
	linux-headers-2.6.32-21-generic-pae.tar.gz

# TODO: should probably be $(PBROOT)/es_core/kernel/ubuntu
MY_BUILDROOT = $(PBROOT)/kernel_headers

TIMESTAMP_FILES = $(patsubst %,$(MY_BUILDROOT)/%.timestamp,$(ARCHIVES_TO_EXTRACT))

.PHONY: all default
ifdef LOCAL_BUILD
all default:
	# nothing to do
else
all default: $(TIMESTAMP_FILES)
endif

# Extract a .tar.gz and create/update the timestamp.
$(MY_BUILDROOT)/%.tar.gz.timestamp: %.tar.gz
	mkdir -p $(MY_BUILDROOT)
	rm -rf $(MY_BUILDROOT)/$* # This assumes that all of the tar's contents are under a subdirectory that matches the tar's name.
	tar -C $(MY_BUILDROOT) -zxf $*.tar.gz
	touch $@
