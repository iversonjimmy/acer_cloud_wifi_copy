#!/usr/bin/python -u

import os
import struct
import sys

def main():
    filename = sys.argv[1]
    blocksize = int(sys.argv[2])

    if not os.path.isfile(filename) :
        print "Cannot find %s" % filename
        sys.exit(1)

    filesize = os.path.getsize(filename)
    filesize_size = struct.calcsize("!Q")
    padsize = blocksize - (filesize % blocksize)
    if padsize < filesize_size :
        padsize += blocksize
    padsize -= filesize_size

    f = open(filename, "ab")
    f.write("".join(chr(0) for i in range(padsize)))
    f.write(struct.pack("!Q", filesize))
    f.close()

if __name__ == '__main__':
    main()
