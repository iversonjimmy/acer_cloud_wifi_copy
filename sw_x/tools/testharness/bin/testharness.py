#!/usr/bin/python -u


#####
# USAGES
#####
import getopt
import sys
import re
import time
import os
import commands
import subprocess
import string
# add the dir containing the testharness script to the library search path
calling_dir = os.path.dirname(sys.argv[0])
sys.path.append(calling_dir)
from TestRun import TestRun
from TestSuite import TestSuite
from TestCase import TestCase
from EnvVar import EnvVar


#####
# GLOBAL VARS
#####
debug = 0
test_run = TestRun()
is_running_on_win = 0
win_file_prefix = "C:/cygwin"
final_log_location = ""
sleep_interval_secs = 1
default_output_dir = "/tmp/testharness/output"
default_summary_file = "testharness_summary.out"
default_test_suite_dir = calling_dir + "/../../../tests"
testharness_front_end_host = "www.ctbg.acer.com"
harness_return_code = 0
kill_signal = 9
print_alive_msg = 0
alive_msg_interval = 30
expected_statuses = ['PASS', 'EXPECTED_TO_FAIL']


#####
# FUNCTIONS
#####

def usage() :
    # print usage

    print ""
    print "Usage:"
    print sys.argv[0],"-c <conf_file> [-d <debug_level>] [-o <output_dir>] [-s <summary_file>] [-b <final_log_location>] [-t <test_suite_dir>] [-k]"
    print ""
    print "   -c conf_file : the run configuration file to use"
    print "   -d debug_level : print debug statements"
    print "   -o output_dir : directory where to save run output files; default is in /tmp/testharness/output"
    print "   -s summary_file : name of the file where summary information is written; default is output_dir/testharness_summary.out"
    print "   -b final_log_location : for buildbot use; archive location where to scp the output files (e.g. build@pcstore.ctbg.acer.com:/a/pc/test_outputs/TOT/20110101-010000"
    print "   -t test_suite_dir : directory containing the test suites to execute; defaults to sw_x/tests in the tree containing the testharness script"
    print "   -k : print an I'm alive message to STDOUT every %s seconds; meant to be used by buildbot" % (alive_msg_interval)
    print ""
# end of function


def dprint(level,txt) :
    # debug print; print only if the debug level matches
    global debug
    if int(debug) >= int(level) :
        # print it
        print txt
    # end if
# end of function
    

def process_command_line_args(argv,test_run) :
    # process command line args

    try :
        optlist,args = getopt.getopt(argv,"c:d:o:s:b:t:k")
    except getopt.GetoptError :
        print "\nERROR: Unknown option"
        usage()
        sys.exit(1)
    # end try

    # process the arguments
    for opt,arg in optlist :
        if opt == '-c' :
            test_run.conf_file = arg
        elif opt == '-d' :
            global debug
            debug = arg
        elif opt == '-o' :
            test_run.output_dir = arg
        elif opt == '-s' :
            test_run.summary_file = arg
        elif opt == '-b' :
            global final_log_location
            final_log_location = arg
        elif opt == '-t' :
            test_run.test_suite_dir = arg
        elif opt == '-k' :
           global print_alive_msg
           print_alive_msg = 1
        # end if
    # end for

    # conf file must be given
    if test_run.conf_file == "" :
        print "ERROR: -c <conf_file> required"
        usage()
        sys.exit(2)
    else :
        # construct the full-path name for this file
        if test_run.conf_file[0] == '/' :
            # full-path already given
            dprint(3,"   * Conf file name is already full-path format (unix)")
        elif test_run.conf_file[0:3] == 'C:/' :
            # full-path already given
            dprint(3,"   * Conf file name is already full-path format (unix)")
        else :
            # append CWD to front of name
            test_run.conf_file = os.getcwd() + "/" + test_run.conf_file
        # end if
    # end if

    # determine the OS this script is running on
    global is_running_on_win
    if sys.platform == 'win32' :
        is_running_on_win = 1
    else :
        is_running_on_win = 0
    # end if
    
# end of function


