import optparse
import os
import time
import sys

# Perform binary file creation on the target machine

def setupOptParser():
    p = optparse.OptionParser(add_help_option=False)
    p.add_option("-h", "--help", action="help",
                 help="Show this help message and exit")
    p.add_option("--filename", "", action="store", dest="filename",
                 help="File name", metavar="FILENAME")
    p.add_option("--dstdir", "", action="store", dest="dstdir",
                 help="Destination directory", metavar="DSTDIR")
    p.add_option("--int", "", action="store", type="float", dest="int",
                 help="Polling internal in seconds", metavar="INT")
    p.add_option("--to", "", action="store", type="float", dest="to",
                 help="Polling timeout in seconds", metavar="TO")
    return p

def main():
    loop_end='n'
    p = setupOptParser()
    opts, args = p.parse_args()

    if opts.filename == None or opts.dstdir == None or opts.int == None:
        return 0

    before = dict ([(f, None) for f in os.listdir (opts.dstdir)])
    for f in before:
        if f == opts.filename:
            print "log file found: ", f
            loop_end='y'
            return;

    print "waiting for ", opts.filename
    curto=0
    while (loop_end == 'n'):
        if curto > opts.to:
            print "timeout!"
            loop_end='y'
            break; 
        time.sleep (opts.int)
        curto = curto + opts.int
        after = dict ([(f, None) for f in os.listdir (opts.dstdir)])
        added = [f for f in after if not f in before]
        removed = [f for f in before if not f in after]

        # if added: 
        #    print "Added: ", ", ".join (added)
        # if removed: 
        #    print "Removed: ", ", ".join (removed)
        before = after
        for f in added:
            if f == opts.filename:
                print "log file found: ", f
                loop_end='y'
                break;

if __name__ == '__main__':
    main()

