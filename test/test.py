#!/usr/bin/env python
import unittest
import subprocess
import os
import fnmatch
import tempfile
import gzip

devnull = None

def inCoverageTest():
    """ Returns true if we are doing a test with gcov enabled """
    return os.environ.get('COVERAGE', '0') == '1'

def extractFileToTmpFile(compressedFile):
    tempFileHandle, tempFileName = tempfile.mkstemp()
    os.close(tempFileHandle)
    with open(tempFileName, 'wb') as f:
        with gzip.open(compressedFile, 'rb') as src:
            f.write(src.read())
    return tempFileName

class TestBuild(unittest.TestCase):
    def test_make_wo_args_completes_returns_0(self):
        """ 'make -j4' should return 0 """
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', '-j4'], stdout = devnull), 0)

class TestUsingBuildResult(unittest.TestCase):
    def buildDRAMPower(self):
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', '-j4'], stdout = devnull), 0)

    def setUp(self):
        self.buildDRAMPower()
        self.tempFileHandle, self.tempFileName = tempfile.mkstemp()
        os.close(self.tempFileHandle)
        self.tempFiles = [self.tempFileName]

    def getFilteredOutput(self, fName):
        with open(fName, 'r') as f:
            lines = f.readlines()
        return [x for x in lines if not x.startswith('*') and len(x) > 1]

    def tearDown(self):
        for f in self.tempFiles:
            try:
                os.unlink(f)
            except:
                # We don't really care, since it is a /tmp file anyway.
                pass

class TestOutput(TestUsingBuildResult):
    def run_and_compare_to_reference(self, cmd, referenceFile):
        self.maxDiff = None  # Show full diff on error.
        with open(self.tempFileName, 'w') as f:
            subprocess.call(cmd, stdout = f)

        new = self.getFilteredOutput(self.tempFileName)
        ref = self.getFilteredOutput(referenceFile)
        self.assertListEqual(new, ref)

    def test_commands_trace_output_matches_reference(self):
        """ drampower output for commands.trace example should be equal to test_commands_trace_output_matches_reference.out
            Ignores all lines starting with * and empty lines. All remaining lines should be equal.
            Reference output is based on commit 4981a9856983b5d0b73778a00c43adb4cac0fcbc.
        """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml', '-c', 'traces/commands.trace']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_commands_trace_output_matches_reference.out')

    def test_no_arguments_error(self):
        """ running drampower w/o arguments returns 1 """
        self.assertEqual(subprocess.call(['./drampower'], stdout = devnull), 1)

    def get_LPDDR2_1066_trace_file(self):
        cmdTrace = extractFileToTmpFile('test/data/LPDDR2-1066.commands.trace.gz')
        self.tempFiles.append(cmdTrace)
        return cmdTrace

    def test_LPDDR2_1066_matches_reference(self):
        """ drampower output for an LPDDR2-1066 trace matches reference """
        cmdTrace = self.get_LPDDR2_1066_trace_file()
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', cmdTrace]
        self.run_and_compare_to_reference(cmd, 'test/reference/test_LPDDR2_1066_matches_reference.out')

    def test_LPDDR2_1066_termination_matches_reference(self):
        """ drampower output for an LPDDR2-1066 trace with termination power enabled matches reference """
        cmdTrace = self.get_LPDDR2_1066_trace_file()
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', cmdTrace, '-r']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_LPDDR2_1066_termination_matches_reference.out')

    def test_transaction_scheduler(self):
        """ drampower output for LPDDR3-1333 with the jpegencode transaction trace matches reference """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_4Gb_LPDDR3-1333_32bit_A.xml',
                              '-t', 'traces/mediabench-jpegencode.trace']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_transaction_scheduler.out')

    def test_transaction_scheduler_with_self_refresh(self):
        """ drampower output for LPDDR3-1333 with the jpegencode transaction trace matches reference """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_4Gb_LPDDR3-1333_32bit_A.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-p', '2']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_transaction_scheduler_with_self_refresh.out')

    def test_broken_trace(self):
        """ running drampower with an invalid trace returns 0 """
        self.assertEqual(subprocess.call(['./drampower',
                                          '-m', 'memspecs/MICRON_1Gb_DDR2-800_16bit_H.xml',
                                          '-c', 'test/data/warnings.trace'], stdout=devnull), 0)

class TestLibDRAMPower(TestUsingBuildResult):
    testPath = 'test/libdrampowertest'

    def buildLibDRAMPowerExecutable(self, useXerces = True):
        xerces = 'USE_XERCES=%d' % (1 if useXerces else 0)
        coverage = 'COVERAGE=%d' % (1 if inCoverageTest() else 0)
        self.assertEqual(subprocess.call(['make', '-f', TestLibDRAMPower.testPath + '/Makefile', 'DRAMPOWER_PATH=.', xerces, coverage], stdout = devnull), 0)

    def test_libdrampower_with_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutable()

    def test_libdrampower_without_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutable(useXerces = False)
        if inCoverageTest():
            os.unlink('lib_test.gcno')

    def test_libdrampower_output_matches_reference(self):
        """ libdrampower-based executable output should match reference """
        self.buildLibDRAMPowerExecutable()

        with open(self.tempFileName, 'w') as f:
            self.assertEqual(subprocess.call([TestLibDRAMPower.testPath + '/library_test',
                                              'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml'], stdout = f), 0)
            try:
                """ Copy coverage statistics to test subfolder. Otherwise the coverage tool gets confused. """
                if inCoverageTest():
                    for cp in ['lib_test.gcno', 'lib_test.gcda']:
                        os.rename(cp, '%s/%s' % (TestLibDRAMPower.testPath, cp))
            except:
                pass

        new = self.getFilteredOutput(self.tempFileName)
        ref = self.getFilteredOutput('test/reference/test_libdrampower_output_matches_reference.out')
        self.assertListEqual(new, ref)

class TestClean(unittest.TestCase):
    def setUp(self):
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', 'clean'], stdout = devnull), 0)

    def test_make_clean_removes_compiler_output(self):
        """ 'make clean' should remove all .o and .a and .d files """
        count = 0
        pattern = '*.[oad]'
        for root, dirnames, files in os.walk('.'):
            for f in files:
                count += 1 if fnmatch.fnmatch(f, pattern) else 0
        self.assertEqual(count, 0, msg = self.shortDescription())

if __name__ == '__main__':
    with open(os.devnull, 'wb') as devnull:
        unittest.main()