def parse_test_run_config(test_run) :
    # read the given test run config file
    # load any ENV vars into the environment
    # load the suite config file names into the config_file list

    dprint(1,"* Entering parse_test_run_config")

    # open file
    file = test_run.conf_file
    try :
        dprint(3,"   * Opening file " + file)
        inputfile = open(file,'r')
    except IOError,e :
        print "Problem opening file %s" % (file)
        print e[0], e[1]
        sys.exit(1)
    # end try 

    # read each line of the file
    for line in inputfile :
        # skip comments
        if re.match(r'\s*#',line) :
            continue
        # end if

        # skip whitespace-only lines
        if re.match(r'^\s*$',line) :
            continue
        # end if

        # otherwise split line into key/value pairs on the first '=' found
        pair = re.split(r'=',line,1)

        # verify that there are only 2 items in the list
        if len(pair) != 2 :
            print "ERROR: Parse of test run config file",file,"failed"
            print "       Unable to parse line:"
            print line
            inputfile.close()
            sys.exit(1)
        # end if

        # strip the white space from the key/value pairs
        for i in range(0,2) :
            pair[i] = pair[i].strip()
        # end for
        
        # run name
        if re.match(r'^TH_RUN_NAME$',pair[0]) :
            dprint(3,"   * Found run name " + pair[1])
            test_run.name = pair[1]
            continue
        # end if

        # load up the DB connection info if the key/values are found
        if re.match(r'^TH_DBNAME$',pair[0]) :
            test_run.db_conn['TH_DBNAME'] = pair[1]
            dprint(3,"   * Found db_conn var TH_DBNAME = " + pair[1])
            continue
        # end if
        if re.match(r'^TH_DBHOST$',pair[0]) :
            test_run.db_conn['TH_DBHOST'] = pair[1]
            dprint(3,"   * Found db_conn var TH_DBHOST = " + pair[1])
            continue
        # end if
        if re.match(r'^TH_DBUSER$',pair[0]) :
            test_run.db_conn['TH_DBUSER'] = pair[1]
            dprint(3,"   * Found db_conn var TH_DBUSER = " + pair[1])
            continue
        # end if
        if re.match(r'^TH_DBPWD$',pair[0]) :
            test_run.db_conn['TH_DBPWD'] = pair[1]
            dprint(3,"   * Found db_conn var TH_DBPWD = " + pair[1])
            continue
        # end if

        # output directory - do not overwrite if specified on command line
        if re.match(r'^TH_OUTPUT_DIR$',pair[0]) :
            dprint(3,"   * Found test run output dir = " + pair[1])
            # this can be set from the command line, so only set it if it's not already set
            if test_run.output_dir == "" :
                test_run.output_dir = pair[1] + "/" + test_run.get_local_start_time()
                dprint(3,"   * Output dir for this run is " + test_run.output_dir)
            else :
                dprint(3,"   * Test run output dir was specified on the command line")
            # end if
            continue
        # end if

        # summary file - do not overwrite if specified on command line
        if re.match(r'^TH_SUMMARY_FILE$',pair[0]) :
            dprint(3,"   * Found name of summary file = " + pair[1])
            # this can be set from the command line, so only set it if it's not already set
            if test_run.summary_file == "" :
                test_run.summary_file = pair[1]
                dprint(3,"   * Summary file for this run is " + test_run.summary_file)
            else :
                dprint(3,"   * Summary file was specified on the command line")
            # end if
            continue
        # end if

        # test suite directory - do not overwrite if specified on command line
        if re.match(r'^TH_TEST_SUITE_DIR$',pair[0]) :
            dprint(3,"   * Found test suite directory = " + pair[1])
            # this can be set from the command line, so only set it if it's not already set
            if test_run.test_suite_dir == "" :
                test_run.test_suite_dir = pair[1]
                dprint(3,"   * Test suite directory for this run is " + test_run.test_suite_dir)
            else :
                dprint(3,"   * Test suite directory was specified on the command line")
            # end if
            continue
        # end if

        # if it's a test suite config, get a new test suite object
        if re.match(r'^TH_SUITE$',pair[0]) :
            dprint(3,"   * Found suite " + pair[1])

            # suites can have attributes - they are separated by ::
            # Ex: TH_SUITE = foo::timeout=45::attr2=val2
            # NOTE: buildbot will currently timeout the entire testharness if any test suite lasts
            #   more than 30 minutes.  If we ever have a suite that needs more than 30 minutes, we
            #   need testharness to print to stdout every once in a while.

            # separate the TH_SUITE value portion by the :: string
            items = re.split(r'::',pair[1])

            # first item in the list must be the name of the suite
            suite = TestSuite(items[0].strip())
            dprint(3,"   *  name = " + suite.name)
            test_run.test_suite_list.append(suite)

            # go through the rest of the attributes (if any)
            attr_count=1
            while attr_count < len(items) :

                # split the attribute on the first '='
                attr = re.split(r'=',items[attr_count],1)
                attr_count+=1

                # strip the white space from the key/value pairs
                for i in range(0,2) :
                    attr[i] = attr[i].strip()
                # end for

                # search for known attributes
                if re.match(r'^timeout$',attr[0]) :
                    # suite timeout; given in min, save in secs
                    suite.timeout = int(attr[1]) * 60
                    dprint(3,"   *  suite timeout = %d (%d secs)" % (int(attr[1]),suite.timeout))
                    continue
                elif re.match(r'^target$',attr[0]) :
                    # execute a target different than default
                    suite.target = attr[1]
                    dprint(3,"   *  suite target = %s" % (suite.target))
                    continue
                else :
                    # unknown suite attribute
                    print "ERROR: Unknown suite attribute: %s" % (attr[0])
                    sys.exit(4)
                # end if
            # end while

            continue
        # end if

        # if here, then this is a global env var; get a new env var object
        env = EnvVar(pair[0],pair[1])
        # add it to the list
        test_run.env_var_list.append(env)
        dprint(3,"   * Found global var " + env.key + " with value = " + env.value)
    # end for

    # done processing the file
    inputfile.close()

    # determine that all things which need a value have a value
    if test_run.output_dir == "" :
        test_run.output_dir = default_output_dir

        # if running on Windows, add the Windows prefix
        if is_running_on_win == 1 :
            test_run.output_dir = win_file_prefix + test_run.output_dir
        # end if

        dprint(3,"   * Output dir not specified, using default.  Output dir for this run is " + test_run.output_dir)
    # end if
    if is_running_on_win == 1 :
        # set the lnx version of the output dir name to use with scp in Cygwin
        # remove teh win_file_prefix
        prefix_len = len(win_file_prefix)
        test_run.lnx_output_dir = test_run.output_dir[prefix_len:]
    # end if
    if test_run.summary_file == "" :
        test_run.summary_file = test_run.output_dir + "/" + default_summary_file

        dprint(3,"   * Summary file not specified, using default.  Summary file for this run is " + test_run.summary_file)
    # end if
    if test_run.test_suite_dir == "" :
        test_run.test_suite_dir = default_test_suite_dir

        dprint(3,"   * Test suite directory not specified, using default.  Test suite directory for this run is " + test_run.test_suite_dir)
    # end if

    # must be able to find the given test_suite_dir
    if not os.path.exists(test_run.test_suite_dir) :
        print "ERROR: Cannot find test suite directory: " + test_run.test_suite_dir
        sys.exit(3)
    # end if

    dprint(1,"* Leaving parse_test_run_config")
