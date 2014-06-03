#/*
# * Copyright (c) 2012 TU Delft, TU Eindhoven and TU Kaiserslautern.
# * All rights reserved.
# *
# * Licensed under BSD 3-Clause License
# *
# * Authors: Karthik Chandrasekar, Yonghui Li and Benny Akesson
# *
# */

# Name of the generated binary.
BINARY := drampower

# Identifies the source files and derives name of object files.
SOURCES := PowerCalc.cc $(wildcard src/*.cc)
OBJECTS := ${SOURCES:.cc=.o}
DEPENDENCIES := ${SOURCES:.cc=.d}

##########################################
# Compiler settings
##########################################

# State what compiler we use.
CXX := g++

# Optimization flags. Usually you should not optimize until you have finished
# debugging, except when you want to detect dead code.
OPTCXXFLAGS =

# Debugging flags.
DBGCXXFLAGS = -g

# Common warning flags shared by both C and C++.
WARNFLAGS := -W -Wall -pedantic-errors -Wextra -Werror \
             -Wformat -Wformat-nonliteral -Wpointer-arith \
             -Wcast-align -Wconversion

# Sum up the flags.
CXXFLAGS := -O #${WARNFLAGS} ${DBGCXXFLAGS} ${OPTCXXFLAGS}

# Linker flags.
LDFLAGS := -Wall

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

$(BINARY): ${OBJECTS}
	$(CXX) $(LDFLAGS) -o $@ $^ $(XERCES_LDFLAGS)

# From .cpp to .o. Dependency files are generated here
${OBJECTS}: %.o: %.cc
	$(CXX) ${CXXFLAGS} -MMD -MF $(subst .o,.d,$@) -o $@ -c $<


all: ${BINARY}

clean:
	$(RM) $(OBJECTS) $(DEPENDENCIES) $(BINARY)

.PHONY: clean

-include $(DEPENDENCIES)
