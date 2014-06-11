#! /usr/bin/python

import sys
import os
import subprocess
import time

def doSystemCall(syscall):
    print 'START: "%s"' % (syscall)
    rc = os.system(syscall)
    print 'END: "%s" RESULT:%d' % (syscall, rc)
    return rc

if sys.argv[2] == 'Windows' or sys.argv[2] == 'Android':
    doSystemCall('cd %s' % sys.argv[1])
    if len(sys.argv) > 3:
        execFile = 'cygstart -w ./dx_remote_agent.exe \"%s\"' % sys.argv[3]
    else:
        execFile = 'cygstart -w ./dx_remote_agent.exe'

    subprocess.Popen(execFile, shell=True)

elif sys.argv[2] == 'Linux':
    doSystemCall('ulimit -c unlimited')
    doSystemCall('ulimit -c')
    doSystemCall('cd %s' % sys.argv[1])

    if len(sys.argv) > 3:
        execFile = './dx_remote_agent \"%s\"' % sys.argv[3]
    else:
        execFile = './dx_remote_agent'

    subprocess.Popen(execFile, shell=True)

elif sys.argv[2] == 'Orbe':
    doSystemCall('ulimit -c unlimited')
    doSystemCall('ulimit -c')
    doSystemCall('cd %s' % sys.argv[1])
    if len(sys.argv) > 4:
        execFile = 'su %s -c \"./dx_remote_agent %s\"' % (sys.argv[3], sys.argv[4])
    else:
        execFile = 'su %s -c ./dx_remote_agent' % sys.argv[3]

    subprocess.Popen(execFile, shell=True)

elif sys.argv[2] == 'WindowsRT':
    doSystemCall('cd %s' % sys.argv[1])
    if sys.argv[3] == 'UnInstallApp':
        doSystemCall('./metro_app_utilities.exe remove 43d863b7-f317-4bf4-a669-9ba42a052c53_1.0.0.0_x86__bzrwd8f9mzhby')

    if sys.argv[3] == 'InstallApp':
        addPackageCommand = "./metro_app_utilities.exe add \"file://c|/cygwin%s/dx_remote_agent_1.0.0.0_Win32_Test/dx_remote_agent_1.0.0.0_Win32.appx\"" % (sys.argv[1])
        doSystemCall(addPackageCommand)
    if sys.argv[3] == 'LaunchApp':
        doSystemCall('./metro_app_utilities.exe launch 43d863b7-f317-4bf4-a669-9ba42a052c53_bzrwd8f9mzhby\\!app')
    if sys.argv[3] == 'StopApp':
        doSystemCall('taskkill /F /IM dx_remote_agent.exe')
        
elif sys.argv[2] == 'iOS':
    doSystemCall('export IOS_SW_X_PATH=%s' % (sys.argv[3]));
    if sys.argv[4] == 'UnInstallApp':
        doSystemCall('osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/CloseProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud dx_remote_agent dx_remote_agent' % (sys.argv[1], sys.argv[3], sys.argv[1], sys.argv[3]));
        doSystemCall('osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/BackupAndDeleteApp.scpt Delete dx_remote_agent 1' % (sys.argv[1], sys.argv[3]));
        doSystemCall('osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/CloseXcode.scpt' % (sys.argv[1], sys.argv[3]));
        time.sleep(5)
    if sys.argv[4] == 'LaunchApp':
        doSystemCall('security unlock-keychain -p notsosecret ~/Library/Keychains/act-mve.keychain')
        doSystemCall('osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/RunProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud/dx_remote_agent/dx_remote_agent.xcodeproj dx_remote_agent'  % (sys.argv[1], sys.argv[3], sys.argv[1], sys.argv[3]))
    if sys.argv[4] == 'StopApp':
        doSystemCall('osascript %s/../%s/sw_x/projects/xcode/PersonalCloud/applescripts/CloseProject.scpt %s/../%s/sw_x/projects/xcode/PersonalCloud dx_remote_agent dx_remote_agent' % (sys.argv[1], sys.argv[3], sys.argv[1], sys.argv[3]));