# end of function


def open_db_connection(test_run) :
    # If DB connection info is given, set the update_db parameter
    # and open the connection to the DB

    dprint(1,"* Entering open_db_connection")

    # determine if db updates should be done
    for item in ("TH_DBHOST","TH_DBNAME","TH_DBUSER","TH_DBPWD") :
        if item not in test_run.db_conn :
            # at least one of the DB settings is not set
            print "INFO: At least one of TH_DBNAME, TH_DBHOST, TH_DBUSER, and TH_DBPWD are not set so DB updates will not be performed"
            test_run.update_db = False

            # exit now
            dprint(1," * Leaving open_db_connection")
            return (None,None)
        # end if
    # end for 

    # set the flag to say that we're doing DB updates
    test_run.update_db = True

    # open the connection to the database 
    try :
        test_run.open_connection_to_db()
    except ImportError, e :
        # if module is not available, no point in trying to do more DB stuff
        print "ERROR: It is likely that the MySQLdb module has not been installed on system.  Will not attempt to do any more DB updates."
        print "    " + repr(e)
        test_run.had_db_error = True
        test_run.update_db = False
    except Exception, e:
        print "WARNING: Open of connection to DB failed, but it can be retried later"
        print "    " + repr(e)
    # end try

    dprint(1,"* Leaving open_db_connection")
# end of function

def fix_old_run_status(test_run) :
    # update the status of any runs that have a status of RUNNING and a start date of more than 24 hours ago
    dprint(1,"* Entering fix_old_run_status")

    dprint(1," * Resolve status of any jobs still running after 24 hours")
    curr_time = int(time.time())
    day_ago = curr_time - (24 * 60 * 60)

    try :
        sqlcmd = "UPDATE test_run SET status='FAIL' WHERE status='RUNNING' and start_time < %d" % (day_ago)
        dprint(1," * SQL: " + sqlcmd)
        test_run.exec_sql_with_retry(sqlcmd)
    except :
        print "WARNING: DB update failed, but moving on"
    # end try

    dprint(1,"* Leaving fix_old_run_status")
# end of function


def print_test_run_info(test_run) :
    # Print out info on the test run
    dprint(1,"* Entering print_test_run_info")

    dprint(2,"  * TEST SUITE CONFIGURATIONS:")
    for suite in test_run.test_suite_list :
        dprint(2,"  *  " + suite.name)
    # end for

    dprint(2,"  * DB VARS:")
    for item in ("TH_DBHOST","TH_DBNAME","TH_DBUSER","TH_DBPWD") :
        if item in test_run.db_conn :
            # it's been defined, so print out the value
            value = test_run.db_conn[item]
            dprint(2,"  *  " + item + " = " + value)
        else :
            # it's not been defined, so print that out
            dprint(2,"  *  " + item + " = <value not set>")
        # end if
    # end for

    dprint(2,"  * RUN-LEVEL VARS:")
    for env in test_run.env_var_list :
        dprint(2,"  *  key = " + env.key + "; value = " + env.value)
    # end for

    dprint(1,"* Leaving print_test_run_info")
# end of function


def upload_initial_db_info(test_run) :
    dprint(1,"* Entering upload_initial_db_info")

    # if final_log_location is set, use the given location to update the database
    global final_log_location
    if len(final_log_location) > 0 :
        # put this location in the database
        dprint(3,"   * Using log location " + final_log_location + " instead of " + test_run.output_dir) 
        output_dir_for_db = final_log_location
    else :
        output_dir_for_db = test_run.output_dir
        dprint(4,"    * Using standard log location " + output_dir_for_db)
    # end if

    try :
        # insert a test_run record
        sqlcmd = "INSERT INTO test_run (name,start_time,status,tc_count,tc_pass,tc_fail,tc_skip,tc_indeterminate,tc_remaining,output_dir) \
                 VALUES('%s','%d','%s','%d','%d','%d','%d','%d','%d','%s')" % \
                 (test_run.name,test_run.start_time,test_run.status,test_run.tc_count,test_run.tc_pass,test_run.tc_fail,test_run.tc_skip,test_run.tc_indeterminate,test_run.tc_remaining,output_dir_for_db)
        dprint(1," * SQL: " + sqlcmd)
        test_run.exec_sql_with_retry(sqlcmd)
        test_run.id = test_run.cursor.lastrowid
        dprint(1," * SQL: row " + str(test_run.id) + " inserted")
    except Exception, e:
        # if unable to create the test_run, no point in making any more DB updates
        print "ERROR: Unable to create test_run record.  Disabling further DB updates."
        test_run.update_db = False

        # no point in going on, exit now
        dprint(1,"* Leaving upload_initial_db_info")
        return
    # end try

    try :
        # for each test suite...
        for suite in test_run.test_suite_list :
            # make a suite entry if needed
            sqlcmd = "SELECT id FROM test_suite \
                      WHERE name = '%s'" % \
                      suite.name
            dprint(1," * SQL: " + sqlcmd)
            test_run.exec_sql_with_retry(sqlcmd)

            row = test_run.cursor.fetchone()
            if row == None :
                # entry does not exist yet, create it
                sqlcmd = "INSERT INTO test_suite (name) \
                          VALUES('%s')" % \
                          (suite.name)
                dprint(1," * SQL: " + sqlcmd)
                test_run.exec_sql_with_retry(sqlcmd)
                suite.id = test_run.cursor.lastrowid
                dprint(1," * SQL: row " + str(suite.id) + " inserted")
            else :
                # entry exists
                suite.id = row[0]
                dprint(1," * SQL: test_suite entry found (row " + str(suite.id) + ")");
            # end if
        # end for
    except :
        # if we could not get all test suite ids, we should turn off all DB updates
        print "ERROR: Unable to create test_suite record. Disabling further DB updates"
        test_run.update_db = False

        # no point in going on, exit now
        dprint(1,"* Leaving upload_initial_db_info")
        return
    # end try

    print "\n* Run info will be available at http://%s/testharness/show_test_runs_v2.php?runID=%d\n" % (testharness_front_end_host,test_run.id)

    dprint(1,"* Leaving upload_initial_db_info")
