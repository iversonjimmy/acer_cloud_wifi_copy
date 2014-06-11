#!/usr/bin/python -u
# coding=UTF-8


# Upload and download test files to personal cloud
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

logDir = ""
c1Root = ""
c2Root = ""
c1Cache = ""
c2Cache = ""
syncFolder = "/docs"
userid = 0
opsLoginName = 'syncWbTester'
opsLoginPassword = 'password'
userid = 0      # tester's linux user id

uppercase = "CASETEST"
lowercase = "casetest"
withreserved = "reserved.!?\""
withspace = "with space"
chinese = "iGware 中文檔案名測試 no.1"

testfiles = [uppercase, lowercase, withreserved, withspace, chinese ]

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--testRoot", "", action="store", dest="testRoot",
                 help="Specify the root path for test", metavar="TESTROOT")
    p.add_option("--c1Root", "", action="store", dest="c1Root",
                 help="Specify the root path of client1", metavar="C1ROOT")
    p.add_option("--c2Root", "", action="store", dest="c2Root",
                 help="Specify the root path of client2", metavar="C2ROOT")
    p.add_option("--testLog", "", action="store", dest="testLog",
                 help="Specify the log directory", metavar="TESTLOG")
    return p

def rm_rf_subdir(d):
    for path in (os.path.join(d,f) for f in os.listdir(d)):
        if os.path.isdir(path):
            shutil.rmtree(path)
        else:
            os.unlink(path)

def runC1Sync():
    print "C1 sync in prog..."
    logFile = logDir + "/runsync_c1_log.txt"
    log = open(logFile, 'w+')
    runCmd = ['sudo', 'chroot', c1Root, '/bin/runSync', 'C1', opsLoginName, opsLoginPassword, '%d' % userid]
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    p.wait()
    log.close()
    #os.system("sudo chmod -R 777 %s" % c1Cache) 
    return 0

def runC2Sync():
    print "C2 sync in prog..."
    logFile = logDir + "/runsync_c2_log.txt"
    log = open(logFile, 'w+')
    runCmd = ['sudo', 'chroot', c2Root, '/bin/runSync', 'C2', opsLoginName, opsLoginPassword, '%d' % userid]
    p = subprocess.Popen(runCmd, stdout=log, stderr=log) 
    p.wait()
    log.close()
    #os.system("sudo chmod -R 777 %s" % c2Cache) 
    return 0


# Function: cleanFiles
# 
# Description:
#     Clear all files and records on S, C1 and C2
#
# Procedure
#     Run:
#         1) C1 empty and C2 empty
#         2) C1 agent sync
#         3) Remove all files in sync folder
#         4) C1 agent sync
#         5) C2 agent sync
#     Check:
#         Success if C1 and C2 sync folders have no data
def cleanFiles():
    print "Cleaning files and records in %s..." % syncFolder
    runC1Sync()
    runC2Sync()
    print "Remove local contents in %s" % c1Cache
    rm_rf_subdir(c1Cache)
    print "Remove local contents in %s" % c2Cache
    rm_rf_subdir(c2Cache)
    runC1Sync()
    if os.listdir(c1Cache) != []:
        print("Sync fail to clear C1 %s record" % c1Cache)
        return False
    runC2Sync()
    if os.listdir(c2Cache) != []:
        print("Sync fail to clear C2 %s record" % c2Cache)
        return False
    return True

def main():
    global logDir 
    global c1Root
    global c2Root
    global c1Cache
    global c2Cache
    global userid

    p = setupOptParser()
    opts, args = p.parse_args()

    logDir = opts.testLog
    c1Root = opts.c1Root
    c2Root = opts.c2Root
    c1Cache = c1Root + "/cache" + syncFolder
    c2Cache = c2Root + "/cache" + syncFolder
    userid = os.getuid()
    print "userid %d" % userid

    success = cleanFiles()
    if (success == False):
        print "cleanFiles() fail"
        return False

    print "creating files on c1 %s" % syncFolder
    for f1 in testfiles:
        print "    %s" % f1
        f = open(os.path.join(c1Cache, f1), 'w')
        f.write(f1)
        f.close()

    runC1Sync()
    runC2Sync()

    success = True
    for f2 in testfiles:
        if (os.path.exists(os.path.join(c2Cache,f2)) == True):
            print "    %s synced" % f2
            if (filecmp.cmp(os.path.join(c2Cache,f2), os.path.join(c1Cache,f1)) == False): 
                print "    error - file mismatch" 
                success = False
        else:
            print "    %s missing" % f2
            success = False

    if (success == False):
        print "TC_RESULT=FAIL ;;; TC_NAME=syncFilenameTest"
    else:
        print "TC_RESULT=PASS ;;; TC_NAME=syncFilenameTest"

if __name__ == '__main__':
    main()
