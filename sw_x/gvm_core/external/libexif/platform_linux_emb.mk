CROSS_PREFIX := /home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/bin/mips-linux-
LIBEXIF_CONF_OPTS =
LIBEXIF_CONFIG_OPTS = \
        --host=mips-linux \
        CC=$(CROSS_PREFIX)gcc \
        CXX=$(CROSS_PREFIX)g++ \
        CFLAGS="-isystem -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/include" \
        CPPFLAGS="-isystem -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/include" \
        CXXFLAGS="-isystem -I/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/include" \
        LDFLAGS="-L/home/jimmy.tsai/prebuilt_tools/rtl819xD/rsdk-1.5.5-5281-EB-2.6.30-0.9.30.3-110714/lib"
# end of list
# #
#
