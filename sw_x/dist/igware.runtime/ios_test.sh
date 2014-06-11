#!/bin/sh
set -e

export LAB_DOMAIN_NAME="pc-int.igware.net"
export CCD_TEST_ACCOUNT="syncTesterMacBuilder1@igware.com"

make -C ../../tests/vpl
make -C ../../tests/ccd
osascript ../../projects/xcode/PersonalCloud/applescripts/CloseXcode.scpt
