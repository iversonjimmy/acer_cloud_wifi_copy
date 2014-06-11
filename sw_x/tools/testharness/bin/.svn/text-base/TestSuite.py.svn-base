class TestSuite :
    def __init__(self,name) :
        self.name = name
        self.output_log = ""
        self.workdir = ""
        self.status = ""
        self.set_status("NOT YET RUN")
        self.start_time = 0
        self.end_time = 0
        self.tc_count = 0
        self.tc_pass = 0
        self.tc_fail = 0
        self.tc_expected_to_fail = 0
        self.tc_skip = 0
        self.tc_indeterminate = 0
        self.test_case_list = []
        self.id = -1
        self.instance_id = -1
        self.timeout = 1200
        self.post_timeout_timeout = 600
        self.post_test_run_timeout = 600
        self.command_pid = -1
        self.target = ""
#        self.conf_file = conf_file
#        self.env_var_list = []
#        self.required_var_list = []
    # end of function


    def __del__(self) :
        # kill any still-running commands
        if self.command_pid != -1 :
            #print "!!! PID is %d" % (self.command_pid)
            self.kill_child_processes(self.command_pid)
        # end if
    # end of function


    def is_min_info_set(self) :
        if self.name != "" :
            # minimum info is set
            return True
        else :
            return False
    # end of function


    def set_status(self,status) :
        # set the status of the test case; only valid statuses will be set
        # return the status that was set; set to INVALID_STATUS if an invalid status is attempted

        if status in ("NOT YET RUN","RUNNING","PASS","FAIL","SKIPPED","INDETERMINATE","BYPASSED") :
            # this is a valid status

            # only set it, though, if status is not already BYPASSED
            if self.status == "BYPASSED" :
                pass
                #print " * Not overriding the current BYPASSED status of the TC"
            else :
                self.status = status
            # end if
        else :
            # invalid status
            self.status = "INVALID_STATUS"
        # end if

        return self.status
    # end of function


    def set_start_time(self) :
        # set the start time of the test case to current time
        # returns the time that was set

        return self.set_time("start")
    # end of function


    def set_end_time(self) :
        # set the end time of the test case to current time
        # returns the time that was set

        return self.set_time("stop")
    # end of function


    def set_time(self,action) :
        # set a time element for the test case to current time
        # returns the time that was set

        import time

        curr_time = int(time.time())

        if action == "start" :
            self.start_time = curr_time
        elif action == "stop" :
            self.end_time = curr_time
        else :
            print "INTERNALERROR: unsupported action %s" % action
        # end if

        return curr_time
    # end of function


    def find_children_pids(self, proc_pid_list, proc_ppid_list, parent_pid) :
        #print "* Entering find_children_pids"
        children_pids_list = []

        count = len(proc_ppid_list)
        index = 0

        while index < count :
            if proc_ppid_list[index] == parent_pid :
                # save this "child" pid to return
                print "   * Proc %d has ppid %d" % (proc_pid_list[index], parent_pid)
                children_pids_list.append(proc_pid_list[index])
            #else :
                #print "     * Skipping proc %d with ppid %d; looking for %d" % (proc_pid_list[index], proc_ppid_list[index], parent_pid)
            # end if
            index += 1
        # end while

        #print "* Leaving find_children_pids"
        return children_pids_list
    # end find_children_pids


    def kill_child_processes(self, pid_to_kill) :
        import subprocess
        import re
        import os

        #print "* Entering kill_child_processes"

        kill_signal = 9
        proc_pid_list = []
        proc_ppid_list = []
        kill_list = []
        child_list = []

        #print "* Looking to terminate PID %d and all its children" % (pid_to_kill)

        # get list of running processes
        cmd_output = subprocess.Popen(['ps', '-ef'], stdout=subprocess.PIPE).communicate()[0]
        process_list = cmd_output.split('\n')

        # split processes into columns; we only care about PID and PPID columns, though
        separator = re.compile('[\s]+')
        for process in process_list :
            # skip the header - contains PPID
            if 'PPID' in process:
                continue
            # end if
            if len(process) > 0 :
                process_items = separator.split(process)
                proc_pid_list.append(int(process_items[1]))
                proc_ppid_list.append(int(process_items[2]))
            # end if
        # end for

        # check to see if the given pid_to_kill is still a valid process
        if proc_pid_list.count(pid_to_kill) == 0 :
            # process no longer exists; terminate now
            print "* Verified that process (with PID %d) is no longer running" % (pid_to_kill)
            return
        else :
            print "* Terminating process with PID %d and all its children" % (pid_to_kill)
        # end if

        # add the pid of process we are looking at to the kill list
        kill_list.append(pid_to_kill)

        # for each pid that is in the kill list, look for its children and add them to the list too
        kill_index = 0
        while kill_index < len(kill_list) :
            pid_to_kill = kill_list[kill_index]
            child_list = self.find_children_pids(proc_pid_list, proc_ppid_list, pid_to_kill)
            kill_list.extend(child_list)
            kill_index += 1
        # end while

        # try to kill each process in the kill_list
        # start with the parent processes first so that they cannot spawn new processes
        for pid in kill_list :
            print " * sending signal %d to PID %d" % (kill_signal, pid)
            try :
                os.kill(pid,kill_signal)
            except OSError, e:
                print "WARNING: os.kill failed"
                print "    " + repr(e)
        # end while

        #print "* Leaving kill_child_processes"
    # end kill_child_processes

