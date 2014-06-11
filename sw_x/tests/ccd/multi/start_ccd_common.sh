# Exit on first error.
set -e
# Exit on use of unset variable
set -u
# echo commands before executing them.
set -x

#export LAB_DOMAIN=pc-test.igware.net
export LAB_DOMAIN=lab1.routefree.com

export THIS_CCD_DIR=~/temp/ccd${THIS_CCD_NUM}
export SRCROOT=/home/steveo/winWorkspaces/workspace1dev/client_sw_x/sw_x
export CCD_EXE=/home/steveo/out/SWX1/debug/linux/gvm_core/daemons/ccd/ccd

${SRCROOT}/tests/tools/actool.sh ${LAB_DOMAIN} ${THIS_CCD_DIR}
echo >> ${THIS_CCD_DIR}/conf/ccd.conf
echo "testInstanceNum = ${THIS_CCD_NUM}" >> ${THIS_CCD_DIR}/conf/ccd.conf
ulimit -c unlimited
valgrind --log-file=valgrind${THIS_CCD_NUM}.log --suppressions=${SRCROOT}/tests/ccd/valgrind.supp --gen-suppressions=all --leak-check=full --track-origins=yes ${CCD_EXE} ${THIS_CCD_DIR} 2>&1 | tee ccd${THIS_CCD_NUM}.log
