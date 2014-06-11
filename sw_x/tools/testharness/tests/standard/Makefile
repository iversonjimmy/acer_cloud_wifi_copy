.PHONY: first_target pass_tc fail_tc expected_to_fail_tc unnamed post_test_run

first_target: pass_tc fail_tc expected_to_fail_tc unnamed

pass_tc:
	@echo "TC_RESULT=PASS ;; TC_NAME=pass_tc"

fail_tc:
	@echo "TC_RESULT=FAIL ;; TC_NAME=fail_tc"

expected_to_fail_tc:
	@echo "TC_RESULT=EXPECTED_TO_FAIL ;; TC_NAME=expected_to_fail_tc"

unnamed:
	@echo "TC_RESULT=PASS"
	@echo "TC_RESULT=PASS"

post_test_run:
	@echo "TC_RESULT=PASS ;; TC_NAME=post_test_run_target_is_executed"
