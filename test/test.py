#!/usr/bin/env python
import unittest
import subprocess
import os
import fnmatch
import tempfile

class TestBuild(unittest.TestCase):
    def test_make_wo_args_completes_returns_0(self):
        """ 'make -j4' should return 0 """
        with open('/dev/null', 'w') as f:
            self.assertEqual(subprocess.call(['make', '-f', 'Makefile', '-j4'], stdout = f), 0)

class TestUsingBuildResult(unittest.TestCase):
    def buildDRAMPower(self):
        with open('/dev/null', 'w') as f:
            self.assertEqual(subprocess.call(['make', '-f', 'Makefile', '-j4'], stdout = f), 0)
        self.tempFileHandle, self.tempFileName = tempfile.mkstemp()
        os.close(self.tempFileHandle)

    def setUp(self):
        self.buildDRAMPower()

    def getFilteredOutput(self, fName):
        with open(fName, 'r') as f:
            lines = f.readlines()
        return [x for x in lines if not x.startswith('*') and len(x) > 1]

    def tearDown(self):
        try:
            os.unlink(self.tempFileName)
        except:
            # We don't really care, since it is a /tmp file anyway.
            pass

class TestOutput(TestUsingBuildResult):
    def test_commands_trace_output_matches_reference(self):
        """ drampower output for commands.trace example should be equal to test_commands_trace_output_matches_reference.out
            Ignores all lines starting with * and empty lines. All remaining lines should be equal.
            Reference output is based on commit 4981a9856983b5d0b73778a00c43adb4cac0fcbc.
        """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml', '-c', 'traces/commands.trace']
        with open(self.tempFileName, 'w') as f:
            subprocess.call(cmd, stdout = f)

        new = self.getFilteredOutput(self.tempFileName)
        ref = self.getFilteredOutput('test/test_commands_trace_output_matches_reference.out')
        self.assertListEqual(new, ref)

class TestLibDRAMPower(TestUsingBuildResult):
    testPath = 'test/libdrampowertest'

    def buildLibDRAMPowerExecutable(self, useXerces = True):
        with open('/dev/null', 'w') as f:
            xerces = 'USE_XERCES=%d' % (1 if useXerces else 0)
            self.assertEqual(subprocess.call(['make', '-f', TestLibDRAMPower.testPath + '/Makefile', 'DRAMPOWER_PATH=.', xerces], stdout = f), 0)

    def test_libdrampower_with_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutable()

    def test_libdrampower_without_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutable(useXerces = False)

    def test_libdrampower_output_matches_reference(self):
        """ libdrampower-based executable output should match reference """
        self.buildLibDRAMPowerExecutable()

        with open(self.tempFileName, 'w') as f:
            self.assertEqual(subprocess.call([TestLibDRAMPower.testPath + '/library_test',
                                              'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml'], stdout = f), 0)

        new = self.getFilteredOutput(self.tempFileName)
        ref = self.getFilteredOutput('test/test_libdrampower_output_matches_reference.out')
        self.assertListEqual(new, ref)

class TestClean(unittest.TestCase):
    def setUp(self):
        with open('/dev/null', 'w') as f:
            self.assertEqual(subprocess.call(['make', '-f', 'Makefile', 'clean'], stdout = f), 0)

    def test_make_clean_removes_compiler_output(self):
        """ 'make clean' should remove all .o and .a and .d files """
        count = 0
        pattern = '*.[oad]'
        for root, dirnames, files in os.walk('.'):
            for f in files:
                count += 1 if fnmatch.fnmatch(f, pattern) else 0
        self.assertEqual(count, 0, msg = self.shortDescription())

if __name__ == '__main__':
    unittest.main()