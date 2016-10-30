#!/usr/bin/env python
import unittest
import subprocess
import os
import fnmatch
import tempfile
import gzip
import multiprocessing

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
    def test_make_completes_returns_0(self):
        """ 'make' should return 0 """
        makejobs = "-j" + str(multiprocessing.cpu_count())
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', makejobs], stdout=devnull), 0)


class TestUsingBuildResult(unittest.TestCase):
    def buildDRAMPower(self):
        makejobs = "-j" + str(multiprocessing.cpu_count())
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', makejobs], stdout=devnull), 0)

    def setUp(self):
        self.buildDRAMPower()
        self.tempFileHandle, self.tempFileName = tempfile.mkstemp()
        os.close(self.tempFileHandle)
        self.tempFiles = [self.tempFileName]

    def getFilteredOutput(self, fName):
        with open(fName, 'r') as f:
            lines = f.readlines()
        return [x for x in lines if not x.startswith('*') and len(x) > 1]

    def getResults(self, fName):
        with open(fName, 'r') as f:
            lines = f.readlines()
        return [x for x in lines if x.startswith('Total Trace Energy')  or x.startswith('Average Power')]

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
            subprocess.call(cmd, stdout=f)

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
        self.assertEqual(subprocess.call(['./drampower'], stdout=devnull), 1)

    def get_LPDDR2_1066_trace_file(self):
        cmdTrace = extractFileToTmpFile('test/data/LPDDR2-1066.commands.trace.gz')
        self.tempFiles.append(cmdTrace)
        return cmdTrace

    def get_LPDDR2_1066_short_trace_file(self):
        cmdTrace = extractFileToTmpFile('test/data/LPDDR2-1066.commands.trace.gz')
        self.tempFiles.append(cmdTrace)
        return cmdTrace

    def test_LPDDR2_1066_matches_reference(self):
        """ drampower output for an LPDDR2-1066 trace matches reference """
        cmdTrace = self.get_LPDDR2_1066_trace_file()
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', cmdTrace]
        self.run_and_compare_to_reference(cmd, 'test/reference/test_LPDDR2_1066_matches_reference.out')

    def test_LPDDR2_1066_short_matches_reference(self):
        """ drampower output for an LPDDR2-1066 trace matches reference """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', 'test/data/LPDDR2-1066_short.commands.trace']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_LPDDR2_1066_short_matches_reference.out')

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

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_refresh(self):
        """ drampower output for REFB trace matches reference """
        refBCmdTrace = 'test/data/REFB.commands.trace'
        cmd = ['./drampower', '-m', 'memspecs/modified_MICRON_1Gb_DDR3-1600_8bit_G_3s.xml', '-c', refBCmdTrace]
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_refresh.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_25(self):
        """Bank-wise drampower output for MICRON_1Gb_DDR3-1600_8bit_G_3s with the jpegencode transaction trace with Rho = 25% """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G_3s.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-b', '25']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_25_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_50(self):
        """Bank-wise drampower output for MICRON_1Gb_DDR3-1600_8bit_G_3s with the jpegencode transaction trace with Rho = 50% """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G_3s.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-b', '50']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_50_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_75(self):
        """Bank-wise drampower output for MICRON_1Gb_DDR3-1600_8bit_G_3s with the jpegencode transaction trace with Rho = 75% """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G_3s.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-b', '75']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_75_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_100(self):
        """Bank-wise drampower output for MICRON_1Gb_DDR3-1600_8bit_G_3s with the jpegencode transaction trace with Rho = 100% """
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G_3s.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-b', '100']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3_1600_8bit_G_3s_bankwise_Rho_100_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_0(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 0 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','0']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_0_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_0(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 0 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','0']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_0_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_0(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 0 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','0']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_0_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_0(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 0 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','0']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_0_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_1(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 1 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','1']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_1_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_1(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 1 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','1']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_1_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_1(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 1 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','1']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_1_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_1(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 1 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','1']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_1_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_2(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 2 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','2']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_2_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_2(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 2 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','2']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_2_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_2(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 2 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','2']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_2_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_2(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 2 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','2']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_2_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_3(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 3 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','3']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_3_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_3(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 3 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','3']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_3_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_3(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 3 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','3']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_3_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_3(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 3 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','3']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_3_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_4(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 4 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','4']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_4_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_4(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 4 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','4']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_4_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_4(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 4 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','4']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_4_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_4(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 4 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','4']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_4_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_5(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 5 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','5']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_5_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_5(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 5 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','5']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_5_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_5(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 5 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','5']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_5_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_5(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 5 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','5']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_5_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_6(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 6 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','6']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_6_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_6(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 6 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','6']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_6_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_6(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 6 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','6']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_6_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_6(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 6 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','6']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_6_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_25_pasr_7(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 25% and PASR mode = 7 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,25' , '-pasr','7']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_25_pasr_7_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_50_pasr_7(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 50% and PASR mode = 7 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,50' , '-pasr','7']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_50_pasr_7_reference.out')
    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_75_pasr_7(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 75% and PASR mode = 7 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,75' , '-pasr','7']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_75_pasr_7_reference.out')

    def test_MICRON_1Gb_DDR3_1600_8bit_G_3s_Sigma_100_pasr_7(self):
        """Bank-wise drampower output for MICRONMICRON_1Gb_DDR3-1600_8bit_G with the PASR commands trace with Sigma = 100% and PASR mode = 7 """''
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR3-1600_8bit_G.xml',
                              '-c', 'test/data/PASR.commands.trace', '-b', '100,100' , '-pasr','7']
        self.run_and_compare_to_reference(cmd, 'test/reference/test_MICRON_1Gb_DDR3-1600_8bit_G_Sigma_100_pasr_7_reference.out')

    def test_broken_trace(self):
        """ running drampower with an invalid trace returns 0 """
        self.assertEqual(subprocess.call(['./drampower',
                                          '-m', 'memspecs/MICRON_1Gb_DDR2-800_16bit_H.xml',
                                          '-c', 'test/data/warnings.trace'], stdout=devnull, stderr=devnull), 0)


