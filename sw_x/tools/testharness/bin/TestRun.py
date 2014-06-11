class TestRun :

    def __init__(self) :
        import time

        self.name = ""
        self.conf_file = ""
        self.output_dir = ""
        self.summary_file = ""
        self.test_suite_dir = ""
        self.env_var_list = []
        self.test_suite_list = []
        self.tc_count = 0
        self.tc_pass = 0
        self.tc_fail = 0
        self.tc_expected_to_fail = 0
        self.tc_indeterminate = 0
        self.tc_skip = 0
        self.tc_remaining = 0
        self.db_conn = {}
        self.update_db = False
        self.conn = ""
        self.cursor = ""
        self.db_max_retries = 3
        self.db_retry_wait_secs = 60
        self.had_db_error = False
        self.start_time = int(time.time())
        self.end_time = 0
        self.id = -1
        self.status = ""
        self.set_status("RUNNING")
    # end of function


    def get_local_start_time(self) :
        import time

        (year,mon,mday,hour,min,sec,wday,yday,isdst) = time.localtime(self.start_time)

        return "%d%02d%02d_%02d%02d%02d" % (year,mon,mday,hour,min,sec)
    # end of function


    def set_end_time(self) :
        import time

        self.end_time = int(time.time())

        return self.end_time
    # end of function


    def set_status(self,status) :
        if status in ("RUNNING","PASS","FAIL","TERMINATED","PASS_BUT_DBERR","FAIL_AND_DBERR") :
            # this is a valid status
            self.status = status
        else :
            # invalid status
            self.status = "INVALID_STATUS"

        return self.status
    # end of function


    def create_file_directory(self,filename) :
        # given a filename, create the directory that the file is expected to reside in
        import os

        file_dir = os.path.dirname(filename)

        if not os.path.exists(file_dir) :
            # directory does not exist, so try to create it
            try :
                os.makedirs(file_dir)
            except OSError,(errno, strerror):
                print "Problem creating directory " + file_dir
                print "Error({0}): {1}".format(errno, strerror)
                sys.exit(99)
            # end try
        # end if
    # end of function


    def open_connection_to_db(self) :
        # callers should handle exceptions
        # put the import statement here so that users only need to install/import the module if they will do DB activity
        import MySQLdb

        self.conn = MySQLdb.connect(
                        host = self.db_conn["TH_DBHOST"],
                        user = self.db_conn["TH_DBUSER"],
                        passwd = self.db_conn["TH_DBPWD"],
                        db = self.db_conn["TH_DBNAME"],
                    )

        # set the cursor that is used to send communication to the DB
        self.cursor = self.conn.cursor()
    # end of function


    def close_db_connection(self) :
        # close everything
        self.cursor.close()
        self.conn.commit()
        self.conn.close()
    # end of function


    def exec_sql_with_retry(self,sql_statement) :
        # Execute a sql command.  If there is any failure, close the connection,
        # sleep a while, open the connection, and retry submitting the command.
        # Stop after retrying a number of times.  Presumes that the connection
        # to the DB is good coming into this function.
        import time

        num_retries = 0

        while (self.update_db) and (num_retries < self.db_max_retries) :
            try :
                self.cursor.execute(sql_statement)
                # break out of here if the command was successful
                break
            except Exception, e:
                print "ERROR when trying to execute statement: %s" % (sql_statement)
                print "    " + repr(e)

                num_retries += 1
                print "Waiting %d seconds before attempting retry %d of %d" % (self.db_retry_wait_secs, num_retries, self.db_max_retries)

                # wait before retrying
                time.sleep(self.db_retry_wait_secs)

                # close previous connection
                try :
                    self.close_db_connection()
                except Exception, e:
                    print "ERROR: problem when closing db connection"
                    print "    " + repr(e)
                # end try

                # open new connection
                try :
                    self.open_connection_to_db()
                except Exception, e:
                    print "ERROR: problem when opening db connection"
                    print "    " + repr(e)
                # end try
            # end of try
        # end of while

        # handle case where none of the retries worked
        # set the flag that indicates there was at least one error
        # but then move on - re-raise the exception so that failure can be 
        # determined by caller
        if num_retries >= self.db_max_retries :
            print "ERROR: Unable to update database"
            self.had_db_error = True
            raise
        # end if
    # end of function

# end of class