# end of function


def do_env_vars(run_list,action) :
    dprint(1,"* Entering do_env_vars")

    # make just one list
    all_items = run_list[:]

    # set or unset each item
    for item in all_items :
        if action == "set" :
            # in the event that the var contains previously-defined vars, evaluate them in the shell first
            cmd = "echo " + item.value
            stat,out = commands.getstatusoutput(cmd)
            dprint(3,"   * Setting env var " + item.key + " = " + out + " (" + item.value + ")")
            os.putenv(item.key,out)
        elif action == "unset" :
            dprint(3,"   * Unsetting env var " + item.key + " = " + item.value)
            os.unsetenv(item.key)
        else :
            # unknown action
            print "INTERNALERROR: unsupported action %s" % action
            sys.exit(10)
        # end if
    # end for

    dprint(1,"* Leaving do_env_vars")
# end of function


def do_pre_suite_execute_work(test_run,suite) :
    # stuff done before executing a suite's tests
    dprint(1,"* Entering do_pre_suite_execute_work")

    # set the start time, status, output log, and workdir for this suite
    suite.set_start_time()
    suite.set_status("RUNNING")
    suite.output_log = "%s/%s.output" % (test_run.output_dir,suite.name)
    suite.workdir = "%s/%s" % (test_run.output_dir,suite.name)
    test_run.create_file_directory(suite.output_log)

    # create the TEST_WORKDIR for the suite
    if not os.path.isdir(suite.workdir) :
        # directory does not exist, so create it
        dprint(3,"   * Creating TEST_WORKDIR directory %s" % (suite.workdir))
        os.makedirs(suite.workdir)
    else :
        dprint(3,"   * TEST_WORKDIR directory %s already exists" % (suite.workdir))
    # end if

    # add the TEST_WORKDIR as an env var for the suite
    env = EnvVar("TEST_WORKDIR",suite.workdir)
    test_run.env_var_list.append(env)

    # setup the environment variables
    do_env_vars(test_run.env_var_list,"set")

    # if configured, update the DB
    if test_run.update_db :
        try :
            # create a test_suite_instance record
            sqlcmd = "INSERT INTO test_suite_instance (test_run_id,test_suite_id,start_time,status) \
                      VALUES('%d','%d','%d','%s')" % \
                      (test_run.id,suite.id,suite.start_time,suite.status)
            dprint(1," * SQL: " + sqlcmd)
            test_run.exec_sql_with_retry(sqlcmd)
            suite.instance_id = test_run.cursor.lastrowid
            dprint(1," * SQL: row " + str(suite.instance_id) + " inserted")
        except Exception, e:
            # if we could not create the suite instance, we should turn off all DB updates
            print "ERROR: Unable to create test_suite_instance record.  Disabling further DB updates."
            print "    " + repr(e)
            test_run.update_db = False
        # end try
    # end if

    dprint(1,"* Leaving do_pre_suite_execute_work")
# end of function


