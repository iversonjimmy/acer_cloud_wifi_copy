# TODO: Move the files out of the CVS tree, to source.routefree.com as per
#       http://intwww.routefree.com/wiki/index.php/Third_Party_Source_and_Binaries

# TODO: get this programatically
MY_DIR = gvm_core/external/jackson

ifndef PBROOT
$(error Need to define PBROOT (or run this from a higher level Makefile))
endif
ifndef SRCROOT
$(error Need to define SRCROOT (or run this from a higher level Makefile))
endif

DEST_DIR = $(PBROOT)/$(MY_DIR)

JARS = \
	jackson-core-asl-1.3.0.jar \
	jackson-jaxrs-1.3.0.jar \
	jackson-mapper-asl-1.3.0.jar \
	jackson-xc-1.3.0.jar \
# end of list

.PHONY: default
default: $(addprefix $(DEST_DIR)/,$(JARS))

$(addprefix $(DEST_DIR)/,$(JARS)): $(DEST_DIR)/%: $(SRCROOT)/$(MY_DIR)/%
	mkdir -p $(DEST_DIR)
	cp -f $< $@