class TestLibDRAMPower(TestUsingBuildResult):
    testPath = 'test/libdrampowertest'

    def buildLibDRAMPowerExecutables(self, useXerces=True):
        xerces = 'USE_XERCES=%d' % (1 if useXerces else 0)
        coverage = 'COVERAGE=%d' % (1 if inCoverageTest() else 0)
        self.assertEqual(subprocess.call(['make', '-f', TestLibDRAMPower.testPath + '/Makefile', 'DRAMPOWER_PATH=.', xerces, coverage], stdout=devnull), 0)

    def test_libdrampower_with_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutables()

    def test_libdrampower_without_xerces_test_builds(self):
        """ libdrampower-based executable build should return 0 """
        self.buildLibDRAMPowerExecutables(useXerces=False)
        if inCoverageTest():
            os.unlink('lib_test.gcno')

    def test_libdrampower_output_matches_reference(self):
        self.maxDiff = None  # Show full diff on error.
        """ libdrampower-based executable output should match reference """
        self.buildLibDRAMPowerExecutables()

        with open(self.tempFileName, 'w') as f:
            self.assertEqual(subprocess.call([TestLibDRAMPower.testPath + '/library_test',
                                              'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml'],
                                              stdout=f, stderr=devnull), 0)
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

    def test_window_example(self):
        """ libdrampower-based window example output should match drampower output for DDR2-1066 with the window command trace """
        self.buildLibDRAMPowerExecutables()
        with open(self.tempFileName, 'w') as f:
            self.assertEqual(subprocess.call(['./drampower', '-m',  'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml', '-c', TestLibDRAMPower.testPath + '/window.trace'], stdout=f), 0)

        drampower = self.getResults(self.tempFileName)

        with open(self.tempFileName, 'w') as f:
            self.assertEqual(subprocess.call([TestLibDRAMPower.testPath + '/window_example', 'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml'], stdout=f), 0)

        libdrampower = self.getResults(self.tempFileName)

        self.assertEqual(drampower, libdrampower)

class TestClean(unittest.TestCase):
    def setUp(self):
        self.assertEqual(subprocess.call(['make', '-f', 'Makefile', 'clean'], stdout=devnull), 0)

    def test_make_clean_removes_compiler_output(self):
        """ 'make clean' should remove all .o and .a and .d files """
        count = 0
        pattern = '*.[oad]'
        for root, dirnames, files in os.walk('.'):
            for f in files:
                count += 1 if fnmatch.fnmatch(f, pattern) and not f.startswith('conftest') else 0
        self.assertEqual(count, 0, msg=self.shortDescription())

if __name__ == '__main__':
    with open(os.devnull, 'wb') as devnull:
        unittest.main()
