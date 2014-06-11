#!/bin/sh

ACTION=""
STORE_USER=""
STORE_HOST=""
ARCHIVE_DIR=""
SUB_DIRS=""
PLATFORM=""
TEMP_DIR=""
THIS_OS=""
SAVE_FILE_NAME=items_under_test.txt

usage()
{
    echo "./make_validated_links.sh -a <ACTION> -u <STORE_USER> -n <STORE_HOST> -d <ARCHIVE_DIR> [-s <SUBDIR>] [-p <PLATFORM>] [-h]"
    echo "   -a <action> (one of 'save' or 'link')" 
    echo "   -u <name of user to connect as on storage host>" 
    echo "   -n <name of storage host>"
    echo "   -d <name of archive directory to check>"
    echo "   -s <subdir that was validated> (can give -s multiple times for multiple subdirectories"
    echo "      not required when action=link, but at least one required when action=save"
    echo "   -p <platform> (optional; used to differentiate when there are multiple platforms testing the same binaries"
    echo "   -h show usage"
    echo ""
}

check_inputs()
{
    while getopts a:d:n:s:u:p:h OPT; do
        case "$OPT" in
            a) # action
               ACTION="$OPTARG"
               ;;
            d) # archive directory
               ARCHIVE_DIR="$OPTARG"
               SAVE_FILE_DIR="$ARCHIVE_DIR/VALIDATED"
               SAVE_FILE="$SAVE_FILE_DIR/$SAVE_FILE_NAME"
               ;;
            h) # help
               usage
               exit 0
               ;;
            n) # storage host
               STORE_HOST="$OPTARG"
               ;;
            p) # platform
               PLATFORM="$OPTARG"
               ;;
            s) # subdir
               SUB_DIRS="$SUB_DIRS $OPTARG"
               ;;
            u) # storage user
               STORE_USER="$OPTARG"
               ;;
            *) echo ""
               echo "ERROR: Unknown flag \"$OPT\""
               usage
               exit 1
               ;;
        esac
    done

    # validate that everything required is given
    MISSING_OPTION=0

    if [ "x${ACTION}" = "x" ]; then
        echo "ERROR: -a <ACTION> not found"
        MISSING_OPTION=1
    fi
    if [ "x${STORE_USER}" = "x" ]; then
        echo "ERROR: -u <STORE_USER> not found"
        MISSING_OPTION=1
    fi
    if [ "x${STORE_HOST}" = "x" ]; then
        echo "ERROR: -n <STORE_HOST> not found"
        MISSING_OPTION=1
    fi
    if [ "x${ARCHIVE_DIR}" = "x" ]; then
        echo "ERROR: -d <ARCHIVE_DIR> not found"
        MISSING_OPTION=1
    fi
    if [ "x${SUB_DIRS}" = "x" -a "$ACTION" = "save" ]; then
        echo "ERROR: -s <SUBDIR> not found"
        MISSING_OPTION=1
    fi

    if [ $MISSING_OPTION -eq 1 ]; then
        echo ""
        usage
        exit 1
    fi

    return 0
}

set_os()
# set the THIS_OS variable; exit with error if it cannot be determined
{
    if [ -e /etc/issue ]; then
        FOUND=`grep "Enterprise Linux Server" /etc/issue`
        if [ "$FOUND" != "" ]; then
            THIS_OS="OEL"
            return 0
        fi
        FOUND=`grep "Ubuntu" /etc/issue`
        if [ "$FOUND" != "" ]; then
            THIS_OS="UBUNTU"
            return 0
        fi
        echo "ERROR: unable to determine OS"
        exit 1
    else
        FOUND=`uname -a | grep CYGWIN`
        if [ "$FOUND" != "" ]; then
            THIS_OS="CYGWIN"
            return 0
        fi
        FOUND=`uname -a | grep Darwin`
        if [ "$FOUND" != "" ]; then
            THIS_OS="IOS"
            return 0
        fi
        echo "ERROR: unable to determine OS"
        exit 1
    fi
}


#
# main
#

check_inputs $*

if [ "$ACTION" = "save" ]; then
    # based on the sub_dir list, find the items under test; save the names in a file

    # delete the file if it already exists
    CMD="ssh $STORE_USER@$STORE_HOST \"rm -f $SAVE_FILE\""
    echo $CMD
    eval $CMD

    # create the base directory if it doesn't exist
    CMD="ssh $STORE_USER@$STORE_HOST \"mkdir -p $SAVE_FILE_DIR\""
    echo $CMD
    eval $CMD
    
    # for each subdir given, save the name of the one that is symlinked to, which should be the latest
    for DIR in $SUB_DIRS; do
        CMD="ssh $STORE_USER@$STORE_HOST \"cd $ARCHIVE_DIR/$DIR && find . -type l | xargs ls -l\""
        echo $CMD
        RESULT=`eval $CMD`
        FILE=`echo $RESULT | awk '{ print $NF }'`
        CMD="ssh $STORE_USER@$STORE_HOST \"cd $ARCHIVE_DIR/$DIR && echo $DIR $FILE >> $SAVE_FILE\""
        echo $CMD
        eval $CMD
    done

    exit 0
