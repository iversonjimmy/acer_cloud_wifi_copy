.PHONY: first_target post_timeout post_test_run

first_target:
	# initial tests in the suite that complete should show their status
	@echo "TC_RESULT=PASS ;; TC_NAME=some_test_in_a_suite_that_timesout"

	# timeout was set to 2 minutes; this test should be killed by the testharness for running overtime
	sleep 150
	@echo "TC_RESULT=FAIL ;; TC_NAME=timeout_override"

post_suite_timeout:
	# this target should be executed once a test suite is timed out by the testharness
	@echo "TC_RESULT=PASS ;; TC_NAME=execute_post_suite_timeout_target_after_timeout"

post_test_run:
	@echo "TC_RESULT=PASS ;; TC_NAME=post_test_run_target_after_timeout"
