# run sdk_release_picstream over VSSI tunnel

export CCDCONF_ENABLETS = 7
export CCDCONF_CLEARFIMODE = 6

include ../sdk_release_picstream/Makefile

override MY_TEST_NAME=sdk_release_picstream_alt_tun

.PHONY: sdk_release_picstream_alt_tun
sdk_release_picstream_alt_tun: sdk_release_picstream
