# Point of this test is that the testharness doesn't fail horribly if there is *not* a
# post_timeout target when a test suite times out.

.PHONY: first_target

first_target:
	# timeout was set to 2 minutes; this test should be killed by the testharness for running overtime
	sleep 150
	@echo "TC_RESULT=FAIL ;; TC_NAME=timeout_override_negative"
