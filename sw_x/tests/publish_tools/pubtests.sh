#!/bin/bash

usage()
{
	echo "Usage: pubtests [-p <pubtools_home>] [-d <dxtools_home] [-u <ccd_user>] [-d <ccdRoot>] [-S] [-x]"  1>&2
    echo "        -x -- extended tests"
	exit 1
}

PTDIR=$PWD
dxtools_home=$PWD
ccdRoot=$PWD
CCD_TEST_ACCOUNT=${CCD_TEST_ACCOUNT:-whocares@nowhere.com}
ccdBin=$PWD/
LABINFRA=pc-int.igware.net
logdir=$PWD
use_dxtool=0
extended=0

default_titleid=00000006FF00000B
default_guid=ts-dev-test-01

default_large_titleid=00000006FF00000C
default_large_guid=ts-dev-test-02


while [ "x$1" != x ] ; do
	case "$1" in 
		-p)
			PTDIR="$2"
			shift ; shift
			;;
		-d)
			ccdBin="$2"
			shift ; shift
			;;
		-f)
			ccdRoot="$2"
			shift ; shift
			;;
        -g)
            default_guid="$2"
            shift; shift
            ;;
        -G)
            default_large_guid="$2"
            shift; shift
            ;;
        -l)
            LABINFRA="$2"
            shift; shift
            ;;
        -S)
            use_dxtool=0
            shift
            ;;
        -t)
            default_titleid="$2"
            shift; shift
            ;;
        -T)
            default_large_titleid="$2"
            shift; shift
            ;;
        -x)
            extended=$(expr $extended + 1)
            shift; 
            ;;
        -u)
            CCD_TEST_ACCOUNT="$2"
            shift; shift;
            ;;
		*)
			usage
			shift
			;;
	esac
done

if [ ${OSTYPE:-unknown} = "cygwin" ] ; then
	SEPARATOR=";"
	BINSUF=cygwin
    if [ $use_dxtool = 1 ] ; then
        swu_get=swu_get_dxtool
    	swu_init=swu_init_dxtool
    else
    	swu_get=swu_get_dxshell
    	swu_init=swu_init_dxshell
    fi
else
	swu_get=swu_get_linux
	swu_init=swu_init_linux
fi


logfile=pt.log
log()
{
	local ts=$(date)
	if [ x"$1" = x-s ] ; then
		shift
	else
		echo `date` "$@" 
	fi
	echo `date` "$@" >> $logfile
}

