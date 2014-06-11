#!/usr/bin/python

##########
# Usages
##########
import sys
import subprocess
import time
import string
import getopt


def usage() :
    # print usage

    print ""
    print "Usage:"
    print sys.argv[0],"-f <file> -v <version>"
    print ""
    print "   -f file : the full path of the CCD.pdb file"
    print "   -v version : the version of the .pdb file"
    print ""
# end of function


class SymStore :

    def __init__(self) :
        #print "! Entering __init__()"

        self.lockfile_host_user = "build"
        self.lockfile_host = "pcstore.ctbg.acer.com"
        self.lockfile = "/a/pc/symbol_store.lock"
        self.symstore_root = "/a/pc/symbol_store"
        self.lock_check_interval = 3
        self.max_wait_secs = 600
        self.lock_was_gotten = 0
        self.lock_was_released = 0
        self.ping_count = "3"
        self.ping_pkt_size = "3"
        self.symstore_exe = "/cygdrive/c/Program Files (x86)/Windows Kits/8.0/Debuggers/x86/symstore.exe"
        self.ccd_pdb_file = ""
        self.symstore_root = "\\pcstore.ctbg.acer.com\pc\symbol_store"
        self.product = "CCD"
        self.prod_version = ""
        self.comment = ""

        #print "! Leaving __init__()"
    # end of function
    # ====================


    def ping_remote_host(self) :
        # ping the remote host to make sure it's alive and reachable
        #print "! Entering ping_remote_host()"
 
        print "Pinging remote host %s to verify it is alive" % (self.lockfile_host)

        cmd = ["ping", self.lockfile_host, self.ping_pkt_size, self.ping_count]
        #print ["! Command is"] + cmd
        return_code = subprocess.call(cmd)

        if return_code == 0 :
            # it is pingable
            print "%s is alive" % (self.lockfile_host)
        else :
            # can't reach it
            print "ERROR: Unable to ping %s" % (self.lockfile_host)
            print "       QUITTING!"
            sys.exit(4)
        # end if

        #print "! Leaving ping_remote_host()"
    # end of function
    # ====================


    def get_lock(self) :
        # get the lock to be able to use symstore
        # this means writing output to a lock file on shared storage
        # if the file does not exist, create it, thereby "getting" the lock
        # if the file does exist, that means someone has the lock
        # check every LOCK_CHECK_INTERVAL until the file does not exist
        #print "! Entering get_lock()"

        print ""
        print "Getting lock (file %s@%s:%s)" % (self.lockfile_host_user, self.lockfile_host, self.lockfile)
        cmd = ["ssh", self.lockfile_host_user + "@" + self.lockfile_host, "ls", self.lockfile, ">", "/dev/null", "2>&1"]
        #print ["! Command is"] + cmd
        return_code = 0
        secs_waited = 0

        while return_code == 0 :
            return_code = subprocess.call(cmd)

            if return_code == 0 :
                # someone has the lock

                if secs_waited >= self.max_wait_secs :
                    # we've waited long enough, error out
                    print "ERROR: Unable to obtain lock (file %s@%s:%s) within allotted time (%d secs)" % (self.lockfile_host_user, self.lockfile_host, self.lockfile, self.max_wait_secs)
                    print "       QUITTING!"
                    sys.exit(3)
                # end if

                # if here, then under wait timeout to get lock, so wait for it
                print "Lock file %s@%s:%s exists; check again in %d seconds" % (self.lockfile_host_user, self.lockfile_host, self.lockfile, self.lock_check_interval)
                time.sleep(self.lock_check_interval)
                secs_waited = secs_waited + self.lock_check_interval
            # end if
        # end while

        # if here, then lock was released
        cmd = ["ssh", self.lockfile_host_user + "@" + self.lockfile_host, "touch", self.lockfile]
        return_code = subprocess.call(cmd)
        #print ["! Command is"] + cmd

        if return_code == 0 :
            # turn on flag saying lock was gotten
            self.lock_was_gotten = 1
        else :
            # could not create lock file
            print "ERROR: could not create lock file %s@%s:%s" % (self.lockfile_host_user, self.lockfile_host, self.lockfile) 
            print "       QUITTING!"
            sys.exit(1)
        # end if

        #print "! Leaving get_lock()"
    # end of function
    # ====================


    def release_lock(self) :
        # release the lock
        # this comes down to deleting the lock file
        #print "! Entering release_lock()"

        print ""
        print "Releasing lock (file %s@%s:%s)" % (self.lockfile_host_user, self.lockfile_host, self.lockfile)

        cmd = ["ssh", self.lockfile_host_user + "@" + self.lockfile_host, "rm", self.lockfile]
        return_code = subprocess.call(cmd)

        if return_code == 0 :
            # turn on flag saying lock was released
            self.lock_was_released = 1
        else :
            # could not remove lock file
            print "ERROR: Unable to clear lock file %s@%s:%s" % (self.lockfile_host_user, self.lockfile_host, self.lockfile)
            print "       To manually clear lock, delete the lock file"
            print "       QUITTING!"
            sys.exit(2)
        # end if

        #print "! Leaving release_lock()"
    # end of function
    # ====================


    def execute_symstore_command(self) :
        # now that we have the lock, execute the symstore command
        #print "! Entering execute_symstore_command()"

        cmd = [self.symstore_exe, "add", "/r", "/f", self.ccd_pdb_file, "/s", self.symstore_root, "/t", self.product, "/v", self.prod_version, "/c", self.comment]
        print ""
        print "Executing command:"
        print cmd
        return_code = subprocess.call(cmd)

        time.sleep(3)

        #print "! Leaving execute_symstore_command()"
    # end of function
    # ====================


    def set_ccd_pdb_file(self, lnx_file_path) :
        # The file name is given in lnx format (e.g. /home/build/blah/CCD.pdb)
        # Need to convert it to C:\\cygwin\\home\\build\\blah\\CCD.pdb
        #print "! Entering set_ccd_pdb_file()"

        converted = string.replace(lnx_file_path, "/", "\\")
        self.ccd_pdb_file = "C:\\cygwin" + converted

        #print "! Leaving set_ccd_pdb_file()"
    # end of function
    # ====================


    def set_prod_version(self, version) :
        #print "! Entering set_prod_version()"

        self.prod_version = version

        #print "! Leaving set_prod_version()"
    # end of function
    # ====================


    def set_comment(self, comment) :
        #print "! Entering set_comment()"

        self.comment = comment

        #print "! Leaving set_comment()"
    # end of function
    # ====================


    def __del__(self) :
        # destructor method
        # make sure the last thing we do is release the lock
        #print "! Entering __del__()"

        # only need to release the lock in the destroy method if we got the lock
        # and did not go thru the normal exit that sets the lock_was_released flag
        # note: you really don't want to delete the file if you did not get the 
        # lock since you'd be deleting someone else's lock!
        if (self.lock_was_gotten == 1) and (self.lock_was_released == 0) :
            self.release_lock()
        # end if

        #print "! Leaving __del__()"
    # end of function
    # ====================
