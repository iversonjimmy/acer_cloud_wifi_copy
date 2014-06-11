#!/bin/sh

SCRIPT_DIRECTORY="${0%/*}"
PATH=$PATH:$SCRIPT_DIRECTORY

## Use pid to get unique file timestamps
PID=$$"."
## ... But don't use pid, for now. Instead we put each file in a 'stable date' directory.
PID=


self_describe ()  {
    echo "*** Running VPl timing tests on `hostname`"
    echo "*** uname: "
    uname -a
    echo "***"
    echo  "*** lsmod:"
    lsmod
    echo "***"
    return 0
}

if [ "x$BUILD_DATE" = "x" ] ; then
    BUILD_DATE=`date +%Y%m%d-%H%M`
fi
if [ "x$DATA_DIR" = "x" ]; then
    DATA_DIR=/home/tmp/vpl-data.$BUILD_DATE
fi


if [ "x$OUTDIR" = "x" ]; then
 OUTDIR=$DATA_DIR/
fi


if [ x$ROOT != x ]; then
  export LD_LIBRARY_PATH=$ROOT/gvm/lib
fi

mkdir -p $OUTDIR

self_describe > $OUTDIR/host

for test in \
    atomic \
    file \
    socket \
    thread \
    mutex  \
    sem \
    condvar \
    msgq \
    threadprio \
 ; do
    PROG="vpl_"$test"_timing"
    if [ -d ./$test -a -f ./$test/$PROG ]; then
        echo "Found test at ./$test/$PROG"
	PROG=./$test/$PROG
    fi
    OPT_COUNT=
    if [ x$test != xfile ]; then
        OPT_COUNT="-n 16777216"
    fi
    $PROG $OPT_COUNT >  $OUTDIR$test"-ops."$PID"dat"
done
