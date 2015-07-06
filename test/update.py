#!/usr/bin/env python
import subprocess

from test import extractFileToTmpFile


class TestUpdate():
    def run_and_use_as_reference(self, cmd, refFile):
        with open(refFile, 'w') as f:
            subprocess.call(cmd, stdout=f)

    def get_LPDDR2_1066_trace_file(self):
        cmdTrace = extractFileToTmpFile('test/data/LPDDR2-1066.commands.trace.gz')
        return cmdTrace

    def newReferenceFiles(self):
        cmdTrace = extractFileToTmpFile('test/data/LPDDR2-1066.commands.trace.gz')
        cmd = ['./drampower', '-m', 'memspecs/MICRON_1Gb_DDR2-1066_16bit_H.xml', '-c', 'traces/commands.trace']
        self.run_and_use_as_reference(cmd, 'test/reference/test_commands_trace_output_matches_reference.out')

        cmdTrace = self.get_LPDDR2_1066_trace_file()
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', cmdTrace]
        self.run_and_use_as_reference(cmd, 'test/reference/test_LPDDR2_1066_matches_reference.out')

        cmdTrace = self.get_LPDDR2_1066_trace_file()
        cmd = ['./drampower', '-m', 'memspecs/MICRON_2Gb_LPDDR2-1066-S4_16bit_A.xml', '-c', cmdTrace, '-r']
        self.run_and_use_as_reference(cmd, 'test/reference/test_LPDDR2_1066_termination_matches_reference.out')

        cmd = ['./drampower', '-m', 'memspecs/MICRON_4Gb_LPDDR3-1333_32bit_A.xml',
                              '-t', 'traces/mediabench-jpegencode.trace']
        self.run_and_use_as_reference(cmd, 'test/reference/test_transaction_scheduler.out')

        cmd = ['./drampower', '-m', 'memspecs/MICRON_4Gb_LPDDR3-1333_32bit_A.xml',
                              '-t', 'traces/mediabench-jpegencode.trace', '-p', '2']
        self.run_and_use_as_reference(cmd, 'test/reference/test_transaction_scheduler_with_self_refresh.out')


if __name__ == '__main__':
    t = TestUpdate()
    t.newReferenceFiles()
