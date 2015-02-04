#* ---------------------------------------------------------------------------------------------
#* Copyright (c) 2015 Seagate Technologies
#* All rights reserved.
#* ---------------------------------------------------------------------------------------------

## Tools ##
RM		= rm -f

## Build Verbosity ##
ifeq ($(VERBOSE),1)
BE_QUIET=
else
BE_QUIET=@
endif

SUBDIRS = ext4_ioctl sd_report_zones sd_identify sd_reset_wp sg-reset-wp
SUBCLEAN = $(addsuffix .clean,$(SUBDIRS))
SUBMORECLEAN = $(addsuffix .distclean,$(SUBDIRS))
     
.PHONY: subdirs $(SUBDIRS)
     
all: subdirs

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(BE_QUIET)$(MAKE) --quiet -C $@

.PHONY: clean $(SUBCLEAN)

clean: $(SUBCLEAN)

$(SUBCLEAN): %.clean:
	$(BE_QUIET)$(MAKE) --quiet -C $* clean

.PHONY: distclean $(SUBMORECLEAN)

distclean: $(SUBMORECLEAN)

$(SUBMORECLEAN): %.distclean:
	$(BE_QUIET)$(MAKE) --quiet -C $* distclean


