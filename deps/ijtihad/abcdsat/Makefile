###################################################################################################

.PHONY:	r d p sh cr cd cp csh lr ld lp lsh config all install install-headers install-lib\
        install-bin clean distclean
all:	r lr lsh

## Load Previous Configuration ####################################################################

-include config.mk

## Configurable options ###########################################################################

# Directory to store object files, libraries, executables, and dependencies:
BUILD_DIR      ?= build

# Include debug-symbols in release builds
ABCDSAT_RELSYM ?= -g

# Sets of compile flags for different build types
ABCDSAT_REL    ?= -O3 -D NDEBUG
ABCDSAT_DEB    ?= -O0 -D DEBUG 
ABCDSAT_PRF    ?= -O3 -D NDEBUG
ABCDSAT_FPIC   ?= -fpic

# GNU Standard Install Prefix
prefix         ?= /usr/local

## Write Configuration  ###########################################################################

config:
	@( echo 'BUILD_DIR?=$(BUILD_DIR)'           ; \
	   echo 'ABCDSAT_RELSYM?=$(ABCDSAT_RELSYM)' ; \
	   echo 'ABCDSAT_REL?=$(ABCDSAT_REL)'       ; \
	   echo 'ABCDSAT_DEB?=$(ABCDSAT_DEB)'       ; \
	   echo 'ABCDSAT_PRF?=$(ABCDSAT_PRF)'       ; \
	   echo 'ABCDSAT_FPIC?=$(ABCDSAT_FPIC)'     ; \
	   echo 'prefix?=$(prefix)'                 ) > config.mk

## Configurable options end #######################################################################

INSTALL ?= install

# GNU Standard Install Variables
exec_prefix ?= $(prefix)
includedir  ?= $(prefix)/include
bindir      ?= $(exec_prefix)/bin
libdir      ?= $(exec_prefix)/lib
datarootdir ?= $(prefix)/share
mandir      ?= $(datarootdir)/man

# Target file names
ABCDSAT      = abcdsat#       Name of abcdSat main executable.
ABCDSAT_CORE = abcdsat_core#  Name of simplified abcdSat executable (only core solver support).
ABCDSAT_SLIB = lib$(ABCDSAT).a#  Name of abcdSat static library.
ABCDSAT_DLIB = lib$(ABCDSAT).so# Name of abcdSat shared library.

# Shared Library Version
SOMAJOR=2
SOMINOR=1
SORELEASE?=.0#   Declare empty to leave out from library file name.

ABCDSAT_CXXFLAGS = -I. -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS -Wall -Wno-parentheses -Wextra
ABCDSAT_LDFLAGS  = -Wall -lz

ECHO=@echo
ifeq ($(VERB),)
VERB=@
else
VERB=
endif

