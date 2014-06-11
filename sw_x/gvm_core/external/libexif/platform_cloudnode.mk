CROSS_PREFIX := $(CLOUDNODE_CROSS_ROOT)/cross/bin/arm-mv5sft-linux-gnueabi-
LIBEXIF_CONF_OPTS = 
LIBEXIF_CONFIG_OPTS = \
        --host=arm-mv5sft-linux-gnueabi \
        CC=$(CROSS_PREFIX)gcc \
        CXX=$(CROSS_PREFIX)g++ \
        CFLAGS="-isystem -I$(CLOUDNODE_CROSS_ROOT)/cross/lib/gcc/arm-mv5sft-linux-gnueabi/4.3.2/include -I$(CLOUDNODE_CROSS_ROOT)/cross/include -I$(CLOUDNODE_CROSS_ROOT)/sdk/usr/include" \
        CPPFLAGS="-isystem -I$(CLOUDNODE_CROSS_ROOT)/cross/lib/gcc/arm-mv5sft-linux-gnueabi/4.3.2/include -I$(CLOUDNODE_CROSS_ROOT)/cross/include -I$(CLOUDNODE_CROSS_ROOT)/sdk/usr/include" \
        CXXFLAGS="-isystem -I$(CLOUDNODE_CROSS_ROOT)/cross/lib/gcc/arm-mv5sft-linux-gnueabi/4.3.2/include -I$(CLOUDNODE_CROSS_ROOT)/cross/include -I$(CLOUDNODE_CROSS_ROOT)/sdk/usr/include" \
        LDFLAGS="-L$(CLOUDNODE_CROSS_ROOT)/cross/lib -L$(CLOUDNODE_CROSS_ROOT)/cross/lib/gcc/arm-mv5sft-linux-gnueabi/4.3.2 -L$(CLOUDNODE_CROSS_ROOT)/cross/arm-mv5sft-linux-gnueabi/sys-root/lib -L$(CLOUDNODE_CROSS_ROOT)/cross/arm-mv5sft-linux-gnueabi/sys-root/usr/lib -L$(CLOUDNODE_CROSS_ROOT)/rootfs/lib"


# end of list
