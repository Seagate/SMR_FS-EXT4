#* ---------------------------------------------------------------------------------------------
#* Copyright (c) 2015 Seagate Technologies
#* All rights reserved.
#* ---------------------------------------------------------------------------------------------

## Tools ##
RM		= rm -f

CC=gcc
CFLAGS=-I. -Wall
LD=gcc

CPP=g++
CPPFLAGS=-I. -Wall

## Debug [Conditional compilation] ##
ifeq ($(DEBUG),1)
DEFINES     	+= -DDEBUG
endif

## Build Verbosity ##
ifeq ($(VERBOSE),1)
BE_QUIET=
else
BE_QUIET=@
endif

## Targets ##
TARGETS		+= sd_report_zones

## Source Files ##
SOURCES		 = sd_report_zones

## Output directory ##
OBJECTS_DIR	 =
OBJECTS		 = $(addsuffix .o, $(SOURCES))

## Libraries ##
# LIBS=`pkg-config libssl --libs`
LIBS=

# NOTE: for sha support:
#   sudo apt-get install libssl-dev

##
## Compiling Rules
##

all: $(TARGETS)

sd_report_zones: sd_report_zones.o
	@echo "Linking $@"
	$(BE_QUIET)$(LD) $(LFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp $(INCFILES)
	@echo "Compiling $<"
	$(BE_QUIET)$(CPP) $(DEFINES) $(CPPFLAGS) $(INCPATH) -c $< -o $@

%.o: %.c $(INCFILES)
	@echo "Compiling $<"
	$(BE_QUIET)$(CC) $(DEFINES) $(CFLAGS) $(INCPATH) -c $< -o $@

test: sd_report_zones
	echo "Assumes /dev/sdm is an SMR drive."
	./sd_report_zones 0 /dev/sdm

check: test

clean:
	$(BE_QUIET)$(RM) $(OBJECTS)

distclean: clean
	$(BE_QUIET)$(RM) $(TARGETS)
