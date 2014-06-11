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
    p.add_option("--size", "", action="store", dest="size",
                 help="File size in bytes", metavar="FILESIZE")
    p.add_option("--value", "", action="store", dest="value",
                 help="File content value", metavar="VALUE")
    return p

def main():
    p = setupOptParser()
    opts, args = p.parse_args()

    if opts.filename == None or opts.size == None or opts.value == None:
        return 0

    size = int(opts.size)

    print 'Creating file %s (sz %d) filled with %s' % (opts.filename, size, opts.value) 
    value = opts.value
    f = open(opts.filename, 'wb')
    while size > 0:
        if len(value) > size:
            value = '0'
        f.write(str(value))
        size -= len(value)
    f.close() 
     
if __name__ == '__main__':
    main()

