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


# Name of the generated binary.
BINARY := drampower
LIBS := src/libdrampower.a src/libdrampowerxml.a

# Identifies the source files and derives name of object files.
CLISOURCES := $(wildcard src/*.cc) $(wildcard src/cli/*.cc)
XMLPARSERSOURCES := $(wildcard src/xmlparser/*.cc)
LIBSOURCES := $(wildcard src/*.cc src/libdrampower/*.cc)
ALLSOURCES := $(wildcard src/cli/*.cc) $(wildcard src/*.cc) $(wildcard src/xmlparser/*.cc) $(wildcard src/libdrampower/*.cc)
ALLHEADERS := $(wildcard src/*.h) $(wildcard src/xmlparser/*.h) $(wildcard src/libdrampower/*.h)

CLIOBJECTS := ${CLISOURCES:.cc=.o}
XMLPARSEROBJECTS := ${XMLPARSERSOURCES:.cc=.o}
LIBOBJECTS := ${LIBSOURCES:.cc=.o}
ALLOBJECTS := ${ALLSOURCES:.cc=.o}

DEPENDENCIES := ${ALLSOURCES:.cc=.d}

##########################################
# Compiler settings
##########################################

# State what compiler we use.
CXX := g++

ifeq ($(COVERAGE),1)
	GCOVFLAGS := -fprofile-arcs -ftest-coverage
else
	GCOVFLAGS :=
endif

# Optimization flags. Usually you should not optimize until you have finished
# debugging, except when you want to detect dead code.
OPTCXXFLAGS ?=

# Debugging flags.
DBGCXXFLAGS ?= -g ${GCOVFLAGS}

# Common warning flags shared by both C and C++.
WARNFLAGS := -W -pedantic-errors -Wextra \
             -Wformat -Wformat-nonliteral -Wpointer-arith \
             -Wcast-align -Wconversion -Wall 

##########################################
# Xerces settings
##########################################

XERCES_ROOT ?= /usr
XERCES_INC := $(XERCES_ROOT)/include
XERCES_LIB := $(XERCES_ROOT)/lib
XERCES_LDFLAGS := -L$(XERCES_LIB) -lxerces-c

# Sum up the flags.
CXXFLAGS := -O ${WARNFLAGS} -I${XERCES_INC} ${DBGCXXFLAGS} ${OPTCXXFLAGS} -std=c++0x

# Linker flags.
LDFLAGS := -Wall ${XERCES_LDFLAGS}

##########################################
# Targets
##########################################

all: ${BINARY} lib parserlib traces

$(BINARY): ${XMLPARSEROBJECTS} ${CLIOBJECTS}
	$(CXX) ${CXXFLAGS} $(LDFLAGS) -o $@ $^ $(XERCES_LDFLAGS)

# From .cpp to .o. Dependency files are generated here
%.o: %.cc
	$(CXX) ${CXXFLAGS} -MMD -MF $(subst .o,.d,$@) -iquote src -o $@ -c $<

lib: ${LIBOBJECTS}
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

.PHONY: clean pretty test traces

-include $(DEPENDENCIES)
