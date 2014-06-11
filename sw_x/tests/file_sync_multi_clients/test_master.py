#!/usr/bin/python -u
import optparse
import os
import shutil
import signal
import socket
import string
import subprocess
import sys
import tempfile
import time
import struct
import filecmp
import subprocess
import glob
import conf
import testUtils


username = "ccdTest"
password = "password"
target1 = "frank-win1"
target2 = "frank-win2"
syncroot = "/cygdrive/c/cache"

normalDatasets = ['My Cloud']
camDatasetUp = 'CR Upload'
camDatasetDn = 'CameraRoll'
manifestDir = '.sync_temp'

numfilesperfolder = 200
bigfilesize = 4 * 1024 * 1024 * 1024

goldenDataHost = "pcstore.ctbg.acer.com"
goldenDataPath = "pc/test_data/pc_tests"
httpPort = "8001"

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--target1", "", action="store", dest="target1",
                 help="Test target", metavar="TARGET1")
    p.add_option("--target2", "", action="store", dest="target2",
                 help="Test target", metavar="TARGET2")
    p.add_option("--targetlogin", "", action="store", dest="targetlogin",
                 help="Test target login name", metavar="TARGETLOGIN")
    p.add_option("--syncroot", "", action="store", dest="syncroot",
                 help="Test target sync root path", metavar="SYNCROOT")
    p.add_option("--os", "", action="store", dest="targetos",
                 help="OS type of target machine", metavar="TARGETOS")
    p.add_option("--username", "", action="store", dest="username",
                 help="Personal cloud user name", metavar="USERNAME")
    p.add_option("--password", "", action="store", dest="password",
                 help="Personal cloud password", metavar="PASSWORD")
    p.add_option("--logdir", "", action="store", dest="logdir",
                 help="Master log directory of the test", metavar="LOGDIR")
    p.add_option("--testcase", "", action="store", dest="testcase",
                 help="Test case name", metavar="TESTCASE")
    return p

################# White Box Tests ##########################

def testSimulSync():
    print "---- testSimulSync ----"
    testrootpath = conf.testrootpath[conf.targetos]
    target1_files = 'target1_files'
    target2_files = 'target2_files'
    all_files = 'all_files'

    for dataset in normalDatasets:
        testUtils.subscribe(target1, dataset)
        testUtils.subscribe(target2, dataset)

    print "Creating golden test files"
    url = '%s:%s/%s/%s.tar.gz' % (goldenDataHost, httpPort, goldenDataPath, target1_files)
    testUtils.remoteRun(target1, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))
    testUtils.remoteRun(target2, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))
    url = '%s:%s/%s/%s.tar.gz' % (goldenDataHost, httpPort, goldenDataPath, target2_files)
    url = '%s:%s/%s/%s.tar.gz' % (goldenDataHost, httpPort, goldenDataPath, target2_files)
    testUtils.remoteRun(target1, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))
    testUtils.remoteRun(target2, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))

    testUtils.remoteRun(target1, 'cd %s; mkdir -p %s; cp %s/* %s; cp %s/* %s' % (testrootpath, all_files, target1_files, all_files, target2_files, all_files));
    testUtils.remoteRun(target2, 'cd %s; mkdir -p %s; cp %s/* %s; cp %s/* %s' % (testrootpath, all_files, target1_files, all_files, target2_files, all_files));

    print "Copying data into dataset"
    for dataset in normalDatasets:
        # Bug 9181 means simply copying the files into the datast directory that one could end up with partial files since the immediate file update may not advance the timestamp.
        testUtils.remoteRun(target1, 'cd %s; rm -Rf target_temp; mkdir target_temp; cp -Rf %s/* target_temp; mv target_temp/* %s/\"%s\"' % (testrootpath, target1_files, conf.cloudRoot(True), dataset))
        testUtils.remoteRun(target2, 'cd %s; rm -Rf target_temp; mkdir target_temp; cp -Rf %s/* target_temp; mv target_temp/* %s/\"%s\"' % (testrootpath, target2_files, conf.cloudRoot(True), dataset))

    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target1, dataset)
    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target2, dataset)
    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target1, dataset)

    print "Checking synced files"
    for dataset in normalDatasets:
        # Check target1
        testdiff1 = conf.logdir + '/' + 'target1.diff'
        testUtils.remoteRun(target1, 'rm -Rf %s/\"%s\"/%s' % (conf.cloudRoot(True), dataset, manifestDir))
        testUtils.remoteRunAndLog(target1, 'cd %s; diff -r %s/\"%s\" %s --exclude=\".sync_temp\"' % (testrootpath, conf.cloudRoot(True), dataset, all_files), testdiff1)
        try:
            st = os.stat('%s' % testdiff1)
        except (IOError, RuntimeError, OSError):
            print "Error: checking dataset diff %s" % testdiff1
            return False
        else:
            if st.st_size > 0:
                print "Error: %s dataset \"%s\" content mismatch" % (target1, dataset)
                return False

        # Check target2
        testdiff2 = conf.logdir + '/' + 'target2.diff' 
        testUtils.remoteRun(target2, 'rm -Rf %s/\"%s\"/%s' % (conf.cloudRoot(True), dataset, manifestDir))
        testUtils.remoteRunAndLog(target2, 'cd %s; diff -r %s/\"%s\" %s --exclude=\".sync_temp\"' % (testrootpath, conf.cloudRoot(True), dataset, all_files), testdiff2)
        try:
            st = os.stat('%s' % testdiff2)
        except (IOError, RuntimeError, OSError):
            print "Error: %s checking dataset diff %s" % (target2, testdiff2)
            return False
        else:
            if st.st_size > 0:
                print "Error: %s dataset \"%s\" content mismatch" % (target2, dataset)
                return False

    return True

