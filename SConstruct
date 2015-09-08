#/*
# * Copyright (c) 2012-2015, TU Eindhoven
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
# * Authors: Sven Goossens
# *
# */

env = Environment()

def CheckLib(options):
    for option in options:
        if conf.CheckLib(options):
            return option
        print("None of these libraries seem to exist within the library path: " + str(options))
        Exit(1)
    return None


def CleanAction(env, action):
    """ Add custom clean action, executed when the -c flag is used. """
    if env.GetOption('clean'):
        if len(COMMAND_LINE_TARGETS) == 0:
            Execute(action)

import os
GCOVFLAGS = Split('')
if os.environ.get('COVERAGE', '0') == '1':
    GCOVFLAGS = Split('-fprofile-arcs -ftest-coverage')

WARNFLAGS = Split("""-W -pedantic-errors -Wextra -Werror -Wformat -Wformat-nonliteral -Wpointer-arith -Wcast-align -Wconversion -Wall -Werror""")

# Debugging flags.
DBGCXXFLAGS = Split('-ggdb -g3')
OPTCXXFLAGS = Split('-O2')
# Include flag to allow include directives relative to the src folder
CXXPATH = Split('-iquote src')
ADDCXXFLAGS = Split('-std=c++0x')

# Sum up the flags for the implicit rules.
CXXFLAGS = WARNFLAGS + DBGCXXFLAGS + OPTCXXFLAGS + ADDCXXFLAGS + CXXPATH + GCOVFLAGS

env.Replace(CXXFLAGS=CXXFLAGS)
env.Replace(LDFLAGS='-Wall')

conf = Configure(env)
if os.environ.get('COVERAGE', '0') == '1':
    gcov = CheckLib(['gcov'])

lxerces = CheckLib(['xerces-c'])

core = Glob('src/*.cc')
cli = Glob('src/cli/*.cc')
xmlparser = Glob('src/xmlparser/*.cc')
libdrampower = Glob('src/libdrampower/*.cc')

default = []
default.append(env.Program('drampower', source=[core, cli, xmlparser]))
default.append(env.StaticLibrary('src/libdrampower.a', source=[core, libdrampower]))
default.append(env.StaticLibrary('src/libdrampowerxml.a', source=[core, libdrampower, xmlparser]))
env.Default(default)

env.Command(target='runtest', source='traces', action=['python test/test.py -v'])
env.Command(target='traces.zip', source='', action=['wget --quiet --output-document=traces.zip https://github.com/Sv3n/DRAMPowerTraces/archive/master.zip'])
env.Command(target='traces', source='traces.zip', action=['unzip traces.zip && mkdir -p traces && mv DRAMPowerTraces-master/traces/* traces/ && rm -rf DRAMPowerTraces-master'])

CleanAction(env, ['make -C test/libdrampowertest clean'])
CleanAction(env, ['rm traces.zip'])
