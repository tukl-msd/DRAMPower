#/*
# * Copyright (c) 2012-2014, TU Delft
# * Copyright (c) 2012-2014, TU Eindhoven
# * Copyright (c) 2012-2014, TU Kaiserslautern
# * All rights reserved.
# *
# * Redistribution and use in source and binary forms, with or without
# * modification, are permitted provided that the following conditions are
# * met:
# *
# * 1. Redistributions of source code must retain the above copyright
# * notice, this list of conditions and the following disclaimer.
# *
# * 2. Redistributions in binary form must reproduce the above copyright
# * notice, this list of conditions and the following disclaimer in the
# * documentation and/or other materials provided with the distribution.
# *
# * 3. Neither the name of the copyright holder nor the names of its
# * contributors may be used to endorse or promote products derived from
# * this software without specific prior written permission.
# *
# * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# *
# * Authors: Karthik Chandrasekar, Benny Akesson, Sven Goossens
# *
# */

include common.mk

# Name of the generated binary.
BINARY := drampower
LIBS := src/libdrampower.a src/libdrampowerxml.a

# Identifies the source files and derives name of object files.

CLISOURCES := src/TraceParser.cc $(wildcard src/cli/*.cc)
LIBSOURCES := $(wildcard src/libdrampower/*.cc) \
			  src/CommandAnalysis.cc \
			  src/CAHelpers.cc \
			  src/CmdHandlers.cc \
			  src/MemArchitectureSpec.cc\
			  src/MemCommand.cc\
			  src/MemoryPowerModel.cc\
			  src/MemorySpecification.cc\
			  src/MemPowerSpec.cc\
			  src/MemTimingSpec.cc\
			  src/Parameter.cc\
			  src/Parametrisable.cc\
			  src/MemBankWiseParams.cc

XMLPARSERSOURCES := $(wildcard src/xmlparser/*.cc)
ALLSOURCES := $(wildcard src/cli/*.cc) $(wildcard src/*.cc) $(wildcard src/xmlparser/*.cc) $(wildcard src/libdrampower/*.cc)
ALLHEADERS := $(wildcard src/*.h) $(wildcard src/xmlparser/*.h) $(wildcard src/libdrampower/*.h)

CLIOBJECTS := ${CLISOURCES:.cc=.o}
XMLPARSEROBJECTS := ${XMLPARSERSOURCES:.cc=.o}
LIBOBJECTS := ${LIBSOURCES:.cc=.o}
ALLOBJECTS := ${ALLSOURCES:.cc=.o}

DEPENDENCIES := ${ALLSOURCES:.cc=.d}

# Warning flags for deprecated files
DEPWARNFLAGS := -W -pedantic-errors -Wextra -Werror \
             -Wformat -Wformat-nonliteral -Wpointer-arith \
             -Wcast-align -Wall -Werror

# Sum up the flags.
DEPCXXFLAGS := -O ${DEPWARNFLAGS} ${DBGCXXFLAGS} ${OPTCXXFLAGS} -std=c++0x

# Linker flags.
LDFLAGS := -Wall -lstdc++

##########################################
# Xerces settings
##########################################

XERCES_ROOT ?= /usr
XERCES_INC := $(XERCES_ROOT)/include
XERCES_LIB := $(XERCES_ROOT)/lib
XERCES_LDFLAGS := -L$(XERCES_LIB) -lxerces-c

##########################################
# Targets
##########################################

all: ${BINARY} src/libdrampower.a parserlib traces

$(BINARY): ${XMLPARSEROBJECTS} ${CLIOBJECTS} src/libdrampower.a
	$(CXX) ${CXXFLAGS} $(LDFLAGS) -o $@ $^ -Lsrc/ $(XERCES_LDFLAGS) -ldrampower

# From .cpp to .o. Dependency files are generated here
%.o: %.cc
	$(CXX) ${CXXFLAGS} -MMD -MF $(subst .o,.d,$@) -iquote src -o $@ -c $<

src/libdrampower.a: ${LIBOBJECTS}
	ar -cvr src/libdrampower.a ${LIBOBJECTS}

parserlib: ${XMLPARSEROBJECTS}
	ar -cvr src/libdrampowerxml.a ${XMLPARSEROBJECTS}

clean:
	$(RM) $(ALLOBJECTS) $(DEPENDENCIES) $(BINARY) $(LIBS)
	$(MAKE) -C test/libdrampowertest clean
	$(RM) traces.zip

coverageclean:
	$(RM) ${ALLSOURCES:.cc=.gcno} ${ALLSOURCES:.cc=.gcda}
	$(MAKE) -C test/libdrampowertest coverageclean

pretty:
	uncrustify -c src/uncrustify.cfg $(ALLSOURCES) --no-backup
	uncrustify -c src/uncrustify.cfg $(ALLHEADERS) --no-backup

test: traces
	python test/test.py -v

traces.zip:
	wget --quiet --output-document=traces.zip https://github.com/Sv3n/DRAMPowerTraces/archive/master.zip

traces: traces.zip
	unzip traces.zip && mkdir -p traces && mv DRAMPowerTraces-master/traces/* traces/ && rm -rf DRAMPowerTraces-master

LCOV_OUTDIR = coverage_report
coveragecheck: coveragecheckclean
ifeq ($(CXX),g++)
	hash lcov 2>/dev/null || { echo >&2 "lcov could not be found. Aborting."; exit 1; }
	COVERAGE=1 $(MAKE) clean || { echo >&2 "make clean failed. Aborting."; exit 1; }
	COVERAGE=1 $(MAKE) || { echo >&2 "make failed. Aborting."; exit 1; }
	lcov --no-external -c -i -d . -o .coverage.base
	COVERAGE=1 $(MAKE) test || { echo >&2 "make test failed. Aborting."; exit 1; }
	lcov --no-external -c -d . -o .coverage.run
	lcov -d . -a .coverage.base -a .coverage.run -o .coverage.total
	genhtml -q --no-branch-coverage -o $(LCOV_OUTDIR) .coverage.total
	rm -f .coverage.base .coverage.run .coverage.total
else
	@{ echo >&2 "The coveragecheck rule is not implemented for $(CXX). Aborting."; exit 1; }
endif

coveragecheckclean:
	rm -rf $(LCOV_OUTDIR)


-include $(DEPENDENCIES)
