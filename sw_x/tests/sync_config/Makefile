# Makefile called by testharness to run the tests in a sub-directory.

# If another test is added, must specify relationships between directories,
# syncConfig: newTest
# in order to make sure they aren't run in parallel.

# Runs basic smoke tests.

SUBDIRS = syncConfig

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

post_test_run:
	cd $(SUBDIRS) && $(MAKE) post_test_run

post_suite_timeout:
	cd $(SUBDIRS) && $(MAKE) post_suite_timeout

$(SUBDIRS):
	$(MAKE) -C $@
