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

# Os independently launch an application and redirect the output to a log file

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--cmd", "", action="store", dest="cmd",
                 help="Command line to be executed", metavar="CMD")
    p.add_option("--log", "", action="store", dest="log",
                 help="Log file to store stdout and stderr", metavar="LOG")
    return p

def main():
    p = setupOptParser()
    opts, args = p.parse_args()

    if opts.cmd == None or opts.log == None:
        return 0

    print "Executing '%s' on %s..." % (opts.cmd, sys.platform) 
    log = open(opts.log, 'w')
    p = subprocess.Popen(opts.cmd, stdout=log, stderr=log) 
    p.wait()
    log.close()

if __name__ == '__main__':
    main()
