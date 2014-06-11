ifndef BUILDROOT
$(error Must define BUILDROOT)
endif

ifneq ($(shell grep 'CYGWIN' /proc/version),)
override BUILDROOT := $(shell cygpath -u $(BUILDROOT))
endif

END_DATE = $(shell date +'%Y-%m-%d')
START_DATE = $(shell date --date="-7 day" +'%Y-%m-%d')
STATDIR=$(BUILDROOT)/sw_x_stat_$(END_DATE)_$(START_DATE)

INCLUDE_LIST=**/*.cpp:**/*.h:**/*.c:**/*.hpp:**/*.java:**/*.py:**/*.js:**/*.properties:**/*.txt:**/*.html:**/*.sql:**/*.conf:**/*.jsp:**/*.coffee:**/Makefile
EXCLUDE_LIST=gvm_core/external/**:external/**

.PHONY: default
default: $(STATDIR)/index.html $(STATDIR)/logfile.log

$(STATDIR)/index.html: $(STATDIR)/logfile.log
ifeq ($(shell uname -o),Cygwin)
	java -jar ./statsvn.jar -verbose -include "$(INCLUDE_LIST)" -exclude "$(EXCLUDE_LIST)" -output-dir "$(shell cygpath -w $(STATDIR))" "$(shell cygpath -w $(STATDIR))/logfile.log" ../../../sw_x 
else
	java -jar ./statsvn.jar -verbose -include "$(INCLUDE_LIST)" -exclude "$(EXCLUDE_LIST)" -output-dir $(STATDIR) $(STATDIR)/logfile.log ../../../sw_x
endif

$(STATDIR)/logfile.log:
	mkdir -p $(STATDIR)
	cd ../../../sw_x && svn log -v --xml -r {$(END_DATE)}:{$(START_DATE)} > $(STATDIR)/logfile.log

sync:
	rsync -rvz --delete $(STATDIR)/ build@pcstore.ctbg.acer.com:/a/pc/stats/sw_x_stat/sw_x_stat_$(END_DATE)_$(START_DATE)
	
clean:
	rm -fr $(STATDIR)
	