def do_post_suite_execute_work(test_run,suite) :
    dprint(1,"* Entering do_post_suite_execute_work")

    # set the stop time as now
    suite.set_end_time()

    # unset the environment variables
    do_env_vars(test_run.env_var_list,"unset")

    # parse the output log
    if os.path.getsize(suite.output_log) <= 0 :
        # file either does not exist or has no content
        suite.set_status("INDETERMINATE")
    else :
        # parse the log to find results

        # open output file
        try :
            dprint(3,"   * Opening file " + suite.output_log)
            log = open(suite.output_log,'r')
        except IOError,e :
            print "Problem opening file %s" % (suite.output_log)
            print e[0], e[1]
        # end try 

        for line in log :
            # see if there is a TC_RESULT line in the output
            if "TC_RESULT" in line :
                suite.tc_count = suite.tc_count + 1

                # get a new TestCase object; temporarily name it "testN"
                temp_name = "test%d" % (suite.tc_count)
                tc = TestCase(temp_name)
                tc.run_order = suite.tc_count

                if re.search(r'TC_RESULT\s*=\s*PASS',line) :
                    suite.tc_pass = suite.tc_pass + 1
                    tc.set_status("PASS")
                    dprint(3,"   * TC status is PASS")
                elif re.search(r'TC_RESULT\s*=\s*EXPECTED_TO_FAIL',line) :
                    suite.tc_expected_to_fail = suite.tc_expected_to_fail + 1
                    tc.set_status("EXPECTED_TO_FAIL")
                    dprint(3,"   * TC status is EXPECTED_TO_FAIL")
                elif re.search(r'TC_RESULT\s*=\s*FAIL',line) :
                    suite.tc_fail = suite.tc_fail + 1
                    tc.set_status("FAIL")
                    dprint(3,"   * TC status is FAIL")
                else :
                    # a line containing TC_RESULT was found, but it is not a known status
                    suite.tc_indeterminate = suite.tc_indeterminate + 1
                    tc.set_status("INDETERMINATE")
                    dprint(3,"   * TC status is INDETERMINATE")
                # end if

                # there should be a name included with the result
                parsed = re.split('TC_NAME\s*=',line)
                if len(parsed) > 1 :
                    # there's a name; take the first word as the TC name
                    remaining = parsed[1]
                    parsed = remaining.rsplit(None)
                    if len(parsed) > 0 and parsed[0] != "" :
                        # name found
                        tc.name = parsed[0]
                        dprint(3,"   * TC name found: " + tc.name)
                    else :
                        # even though 'TC_NAME=' found, no name given, leave the temporary on in place
                        dprint(3,"   * TC_NAME found, but no name given")
                    # end if
                else :
                    # no name given, leave the temporary one in place
                    dprint(3,"   * No TC_NAME found")
                # end if

                # add the TC object to the suite's list
                suite.test_case_list.append(tc)
            # end if
        # end for

        # close the file
        log.close()
    # end if

    # set the suite's status
    if suite.status == 'INDETERMINATE' :
        # do nothing if the state is already INDETERMINATE (set if the suite timed out)
        dprint(4,"   * Status of suite is already INDETERMINATE")
        if suite.tc_indeterminate == 0 :
            # do this so we can count indeterminate type suite failures
            suite.tc_indeterminate = 1
        # end if
    elif suite.tc_count == 0 :
        # no result status found in the log
        suite.set_status("INDETERMINATE")
        if suite.tc_indeterminate == 0 :
            # do this so we can count indeterminate type suite failures
            suite.tc_indeterminate = 1
        # end if
    elif suite.tc_fail > 0 :
        # at least one failure
        suite.set_status("FAIL")
    elif suite.tc_fail == 0 :
        # no true failures, so we'll call the suite status PASS (there can be EXPECTED_TO_FAIL)
        suite.set_status("PASS")
    else :
        # don't know how we got here
        print("INTERNALERR: In an unexpected place when determining test suite status")
        suite.set_status("INDETERMINATE")
    # end if

    # update the test run counters
    test_run.tc_count = test_run.tc_count + suite.tc_count
    test_run.tc_pass = test_run.tc_pass + suite.tc_pass
    test_run.tc_fail = test_run.tc_fail + suite.tc_fail
    test_run.tc_expected_to_fail = test_run.tc_expected_to_fail + suite.tc_expected_to_fail
    test_run.tc_indeterminate = test_run.tc_indeterminate + suite.tc_indeterminate

    # if configured, update the DB
    if test_run.update_db :
        try :
            # update the test_run results
            sqlcmd = "UPDATE test_run \
                      SET tc_count = '%d', \
                          tc_pass = '%d', \
                          tc_fail = '%d', \
                          tc_expected_to_fail = '%d', \
                          tc_skip = '%d', \
                          tc_indeterminate = '%d' \
                      WHERE id = %d" \
                % (test_run.tc_count,test_run.tc_pass,test_run.tc_fail,test_run.tc_expected_to_fail,test_run.tc_skip,test_run.tc_indeterminate,test_run.id)
            dprint(1," * SQL: " + sqlcmd)
            test_run.exec_sql_with_retry(sqlcmd)

            # update the test_suite_instance results
            sqlcmd = "UPDATE test_suite_instance \
                      SET end_time = '%d', \
                          status = '%s', \
                          tc_count = '%d', \
                          tc_pass = '%d', \
                          tc_fail = '%d', \
                          tc_expected_to_fail = '%d', \
                          tc_skip = '%d', \
                          tc_indeterminate = '%d' \
                      WHERE id = %d" \
                % (suite.end_time,suite.status,suite.tc_count,suite.tc_pass,suite.tc_fail,suite.tc_expected_to_fail,suite.tc_skip,suite.tc_indeterminate,suite.instance_id)
            dprint(1," * SQL: " + sqlcmd)
            test_run.exec_sql_with_retry(sqlcmd)
        except Exception, e:
            print "WARNING: DB update failed, but will continue to try DB activities later"
            print "    " + repr(e)
        # end try

        # create test_case_instance entries for each test case
        for tc in suite.test_case_list :
            try :
                # make a test_case entry if needed
                sqlcmd = "SELECT id FROM test_case \
                          WHERE name = '%s'" % \
                          (tc.name)
                dprint(1," * SQL: " + sqlcmd)
                test_run.exec_sql_with_retry(sqlcmd)

                row = test_run.cursor.fetchone()
                if row == None :
                    # entry does not exist yet, create it
                    sqlcmd = "INSERT INTO test_case (name) \
                              VALUES('%s')" % \
                              (tc.name)
                    dprint(1," * SQL: " + sqlcmd)
                    test_run.exec_sql_with_retry(sqlcmd)
                    tc.id = test_run.cursor.lastrowid
                    dprint(1," * SQL: row " + str(tc.id) + " inserted")
                else :
                    # entry exists
                    tc.id = row[0]
                    dprint(1," * SQL: test_case entry found (row " + str(tc.id) + ")");
                # end if

                # create the test_case_instance
                sqlcmd = "INSERT INTO test_case_instance \
                              (test_run_id,test_suite_instance_id,test_case_id,run_order,status) \
                          VALUES('%d','%d','%d','%d','%s')" % \
                              (test_run.id,suite.instance_id,tc.id,tc.run_order,tc.status)
                dprint(1," * SQL: " + sqlcmd)
                test_run.exec_sql_with_retry(sqlcmd)

            except Exception, e:
                print "WARNING: DB update of test_case_instances failed, but will try to resume DB activities later"
                print "    " + repr(e)
            # end try
        # end for
    # end if

    # print the TC status
    for tc in suite.test_case_list :
        if tc.status in expected_statuses :
            print "        %s %s" % (tc.status,tc.name)
        else:
            print "  ***** %s %s" % (tc.status,tc.name)
    # end for

    # handle an indeterminate test suite
    if suite.status == "INDETERMINATE" :
        print "  ***** %s" % (suite.status)
    # end if

    dprint(1,"* Leaving do_post_suite_execute_work")