elif [ "$ACTION" = "link" ]; then
    # retrieve the file that contains the names of the items that were tested
    # create a temp directory in which to work
    TEMP_DIR=`mktemp -d /tmp/tmp.XXXXX`
    CMD="scp $STORE_USER@$STORE_HOST:$SAVE_FILE $TEMP_DIR"
    echo $CMD
    eval $CMD

    DATESTAMP=""
    VALIDATED_DIR=""

    # make a symlink back to each item
    cat $TEMP_DIR/$SAVE_FILE_NAME | while read DIR FILE; do
        if [ "$DATESTAMP" = "" ]; then
            # this is the first time thru the while loop; do initial stuff
            # NOTE: need to close the STDIN fd when doing SSH; for some reason 
            #       it will mess up the while loop's STDIN so that only one
            #       line of the input file gets read

            # get the timestamp from the first entry
            DATESTAMP=`echo $FILE | sed 's/^.*\.\([0-9]*-[0-9]*\).*$/\1/g'`
            VALIDATED_DIR="$ARCHIVE_DIR/VALIDATED/$DATESTAMP"
            LINK_TARGET=$DATESTAMP
            if [ "$PLATFORM" != "" ]; then
                VALIDATED_DIR="$VALIDATED_DIR.$PLATFORM"
                LINK_TARGET="$LINK_TARGET.$PLATFORM"
            fi

            echo $LINK_TARGET > $TEMP_DIR/link_target

            # create the directory that holds all the links to the validated items
            # if it already exists; delete it
            CMD="ssh $STORE_USER@$STORE_HOST \"rm -rf $VALIDATED_DIR\" < /dev/null"
            echo $CMD
            eval $CMD
            CMD="ssh $STORE_USER@$STORE_HOST \"mkdir -p $VALIDATED_DIR\" < /dev/null"
            echo $CMD
            eval $CMD
        fi

        # create a link back to each item
        CMD="ssh $STORE_USER@$STORE_HOST \"cd $VALIDATED_DIR && ln -s ../../$DIR/$FILE\" < /dev/null"
        echo $CMD
        eval $CMD
    done

    LINK_TARGET=`cat $TEMP_DIR/link_target`
    LINK_NAME="latest"
    if [ "$PLATFORM" != "" ]; then
        LINK_NAME="$LINK_NAME.$PLATFORM"
    fi

    # create the link 'latest' that points to the most-recent passed files
    # for those with multiple test platforms, there's a latest.<platform> link
    # as well as a standard latest link that points to the "best" one
    CMD="ssh $STORE_USER@$STORE_HOST \"rm -f $ARCHIVE_DIR/VALIDATED/$LINK_NAME\" < /dev/null"
    echo $CMD
    eval $CMD
    CMD="ssh $STORE_USER@$STORE_HOST \"cd $ARCHIVE_DIR/VALIDATED && ln -s $LINK_TARGET $LINK_NAME\" < /dev/null"
    echo $CMD
    eval $CMD
    # special handling for the cloudnode and win_desk platforms
    if [ "$PLATFORM" = "win7" ]; then
        # latest is for win7
        CMD="ssh $STORE_USER@$STORE_HOST \"rm -f $ARCHIVE_DIR/VALIDATED/latest\" < /dev/null"
        echo $CMD
        eval $CMD
        CMD="ssh $STORE_USER@$STORE_HOST \"cd $ARCHIVE_DIR/VALIDATED && ln -s $LINK_TARGET latest\" < /dev/null"
        echo $CMD
        eval $CMD
    elif [ "$PLATFORM" = "orbe" ]; then
        CMD="ssh $STORE_USER@$STORE_HOST \"rm -f $ARCHIVE_DIR/VALIDATED/latest\" < /dev/null"
        echo $CMD
        eval $CMD
        CMD="ssh $STORE_USER@$STORE_HOST \"cd $ARCHIVE_DIR/VALIDATED && ln -s $LINK_TARGET latest\" < /dev/null"
        echo $CMD
        eval $CMD
    fi

    rm -rf $TEMP_DIR

    exit 0
else
    # invalid action
    echo "ERROR: unsupported action (-a) $ACTION"
    usage
    exit 1
fi