cleanup()
{
    mkdir -p $logdir/ccd
    cp $ccdlogdir/* $logdir/ccd
}

get_detail()
{
    local dfile=$1
    local vset=$2       # Online or QA
    local attr=$3
    awk 'BEGIN {ourvalue = 0 }
        /===.*Value ===/{ 
            if ( $2 == "'$vset'") { 
                    ourvalue=1
            } else { 
                    ourvalue=0
            }
        }
        ourvalue == 1 && $1 == "'$attr'" {
            print substr($0, index($0, ": ") + 2)
        } ' < $dfile

}

tcfail()
{
	echo "TC_RESULT=FAIL ;;;TC_NAME=$1 # $2"
}

tcexpfail()
{
	echo "TC_RESULT=EXPECTED_TO_FAIL ;;;TC_NAME=$1 # $2"
}

tcpass()
{
	echo "TC_RESULT=PASS ;;;TC_NAME=$1 # $2"
}


nativepath()
{
    if [ ${OSTYPE:-unknown} = cygwin ] ; then
        cygpath "$@"
    else
        shift
        echo "$@"
    fi
}

swu_init_dxshell()
{
    local dxs=$dxtools_home/dxshell.exe
    local setlog=$logdir/setdomain.log
    $dxs SetDomain $LABINFRA > $setlog
    if grep -s TC_RESULT=PASS < $setlog ; then
        log "Set domain to $LABINFRA"
        local querylog=$logdir/queryappdatapath.log
        $dxs Target GetCCDAppDataPath > $querylog
        nccdappdatadir=$(sed -n -e '/^CCD App Data path: \(.*\)$/s//\1/p' < $querylog )
        ccdappdatadir=$(cygpath -u "$nccdappdatadir")
        ccdconfdir=$ccdappdatadir/conf
        ccdlogdir=$ccdappdatadir/logs/ccd
        log "Clearing CCD log directory $ccdlogdir"
        rm -rf $ccdlogdir/*
    else
        log "ERRSetDomain failed"
        cat $logdir/setdomain.log
        return 1
    fi
}

swu_init_dxtool()
{
    ccdconfdir="$ccdRoot/conf"
    ccdlogdir=$ccdRoot/logs/ccd
    log "ccdconfdir $ccdconfdir ccdlogdir $ccdlogdir"
}

swu_init_linux()
{
    true
}

swu_get_dxshell()
{
	local guid=$1
	local resultfile=$2
	local rc
    local dxs=$dxtools_home/dxshell.exe
    local ccd_started=0
	log "IN SWU_GET_DXSHELL"
	if [ x"$guid" = x ] ; then
		log Bad setup
		exit 1
	fi
    
    # First task is to get ccd started
    startlog=$logdir/dxstart.$$
    $dxs StartCCD > $startlog
    rc=$?
    if grep -s TC_RESULT=PASS  $startlog ; then
        ccd_started=1
        log CCD started ok
    else
        log  CCD failed to start
        return 1
    fi
    fetchlog=$logdir/dxout.$$

    $dxs DownloadUpdates -g $guid -s >> $fetchlog 2>&1
	grep TC_RESULT=PASS $fetchlog > /dev/null
	rc=$?
	if [ $rc != 0 ] ; then
		log swu_get failed -- logfile $fetchlog
	else
        echo $(nativepath -u $(awk '/GUID.*current version/ { version=$(NF-3)}
            /Downloaded/ {file=$NF} END {print file, version}' < $fetchlog))  > $resultfile
		rm -f $fetchlog
	fi

    if [ $ccd_started = 1 ] ; then
        $dxs StopCCD >> $startlog
    fi
	return $rc

}

# Usage: swu_get_dxtool <guid> <resultfile>
# Returns path to the file retrieved if any in <resultfile>
#
swu_get_dxtool()
{
	local guid=$1
	local resultfile=$2
    local user_account=${3:-CCD_TEST_ACCOUNT}
	local rc
	log "IN SWU_GET_DXTOOL"
	if [ x"$guid" = x ] ; then
		log Bad setup
		exit 1
	fi
	dxout=$logdir/dxout.$$

	log -s retrieving $guid
	rm -f swupdate/${guid}_*.zip
	$dxtools_home/dx_win.exe -f . -u $user_account -p password -t 6 -o 1 -s -d $(nativepath -w  $ccdBin) -a $(nativepath -w $ccdRoot) -g $guid > $dxout 2>&1
	grep TC_RESULT=PASS $dxout > /dev/null
	rc=$?
	if [ $rc != 0 ] ; then
		log swu_get failed -- logfile $dxout
	else
        echo $(nativepath -u $(awk '/downloaded version/ {print $(NF-2), $(NF-4)}' < $dxout))  > $resultfile
		rm $dxout
	fi
	return $rc
}


# Usage: swu_get_wget <guid> <app_ver> <outfile>
# wget https://www.<lab>/ops/download/installationFile:<filename>_Ver<ver>.<ext>?downloadGUID=<guid>

swu_get_wget()
{
	local guid=$1
	local appver=$2
	local outfile=$3
    local download="installationFile:ops_Ver$appver.zip?downloadGUID=${guid}";
	local rc
	log "IN SWU_GET_WGET"
	if [ x"$guid" = x ] ; then
		log Bad setup guid
		exit 1
	fi
	if [ x"$appver" = x ] ; then
		log Bad setup appver
		exit 1
	fi
	wgetout=$logdir/wget.$$

	log -s retrieving ${guid}.${appver}
    log infra $LABINFRA ncw $ncw
    wget $ncw -O $outfile https://www.${LABINFRA}/ops/download/${download}
	rc=$?
	return $rc
}

# Run through basic Software update API using dxshell
test_swupapi()
{
    local rc
    local dxs=$dxtools_home/dxshell.exe
    local dxout=$logdir/swapitmp.$$
    local dxtmp=$logdir/swapilog.$$
    local tcname

    
    for login_user in "$CCD_TEST_ACCOUNT" "" ; do
        log "SW API test for user $login_user -- logfile $dxout"
        if [ x$login_user = x ] ; then
            tcname=SWUpdateAPI_nologin
        else
            tcname=SWUpdateAPI_login
        fi

        rv=0
        # First task is to get ccd started
        $dxs StartCCD > $dxtmp
        cat $dxtmp >> $dxout
        if grep -s TC_RESULT=PASS  $dxtmp  > /dev/null ; then
            log CCD started ok
        else
            log  CCD failed to start
            rv=1
        fi


        # We may want to login...
        if [ $rv = 0 -a x"$login_user" != x ] ; then
            $dxs StartClient  $login_user password > $dxtmp
            rc=$?
            cat $dxtmp >> $dxout
            if grep TC_RESULT=PASS $dxtmp > /dev/null ; then
                log login succeeded -- dxshell rc $rc
            else
                log login failed -- dxshell rc $rc
                $dxs StopCCD >> $dxout
                rv=2
            fi
        fi

        if [ $rv = 0 ] ; then
            $dxs DownloadUpdates -T > $dxtmp 2>&1
            rc=$?
            cat $dxtmp >> $dxout
            grep TC_RESULT=PASS $dxtmp > /dev/null
            if [ $rc != 0 ]  ; then
                log "API test failed -- dxshell rc $rc"
            fi
        fi

        if [ $rv != 0 ] ; then
            tcfail $tcname
        else
            tcpass $tcname
        fi

        $dxs StopCCD >> $dxout
    done
}

# Go through publish release approve fetch loop
testpub()
{
	rcexp=${1:-0}
	rcrexp=${2:-0}
	rcaexp=${3:-0}
	rctexp=${4:-0}
    local foundver
    log "file $file cmpfile $cmpfile"
	if [ x$nopub = x ] ; then
		log $PTDIR/bin/onlinePublish publish $bws $titleid $tin "$file" $nc
		$PTDIR/bin/onlinePublish publish $bws $titleid $tin "$file" $nc
		rc=$?
		if [ $rc != $rcexp ]  ; then
			log ERR unexpected publish return value  $rc wanted $rcexp for $file
			fails=$(expr $fails + 1)
			return 1
		fi
		log publish pass $file
	fi
	if [ x$norel = x ] ; then
		log $PTDIR/bin/onlinePublish release $bws $titleid $tin -app-guid $appguid -app-version $appver -app-min-version $appminver -ccd-min-version $ccdminver -app-message "$appmess" $nc
		$PTDIR/bin/onlinePublish release $bws $titleid $tin -app-guid $appguid -app-version $appver -app-min-version $appminver -ccd-min-version $ccdminver -app-message "$appmess" $nc
		rc=$?
		if [ $rc != $rcrexp ] ; then
			log ERR release fail $rc not $rcrexp
			fails=$(expr $fails + 1)
			return 2
		fi
		log  release pass guid $appguid ver $appver
	fi

        $PTDIR/bin/onlinePublish detail $bws $titleid $tin $nc > $logdir/detail.pre
	rc=$?
	if [ $rc != 0 ] ; then 
		log ERR cannot get detail for $titleid before approve $logdir/detail.pre
		return 4
	fi
	preapprove=$(awk '/ApproveDate/{print  $NF}' < $logdir/detail.pre)
	postapprove=$(awk '/ApproveDate/{print  $NF}' < $logdir/detail.pre)

	if [ x$noapp = x ] ; then
		$PTDIR/bin/onlinePublish approve $bws $appuser $apppw $titleid $nc
		rc=$?
		if [ $rc != $rcaexp  ] ; then
			log ERR approve fail $rc not $rcaexp
			fails=$(expr $fails + 1)
			return 3
		fi
		log approve pass

		count=0
        maxcount=4
		while [ $count -lt $maxcount -a $preapprove = $postapprove ] ; do
			$PTDIR/bin/onlinePublish detail $bws $titleid $tin $nc > $logdir/detail.post
			sleep 10
			postapprove=$(awk '/ApproveDate/{print  $NF}' < $logdir/detail.post)
			count=$(expr $count + 1)
		done
		if [ $count -ge $maxcount ] ; then
			log ERR "Timed out getting approval $preapprove"
			return 4
		fi
		log Done Waiting for approval $count $preapprove $postapprove
        sleep 60
		
	fi
    $PTDIR/bin/onlinePublish detail $bws $titleid $tin $nc > $logdir/detail.post

	if [ x$noget = x ] ; then
		
        # pull down the file via OPS
        local ops_file=ops-${appguid}.zip
        swu_get_wget $appguid $appver $ops_file
		rc=$?
        swu_get_wget $appguid $appver $ops_file
		rc=$?
		if [ $rc != 0 ] ; then
			log ERR OPS fetch failed rc $oc
            tcfail "OPS download $appguid $appver"
        else
            tcpass "OPS download $appguid $appver"
			log OPS Fetch $appguid -- $ops_file return $rc
			log Cmpfile $cmpfile
			if [ -n "$ops_file" ] ; then
				cmp "$ops_file" "$cmpfile"
				rc=$?
			else
				log "ERR no ops file found"
				rc=7
			fi
            if [ $rc != 0 ] ; then
                tcfail "OPS compare $appguid $appver - $rc"
            else
                tcpass "OPS compare $appguid $appver - $rc"
            fi
        fi

		$swu_get $appguid outfile
		rc=$?
		if [ $rc != 0 ] ; then
			log ERR fetch failed rc $rc
		else
			destfile=$(awk '{print $1}' <  outfile)
			log Fetch $appguid -- $destfile return $rc
			log Cmpfile $cmpfile
			if [ -n "$destfile" ] ; then
				cmp "$destfile" "$cmpfile"
				rc=$?
                newfsize=$(ls -l $destfile | awk '{print $5}')
                origfsize=$(ls -l $cmpfile | awk '{print $5}')
                remainder=$(expr $origfsize % 16)
                if [ $remainder != 0 ] ; then
                    log "original file $cmpfile is $origfsize mod 16 remainder $remainder"
                    if [ $rc = 0 ] ; then
                        tcpass "PubToolDecryptPadding"
                    else
                        log "Failed match $destfile $cmpfile"
                        tcexpfail "PubToolDecryptPadding"
                        # tcfail "PubToolDecryptPadding"
                    fi
                fi
                if [ -n "$expectver" ] ; then
                    foundver=$(awk '{print $2}' < outfile)
                    if [ x"$foundver" = x"$expectver" ]  ;then
                        log "Matched version $foundver"
                    else
                        log ERR "Bad version '$foundver' instead of '$expectver'"
                        rc=4
                    fi
                else
                    foundver=$(awk '{print $2}' < outfile)
                    log "Found version $foundver"
                fi
                rm -f "$destfile"
			else
				log "ERR no file found"
				rc=1
			fi
		fi
		if [ $rc != $rctexp ] ; then
			log ERR fetch failed $rc not $rcaexp
            rc=5
		fi
	fi
    return $rc
}


$swu_init
rc=$?
if [ $rc != 0 ] ; then
    tcfail PubToolSetup "Setup failed"
    cleanup
    exit 1
fi

test_swupapi

lab=$(awk '/^infra/{print $NF}' < $ccdconfdir/ccd.conf)
if [ $lab = "pc-int.igware.net" ] ; then
    nc_default=-no-check-certificate
    ncw_default=--no-check-certificate
else
    nc_default=
    ncw_default=
fi

titleid=$default_titleid
testguid=$default_guid
tin=123
nc=$nc_default
ncw=$ncw_default

bws=acp.$lab
log Infra is $lab bws $bws

pt="$PTDIR/bin/onlinePublish"
$pt status $bws $titleid $tin $nc -days 2 > $logdir/initstatus.log
rc=$?
if [ "$rc" != 0 ] ; then
	tcfail PubToolSetup "Expected title $titleid to exist. Could not obtain title status rc $rc"
    cleanup
	exit 1
fi


$swu_get $testguid outdata
rc=$?
if [ $rc != 0 ] ; then
	tcfail PubToolSetup "Could not retrieve pre-existing update for GUID $testguid"
    cleanup
	exit 1
else
	d=$(cat outdata)
	version=${d%.zip}
	version=${version##*_}
	log "Retrieved version $version for $testguid " # $(md5sum $d)
fi

# Ready to start testing.
tcpass PubToolSetup

oldversion=$version

# Test modifying version
file=data.in

dd if=/dev/random of=$file bs=1024k count=8
# Make sure file 
echo 'xx' >> $file

appuser=pc-all@igware.com
apppw=Publ1shMe
appguid=$testguid
appver=1.2.$(date +%y%m%d%H%M)
appminver=1.2.3
ccdminver=0.0.2
appmess=testfromlooper
cmpfile=$file
log "About to call testpub"

expectver=$appver testpub
rc=$?
if [ $rc = 0 ] ; then 
	tcpass PubBasicLoop
else
    tcexpfail PubBasicLoop
    # tcfail PubBasicLoop
fi

# Test that publish does not impact existing live content
# -- publish without approving, fetch and fetch metadata
file2=data2.in
dd if=/dev/random of=$file2 bs=1024k count=8

oappminver=$appminver
oappver=$appver
occdminver=$ccdminver
oappguid=$appguid
cmpfile=$file
file=$file2
appminver=1.2.4
ccdminver=0.0.3
appver=1.2.$(date +%y%m%d%H%M)
appmess=testfromlooper
expectver=$oappver nopub= norel= noapp=yes noget= testpub
rc=$?
echo "rc $rc Details before publish and release"
if [ $rc != 0 ] ; then
    tcexpfail "PubToolPreApproveDetails" "basic publishing steps"
    #tcfail "PubToolPreApproveDetails" "basic publishing steps"
else
    avfails=0
    for av in AppVersion:$oappver AppMinVersion:$oappminver CCDMinVersion:$occdminver ; do
        a=${vp%%:*}
        v=${vp#*:}
        nv=$(get_detail $logdir/detail.post Online $a)
        if [ x$nv !=  x$v ] ; then
            log ERROR Bad value for $a found $nv instead of $v
            avfails=$(expr $avfiles + 1)
        fi
    done

    if  [ $avfails = 0 ] ; then
        tcpass "PubToolPreApproveDetails"
    else
        tcexpfail "PubToolPreApproveDetails" "attribute mismatch count $avfails"
        #tcfail "PubToolPreApproveDetails" "attribute mismatch count $avfails"
    fi

    apiver=$(awk '{print $2}' < outfile)
    if [ "$apiver" = $oappver ] ; then
        tcpass "PubToolPreApproveAPIattrs"
    else
        tcfail "PubToolPreApproveAPIattrs" "version retrieved $apiver instead of $oappver"
    fi
fi

log "approving pending changes"
$pt approve $bws $appuser $apppw $titleid $nc

if [ $extended -gt 0 ] ; then
    log "Beginning extended tests"
    file=data3.in
    cmpfile=$file
    titleid=$default_large_titleid
    appguid=$default_large_guid
    appver=1.2.$(date +%y%m%d%H%M)
    dd if=/dev/random of=$file bs=1024k count=160
    log "file $file cmpfile $cmpfile"
    testpub
    rc=$?
    if [ $rc = 0 ] ; then
        tcpass PubLargeLoop
    else
        tcfail PubLargeLoop
    fi

fi

cleanup