# end of function


def write_to_suite_log(test_suite, message) :
    # Logs the message to the test_suite's output log file.
    # This adds its own newlines so message doesn't need to contain any.
    
    # open file
    try :
        dprint(3,"   * Opening file " + test_suite.output_log)
        suitelog = open(test_suite.output_log,'a')
    except IOError,e :
        print "Problem opening file %s" % (test_suite.output_log)
        print e[0], e[1]
    # end try
    # write the log message
    suitelog.write("\n<testharness time=\"" + get_printable_time(int(time.time())) + "\">\n")
    suitelog.write(message)
    suitelog.write("\n</testharness>\n")
    # close file
    suitelog.close()
# end of function


def execute_command_and_wait(test_suite, suite_dir, command_type, timeout) :
    # Execute a make command for the test suite, then wait for it to finish.
    # Supported command_types are "test_suite_tests", "post_timeout", "post_test_run".

    # Build the command that will be executed based on the command_type given
    command = ""
    if command_type == "test_suite_tests" :
        # note: we use an append since the harness will put the command that is executed as the first line in the output log
        # if a target has been specified, execute the suite's make target
        command = "ulimit -c unlimited && make -C " + suite_dir + " " + test_suite.target + " >> " + test_suite.output_log + " 2>&1"
    elif command_type == "post_timeout" :
        command = "make -C " + suite_dir + " post_suite_timeout >> " + test_suite.output_log + " 2>&1"
    elif command_type == "post_test_run" :
        command = "make -C " + suite_dir + " post_test_run >> " + test_suite.output_log + " 2>&1"
    else :
        # unknown command_type
        print "\nERROR: Unknown command_type %s\n" % (command_type)
        sys.exit(6)
    # end if
    dprint(1," * COMMAND: " + command)

    # write the command to the output log
    write_to_suite_log(test_suite, "Executing command on the test driver:\n" + command)

    # execute the command
    process = subprocess.Popen(command,shell=True,close_fds=True)
    # save the PID
    test_suite.command_pid = process.pid
    dprint(1," * Command PID is %s" % (test_suite.command_pid))

    # wait for it
    process.poll()
    seconds_waited = 0
    while process.returncode == None :
        dprint(5,"    * Waiting (" + str(seconds_waited) + " secs passed)")
        time.sleep(sleep_interval_secs)

        # check for timeout
        seconds_waited = seconds_waited + sleep_interval_secs

        # print an "I'm alive" message every so often; used by buildbot to prevent timeout due to no STDOUT
        if print_alive_msg == 1 :
            if seconds_waited % alive_msg_interval == 0 :
                print "    INFO: have waited %d seconds for %s to complete" % (seconds_waited, command_type)
            # end if
        # end if

        if seconds_waited > timeout :
            # kill the processes started
            print "ERROR: %s did not complete within allotted time (%d secs)" % (command_type, timeout)
            print "       killing processes"

            # kill the command we started and any of its children
            test_suite.kill_child_processes(test_suite.command_pid)
            
            write_to_suite_log(test_suite, command_type + " command timed out (" + str(timeout) + "s)!")

            if command_type == "test_suite_tests" :
                print "        INDETERMINATE (test suite timeout)"
                test_suite.set_status("INDETERMINATE")
            # end if

            return 1
        # end if

        process.poll()
    # end while

    return 0
# end of function

def execute_test_cases(test_run) :
    dprint(1,"* Entering execute_test_cases")

    global goto_tc_name
    global print_alive_msg

    for suite in test_run.test_suite_list :
        print "=========="
        print "= Start execution of test suite \"%s\"" % (suite.name)
        print "=========="

        # determine that the test suite directory exists
        suite_dir = test_run.test_suite_dir + "/" + suite.name
        if not os.path.exists(suite_dir) :
            print "!!! Suite " + suite_dir + " not found; skipping this test suite"
            suite.set_status("SKIPPED")
            continue
        # end if

        # do the pre execute stuff
        do_pre_suite_execute_work(test_run,suite)

        # execute the command to run the tests
        command_return = execute_command_and_wait(suite, suite_dir, "test_suite_tests", suite.timeout)

        if command_return == 1 :
            # running the tests timed out; execute the post_timeout target
            command_return = execute_command_and_wait(suite, suite_dir, "post_timeout", suite.post_timeout_timeout)
        # end if

        # execute the post_test_run target
        command_return = execute_command_and_wait(suite, suite_dir, "post_test_run", suite.post_test_run_timeout)

        # process the output file to determine test status
        do_post_suite_execute_work(test_run,suite)

        write_to_suite_log(suite, "Done with test suite \"" + suite.name + "\"")

    # end for

    dprint(1,"* Leaving execute_test_cases")
