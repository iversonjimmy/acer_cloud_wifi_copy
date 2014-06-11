#!/bin/sh

# This test verifies behavior with respect to cloudnode db backup,
# db corruption detection and response, db error handling,
# and db fsck.
#
# It is specifically to test error cases and fsck, but 
# much of normal db backup behavior is tested as a side effect.
#
# dxshell and ccd must be in cwd.
#
# This script is normally run via ssh from the sw_x/tests/vs_psn/Makefile.
# Log messages to stdout go to vs_psn test log.
# It expects the ccd user is logged in, normally CCD is aready running,
# but this test will start ccd if not running on entry.
# CCD is stopped and started multiple times during the test.
# On successful exit, CCD is still running.
#
# See detailed comments further down about test specifics.


# to comment out a block
: <<'ENCLOSED_BLOCK_IS_DISABLED'
ENCLOSED_BLOCK_IS_DISABLED


use_devlopment_short_cuts=0


usage()
{
    echo
    [ -n "$1" ] && { echo; echo "    $1"; }
    echo
    echo ">>>>  Usage:"
    echo
    echo "$script [-h]"
    echo
    echo "  Display this help."
    echo
    echo "OR"
    echo
    echo "$script [-L] CCD_APPDATA_PATH <full cmd and args to call vsTest_psn>"
    echo
    echo "  Test cloudnode database error handling, db backup, and db fsck."
    echo
    echo "  -L Specifies to run the long version of the tests."
    echo
    echo "  CCD_APPDATA_PATH is the path to the directory that contains cc/sn/*/*/<db files>"
    echo
    echo "  Everything after CCD_APPDATA_PATH is the cmd and args to call vsTest_psn. These "
    echo "  are the same cmd and arguments that you would normally use when running vsTest_psn."
    echo
    echo "  This script manipulates db files, stops and starts the CCD, and calls vsTest_psn multiple times."
    echo "  When it runs vsTest_psn it passes \"-b $CCD_APPDATA_PATH/cc/sn/<number>/<number>\""
    echo "  and \"-e <db_err_hand_param>\" to run test_db_error_handling() with those parameters."
    echo
    fail
}

