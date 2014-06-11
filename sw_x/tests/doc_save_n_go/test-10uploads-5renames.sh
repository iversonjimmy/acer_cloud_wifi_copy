#!/bin/bash

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
RENAME=$BUILDROOT/release/linux/tests/test_utils/dsngRename

WORKDIR=/tmp/test$$

mkdir -p $WORKDIR
for n in 0 1 2 3 4 5 6 7 8 9; do
    dd if=/dev/urandom of=$WORKDIR/file$n bs=1024 count=8192
    dd if=/dev/urandom of=$WORKDIR/thumb$n bs=1024 count=1
done

$CCD > ccd.log 2>&1 &

$DUMPQUEUE -t 1000 $USERNAME $PASSWORD > dumpEventQueue.log 2>&1 &

for n in 0 1 2 3 4 5 6 7 8 9; do
    $UPDATE $USERNAME $PASSWORD $WORKDIR/file$n $WORKDIR/thumb$n > upload$n.log 2>&1
done
for n in 0 2 4 6 8; do
    $RENAME $USERNAME $PASSWORD $WORKDIR/file$n $WORKDIR/newfile$n > rename$n.log 2>&1
done

while true; do
    nCompleted=`grep doc_save_and_go_completion dumpEventQueue.log | wc -l`
    echo $nCompleted requests completed
    if [[ $nCompleted -eq 15 ]]; then
	break
    fi
    sleep 1
done

killall ccd dumpEventQueue

rm -rf $WORKDIR
