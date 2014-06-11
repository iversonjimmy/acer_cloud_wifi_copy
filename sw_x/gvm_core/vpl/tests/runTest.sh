#!/autotest/bin/sh

#
# Test driver for vplTest
#

UTIL=/autotest/bin

# determine FQ name of directory containing this script
CMD_DIR=`$UTIL/get_fq_cmd_dir $0`
cd $CMD_DIR

# TEST_WORKDIR should be set by the testharness
if [ x$TEST_WORKDIR = "x" ]; then
    # not set; default to test's dir
    export TEST_WORKDIR=$CMD_DIR
fi

# copy any test files to the workdir
# n/a

# cd to the workdir
# n/a - this test must execute in its original directory to be able to load the
#       library

# Provide empty string for parameters. Test takes no parameters.
$CMD_DIR/vplTest
$UTIL/print_result $? vplTest