# end of function


def finalize_test_run_status(test_run) :
    dprint(1,"* Entering update_test_run_status")

    # figure out the final status of the run
    global harness_return_code
    # it's PASS unless a suite failure is found
    harness_return_code = 0
    test_run.set_status("PASS")

    # check each suite for non-PASS and one is found, set run status to FAIL
    # note that EXPECTED_TO_FAIL test case results do NOT cause a suite to FAIL
    for suite in test_run.test_suite_list :
        if suite.status != 'PASS' :
            test_run.set_status("FAIL")
            dprint(3,"   * Test run status is FAIL")
            harness_return_code = 1
        # end if
    # end for

    # take this time to "end" the test run
    test_run.set_end_time()

    # take into consideration any DB error and correct the status accordingly
    if test_run.had_db_error :
        if test_run.status == "PASS" :
            test_run.set_status("PASS_BUT_DBERR")
        elif test_run.status == "FAIL" :
            test_run.set_status("FAIL_AND_DBERR")
        # end if
    # end if

    # if configured, update the database
    if test_run.update_db :
        try :
            sqlcmd = "UPDATE test_run SET \
                          end_time = '%d', \
                          status = '%s' \
                      WHERE id = %d" \
                % (test_run.end_time,test_run.status,test_run.id)
            dprint(1," * SQL: " + sqlcmd)
            test_run.exec_sql_with_retry(sqlcmd)
        except Exception, e :
            print "WARNING: DB update of final test_run status failed"
            print "    " + repr(e)
        # end try
    # end if

    dprint(1,"* Leaving update_test_run_status")
# end of function


def get_printable_time(epoch_secs) :
    dprint(1,"* Entering get_printable_time")

    (year,mon,mday,hour,min,sec,wday,yday,isdst) = time.localtime(epoch_secs)

    dprint(1,"* Leaving get_printable_time")
    return "%02d/%02d/%d %02d:%02d:%02d" % (year,mon,mday,hour,min,sec)
# end of function


def get_time_difference(epoch_secs1,epoch_secs2) :
    dprint(1,"* Entering get_time_difference")

    secs_in_min = 60
    secs_in_hour = 3600
    secs_in_day = 86400

    num_secs = epoch_secs1 - epoch_secs2
    num_secs = abs(num_secs)

    days = num_secs / secs_in_day
    num_secs = num_secs - (days * secs_in_day)

    hours = num_secs / secs_in_hour
    num_secs = num_secs - (hours * secs_in_hour)

    mins = num_secs / secs_in_min
    num_secs = num_secs - (mins * secs_in_min)

    dprint(1,"* Leaving get_time_difference")

    return "%d day(s) %d hour(s) %d minute(s) %d second(s)" % (days,hours,mins,num_secs)
# end of function


def print_result_summary(test_run) :
    # Print out a summary of the status
    dprint(1,"* Entering print_result_summary")

    text = []

    dprint(3,"   * Creating directory for " + test_run.summary_file + " if needed")
    test_run.create_file_directory(test_run.summary_file)

    # open summary file
    try :
        dprint(3,"   * Opening file " + test_run.summary_file)
        summ_file = open(test_run.summary_file,'w')
    except IOError,e :
        print "Problem opening file %s" % (test_run.summary_file)
        print e[0], e[1]
    # end try 

    text.append("\n")
    text.append("==========\n")
    text.append("= RESULTS SUMMARY\n")
    text.append("==========\n")

    # print suite totals
    for suite in test_run.test_suite_list :
        text.append("SUITE: %s   _%s_\n" % (suite.name,suite.status))
        text.append(" LOG: %s\n" % (suite.output_log))

        for tc in suite.test_case_list :
            text.append("    %16s %s\n" % (tc.status,tc.name))
        # end for

        text.append(" Total - %d   Pass - %d   Fail - %d   Expected_to_Fail - %d   Indeterminate - %d\n" % (suite.tc_count,suite.tc_pass,suite.tc_fail,suite.tc_expected_to_fail,suite.tc_indeterminate))
        text.append(" Start Time - %s   End Time - %s   Duration - %s\n" % (get_printable_time(suite.start_time),get_printable_time(suite.end_time),get_time_difference(suite.end_time,suite.start_time)))
        text.append("\n")
    # end for

    # print list of failures/indeterminate
    text.append("\n")
    any_failures = False
    for suite in test_run.test_suite_list :
        if suite.status not in expected_statuses :
            if not any_failures :
                text.append("Problems:\n")
                any_failures = True
            text.append(" *** %s: %s (suite)\n" % (suite.status, suite.name))
        for tc in suite.test_case_list :
            if tc.status not in expected_statuses :
                if not any_failures :
                    text.append("Problems:\n")
                    any_failures = True
                text.append(" *** %s: %s: %s\n" % (tc.status, suite.name, tc.name))
        # end for
    # end for
    if not any_failures :
        text.append("No problems!\n")

    # print test run totals
    text.append("\n")
    text.append("        Total TCs: %d\n" % test_run.tc_count)
    text.append("           Passed: %d\n" % test_run.tc_pass)
    text.append("           Failed: %d\n" % test_run.tc_fail)
    text.append(" Expected To Fail: %d\n" % test_run.tc_expected_to_fail)
    text.append("    Indeterminate: %d\n" % test_run.tc_indeterminate)

    text.append("\n")
    text.append("    Start Time: %s\n" % get_printable_time(test_run.start_time))
    text.append("      End Time: %s\n" % get_printable_time(test_run.end_time))
    text.append("      Duration: %s\n" % get_time_difference(test_run.end_time,test_run.start_time))

    text.append("\n")
    text.append("TEST RUN STATUS %s\n" % test_run.status)
    text.append("\n")
    text.append("LOGDIR %s\n" % test_run.output_dir)

    # if db updates were made, give a link to the website
    if test_run.update_db :
        text.append("Run info also available at http://%s/testharness/show_test_runs_v2.php?runID=%d\n" % (testharness_front_end_host,test_run.id))
        text.append("\n")
    # end if

    text.append("TEST RUN FINISHED\n")
    text.append("\n")

    # write summary to the file
    dprint(3,"   * printing summary to file")
    summ_file.writelines(text)

    # close file
    summ_file.close()

    # print summary to screen
    dprint(3,"   * printing summary to stdout")
    for line in text :
        # comma is necessary since text already includes newline and we don't 
        # want print to add another one
        print line,
    # end for
    
    dprint(1,"* Leaving print_result_summary")
