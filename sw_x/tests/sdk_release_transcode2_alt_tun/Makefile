# run sdk_release_transcode2 over VSSI tunnel

export CCDCONF_ENABLETS = 7
export CCDCONF_CLEARFIMODE = 6

include ../sdk_release_transcode2/Makefile

override MY_TEST_NAME=sdk_release_transcode2

.PHONY: sdk_release_transcode2_alt_tun
sdk_release_transcode2_alt_tun: sdk_release_transcode2