# end of class


def process_command_line_args(argv,sym_store) :
    # process command line args
    file_given = 0
    version_given = 0

    try :
        optlist,args = getopt.getopt(argv,"f:v:")
    except getopt.GetoptError :
        print "\nERROR: Unknown option"
        usage()
        sys.exit(1)
    # end try

    # process the arguments
    for opt,arg in optlist :
        if opt == '-f' :
            sym_store.set_ccd_pdb_file(arg)
            file_given = 1
        elif opt == '-v' :
            sym_store.set_prod_version(arg)
            sym_store.set_comment(arg)
            version_given = 1
        # end if
    # end for

    if file_given == 0 or version_given == 0 :
        # one of the required values not given
        usage()
        sys.exit(1)
    # end if
# end of function


##########
# MAIN
##########

sym_store = SymStore()

# any error condition causes an exit, so each next step should only run if 
# there is no error in the previous step; __del__ is always called so if the 
# lock is obtained it will always be released

# process command line args
process_command_line_args(sys.argv[1:],sym_store)

# make sure symbol store repository is reachable
sym_store.ping_remote_host()

# create the lock file on the symbol store host
sym_store.get_lock()

# execute command
sym_store.execute_symstore_command()

# release/delete the lock file
sym_store.release_lock()

sys.exit(0)
