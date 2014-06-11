#!/bin/bash
#
# prepChroot - copy to chrooted env
# - copy symlink with target
# - copy elf executable with linked libraries
# - copy character/block devices
#

function print_help {
    echo "Usage: $0 chroot_path app_binary_path"
    exit 1
}

test -z "$*" && print_help
test -d $1 || print_help
test -d $2 && print_help
test -e $1$2 && exit 2

BASEDIR=`dirname $2`
test -d $1/$BASEDIR || mkdir -p $1/$BASEDIR
cp -r -p $2 $1/$BASEDIR

if [ -h $2 ]; then # symlink
    TARGET=`readlink $2`
    if [ '.' = `dirname $TARGET` ]; then
	TARGET=$BASEDIR/$TARGET
    fi

    $0 $1 $TARGET

else # regular, special and other
    ldd $2 1>/dev/null 2>/dev/null
    if [ $? = 0 ]; then
	LIBS="$(ldd $2 | awk '{ print $3 }' | egrep -v ^'\(') \
              $(ldd $2 | grep 'ld-linux' | awk '{ print $1}')"
	for LIB in $LIBS; do

	    $0 $1 $LIB

	done
    fi

fi
exit 0