# end of function

def archive_output_files(test_run) :
    # scp the output files to a particular location
    dprint(1,"* Entering archive_output_files")
    
    # only do this if asked to do it
    global final_log_location
    if len(final_log_location) == 0 :
        dprint(2,"  * No request to archive logs; doing nothing for this step")
        return 0
    # end if

    # get the name of the directory we want created
    dprint(2,"  * Attempting to create the parent archive directory")
    parsed = re.split(':',final_log_location)
    if len(parsed) == 2 :
        # do a mkdir on the parent directory
        parent_dir = os.path.dirname(parsed[1])

        dprint(2,"  * COMMAND: ssh %s mkdir -p %s" % (parsed[0],parent_dir))
        process = subprocess.Popen(["ssh",parsed[0],"mkdir","-p",parent_dir],stderr=subprocess.STDOUT)
        process.wait()
        dprint(2,"  *  returncode is %d" % (process.returncode))
    else :
        dprint(2,"  * Could not find separator ':' in final_log_location (%s)" % (final_log_location))
    # end if

    # scp the contents of the output_dir to the specified location
    print ""
    print "INFO: Archiving output files to %s" % (final_log_location)
    # need to use the Cygwin-style naming for this command
    if is_running_on_win == 1 :
        output_dir_name = test_run.lnx_output_dir
    else :
        output_dir_name = test_run.output_dir
    # end if

    # Apply "chmod -R a+r *" to the directory first.
    dprint(1," * COMMAND: chmod -R a+r %s" % (output_dir_name))
    process = subprocess.Popen(["chmod","-R","a+r",output_dir_name],stderr=subprocess.STDOUT)
    process.wait()
    if process.returncode != 0 :
        dprint(2,"  *  returncode from chmod is %d" % (process.returncode))
    
    dprint(1," * COMMAND: scp -qpr %s %s" % (output_dir_name,final_log_location))
    process = subprocess.Popen(["scp","-qpr",output_dir_name,final_log_location],stderr=subprocess.STDOUT)
    process.wait()

    # if scp fails, fail the harness
    if process.returncode != 0 :
        dprint(2,"  * return code from scp is %d" % (process.returncode))
        global harness_return_code
        harness_return_code = 1
    else :
        # otherwise give the http location of the log files
        print "INFO: archiving is complete"

        # need to remove the leading /a from the location (very much specific to pcstore)
        http_dir = string.replace(parsed[1],'/a/','')
        print "http://pcstore.ctbg.acer.com/%s" % (http_dir)
    # end if

    dprint(1,"* Leaving archive_output_files")
# end of function


#####
# MAIN
#####

# print python version
version = sys.version
print ""
print "PYTHON VERSION: " + version
print ""

# process command line args
process_command_line_args(sys.argv[1:],test_run)

# parse the test run config file
parse_test_run_config(test_run)
print "INFO: Done parsing test run configuration"

# print what we've got for debug purposes (uses debug levels 1&2)
print_test_run_info(test_run)

# try to connect to the DB
# if it cannot be done, then test_run.update_db is set to False
open_db_connection(test_run)

if test_run.update_db :
    # fix the status of old test runs that were left RUNNING
    fix_old_run_status(test_run)
# end if

if test_run.update_db :
    # load the DB with the pre-execution test case info
    upload_initial_db_info(test_run)
# end if

# execute each test case
execute_test_cases(test_run)

# update the database with the test run results
finalize_test_run_status(test_run)

# print final results to screen and file
print_result_summary(test_run)

# do cleanup
if test_run.update_db :
    test_run.close_db_connection()
# end if

# move output files
archive_output_files(test_run)

# exit
print ""
print "harness return code is %d" % (harness_return_code)
print ""
sys.exit(harness_return_code)
