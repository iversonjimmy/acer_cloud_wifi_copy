#!/usr/bin/python -u

# Whitebox test to verify code path coverage. The tests are all conducted 
# through CCD from the perspective of sync agent. So there is no direct
# access to sync server data
#
# There are tests here for swupdate.
#
# Note: Following tests may need to be revised if algorithm is changed in the future
#
# CASE A: 
#   Description:  Server & local copies both modified  
#   Action:       Rename local file
#                 Local version updated with server version 
#
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
import re
import traceback

dsetName = "Unknown"
cloudRoot = "/"
c1DeviceRoot = ""
c2DeviceRoot = ""
logDir = ""
c1Root = ""
c2Root = ""
ccdLog = 0
syncTemp = ".sync_temp"

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
#    p.add_option("--binDir", "", action="store", dest="binDir",
#                 help="Specify the path to test binaries", metavar="BINDIR")
    p.add_option("--cloudRoot", "", action="store", dest="cloudRoot",
                 help="Specify the path for local cloud", metavar="CLOUDROOT")
    p.add_option("--c1Root", "", action="store", dest="c1Root",
                 help="Specify the root path of client1", metavar="C1ROOT")
    p.add_option("--c2Root", "", action="store", dest="c2Root",
                 help="Specify the root path of client2", metavar="C2ROOT")
    p.add_option("--testLog", "", action="store", dest="testLog",
                 help="Specify the log directory", metavar="TESTLOG")
    p.add_option("--case", "", action="store", dest="testCase",
                 help="Specify test case label", metavar="TESTCASE")
    p.add_option("--account", "", action="store", dest="account",
                 help="Specify test account name", metavar="ACCOUNT")
    p.add_option("--testName", "", action="store", dest="testName",
                 help="Specify test name", metavar="MY_TEST_NAME")
    return p

def stopCcd():
    logFile = logDir + '/8_ccd_shutdown'
    log = open(logFile, 'a+')
    print "stop ccd" 
    runCmd = [ccdDir + '/tests/ccd_shutdown_test']
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    p.wait()
    log.close()
    return 0

def login(root, machine, account):
    global cloudRoot
    print "%s login in prog..." % machine 
    logFile = logDir + "/4_login_%s.log" % machine
    log = open(logFile, 'a+')
    runCmd = [binDir + '/ccdLogin', machine, account, 'password', cloudRoot]
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    rv = p.wait()
    log.close()
    if rv != 0:
        raise ValueError("login failed")
    time.sleep(1)
    return rv

def logout(root, machine):
    print "%s logout in prog..." % machine 
    logFile = logDir + "/6_logout_%s.log" % machine
    log = open(logFile, 'a+')
    runCmd = [binDir + '/ccdLogout', machine]
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    p.wait()
    log.close()
    return 0

def runSwuTest(root, machine):
    global c1DeviceRoot

    print "%s SwuTest in prog..." % (machine) 
    logFile = logDir + "/swu_%s_log.txt" % machine 
    log = open(logFile, 'a+')
    runCmd = [binDir + '/swuTest','-r', root]
    print "cmd: %s" % runCmd
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    p.wait()
    log.close()
    time.sleep(1)
    rv = False;
    for line in open(logFile) :
        if "PASSED" in line :
            rv = True;
            break;
    return rv 

# Function: testCaseA 
# 
# Descriptions:
#    swuTest - 
#
# Test procedure:
#     Run: 
#       (ccd already running)
#       Run swu_test
#     Check: 
#       verify output results
def testCaseSw(case = 'A', login = True):
    print "-- Software Update Case %s logged in %s" % (case, login)
    rv = runSwuTest(c1Root, 'C1')
    return rv

test_cases = {
    'swupdate' : {
        'A': { 'name' : "testcaseA", 'func' : testCaseSw,
            'args' : {'case' : 'A', 'login' : True}},
        'B': { 'name' : "testcaseB", 'func' : testCaseSw,
            'args' : {'case' : 'B', 'login' : False},
            'nologin' : True},
        }
}

def main():
    p = setupOptParser()
    opts, args = p.parse_args()

    global logDir 
    global c1Root
    global c2Root
    global cloudRoot
    global dsetName
    global c1DeviceRoot
    global c2DeviceRoot
    global binDir
    global ccdDir
    global actoolDir

    logDir = opts.testLog

    cloudRoot = opts.cloudRoot
    c1Root = opts.c1Root
    c2Root = opts.c2Root
    binDir = os.getenv("BUILDROOT") + '/release/linux/tests/swupdate/runSync/' 
    ccdDir = os.getenv("BUILDROOT") + '/release/linux/gvm_core/daemons/ccd/'
    actoolDir = os.getenv("WORKAREA") + '/sw_x/tests/tools/'

    try:
        try:
            test_case = test_cases[opts.testName][opts.testCase]
        except KeyError:
            print "TC_RESULT=FAIL;;; TC_NAME=UnknownCase%s" % case
            return
        caseName = test_case['name']

        if 'nologin' in test_case:
            nologin = test_case['nologin']
        else:
            nologin = False

        if not nologin:
            login(c1Root, 'C1', opts.account)

            dsetInfoFile = '/tmp/dsetInfo'
            f = open(dsetInfoFile, 'r')
            dsetName = f.read()
            f.close()
            print "dataset %s" % dsetName

        c1DeviceRoot = cloudRoot

        success = test_case['func'](**test_case['args'])

    except Exception as exc:
        print "Test encountered exception ",exc
        traceback.print_exc()
        success = False

    tc_name = "ccdi_%s_%s" % (opts.testName, caseName)
    tc_result = "PASS" if success != False else "FAIL"
    print "TC_RESULT=%s ;;; TC_NAME=%s" % (tc_result, tc_name)

    if not nologin:
        logout(c1Root, 'C1')
    stopCcd()

    os.system("killall ccd")

if __name__ == '__main__':
    main()
