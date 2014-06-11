#!/bin/sh

#
# Test driver for vplTest
#
if [ ${0#/} != "$0" ]; then
    # Rooted pathname specified
    CMD=$0
else
    CMD=$PWD/$0
fi

cd ${CMD%/*}

ATF_SCRIPTS=${ATF_SCRIPTS:-../atf/TestSuite.sh}

. $ATF_SCRIPTS ""

# Provide empty string for parameters. Test takes no parameters.
ATF_runTest vplTest . ./vplTest ""

ATF_reportSummary
