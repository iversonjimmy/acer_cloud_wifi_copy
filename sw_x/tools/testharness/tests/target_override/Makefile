TEST_NAME=target_override

.PHONY: first_target default daily

first_target:
	@echo "TC_RESULT=FAIL ;; TC_NAME=$(TEST_NAME)"

default:
	@echo "TC_RESULT=FAIL ;; TC_NAME=$(TEST_NAME)"

daily:
	sleep 60
	@echo "TC_RESULT=PASS ;; TC_NAME=$(TEST_NAME)"
