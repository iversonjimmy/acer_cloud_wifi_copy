class TestCase :
    def __init__(self,name) :
        self.name = name
        self.numResults = 0
        self.subTCs = []
        self.order = 0
        self.desc = ""
        self.cmd = ""
        self.set_on_fail("NEXT_TC")
        self.goto_tc_name = ""
        self.env_var_list = []
        self.test_case_id = -1
        self.suite_id = -1
        self.instance_id = -1
        self.start_time = 0
        self.end_time = 0
        self.status = None
        self.set_status("NOT YET RUN")
        self.output_log = ""
        self.text = ""
    # end of function


    def set_status(self,status) :
        # set the status of the test case; only valid statuses will be set
        # return the status that was set; set to INVALID_STATUS if an invalid status is attempted

        if status in ("NOT YET RUN","RUNNING","PASS","FAIL","SKIPPED","INDETERMINATE","BYPASSED","EXPECTED_TO_FAIL") :
            # this is a valid status

            # only set it, though, if status is not already BYPASSED
            if self.status == "BYPASSED" :
                pass
                #dprint(1," * Not overriding the current BYPASSED status of the TC")
            else :
                self.status = status
            # end if
        else :
            # invalid status
            self.status = "INVALID_STATUS"
        # end if

        return self.status
    # end of function


    def set_on_fail(self,action) :
        # set the on_fail action of the test case; only valid on_fail actions will be set
        # return the on_fail action that was set; set to INVALID_ACTION if an invalid action is attempted

        if action in ("NEXT_TC","NEXT_SUITE","STOP","GOTO") :
            # this is a valid action
            self.on_fail = action
        else :
            # invalid action
            self.on_fail = "INVALID_ACTION"
        # end if

        return self.on_fail
    # end of function


    def is_min_info_set(self) :
        # determine if the minimum set of info for a test case is set
        # returns True/False

        if self.name != "" and self.desc != "" :
            # minimum info is set
            return True
        else :
            return False
        # end if
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

