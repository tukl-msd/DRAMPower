.PHONY: clean pretty test traces

# CXX is a predefined variable of GNU Make. Thus, the conditional assignment
# here may not take effect. If you want to force another compiler here use
# ':=' instead of '?='.
CXX ?= g++

ifeq ($(COVERAGE),1)
	GCOVFLAGS := -fprofile-arcs -ftest-coverage
else
	GCOVFLAGS :=
endif

# Debugging flags.
DBGCXXFLAGS ?= -g ${GCOVFLAGS}

# Warning flags
WARNFLAGS := -W -pedantic-errors -Wextra -Werror \
             -Wformat -Wformat-nonliteral -Wpointer-arith \
             -Wcast-align -Wconversion -Wall -Werror

CXXFLAGS := -O ${WARNFLAGS} ${DBGCXXFLAGS} ${OPTCXXFLAGS} -std=c++0x

