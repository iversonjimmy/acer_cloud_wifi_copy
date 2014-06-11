#!/bin/bash
# this script tries to setup the environment for bug 11786

if [[ $# -lt 4 ]]; then
    echo Usage: $0 username1 password1 username2 password2
    exit 0
fi

username1=$1
password1=$2
username2=$3
password2=$4

if [[ -z "$BUILDROOT" ]]; then
    echo ERROR: BUILDROOT not defined
    exit 1
fi

CCD=$BUILDROOT/release/linux/gvm_core/daemons/ccd/ccd
DUMPQUEUE=$BUILDROOT/release/linux/tests/test_utils/dumpEventQueue
UPDATE=$BUILDROOT/release/linux/tests/test_utils/dsngUpdate
DXSHELL=$BUILDROOT/release/linux/tests/dxshell/dxshell

WORKDIR=/tmp/test$$

mkdir -p $WORKDIR
for n in 0 1 2 3 4 5 6 7 8 9; do
    dd if=/dev/urandom of=$WORKDIR/file1$n bs=1024 count=8192
    dd if=/dev/urandom of=$WORKDIR/file2$n bs=1024 count=8192
    dd if=/dev/urandom of=$WORKDIR/thumb$n bs=1024 count=1
done

$CCD > ccd.log 2>&1 &

$DXSHELL StartClient $username1 $password1

for n in 0 1 2 3 4 5 6 7 8 9; do
    $UPDATE $username1 $password1 $WORKDIR/file1$n $WORKDIR/thumb$n > upload1$n.log 2>&1
done

$DXSHELL StopClient

$DXSHELL StartClient $username2 $password2

for n in 0 1 2 3 4 5 6 7 8 9; do
    $UPDATE $username2 $password2 $WORKDIR/file2$n $WORKDIR/thumb$n > upload2$n.log 2>&1
done

