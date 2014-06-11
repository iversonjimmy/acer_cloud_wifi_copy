#!/bin/bash
# Test to make sure queue is non-volatile.
# 
# Queue up 10 upload jobs.
# After the first 3 doc files are uploaded, kill ccd.
# Wait for few seconds.
# Start ccd and login.

if [[ -z "$USERNAME" ]]; then
    echo ERROR: USERNAME not defined
    exit 1
fi
if [[ -z "$PASSWORD" ]]; then
    echo ERROR: PASSWORD not defined
    exit 1
fi
if [[ -z "$BUILDROOT" ]]; then
    echo ERROR: BUILDROOT not defined
    exit 1
fi

CCD=$BUILDROOT/release/linux/gvm_core/daemons/ccd/ccd
DUMPQUEUE=$BUILDROOT/release/linux/tests/test_utils/dumpEventQueue
UPDATE=$BUILDROOT/release/linux/tests/test_utils/dsngUpdate

WORKDIR=/tmp/test$$
mkdir -p $WORKDIR

# make test files
for n in 0 1 2 3 4 5 6 7 8 9; do
    dd if=/dev/urandom of=$WORKDIR/file$n bs=1024 count=8192
    dd if=/dev/urandom of=$WORKDIR/thumb$n bs=1024 count=1
done

$CCD > ccd.log 2>&1 &

$DUMPQUEUE -t 1000 $USERNAME $PASSWORD > dumpEventQueue.log 2>&1 &

# queue upload requests
for n in 0 1 2 3 4 5 6 7 8 9; do
    $UPDATE $USERNAME $PASSWORD $WORKDIR/file$n $WORKDIR/thumb$n > upload$n.log 2>&1
done

# wait for the first 3 requests to complete
while true; do
    nCompleted=`grep doc_save_and_go_completion dumpEventQueue.log | wc -l`
    echo $nCompleted upload requests completed
    if [[ $nCompleted -eq 3 ]]; then
	break
    fi
    sleep 1
done
# first 3 uploads have completed

killall ccd dumpEventQueue

sleep 5

$CCD >> ccd.log 2>&1 &

$DUMPQUEUE -t 1000 $USERNAME $PASSWORD >> dumpEventQueue.log 2>&1 &

# wait until all requests (10 in total) are completed
while true; do
    nCompleted=`grep doc_save_and_go_completion dumpEventQueue.log | wc -l`
    echo $nCompleted upload requests completed
    if [[ $nCompleted -eq 10 ]]; then
	break
    fi
    sleep 1
done

rm -rf $WORKDIR