getoptions()
{
    [ $# -eq 0 ] || [ "$1" = "-h" ] && usage "help for $script"
    
    [ "$1" = "-L" ] && doLongTests=1 && shift;
    
    [ $# -lt 2 ] && usage "Invalid number of arguments"

    CCD_APPDATA_PATH="${1%/}"
    
    if [ ! -d "$CCD_APPDATA_PATH" ]; then 
        usage "CCD_APPDATA_PATH ($CCD_APPDATA_PATH) is not a directory"
    fi
    
    shift
    
    testApp="$*"
}


log()
{
    # Ash shell on cloudnode doesn't provide a way to get a script line number. But,
    # forcing an error with "$(L 2>&1)" gives an err msg with embedded line number.
    
    # Argument 1 should be "$(L 2>&1)", i.e. forced error msg with callers line number.
    # Argument 2 should be additional text to display
    # This routine does not fail if the args are not provided, it just displays less info.

    # Don't know why, but with cloudnode ash shell, "$(L 2>&1)" in a called fuction
    # (like below) gives an error msg with the callers line number.
    # On linux you always get the line number where the "$(L 2>&1)" appears

    local line_number=$(echo "$1" | sed -re 's/^.*:([ line]*) ([0-9]+): .*$/\2/' | egrep '^[0-9]+$')

    if [ -n "$line_number" ]; then
        shift
    else
        line_number=$(echo "$(L 2>&1)" | sed -re 's/^.*:([ line]*) ([0-9]+): .*$/\2/' | egrep '^[0-9]+$')
    fi
    
    # echo "###############################################################"

    echo "$(date  "+%Y/%m/%d %H:%M:%S") ${script}:${line_number}| $@"

    return 0
}


fail()
{
    # Ash shell on cloudnode doesn't provide a way to get a script line number. But,
    # forcing an error with "$(L 2>&1)" gives an err msg with embedded line number.
    
    # Argument 1 should be "$(L 2>&1)", i.e. forced error msg with callers line number.
    # Optional 2nd arg should be any additional text to display
    # This routine does not fail if the args are not provided, it just displays less info.

    # Don't know why, but with cloudnode ash shell, "$(L 2>&1)" in a called fuction
    # (like below) gives an error msg with the callers line number.
    # On linux you always get the line number where the "$(L 2>&1)" appears

    local line_number=$(echo "$1" | sed -re 's/^.*:([ line]*) ([0-9]+): .*$/\2/' | egrep '^[0-9]+$')

    if [ -z "$line_number" ]; then
        [ -n "$@" ] && log "$(L 2>&1)" "$@"
        log "$(L 2>&1)" "TC_RESULT = FAIL ;;; TC_NAME = $testName"
    else
        [ -n "$2" ] && log "$@"
        log "$1" "TC_RESULT = FAIL ;;; TC_NAME = $testName"
    fi

   if [ -n "$db_dir" -a -d "$db_dir" ]; then 
        if [ -z "$line_number" ]; then
            log "$(L 2>&1)" "Content of $db_dir when fail is:";  ls -lt "$db_dir/"
            log "$(L 2>&1)" "Content of $db_data_0_0 when fail is:"; ls -lt "$db_data_0_0/"
            # log "$(L 2>&1)" "Logs in $ccd_log_dir when fail is:";  ls -lt "$ccd_log_dir/"
            ccdLog=$(ls -t1 "$ccd_log_dir"/*.log | head -n 1)
            log "$(L 2>&1)" "References to dataset_check in CCD log $ccdLog when fail:"
            grep  dataset_check "$ccdLog"
        else
            log "$1" "Content of $db_dir when fail is:";  ls -lt "$db_dir/"
            log "$1)" "Content of $db_data_0_0 when fail is:"; ls -lt "$db_data_0_0/"
            # log "$1" "Logs in $ccd_log_dir when fail is:";  ls -lt "$ccd_log_dir/"
            ccdLog=$(ls -t1 "$ccd_log_dir"/*.log | head -n 1)
            log "$1" "References to dataset_check in CCD log $ccdLog when fail:"
            grep  dataset_check "$ccdLog"
        fi
   else
        if [ -z "$line_number" ]; then
            log "$(L 2>&1)" "$db_dir_pat does not exist at fail"
        else
            log "$1" "$db_dir_pat does not exist at fail"
        fi
    fi

    if [ -z "$line_number" ]; then
        num_coredumps_now=$(numCoreDumps);
        if [ "$prev_num_coredumps" != "$num_coredumps_now" ]; then
            log "$(L 2>&1)" "Number of coredumps changed from $prev_num_coredumps to $num_coredumps_now:"
            ls -lt $coredumps/core*
        fi
        [ -n "$@" ] && log "$(L 2>&1)" "$@"
        log "$(L 2>&1)" "Exiting $script"
    else
        num_coredumps_now=$(numCoreDumps);
        if [ "$prev_num_coredumps" != "$num_coredumps_now" ]; then
            log "$1" "Number of coredumps changed from $prev_num_coredumps to $num_coredumps_now:";
            ls -lt $coredumps/core*
        fi
        [ -n "$2" ] && log "$@"
        log "$1" "Exiting $script"
    fi
    
    exit 3
}


expected_to_fail()
{
    # Ash shell on cloudnode doesn't provide a way to get a script line number. But,
    # forcing an error with "$(L 2>&1)" gives an err msg with embedded line number.
    
    # Argument 1 should be "$(L 2>&1)", i.e. forced error msg with callers line number.
    # Optional 2nd arg should be any additional text to display
    # This routine does not fail if the args are not provided, it just displays less info.

    # Don't know why, but with cloudnode ash shell, "$(L 2>&1)" in a called fuction
    # (like below) gives an error msg with the callers line number.
    # On linux you always get the line number where the "$(L 2>&1)" appears

    local line_number=$(echo "$1" | sed -re 's/^.*:([ line]*) ([0-9]+): .*$/\2/' | egrep '^[0-9]+$')

    if [ -z "$line_number" ]; then
        [ -n "$@" ] && log "$(L 2>&1)" "$@"
        log "$(L 2>&1)" "TC_RESULT = EXPECTED_TO_FAIL ;;; TC_NAME = $testName"
    else
        [ -n "$2" ] && log "$@"
        log "$1" "TC_RESULT = EXPECTED_TO_FAIL ;;; TC_NAME = $testName"
    fi
    
    numExpectedToFails=$(( numExpectedToFails + 1 ))
}


testAppError()
{
    # Ash shell on cloudnode doesn't provide a way to get a script line number. But,
    # forcing an error with "$(L 2>&1)" gives an err msg with embedded line number.
    
    # Argument 1 should be "$(L 2>&1)", i.e. forced error msg with callers line number.
    # Argument 2 should be the testApp return value
    # Optional 3rd arg should be any additional text to display
    # This routine does not fail if args are not provided, it just displays less info.

    # Don't know why, but with cloudnode ash shell, "$(L 2>&1)" in a called fuction
    # (like below) gives an error msg with the callers line number.
    # On linux you always get the line number where the "$(L 2>&1)" appears

    local line_number=$(echo "$1" | sed -re 's/^.*:([ line]*) ([0-9]+): .*$/\2/' | egrep '^[0-9]+$')
    local rv="$2";

    if [ -z "$line_number" ]; then
        if [ $# -gt 1 ]; then
            shift
            fail "$(L 2>&1)" "test program returned $rv.  $*"
        else
            fail "$(L 2>&1)" "test program returned unexpected value.  $*"
        fi
    else
        errMsgWithLineNum="$1"
        shift
        if [ $# -gt 1 ]; then
            shift
            fail "$errMsgWithLineNum" "test program returned $rv.  $*"
        else
            fail "$errMsgWithLineNum" "test program returned unexpected value.  $*"
        fi
    fi
}


# the shell on the cloudnode does not have cmp or diff, but does have md5sum
compare ()
{
    [ $# -eq 3 ] || fail "$(L 2>&1)" "compare requires 3 arguments"
    
    sum1=$(md5sum "$2" | cut -d ' ' -f 1)
    sum2=$(md5sum "$3" | cut -d ' ' -f 1)
    if [ "$sum1" = "$sum2" ]; then
        log "$1" "files are same: $2 $3"
        return 0
    else
        log "$1" "files are different: $2 $3"
        return 1
    fi
}




checkCCDdoesNotStop()
{
    local loopCount pids
    
    # check that CCD does not stop
    
    loopCount=0
    while [ true ]; do
        pids=$(pgrep '\<ccd\>')
        [ -z "$pids" ] && break
        [ $loopCount -lt $maxSecToWaitForCCDShutdown ] || break
        sleep 1
        loopCount=$(( loopCount + 1 ))
    done

    [ -n "$pids" ] || fail "$1" "CCD has unexpectedly stopped after $loopCount seconds"
    log "$1" "CCD is still running after $loopCount seconds"

    log "$1" "Content of $db_dir while running is:";  ls -lt "$db_dir/"
    log "$1" "Content of $db_data_0_0 while running is:"; ls -lt "$db_data_0_0/"
    # log "$1" "Logs in $ccd_log_dir after stopped is:";  ls -lt "$ccd_log_dir/"
    ccdLog=$(ls -t1 "$ccd_log_dir"/*.log | head -n 1)
    log "$1" "References to dataset_check in CCD log $ccdLog"
    grep  dataset_check "$ccdLog"
}


numCoreDumps()
{
    if [ -d /home/dev/coredump ]; then
        coredumps=/home/dev/coredump
    else
        coredumps=.
    fi

    ls -1 $coredumps/core* 2>/dev/null | wc -l
}


checkCCDstopped()
{
    local loopCount pids name ccdLog
    
    name=${2:-checkCCDstopped}
    
    # check that CCD stopped

    loopCount=0
    while [ true ]; do
        pids=$(pgrep '\<ccd\>')
        [ -z "$pids" ] && break
        [ $loopCount -lt $maxSecToWaitForCCDShutdown ] || break
        sleep 1
        loopCount=$(( loopCount + 1 ))
    done

    if [ "$testName" = "PutTestNameHereToTreatAsExpectedToFail" ]; then 
        [ -z "$pids" ] || expected_to_fail "$1" "CCD still running after $loopCount seconds, pid: $(echo $pids | tr '\n' '\0')"
        killall ccd
    else
        [ -z "$pids" ] || fail "$1" "CCD still running after $loopCount seconds, pid: $(echo $pids | tr '\n' '\0')"
    fi
    
    log "$1" "$name says CCD pid gone after $loopCount seconds"

    log "$1" "Content of $db_dir after stopped is:";  ls -lt "$db_dir/"
    log "$1" "Content of $db_data_0_0 after stopped is:"; ls -lt "$db_data_0_0/"
    # log "$1" "Logs in $ccd_log_dir after stopped is:";  ls -lt "$ccd_log_dir/"
    ccdLog=$(ls -t1 "$ccd_log_dir"/*.log | head -n 1)
    log "$1" "References to dataset_check in CCD log $ccdLog"
    grep  ':dataset_check|' "$ccdLog"

    num_coredumps_now=$(numCoreDumps)
    log "$1" "prev_num_coredumps: $prev_num_coredumps,  num_coredumps_now: $num_coredumps_now"
    if [ "$prev_num_coredumps" != "$num_coredumps_now" ]; then
        if [ "$testName" = "PutTestNameHereToTreatAsExpectedToFail" ]; then 
            expected_to_fail "$1" "Number of coredumps changed"
        else
            fail "$1" "Number of coredumps changed"
        fi
        prev_num_coredumps=$num_coredumps_now
    fi
}


stopCCD()
{
    [ -n "$2" ] && log "$1" "$2"
    
    ./dxshell StopCCD || {
        rv=$?
        fail "$1" "StopCCD returned $rv"
    }

    checkCCDstopped "$1" "stopCCD"
}


startCCD()
{
    local loopCount pids
    
    [ -n "$2" ] && log "$1" "$2"
    
    log "$1" "Content of $db_dir before start ccd is:";  ls -lt "$db_dir/"

    ./dxshell StartCCD || {
        rv=$?
        fail "$1" "StartCCD returned $rv"
    }

    # check that CCD does not stop

    loopCount=0
    while [ true ]; do
        pids=$(pgrep '\<ccd\>')
        [ -z "$pids" ] && break
        [ $loopCount -lt $maxSecToWaitForCCDShutdown ] || break
        sleep 1
        loopCount=$(( loopCount + 1 ))
    done

    [ -n "$pids" ] || fail "$1" "CCD has unexpectedly stopped after $loopCount seconds"
    log "$1" "CCD is still running after $loopCount seconds"

    log "$1" "Content of $db_dir after start CCD is:";  ls -lt "$db_dir/"
}


# setInitialConditions() creates a db and backup db with a directory and file (i.e. db-1-small)

setInitialConditions()
{
    [ $# -eq 2 ] || fail "$(L 2>&1)" "setInitialConditions requires 2 arguments"

    log "$1" "$2"

    log "$(L 2>&1)" "Blow away db, backup db, flags, restart ccd and run test that creates a directory and file"

    pids=$(pgrep '\<ccd\>')
    if [ -n "$pids" ]; then
        stopCCD  "$(L 2>&1)"  "Stop CCD so can re-start with known state"
    fi

    rm -f "$db"
    rm -f "$db_backup"
    rm -f "$db_needs_fsck"
    rm -f "$db_open_fail_1"
    rm -f "$db_fsck_fail_1"
    rm -f "$db_fsck_fail_2"
    rm -f "$db_is_open"
    rm -f "$no_db"

    startCCD "$(L 2>&1)" "Start CCD with no db, no backup, {no_db, db-needs-fsck, db-open-fail-1, db-is-open} markers not set"

    [ -e "$db" ] && fail "$(L 2>&1)" "did not expect db to exist"
    [ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
    [ -e "$no_db" ] && fail "$(L 2>&1)" "did not expect no_db marker"
    [ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "did not expect db_open_fail_1 marker"
    [ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-1 marker"
    [ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-2 marker"
    [ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
    [ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "did not expect to need fsck"
    # when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object

    log "$(L 2>&1)" "Run test program that creates a directory and file"

    $testApp -e $CreateFileExpectSuccess || {
        rv=$?
        testAppError "$(L 2>&1)" $rv "failed to create a directory and file"
    }

    log "$(L 2>&1)" "Content of $db_dir after creating a directory and file is:";  ls -lt "$db_dir/"

    [ -e "$db" ] || fail "$(L 2>&1)" "db does not exist"
    [ -s "$db" ] || fail "$(L 2>&1)" "db is zero size"
    [ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
    [ -e "$no_db" ] && fail "$(L 2>&1)" "did not expect no_db marker"
    [ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "did not expect db_open_fail_1 marker"
    [ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-1 marker"
    [ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-2 marker"
    [ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
    [ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "did not expect to need fsck"

    # create corrupt db as db-corrupt

    dd if=/dev/urandom of=db-corrupt bs=512 count=3 conv=notrunc

    log "$(L 2>&1)" "Wait for backup db to be created. \
    Fail if not created in $(( maxSecToWaitForBackup / 60 )) min and $(( maxSecToWaitForBackup % 60 )) sec."

    # The test script will only create a backup once for small and once for big db.
    # Optionally it can use a backup created on a previous run to shorten development time.
    # To use backup created on previous run, initialize use_devlopment_short_cuts to 1.
    # To force a backup to be created at least once, initialize use_devlopment_short_cuts to 0.
    # To force small backup to be re-created, delete ~/db-1-small before executing this code

    db_1_small_was_copied=0
    [ -s ~/db-1-small ] && cp ~/db-1-small  "$db_backup" && db_1_small_was_copied=1

    loopCount=0
    while [ true ]; do
        if [ $(( loopCount % 60 )) -eq 0 ]; then
            log "$(L 2>&1)" "After $(( loopCount / 60 )) min and $(( loopCount % 60 )) sec, content of $db_dir is:"
            ls -lt "$db_dir"
        fi
        [ -s "$db_backup" ] && break
        [ $loopCount -lt $maxSecToWaitForBackup ] || break
        sleep 1
        loopCount=$(( loopCount + 1 ))
    done

    [ -s "$db_backup" ] || fail "$(L 2>&1)" "DB backup not created after $(( loopCount / 60 )) min and $(( loopCount % 60 )) sec"

    log "$(L 2>&1)" "DB backup exists after $(( loopCount / 60 )) min and $(( loopCount % 60 )) sec"

    log "$(L 2>&1)" "Content of $db_dir after creating a directory and file is:";  ls -lt "$db_dir/"

    cp "$db_backup" db-1-small

    # shorten test during development by saving backup and later  using previously created good backup db
    cp db-1-small ~/db-1-small

    [ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
    [ -e "$db_backup" ]|| fail "$(L 2>&1)" "Expected backup db exists"
    [ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
    [ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
    [ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-1 marker"
    [ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "did not expect db-fsck-fail-2 marker"
    [ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
    [ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"
}






# Trap sig kills
on_kill()
{
    fail "$(L 2>&1)" "Received Kill"
}


###### script execution starts here ######

trap 'on_kill' TERM

testSuiteName=db_error_handling
testName=$testSuiteName
doLongTests=0

CreateFileExpectSuccess=1
ReadFileExpectSuccess=2
ReadFileExpectNotFound=3
ExpectDBCorruptionOnOpen=4
ExpectDBIrrecoverablyFailed=5
CreateFile2ExpectSuccess=6
ReadDir2AndFile2ExpectSuccess=7
ReadDir2AndFile2ExpectNotFound=8
ReadDir2AndFile2ExpectFileNotFound=9
ReadFileExpectSuccessButFileLarger=10
ReadFileExpectSuccessButFileSmaller=11
ReadFileExpectCCDStopped=12
ExpectDBCorruptionOnfsck=13
CheckDBBackupCreation=14

maxSecToWaitForBackup=$(( 60 * 20 ))
maxSecToWaitForCCDShutdown=6

script="$(basename $0)"
scriptdir="$(dirname $0)"
startdir=$(pwd)

log "$(L 2>&1)" "Arguments: $*"

getoptions "$@"

db_dir_pat="$CCD_APPDATA_PATH"/cc/sn/*[0-9A-Fa-f]/*[0-9A-Fa-f]/
ccd_log_dir="$CCD_APPDATA_PATH"/logs/ccd

prev_num_coredumps=$(numCoreDumps)
numExpectedToFails=0



# This test verifies behavior with respect to db backup,
# db corruption detection and response, db error handling,
# and db fsck.
#
# It is specifically to test error cases and fsck, but 
# much of normal db backup behavior is tested as a side effect.
#
# This is not a stress test and does not attempt to verify
# behavior with large numbers of directories and/or files.
#
# This script monitors and manipulates db files and CCD marker files
# to setup and check behavior.
#
# It runs vsTest_psn multiple times with specific command line
# parameters to test specific scenarios. The vsTest_psn program does
# only a single requested db error handling test when run by this script.
#
# On entry it is expected that cdd and cloudPC are already started,
# but will start ccd if not already running.
#
# It verifies 
#     - db backups get created and used when appropriate.
#     - Behavior when a backup db does not exist is as expected.
#     - Appropriate errors are returned when opening the database
#       or accessing files when corruption is detected.
#     - The CCD does not get stuck in a never ending
#       loop of shutting down because of db corruption
#     - after CCD is restarted after shutdown due to corruption,
#       directories and files are either missing or present as expected.
#     - db fsck occurs and behaves as expected
#     - Much of the expected settings of markers are verified as a
#       side effect of monitoring behavior during these tests.
#
# The short version of this test does tests that don't require
# waiting for a backup to be created.  FSCK tests are part of the long test.
#
# Search in this file for "long tests" to find the start of the long tests
# Search for "test dataset fsck" to find the start of the fsck specific tests
#
# Search for "testName=" to find where named tests begin.
#
# Test name convention is that at start of test
#   markers listed after _nm_ don't exist (no marker)
#   markers listed after _m_ are set (marker exists)
#   markers or other status listed after _x_ are expected at end of test case



pids=$(pgrep '\<ccd\>')
if [ -z "$pids" ]; then
    startCCD "$(L 2>&1)" "Start CCD, normally CCD is already started on entry to these tests"
fi

num_db_dirs=$(ls -1d $db_dir_pat 2>/dev/null | wc -l)

if [ $num_db_dirs -eq 0 ]; then
    log "$(L 2>&1)" "$db_dir_pat does not exist at start of test"
elif [ $num_db_dirs -eq 1 ]; then
    db_dir=$(ls -1d $db_dir_pat 2>/dev/null)
    db_dir=${db_dir%/}
    log "$(L 2>&1)" "Content of db dir at start of test:";  ls -lt "$db_dir/"
    testApp="$testApp -b $db_dir"
else
    ls -1d $db_dir_pat
    fail "$(L 2>&1)" "Expected no more than 1 db dir, found $num_db_dirs"
fi

if [ $use_devlopment_short_cuts -eq 0 ]; then
    rm -f ~/db-1-small
    rm -f ~/db-1-big
fi


############################################################

testName=dbeh_setup_initial_db

log "$(L 2>&1)" "Run test program that creates a directory and a file, expect success (testName: $testName)"

$testApp -e $CreateFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "failed to create a directory and file"
}


if [ $num_db_dirs -eq 0 ]; then
    db_dir=$(ls -1d $db_dir_pat 2>/dev/null)
    db_dir=${db_dir%/}
    testApp="$testApp -b $db_dir"
fi

[ -d "$db_dir" ] || fail "$(L 2>&1)" "$db_dir_pat does not exist"

echo "testApp is: $testApp"

db_dir=${db_dir%/}
db="${db_dir}/db"
db_backup="${db_dir}/db-1"
no_db="${db_dir}/no-db"
db_open_fail_1="${db_dir}/db-open-fail-1"
db_fsck_fail_1="${db_dir}/db-fsck-fail-1"
db_fsck_fail_2="${db_dir}/db-fsck-fail-2"
db_needs_fsck="${db_dir}/db-needs-fsck"
db_needs_backup="${db_dir}/db-needs-backup"
db_is_open="${db_dir}/db-is-open"
db_data_0_0="$db_dir/data/0/0"
file1="$db_dir/data/0/0/x2"
zeroSizeFile1="$db_dir/data/0/0/x3"

# uncomment next line to skip tests until ENCLOSED_BLOCK_IS_DISABLED is found
# : <<'ENCLOSED_BLOCK_IS_DISABLED'

log "$(L 2>&1)" "Content of $db_dir after creating a directory and file is:";  ls -lt "$db_dir/"

[ -e "$db" ] || fail "$(L 2>&1)"  "db does not exist."
[ -s "$db" ] || fail "$(L 2>&1)"  "db is zero size."
# it is ok if backup exists.
[ -e "$no_db" ] && fail "$(L 2>&1)"  "Did not expect no_db marker."
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object

# create corrupt db as db-corrupt

dd if=/dev/urandom of=db-corrupt bs=512 count=3 conv=notrunc

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# Test name convention is that at start of test
#   markers listed after _nm_ don't exist (no marker)
#   markers listed after _m_ are set (marker exists)
#   markers or other status listed after _x_ are expected at end of test case

testName=dbeh_corruptdb_nobackup_nm_nodb_x_openfail1_needfsck

log "$(L 2>&1)" "Test when corrupt db, backup does not exist, no_db marker not set (testName: $testName)"

cp -a db-corrupt "$db"
rm -f "$db_backup"

log "$(L 2>&1)" "Content of $db_dir after corrupt db is:";  ls -lt "$db_dir/"

log "$(L 2>&1)" "Run test program, expect no failure because db was already open when db corrupted"

$testApp -e $CreateFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success because CCD already running"
}

[ -d "$db_dir" ] || fail "$(L 2>&1)" "$db_dir does not exist"

log "$(L 2>&1)" "Content of $db_dir after run test with no failure because db was already open is:";  ls -lt "$db_dir/"

##############

stopCCD "$(L 2>&1)" "Stop ccd and restart, run test to detect corrupt db when no backup and no_db marker not set"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "re-corrupt db because shutdown overwrites corrupt db with good one"
dd if=/dev/urandom of=db-corrupt bs=512 count=40 conv=notrunc
cp db-corrupt "$db"

startCCD "$(L 2>&1)" "Start CCD with db corrupted, no backup, no_db marker not set"

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected db to be corrupted version"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "Run test app, expect db corruption detected on open object"

log "$(L 2>&1)" "Content of $db_dir before run test app is:";  ls -lt "$db_dir/"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

[ -e "$db" ] || fail "$(L 2>&1)" "expect db to be present even though it is corrupt"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to still be present"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_corruptdb_nobackup_nm_nodb_m_openfail1_needfsck_x_nodb

startCCD  "$(L 2>&1)"  "Test with db corrupted, no backup, no_db marker not set, db_open_fail_1 set, expect no_db marker (testName: $testName)"


log "$(L 2>&1)" "Run test app, expect db corruption detected on open object"

log "$(L 2>&1)" "Content of $db_dir before run test app is:";  ls -lt "$db_dir"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

[ -e "$db" ] && fail "$(L 2>&1)" "expect db was removed because it is corrupt"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "Expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists: on open object tries to create or open db, will not shut down even if can not create or open db

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# Test name convention is that at start of test
#   markers listed after _nm_ don't exist (no marker)
#   markers listed after _m_ are set (marker exists)
#   markers or other status listed after _x_ are expected at end of test case

testName=dbeh_nodb_nobackup_m_nodb_needfsck_x_markers_cleared_db_created

log "$(L 2>&1)" "Test when no db and no backup and no_db marker set and db needs fsck, expect CCD keeps running, db created and markers cleared (testName: $testName)"

startCCD  "$(L 2>&1)"  "Start CCD with no db, no backup, no_db marker set, expect CCD keeps running, db created on open db"

[ -e "$db" ] && fail "$(L 2>&1)" "did not expect db to exist"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "expected no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists: on open object tries to create or open db, will not shut down even if can not create or open db

##############
log "$(L 2>&1)" "Run test app, expect db created successully on open object, no-db and db-needs-fsck removed, CCD keeps running"

$testApp -e $CreateFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected to successfully open db and create file"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -e "$db" ] || fail "$(L 2>&1)" "expected db to be recreated"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "expected no_db marker to be cleared"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "did not expect db_needs_fsck"


log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_corruptdb_nobackup_m_nodb_x_openfail1

log "$(L 2>&1)" "Test when corrupt db and no backup and no_db marker set, expect VSSI_FAIL on open object (testName: $testName)"

stopCCD "$(L 2>&1)" "Stop ccd and restart, run test to detect corrupt db when no backup and no_db marker set"

log "$(L 2>&1)" "re-corrupt db"
dd if=/dev/urandom of=db-corrupt bs=512 count=30 conv=notrunc
cp  db-corrupt "$db"
:>"$no_db"
:>"$db_needs_fsck"
# Can't get no_db without db_needs_fsck, other test verifies that
# Even so, testing shows this test works the same even if db_needs_fsck is missing

startCCD "$(L 2>&1)" "Start CCD with db corrupted, no backup, no_db marker set"

[ -e "$db" ] || fail "$(L 2>&1)" "expected corrupt db to still be present"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "expected no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "expected db-needs-fsck"
# when no_db marker exists and db corruption exists, will not shut down, will get errors on open object

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to still be present"

log "$(L 2>&1)" "Run test app, expect error returned on open object because corrupt db with no_db set"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected VSSI_ACCESS because db irrecoverably failed"
}

log "$(L 2>&1)" "Content of $db_dir after run test app:";  ls -lt "$db_dir/"

checkCCDstopped "$(L 2>&1)"

[ -e "$db" ] || fail "$(L 2>&1)" "Corrupt db was unexpectedly removed"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "Expected no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "Expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to still be present"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_corruptdb_nobackup_m_nodb_openfail1_needfsck_x_keep_running

startCCD  "$(L 2>&1)"  "Test with db corrupted, no backup, no_db marker set, db_open_fail_1 set (testName: $testName)"

[ -e "$db" ] || fail "$(L 2>&1)" "expected corrupt db to still be present"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "expected no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "Expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to still be present"

log "$(L 2>&1)" "Run test app, expect error returned on open object because corrupt db with no_db set. Expect CCD keeps running"

$testApp -e $ExpectDBIrrecoverablyFailed || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected VSSI_FAILED because db irrecoverably failed"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -e "$db" ] && fail "$(L 2>&1)" "expect db was removed because it is corrupt"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] || fail "$(L 2>&1)" "Expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object


log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_StopCCD_after_db_irrecoverably_failed

log "$(L 2>&1)" "Test stopping CCD after db irrecoverably failed (testName: $testName)"

prev_numExpectedToFails=$numExpectedToFails

stopCCD "$(L 2>&1)" "Stop CCD after db irrecoverably failed"

if [ "$numExpectedToFails" = "$prev_numExpectedToFails" ]; then
    log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"
fi

############################################################

testName=dbeh_corruptdb_corruptbackup_nm_nodb

log "$(L 2>&1)" "Test when corrupt db and corrupt backup and no_db marker not set (testName: $testName)"

cp db-corrupt "$db"
cat db-corrupt db-corrupt >db-1-corrupt
cp db-1-corrupt "$db_backup"
rm -f "$no_db"


startCCD "$(L 2>&1)" "Start CCD after corrupt db and corrupt backup and remove no-db marker, expect CCD runs till access db"


#   run test
#     expect error -16106 VSSI_ACCESS on open object
#   expect ccd has stopped

log "$(L 2>&1)" "Run test app, expect db corruption detected on open object"

log "$(L 2>&1)" "Content of $db_dir before run test app is:";  ls -lt "$db_dir/"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "Expected corrupt db to still be present"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected corrupt backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "Expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"
# when no_db marker exists and db corruption exists, will not shut down, will just get errors on open object


compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to be unchanged"
compare "$(L 2>&1)" "$db_backup" db-1-corrupt || fail "$(L 2>&1)" "Expected corrupt backup db to be unchanged"

startCCD "$(L 2>&1)" "Start CCD with db corrupted,backup corrupted, no_db marker not set, db-open-fail-1 set, expect CCD keeps running until access db, then move backup db to db"

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected corrupt db to be unchanged"
compare "$(L 2>&1)" "$db_backup" db-1-corrupt || fail "$(L 2>&1)" "Expected corrupt backup db to be unchanged"

#   run test
#     expect error -16106 VSSI_ACCESS on open object
#   expect ccd has stopped

log "$(L 2>&1)" "Run test app, expect db corruption detected on open object"

log "$(L 2>&1)" "Content of $db_dir is:";  ls -lt "$db_dir/"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

log "$(L 2>&1)" "Expect backup db replaces db, db_open_fail_1 goes away"

[ -e "$db" ] || fail "$(L 2>&1)" "expected db to be replaced by backup db"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=$testSuiteName

if [ $doLongTests -eq 0 ]; then
    rm -f "$db"
    startCCD  "$(L 2>&1)"  "Start CCD because caller expects it to be running on exit"
    log  "$(L 2>&1)"  "TC_RESULT = PASS ;;; TC_NAME = $testName"
    log  "$(L 2>&1)"  "Exiting $script"
    exit 0
fi

###########################################################
#           long tests
###########################################################

# Setting initial condiditions includes testing creation of db and backup db after create directory and file

if [ -s ~/db-1-small ]; then
    testName=dbeh_create_db_and_backup_from_copy
else
    testName=dbeh_init_create_db_and_backup
fi

setInitialConditions "$(L 2>&1)"  "Test create backup for testing with good backup (testName: $testName)"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_corruptdb_goodbackup_nm_openfail1_needfsck

stopCCD "$(L 2>&1)" "Test with corrupt db, good backup.  Need to stop and re-start db or won't detect problem (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ]|| fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

dd if=/dev/urandom of=db-corrupt bs=512 count=40 conv=notrunc
cp db-corrupt "$db"

startCCD "$(L 2>&1)" "Start CCD with db corrupted, good backup, no_db marker not set"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-corrupt || fail "$(L 2>&1)" "Expected db to be corrupted version"

log "$(L 2>&1)" "Run test program, expect fail on open object"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "Expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"


log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_corruptdb_goodbackup_m_openfail1_needfsck

startCCD "$(L 2>&1)" "Restart ccd, run test, expect corruption detected on db, backup db swapped in, CCD Stopped"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] || fail "$(L 2>&1)" "Expected db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

log "$(L 2>&1)" "Run test program, expect fail on open object"

$testApp -e $ExpectDBCorruptionOnOpen || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open object"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db doesn't exist because moved to db"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be swapped in from good backup"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_swappedbackkupdb_m_needfsck

startCCD "$(L 2>&1)" "Restart ccd, run test, expect success with swapped in db (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db doesn't exist because moved to db"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

$testApp -e $ReadFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success because because good backup swapped in for corrupt db"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"


log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

###########################################################
#  test dataset fsck
###########################################################

testName=dbeh_setup_db_big_for_fsck_test

log "$(L 2>&1)" "Run test program that creates a second level directory and file (testName: $testName)"

$testApp -e $CreateFile2ExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "failed to create a directory and file"
}

log "$(L 2>&1)" "Content of $db_dir after creating a directory and file is:";  ls -lt "$db_dir/"

[ -e "$db" ] || fail "$(L 2>&1)" "db does not exist"
[ -s "$db" ] || fail "$(L 2>&1)" "db is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "did not expect to need fsck"

# save the db to db-big after shutdown ccd

stopCCD "$(L 2>&1)" "Stop ccd to make sure db is saved to disk"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "did not expect backup db to exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

cp -a "$db" db-big

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

if [ -s ~/db-1-big ]; then
    testName=dbeh_creation_of_backupdb_from_copy
else
    testName=dbeh_creation_of_backupdb
fi

startCCD "$(L 2>&1)" "Start CCD with big db to run test that opens db and waits for backup (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"
[ -e "$db_needs_backup" ] || fail "$(L 2>&1)"  "Expected to need backup"

compare "$(L 2>&1)" "$db" db-big || fail "$(L 2>&1)" "Expected db to be db-big"

# The test script will only create a backup once for small and once for big db.
# Optionally it can use a backup created on a previous run to shorten development time.
# To use backup created on previous run, initialize use_devlopment_short_cuts to 1.
# To force a backup to be created at least once, initialize use_devlopment_short_cuts to 0.
# To force big backup to be re-created, delete ~/db-1-big before executing this code

if [ -s ~/db-1-big ]; then
    cp ~/db-1-big  "$db_backup"
else
    log "$(L 2>&1)" "Run test program that verifies db backup occurs when expecteded and VSSI_GetSpace() doesn't delay backup."
    
    $testApp -e $CheckDBBackupCreation || {
        rv=$?
        testAppError "$(L 2>&1)" $rv "Problem reported while checking backup db creation."
    }
    
    checkCCDdoesNotStop  "$(L 2>&1)"

    [ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
    [ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
    [ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
    [ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
    [ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
    [ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

    compare "$(L 2>&1)" "$db" db-big || fail "$(L 2>&1)" "Expected db to be db-big"

fi

cp "$db_backup" db-1-big

# shorten test during development by saving backup and later using previously created good backup db
cp db-1-big ~/db-1-big
compare "$(L 2>&1)" "$db" db-1-big || log "$(L 2>&1)" "db is different from db-1-big even though represent same content"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_small_db_swapped_for_big_m_needfsck

stopCCD "$(L 2>&1)" "Test with small db swapped in for big db, no backup, db-needs-fsck.  Need to stop and re-start db or won't detect needs fsck (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ]|| fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-big || log "$(L 2>&1)" "db is different from db-1-big even though represent same content"

cp db-1-small "$db"
rm -f "$db_backup"
:>$db_needs_fsck

log "$(L 2>&1)" "Content of $db_dir/data after replace big with small db is:";  ls -lR "$db_dir/data"
    
startCCD "$(L 2>&1)" "Start CCD with small db swapped in for big db, no backup, db-needs-fsck."

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be db-1-small"

log "$(L 2>&1)" "Run test program with small db, expect sucess, but second level files deleted by fsck"

$testApp -e $ReadFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success because file is part of small db"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "Content of $db_dir/data after fsck using small db is:";  ls -lR "$db_dir/data"
    
compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to still be db-1-small"

############################################################

log "$(L 2>&1)" "Run test program and expect can't read second level directory and file"

$testApp -e $ReadDir2AndFile2ExpectNotFound || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected 2nd level directory and file not found"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "Content of $db_dir/data after can't read second level directory:";  ls -lR "$db_dir/data"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

testName=dbeh_big_db_swapped_for_small_nobackup_m_needfsck

stopCCD "$(L 2>&1)" "Test with big db swapped in for small db, no backup, db-needs-fsck.  Need to stop and re-start db or won't detect needs fsck (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-big && fail "$(L 2>&1)" "Expected db to be different from db-1-big"
cp db-1-big "$db"
:>$db_needs_fsck

log "$(L 2>&1)" "Content of $db_dir/data after replace small with big db is:";  ls -lR "$db_dir/data"
    
startCCD "$(L 2>&1)" "Start CCD with big db swapped in for small db, no backup, db-needs-fsck."

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-big || fail "$(L 2>&1)" "Expected db to be db-1-big"

log "$(L 2>&1)" "Run test program with big db, expect sucess, and second level db components deleted by fsck"

$testApp -e $ReadFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success because file is part of small db"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "Content of $db_dir/data after read file with big db swapped in for small db is:";  ls -lR "$db_dir/data"
    
compare "$(L 2>&1)" "$db" db-1-big && log "$(L 2>&1)" "Expect big db to be pruned because 2nd level contents missing"

stopCCD "$(L 2>&1)" "db file didn't change so stop ccd to see if it changes after stop"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-big && fail "$(L 2>&1)" "Expected big db to be pruned because 2nd level contents missing"

cp "$db" db-1-pruned
startCCD  "$(L 2>&1)"  "Start CCD so can continue tests"

############################################################

log "$(L 2>&1)" "Run test program and expect can't read second level directory and file"

$testApp -e $ReadDir2AndFile2ExpectFileNotFound || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected 2nd level directory still present but file not found"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-pruned || fail "$(L 2>&1)" "Expected pruned would not change"

stopCCD "$(L 2>&1)" "db file didn't change so stop ccd to see if it changes now"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-pruned || fail "$(L 2>&1)" "Expected pruned would not change"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# uncomment line below and start line earlier in file to skip all tests before here
# ENCLOSED_BLOCK_IS_DISABLED

if [ -s ~/db-1-small ]; then
    testName=dbeh_init_create_db_copy_backup_for_fsck_corrupt_after_header
else
    testName=dbeh_init_create_db_backupdb_for_fsck_corrupt_after_header
fi

setInitialConditions "$(L 2>&1)" "Set initial conditions for testing detection and fsck for db corrupted after header (testName: $testName)"

stopCCD "$(L 2>&1)" "Stop CCD so db will be written to disk"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

if [ $db_1_small_was_copied -eq 1 ]; then
    compare "$(L 2>&1)" "$db" "$db-1" && log "$(L 2>&1)" "Expected db and backup are different because backup copied"
else
    compare "$(L 2>&1)" "$db" "$db-1" || log "$(L 2>&1)" "Expected db and backup are same"
fi

log "$(L 2>&1)" "Corrupt db after header"
cp "$db" db-corrupt
dd if=/dev/urandom bs=512 count=4 | dd conv=notrunc ibs=512 obs=512 seek=10 of=db-corrupt
cp  db-corrupt "$db"
cp  db-corrupt db-corrupt-after-header

# first detects corruption on open file and CCD stops with db-is-open and db-needs-fsck set
# next time started detects corruption on fsck in bg thread started during open object,
# backup db gets swapped in.

startCCD "$(L 2>&1)" "Start ccd, expect success on open object, fail on open file, CCD stopped with db-needs-fsck"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "Run test program, expect success on open object, fail on open file, CCD stopped with db-needs-fsck and db-is-open"

# leaves db-is-open set

$testApp -e $ReadFileExpectCCDStopped || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected on open file"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" "$db-1" && fail "$(L 2>&1)" "Expected db different from backup db"

startCCD "$(L 2>&1)" "Start ccd, run test, expect fsck on open object, corruption detected, CCD Stopped, db_fsck_fail_1, db-needs-fsck"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

log "$(L 2>&1)" "Run test program, expect fsck on open object, corruption detected either on open object or access file, CCD Stopped, db_fsck_fail_1, db-needs-fsck"

# because fsck occurs in bg,
#   error due to detection of corruption may occur on open db or access file

$testApp -e $ExpectDBCorruptionOnfsck || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected when open db or access file"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

startCCD "$(L 2>&1)" "Start ccd, run test, expect fsck on open object, corruption detected either on open object or access file, corruption detected, CCD Stopped, db_fsck_fail_2, db-needs-fsck"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && log "$(L 2>&1)"  "db-is-open marker still set from before."
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

log "$(L 2>&1)" "Run test program, expect expect fsck on open object, corruption detected either on open object or access file"

$testApp -e $ExpectDBCorruptionOnfsck || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected when open db or access file"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

startCCD "$(L 2>&1)" "Start ccd, run test, expect fsck on open object, corruption detected either on open object or access file, backup db swapped in because db_fsck_fail_2 was set, CCD Stopped, db-needs-fsck"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] || fail "$(L 2>&1)" "Expected backup db exists"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] || fail "$(L 2>&1)" "Expected db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && log "$(L 2>&1)"  "db-is-open marker still set from before."
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

log "$(L 2>&1)" "Run test program, expect VSSI_COMM on open object"

$testApp -e $ExpectDBCorruptionOnfsck || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected db corruption detected when open db or access file"
}

checkCCDstopped "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db doesn't exist because moved to db"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be swapped in good backup"

############################################################

startCCD "$(L 2>&1)" "Start CCD with small backup db swapped in, db-needs-fsck. Run test to see can access file"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be db-1-small"

log "$(L 2>&1)" "Run test program with swapped in backup db, expect sucess"

$testApp -e $ReadFileExpectSuccess || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success because file is part of small db"
}

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be db-1-small"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# change size of file1 by adding more of same pattern
# set db_needs_fsck
# run test that reads file

# Why need to stop ccd and restart or fsck will not occur ?
# db_needs_fsck only seems to be checked on startup, not if set while running.
# We don't protect against file size changes while running. i.e.; if you change
# file size while running, and object closed, there is no fsck when object opened

testName=dbeh_fsck_with_file_size_larger

stopCCD "$(L 2>&1)" "Stop CCD so can force fsck on 1st open object (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be db-1-small"

log "$(L 2>&1)" "Before increase file size:"; ls -lt "$db_data_0_0/"

cat "$file1" "$file1" >file1

loopCount=0
while [ true ]; do
    [ $loopCount -lt 12 ] || break
    cp file1 file0
    cat file0 >>file1
    loopCount=$(( loopCount + 1 ))
done

cp file1 "$file1"
cp file1 "$zeroSizeFile1"
log "$(L 2>&1)" "After increase file size:"; ls -lt "$db_data_0_0/"
:>$db_needs_fsck
:>$db_is_open

startCCD "$(L 2>&1)" "Start CCD and Run test program with file1 larger size, needs fsck, expect sucess"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || fail "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-1-small || fail "$(L 2>&1)" "Expected db to be db-1-small"

$testApp -e $ReadFileExpectSuccessButFileLarger  || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success"
}

log "$(L 2>&1)" "After ReadFileExpectSuccessButFileLarger:"; ls -lt "$db_data_0_0/"

stopCCD "$(L 2>&1)" "Stop CCD so db will be written to disk"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-1-small && fail "$(L 2>&1)" "Expected db change"
cp -a "$db" db-bigger-file

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# change size of file1 to smaller size
# set db_needs_fsck
# run test that reads file

testName=dbeh_fsck_with_file_size_smaller

log "$(L 2>&1)" "Before decrease file size:"; ls -lt "$db_data_0_0/"
dd if=file1 of=file2 bs=128 count=1
cp file2 "$file1"
:> "$zeroSizeFile1"
log "$(L 2>&1)" "After decrease file size:"; ls -lt "$db_data_0_0/"
:>$db_needs_fsck

startCCD "$(L 2>&1)" "Start CCD and run test program with file1 smaller size, needs fsck, expect sucess (testName: $testName)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] || fail "$(L 2>&1)"  "Expected to need fsck"

compare "$(L 2>&1)" "$db" db-bigger-file || fail "$(L 2>&1)" "Expected db to be db-bigger-file"


$testApp -e $ReadFileExpectSuccessButFileSmaller  || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success"
}

log "$(L 2>&1)" "After ReadFileExpectSuccessButFileSmaller:"; ls -lt "$db_data_0_0/"

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || log "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && log "$(L 2>&1)"  "Did not expect to need fsck"

stopCCD "$(L 2>&1)" "Stop CCD so db will be written to disk"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

compare "$(L 2>&1)" "$db" db-bigger-file && fail "$(L 2>&1)" "Expected db to change"

startCCD "$(L 2>&1)" "Start CCD and run test program to verify still works."

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] && fail "$(L 2>&1)"  "Did not expect db-is-open marker"
[ -e "$db_needs_fsck" ] && fail "$(L 2>&1)"  "Did not expect to need fsck"

$testApp -e $ReadFileExpectSuccessButFileSmaller  || {
    rv=$?
    testAppError "$(L 2>&1)" $rv "expected success"
}

log "$(L 2>&1)" "After ReadFileExpectSuccessButFileSmaller:"; ls -lt "$db_data_0_0/"

checkCCDdoesNotStop  "$(L 2>&1)"

[ -s "$db" ] || fail "$(L 2>&1)" "db either does not exist or is zero size"
[ -e "$db_backup" ] && fail "$(L 2>&1)" "Expected backup db does not exist"
[ -e "$no_db" ] && fail "$(L 2>&1)" "Did not expect no_db marker"
[ -e "$db_open_fail_1" ] && fail "$(L 2>&1)" "Did not expect db_open_fail_1 marker"
[ -e "$db_fsck_fail_1" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-1 marker"
[ -e "$db_fsck_fail_2" ] && fail "$(L 2>&1)" "Did not expect db-fsck-fail-2 marker"
[ -e "$db_is_open" ] || log "$(L 2>&1)"  "Expected db-is-open marker"
[ -e "$db_needs_fsck" ] && log "$(L 2>&1)"  "Did not expect to need fsck"

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"

############################################################

# leave CCD running on exit because it is expected by automated test

testName=$testSuiteName

log "$(L 2>&1)" "TC_RESULT = PASS ;;; TC_NAME = $testName"
log  "$(L 2>&1)"  "Exiting $script"