SRCS = $(wildcard abcdsat/core/*.cc) $(wildcard abcdsat/simp/*.cc) $(wildcard abcdsat/utils/*.cc)
HDRS = $(wildcard abcdsat/mtl/*.h) $(wildcard abcdsat/core/*.h) $(wildcard abcdsat/simp/*.h) $(wildcard abcdsat/utils/*.h)
OBJS = $(filter-out %Main.o, $(SRCS:.cc=.o))

r:	$(BUILD_DIR)/release/bin/$(ABCDSAT)
d:	$(BUILD_DIR)/debug/bin/$(ABCDSAT)
p:	$(BUILD_DIR)/profile/bin/$(ABCDSAT)
sh:	$(BUILD_DIR)/dynamic/bin/$(ABCDSAT)

cr:	$(BUILD_DIR)/release/bin/$(ABCDSAT_CORE)
cd:	$(BUILD_DIR)/debug/bin/$(ABCDSAT_CORE)
cp:	$(BUILD_DIR)/profile/bin/$(ABCDSAT_CORE)
csh:	$(BUILD_DIR)/dynamic/bin/$(ABCDSAT_CORE)

lr:	$(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB)
ld:	$(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB)
lp:	$(BUILD_DIR)/profile/lib/$(ABCDSAT_SLIB)
lsh:	$(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE)

## Build-type Compile-flags:
$(BUILD_DIR)/release/%.o:			ABCDSAT_CXXFLAGS +=$(ABCDSAT_REL) $(ABCDSAT_RELSYM)
$(BUILD_DIR)/debug/%.o:				ABCDSAT_CXXFLAGS +=$(ABCDSAT_DEB) -g
$(BUILD_DIR)/profile/%.o:			ABCDSAT_CXXFLAGS +=$(ABCDSAT_PRF) -pg
$(BUILD_DIR)/dynamic/%.o:			ABCDSAT_CXXFLAGS +=$(ABCDSAT_REL) $(ABCDSAT_FPIC)

## Build-type Link-flags:
$(BUILD_DIR)/profile/bin/$(ABCDSAT):		ABCDSAT_LDFLAGS += -pg
$(BUILD_DIR)/release/bin/$(ABCDSAT):		ABCDSAT_LDFLAGS += --static $(ABCDSAT_RELSYM)
$(BUILD_DIR)/profile/bin/$(ABCDSAT_CORE):	ABCDSAT_LDFLAGS += -pg
$(BUILD_DIR)/release/bin/$(ABCDSAT_CORE):	ABCDSAT_LDFLAGS += --static $(ABCDSAT_RELSYM)

## Executable dependencies
$(BUILD_DIR)/release/bin/$(ABCDSAT):	 	$(BUILD_DIR)/release/abcdsat/simp/Main.o $(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB)
$(BUILD_DIR)/debug/bin/$(ABCDSAT):	 	$(BUILD_DIR)/debug/abcdsat/simp/Main.o $(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB)
$(BUILD_DIR)/profile/bin/$(ABCDSAT):	 	$(BUILD_DIR)/profile/abcdsat/simp/Main.o $(BUILD_DIR)/profile/lib/$(ABCDSAT_SLIB)
# need the main-file be compiled with fpic?
$(BUILD_DIR)/dynamic/bin/$(ABCDSAT):	 	$(BUILD_DIR)/dynamic/abcdsat/simp/Main.o $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB)

## Executable dependencies (core-version)
$(BUILD_DIR)/release/bin/$(ABCDSAT_CORE):	$(BUILD_DIR)/release/abcdsat/core/Main.o $(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB)
$(BUILD_DIR)/debug/bin/$(ABCDSAT_CORE):	 	$(BUILD_DIR)/debug/abcdsat/core/Main.o $(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB)
$(BUILD_DIR)/profile/bin/$(ABCDSAT_CORE):	$(BUILD_DIR)/profile/abcdsat/core/Main.o $(BUILD_DIR)/profile/lib/$(ABCDSAT_SLIB)
# need the main-file be compiled with fpic?
$(BUILD_DIR)/dynamic/bin/$(ABCDSAT_CORE): 	$(BUILD_DIR)/dynamic/abcdsat/core/Main.o $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB)

## Library dependencies
$(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB):	$(foreach o,$(OBJS),$(BUILD_DIR)/release/$(o))
$(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB):		$(foreach o,$(OBJS),$(BUILD_DIR)/debug/$(o))
$(BUILD_DIR)/profile/lib/$(ABCDSAT_SLIB):	$(foreach o,$(OBJS),$(BUILD_DIR)/profile/$(o))
$(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE)\
 $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR)\
 $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB):	$(foreach o,$(OBJS),$(BUILD_DIR)/dynamic/$(o))

## Compile rules (these should be unified, buit I have not yet found a way which works in GNU Make)
$(BUILD_DIR)/release/%.o:	%.cc
	$(ECHO) Compiling: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $(ABCDSAT_CXXFLAGS) $(CXXFLAGS) -c -o $@ $< -MMD -MF $(BUILD_DIR)/release/$*.d

$(BUILD_DIR)/profile/%.o:	%.cc
	$(ECHO) Compiling: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $(ABCDSAT_CXXFLAGS) $(CXXFLAGS) -c -o $@ $< -MMD -MF $(BUILD_DIR)/profile/$*.d

$(BUILD_DIR)/debug/%.o:	%.cc
	$(ECHO) Compiling: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $(ABCDSAT_CXXFLAGS) $(CXXFLAGS) -c -o $@ $< -MMD -MF $(BUILD_DIR)/debug/$*.d

$(BUILD_DIR)/dynamic/%.o:	%.cc
	$(ECHO) Compiling: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $(ABCDSAT_CXXFLAGS) $(CXXFLAGS) -c -o $@ $< -MMD -MF $(BUILD_DIR)/dynamic/$*.d

## Linking rule
$(BUILD_DIR)/release/bin/$(ABCDSAT) $(BUILD_DIR)/debug/bin/$(ABCDSAT) $(BUILD_DIR)/profile/bin/$(ABCDSAT) $(BUILD_DIR)/dynamic/bin/$(ABCDSAT)\
$(BUILD_DIR)/release/bin/$(ABCDSAT_CORE) $(BUILD_DIR)/debug/bin/$(ABCDSAT_CORE) $(BUILD_DIR)/profile/bin/$(ABCDSAT_CORE) $(BUILD_DIR)/dynamic/bin/$(ABCDSAT_CORE):
	$(ECHO) Linking Binary: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $^ $(ABCDSAT_LDFLAGS) $(LDFLAGS) -o $@

## Static Library rule
%/lib/$(ABCDSAT_SLIB):
	$(ECHO) Linking Static Library: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(AR) -rcs $@ $^

## Shared Library rule
$(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE)\
 $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR)\
 $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB):
	$(ECHO) Linking Shared Library: $@
	$(VERB) mkdir -p $(dir $@)
	$(VERB) $(CXX) $(ABCDSAT_LDFLAGS) $(LDFLAGS) -o $@ -shared -Wl,-soname,$(ABCDSAT_DLIB).$(SOMAJOR) $^
	$(VERB) ln -sf $(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE) $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR)
	$(VERB) ln -sf $(ABCDSAT_DLIB).$(SOMAJOR) $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB)

install:	install-headers install-lib install-bin
install-debug:	install-headers install-lib-debug

install-headers:
#       Create directories
	$(INSTALL) -d $(DESTDIR)$(includedir)/abcdsat
	for dir in mtl utils core simp; do \
	  $(INSTALL) -d $(DESTDIR)$(includedir)/abcdsat/$$dir ; \
	done
#       Install headers
	for h in $(HDRS) ; do \
	  $(INSTALL) -m 644 $$h $(DESTDIR)$(includedir)/$$h ; \
	done

install-lib-debug: $(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 $(BUILD_DIR)/debug/lib/$(ABCDSAT_SLIB) $(DESTDIR)$(libdir)

install-lib: $(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB) $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE)
	$(INSTALL) -d $(DESTDIR)$(libdir)
	$(INSTALL) -m 644 $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE) $(DESTDIR)$(libdir)
	ln -sf $(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE) $(DESTDIR)$(libdir)/$(ABCDSAT_DLIB).$(SOMAJOR)
	ln -sf $(ABCDSAT_DLIB).$(SOMAJOR) $(DESTDIR)$(libdir)/$(ABCDSAT_DLIB)
	$(INSTALL) -m 644 $(BUILD_DIR)/release/lib/$(ABCDSAT_SLIB) $(DESTDIR)$(libdir)

install-bin: $(BUILD_DIR)/dynamic/bin/$(ABCDSAT)
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL) -m 755 $(BUILD_DIR)/dynamic/bin/$(ABCDSAT) $(DESTDIR)$(bindir)

clean:
	rm -f $(foreach t, release debug profile dynamic, $(foreach o, $(SRCS:.cc=.o), $(BUILD_DIR)/$t/$o)) \
          $(foreach t, release debug profile dynamic, $(foreach d, $(SRCS:.cc=.d), $(BUILD_DIR)/$t/$d)) \
	  $(foreach t, release debug profile dynamic, $(BUILD_DIR)/$t/bin/$(ABCDSAT_CORE) $(BUILD_DIR)/$t/bin/$(ABCDSAT)) \
	  $(foreach t, release debug profile, $(BUILD_DIR)/$t/lib/$(ABCDSAT_SLIB)) \
	  $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR).$(SOMINOR)$(SORELEASE)\
	  $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB).$(SOMAJOR)\
	  $(BUILD_DIR)/dynamic/lib/$(ABCDSAT_DLIB)

distclean:	clean
	rm -f config.mk

## Include generated dependencies
-include $(foreach s, $(SRCS:.cc=.d), $(BUILD_DIR)/release/$s)
-include $(foreach s, $(SRCS:.cc=.d), $(BUILD_DIR)/debug/$s)
-include $(foreach s, $(SRCS:.cc=.d), $(BUILD_DIR)/profile/$s)
-include $(foreach s, $(SRCS:.cc=.d), $(BUILD_DIR)/dynamic/$s)