def testCameraSync():
    print "---- testCameraSync ----"
    cam_files = 'cam_files'
    testrootpath = conf.testrootpath[conf.targetos]
    testUtils.subscribe(target1, camDatasetUp)
    testUtils.subscribe(target2, camDatasetDn)

    print 'target1 [%s] adding photos' % target1
    print "Creating golden test files"
    url = '%s:%s/%s/%s.tar.gz' % (goldenDataHost, httpPort, goldenDataPath, cam_files)
    testUtils.remoteRun(target1, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))
    testUtils.remoteRun(target2, 'cd %s; wget --progress=dot:mega -O - %s | tar xzvf -' % (testrootpath, url))
    # Bug 9181 means simply copying the files into the datast directory that one could end up with partial files since the immediate file update may not advance the timestamp.
    testUtils.remoteRun(target1, 'cd %s; cp -Rf %s/* %s/\"%s\"' % (testrootpath, cam_files, conf.cloudRoot(True), camDatasetUp))

    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target1, camDatasetUp)
    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target2, camDatasetDn)
    time.sleep(3)
    testUtils.ccdWaitUntilInSync(target1, camDatasetUp)

    print 'target2 [%s] verifying photos' % target2
    testdiff = conf.logdir + '/' + 'target2_cam.diff'
    testUtils.remoteRun(target2, 'rm -Rf %s/\"%s\"/%s' % (conf.cloudRoot(True), camDatasetDn, manifestDir))
    testUtils.remoteRunAndLog(target2, 'cd %s; diff -r %s/\"%s\" %s --exclude=\".sync_temp\"' % (testrootpath, conf.cloudRoot(True), camDatasetDn, cam_files), testdiff)
    try:
        st = os.stat('%s' % testdiff)
    except (IOError, RuntimeError, OSError):
        print "Error: checking cam dataset diff %s" % testdiff
        return False
    else:
        if st.st_size > 0:
            print "Error: %s  \"%s\" content mismatch" % (target2, camDatasetDn)
            return False

    return True

def main():
    global username
    global password 
    global target1
    global target2
    global syncroot

    p = setupOptParser()
    opts, args = p.parse_args()

    target1 = opts.target1
    target2 = opts.target2
    username = opts.username
    password = opts.password
    syncroot = opts.syncroot
    conf.logdir = opts.logdir
    conf.targetlogin = opts.targetlogin
    conf.targetos = opts.targetos
    conf.testrootpath['win32'] = '/cygdrive/c/Users/%s/igware/testroot' % (conf.targetlogin)
    conf.ccdrootpath['win32'] = '/cygdrive/c/Users/%s/igware/ccd/vs' % (conf.targetlogin)
    conf.testroot = '/Users/%s/igware/testroot' % (conf.targetlogin)

    print "kill running ccd processes"
    testUtils.ccdStop(target1)
    testUtils.ccdStop(target2)

    print "Clearing sync folders"
    for dataset in normalDatasets:
        testUtils.remoteRun(target1, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), dataset))
        testUtils.remoteRun(target2, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), dataset))
    testUtils.remoteRun(target1, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), camDatasetDn))
    testUtils.remoteRun(target2, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), camDatasetDn))
    testUtils.remoteRun(target1, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), camDatasetUp))
    testUtils.remoteRun(target2, 'rm -Rf %s/\"%s\"/*' % (conf.cloudRoot(True), camDatasetUp))

    print "Starting ccd"
    testUtils.ccdStart(target1)
    testUtils.ccdStart(target2)

    testUtils.ccdLogin(target2, username, password)
    testUtils.ccdLogin(target1, username, password)

    testUtils.linkDevice(target1)
    time.sleep(3) # Sleep necessary due to Bug# 11187.  Once Bug# 11187 is fixed, remove this sleep.
    testUtils.linkDevice(target2)

    ################### Test cases begin ##########################

    if (opts.testcase == 'simul_sync') :
        success = testSimulSync()
        if success == False:
            print "TC_RESULT=FAIL ;;; TC_NAME=testSimulSync" 
        else:
            print "TC_RESULT=PASS ;;; TC_NAME=testSimulSync"

    if (opts.testcase == 'cam_sync') :
        success = testCameraSync()
        if success == False:
            print "TC_RESULT=FAIL ;;; TC_NAME=testCameraSync" 
        else:
            print "TC_RESULT=PASS ;;; TC_NAME=testCameraSync"

    ################### Test cases end ###########################

    testUtils.unlinkDevice(target1)
    testUtils.unlinkDevice(target2)

    time.sleep(7)
    # I believe unlinking device implies logout
    #    testUtils.ccdLogout(target1)
    #    testUtils.ccdLogout(target2)

    print "kill running ccd processes"
    testUtils.ccdStop(target1)
    testUtils.ccdStop(target2)

if __name__ == '__main__':
    main()
