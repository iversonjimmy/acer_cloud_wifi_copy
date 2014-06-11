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

# Perform binary file creation on the target machine 

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--filename", "", action="store", dest="filename",
                 help="File name", metavar="FILENAME")
    p.add_option("--dstdir", "", action="store", dest="dstdir",
                 help="Destination directory", metavar="DSTDIR")
    p.add_option("--numfiles", "", action="store", dest="numfiles",
                 help="Destination directory", metavar="NUMFILES")
    return p

def main():
    p = setupOptParser()
    opts, args = p.parse_args()

    if opts.filename == None or opts.dstdir == None or opts.numfiles == None:
        return 0

    numfiles = int(opts.numfiles)
    for i in range(0, numfiles):
        os.system('diff %s %s/%s_%d' % (opts.filename, opts.dstdir, opts.filename, i))

if __name__ == '__main__':
    main()

