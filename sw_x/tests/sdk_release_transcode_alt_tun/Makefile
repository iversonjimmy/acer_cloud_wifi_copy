# run sdk_release_transcode over VSSI tunnel

export CCDCONF_ENABLETS = 7
export CCDCONF_CLEARFIMODE = 6

include ../sdk_release_transcode/Makefile

override MY_TEST_NAME=sdk_release_transcode

.PHONY: sdk_release_transcode_alt_tun
sdk_release_transcode_alt_tun: sdk_release_transcode
