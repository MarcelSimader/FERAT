#/bin/bash
ifndef BACKEND
BACKEND=glucose
endif

VERSION=v970
CFLAGS+=-DVERSION=\"$(VERSION)-$(BACKEND)\"

DEPFILE	 = .depends
DEPTOKEN = '\# MAKEDEPENDS'
DEPFLAGS = -Y -f $(DEPFILE) -s $(DEPTOKEN)
CSRCS    = $(wildcard *.cc)
COBJS    = $(CSRCS:.cc=.o)
LIBS =

ifeq ($(BACKEND),cryptominisat)
	LIBD = -L./cryptominisat/build/lib
	LIBS+=-l:libcryptominisat5.so
	CFLAGS+=-I./cryptominisat/build/include/
	CSRCS+=ipasir/ipasir-cryptominisat.cc
endif

ifeq ($(BACKEND),glucose)
	LIBD = -L./glucose-syrup/simp
	LIBS+=-l:libglucose.a
	CFLAGS+=-I./glucose-syrup/
	CSRCS+=ipasir/ipasir-glucose4.cc
endif

ifeq ($(BACKEND),lingeling)
	LIBD = -L./lingeling/
	LIBS+=-l:liblgl.a
	CFLAGS+=-I./lingeling/
	CSRCS+=ipasir/ipasir-lingeling.cc
endif

ifeq ($(BACKEND),picosat)
	LIBD = -L./picosat/
	LIBS+=-l:libpicosat.a
	CFLAGS+=-I./picosat/
	CSRCS+=ipasir/ipasir-picosat.cc
endif

ifeq ($(BACKEND),abcdsat)
	LIBD = -L./abcdsat/build/release/lib
	LIBS+=-l:libabcdsat.a
	CFLAGS+=-I./abcdsat/
	CSRCS+=ipasir/ipasir-abcdsat.cc
endif


LIBS += -lz
CXX?=g++

# PROF=1

# CACHEGRIND=1

ifdef CACHEGRIND
CFLAGS+=-g
LNFLAGS+=-g
endif

ifdef PROF
CFLAGS+=-pg
LNFLAGS+=-pg
endif

ifdef DBG
CFLAGS+=-O0
CFLAGS+=-ggdb
CFLAGS+=-DDEBUG
else
CFLAGS+=-O3
CFLAGS+=-DNDEBUG
endif

CFLAGS += -Wall -Wno-parentheses
CFLAGS+=-std=c++11

ifdef STATIC
CFLAGS+=-static
LNFLAGS+=-static
endif

.PHONY: glucose cryptominisat lingeling picosat abcdsat

all: mysolver
	@cp mysolver mysolver_$(VERSION)-$(BACKEND)
	@cp mysolver ijtihad

mysolver: $(BACKEND) $(COBJS)
	@echo Linking: $@
	$(CXX) -o $@ $(COBJS) $(LNFLAGS) $(LIBD) $(LIBS)

glucose:
	echo "Building Glucose";
	cd ./glucose-syrup/simp/ && $(MAKE) libr && mv lib_release.a libglucose.a && cd ../..

cryptominisat:
	echo "Building CryptoMiniSAT";
	cd ./cryptominisat/build && $(MAKE) libcryptominisat5 -j4 CXX=$(CXX)

# This fix is to avoid Lingeling constantly re-compiling all of its sources, since one of
# the dependencies listed in its Makefile is the Makefile itself. Problem is, the
# `configure` command constantly changes the creation date of said file...
lingeling:
	echo "Building Lingeling"; \
	cd ./lingeling; \
	if [ -n "${FERAT_RECONFIGURE}" -o ! -e "./makefile" ]; then \
		echo "Reconfiguring..."; ./configure.sh; \
	fi; \
	$(MAKE); \
	cd ..

picosat:
	echo "Building PicoSAT";
	cd ./picosat/ && ./configure.sh && $(MAKE) && cd ..

abcdsat:
	echo "Building abcdSAT";
	cd ./abcdsat/ && $(MAKE) lr && cd ..



depend:
	rm -f $(DEPFILE)
	@echo $(DEPTOKEN) > $(DEPFILE)
	makedepend $(DEPFLAGS) -- $(CFLAGS) -- $(CSRCS)

## Build rule
%.o:	%.cc
	@echo Compiling: $@
	@$(CXX) $(CFLAGS) -c -o $@ $<

## Clean rule
clean:
	@rm -f mysolver mysolver.exe $(COBJS)
	@rm -f $(DEPFILE)
	cd ./glucose-syrup/simp/ && $(MAKE) clean && cd ../..;

### Makefile ends here
sinclude $(DEPFILE)
