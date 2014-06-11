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

def remoteRun(target, cmd):
    cmdLine = '/usr/bin/ssh %s@%s \'%s\'' % (conf.targetlogin, target, cmd)  
    print 'Executing: %s' % cmdLine
    os.system(cmdLine)
    return 0

def remoteRunAndLog(target, cmd, logfile):
    cmdLine = '/usr/bin/ssh %s@%s \'%s\' > %s 2>&1' % (conf.targetlogin, target, cmd, logfile)  
    print 'Executing: %s' % cmdLine
    os.system(cmdLine)
    return 0

def remoteRunBackground(target, cmd):
    cmdLine = '/usr/bin/ssh %s@%s \'%s\' &' % (conf.targetlogin, target, cmd)  
    os.system(cmdLine)
    return 0

def ccdStart(target):
    print 'ccdStart %s' % target
    testrootpath = conf.testrootpath[conf.targetos]
    if conf.targetos == 'win32':
        # Use cygstart to spawn a native win32 process to execute the ccd daemon and redirect output to log file
        cmdLine = 'cd %s; cygstart /cygdrive/c/Python27/python.exe ./createProcess.py --cmd \\\"ccd.exe C:/Users/%s/igware\\\" --log ccd.log; sleep 2' % (testrootpath, conf.targetlogin)
        remoteRun(target, cmdLine)
    else:
        cmdLine = 'cd %s; python ./createProcess.py --cmd ./ccd --log ccd.log' % testrootpath
        remoteRunBackground(target, cmdLine)
    return 0

def ccdStop(target):
    print 'ccdStop %s' % target
    if conf.targetos == 'win32':
        remoteRun(target, 'taskkill /F /IM ccd.exe')
    else:
        remoteRun(target, 'killall ccd')
    return 0

def ccdSync(target):
    print "ccdSync %s" % target
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    remoteRun(target, 'cd %s; ./ccdSync%s %s' % (testrootpath, ext, target))
    return 0

def ccdLogin(target, username, password):
    print "ccdLogin %s (pcloud user: %s)" % (target, username)
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./ccdLogin%s %s %s %s %s \"%s\"' % (testrootpath, ext, target, username, password, conf.testroot, conf.cloudRoot(False)) 
    remoteRun(target, cmdLine)
    return 0

def ccdLogout(target):
    print "ccdLogout %s" % target
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./ccdLogout%s %s %s' % (testrootpath, ext, target, conf.testroot) 
    remoteRun(target, cmdLine)
    return 0

def ccdWaitUntilInSync(target, datasetName):
    print "ccdWaitUntilInSync %s for dataset %s" % (target, datasetName)
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./ccdWaitUntilInSync%s \"%s\"' % (testrootpath, ext, datasetName)
    remoteRun(target, cmdLine)
    return 0

def linkDevice(target):
    print "linkDevice %s" % target
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./linkDevice%s %s %s' % (testrootpath, ext, target, conf.testroot) 
    remoteRun(target, cmdLine)
    return 0

def unlinkDevice(target):
    print "unlinkDevice %s" % target
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./unlinkDevice%s %s %s' % (testrootpath, ext, target, conf.testroot) 
    remoteRun(target, cmdLine)
    return 0

def subscribe(target, dataset):
    print "%s subscribe %s" % (target, dataset)
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./subscribe%s %s %s \"%s\"' % (testrootpath, ext, target, conf.testroot, dataset) 
    remoteRun(target, cmdLine)
    return 0

def subscribeMobileCam(target, dataset):
    print "%s subscribeMobileCam %s" % (target, dataset)
    testrootpath = conf.testrootpath[conf.targetos]
    ext = conf.binext[conf.targetos]
    cmdLine = 'cd %s; ./subscribeMobileCam%s %s %s \"%s\"' % (testrootpath, ext, target, conf.testroot, dataset) 
    remoteRun(target, cmdLine)
    return 0

def createDir(target, folderpath):
    cmdLine = 'mkdir -p %s' % folderpath
    remoteRun(target, cmdLine);
    return 0

# Create a file under testroot
def createFile(target, filepath, size, value):
    testroot = conf.testrootpath[conf.targetos]
    cmdLine = 'cd %s; python ./fileGen.py --filename %s --size %d --value %d; ' % (testroot, filepath, size, value)   
    remoteRun(target, cmdLine);
    return 0

# Clone a file under testroot to many files in sync folder
def cloneFile(target, filepath, dstdir, num):
    testroot = conf.testrootpath[conf.targetos]
    cmdLine = 'cd %s; python ./fileClone.py --filename %s --dstdir %s --numfiles %d; ' % (testroot, filepath, dstdir, num)   
    remoteRun(target, cmdLine);
    return 0

# Compare a file under testroot to many files in sync folder, store diff in local testdiff file
def cmpFile(target, filepath, dstdir, num, testdiff):
    testroot = conf.testrootpath[conf.targetos]
    cmdLine = 'cd %s; python ./fileCmp.py --filename %s --dstdir %s --numfiles %d' % (testroot, filepath, dstdir, num)   
    remoteRunAndLog(target, cmdLine, testdiff);
    return 0

def removeFileFromSync(target, syncfolder, filename):
    syncfolderpath = conf.cloudRoot(True) + '/' + syncfolder
    cmdLine = 'rm -f %s/%s' % (syncfolderpath, filename)  
    remoteRun(target, cmdLine)
    return 0

def deleteManifestFiles(target):
    print 'deleteManifestFiles'
    cmdLine = 'rm -f %s/*.manifest' % (conf.ccdrootpath[conf.targetos])  
    remoteRun(target, cmdLine)
    return 0

def purgeSync(target, syncfolder):
    syncfolderpath = conf.cloudRoot(True) + '/' + syncfolder
    cmdLine = 'rm -Rf %s/*' % syncfolderpath
    remoteRun(target, cmdLine)
    return 0
