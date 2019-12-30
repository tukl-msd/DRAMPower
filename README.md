# DRAM Power Model (DRAMPower)
[![Build Status](https://travis-ci.org/ravenrd/DRAMPower.svg?branch=master)](https://travis-ci.org/ravenrd/DRAMPower)
[![Coverage Status](https://coveralls.io/repos/ravenrd/DRAMPower/badge.png?branch=master)](https://coveralls.io/r/ravenrd/DRAMPower?branch=master)
## 0. Releases

The last official release can be found here:
https://github.com/ravenrd/DRAMPower/releases/tag/4.0

The master branch of the repository should be regarded as the bleeding-edge version, which has all the latest features, but also all the latest bugs. Use at your own discretion.

## 1. Installation

Clone the repository, or download the zip file of the release you would like to use. The source code is available in src folder. [drampower.cc](src/cli/drampower.cc) file gives the user interface, where the user can specify the memory to be employed and the commandtrace to be analyzed. To build, use:
```bash
make -j4
```
This command will download a set of trace files from https://github.com/Sv3n/DRAMPowerTraces which can be used as test input for the tool.

## 2. Required Packages

The tool was verified on Ubuntu >= 14.04. For building a gcc or clang is required which supports C++11.

## 3. Directory Structure
 * src/: contains the source code of the DRAMPower tool that covers the power  model, the command scheduler and the trace analysis tool.
 * memspecs/   : contains the memory specification JSONs, which give the architectural, timing and current/voltage details for different DRAM memories.
 * traces/     : 1 sample command trace (after the installation / compilation)
 * test/       : contains test script and reference output

## 4. Trace Specification
### Command Traces
If the command-level interface is being used, a command trace can be logged in a file.
An example is given in ```traces/commands.trace```

The format it uses is: ```<timestamp>,<command>,<bank>```.
For example, "500,ACT,2", where ACT is the command and 2 is the bank. Timestamp is in clock cycles (cc), the list of supported commands is
mentioned in [MemCommand.h](src/MemCommand.h) and the bank is the target bank number. For non-bank-specific commands, bank can be set to 0. Rank need not be
specified. The timing correctness of the trace is not verified by the tool and is assumed to be accurate. However, warning messages are provided, to identify if the memory or bank state is inconsistent in the trace. A sample command trace is provided in the traces/ folder.

### Transaction Traces
This feature is obsolete and not supported any more. One can check out [commit](https://github.com/tukl-msd/DRAMPower/commit/0e24b8ebfa6144fc543d3acdcc3e6ad845dd98a9) to use this feature with an older version of DRAMPower, until which the feature is included. The usage and other details are documented. The future versions of DRAMPower will rely on simulators like [DRAMSys](https://www.jstage.jst.go.jp/article/ipsjtsldm/8/0/8_63/_article) and [Ramulator](https://github.com/CMU-SAFARI/ramulator) for this purpose.

## 5. Usage

[drampower.cc](src/cli/drampower.cc) is the main interface file, which accepts user inputs to specify memory to be employed and the command trace to be analyzed.

To list out the CLI options supported by DRAMPower, type
```bash
./drampower -h
DRAMPower
Usage: ./drampower [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  -r,--io_term                IO and Termination
  -v,--version                Display DRAMPower version information
  -m,--mem_spec TEXT:FILE REQUIRED
                              Memory specification file
  -c,--cmd_trace TEXT:FILE REQUIRED
                              Commands trace file
  -b,--bank_wise INT:INT in [0 - 100] x 2
                              Bank-wise mode 
                              ρ - ACT Standby bankwise power offset factor
                              σ - Self-Refresh bankwise power offset factor
  -s,--pasr INT:INT in [0 - 7] Needs: --bank_wise
                              Partial Array Self-Refresh mode 
```
The table below gives a detailed overview of the available CLI options.

| Option short | Option long  | Description                                |Usage    | Type |
| --------------|--------------|:------------------------------------------|:----------|:------|
| ```-r``` | ```--io_term```  | The user can optionally include IO and Termination power estimates (obtained from Micron's DRAM Power Calculator)  | Optional | Flag  |
| ```-m``` |```--mem_spec``` | The memory specification file (JSON format) for the corresponding memory to be analyzed. The file contains the necessary DRAM specific parameters to start power simulation.| Mandatory | String | 
| ```-c``` |```--cmd_trace```| The command trace file, which is a special file format for specifying memory controller commands. The syntactic details about command trace is described in Trace Specification/ Command Traces above .|  Mandatory | String |
| ```-b``` |```--bank_wise```| By default the DRAMPower calculated the power consumption for the complete DRAM as a whole. If the option is specified, a bank-wise granular power calculation is performed. The first argument is ρ, which is the ACT standby power factor which determines the minimum current when at least one bank is active. The second parameter is σ, which is partial array self-refresh power offset factor which determines the minimum current when 7 out of 8 banks are in self-refresh mode. Note that both ρ and σ are vendor-specific. Details about both the parameters are in [4] | Optional | 2 x Integers separated by space [0-100]|
| ```-s``` |```--pasr```     | Enables the Partial Array Self Refresh (PASR) mode. Note that this mode can only be enabled in the bank-wise (above) mode. The below table shows the possible PASR modes. <br><br><table><tbody><tr><th>Value&nbsp;&nbsp;</th><th align="left">Mode</th></tr><tr><td align="center">0</td><td align="left">Full Array in Self-Refresh</td></tr><tr><td align="center">1</td><td align="left">Half Array in Self-Refresh</td></tr><tr><td align="center">2</td><td align="left">Quarter Array in Self-Refresh</td></tr><tr><td align="center">3</td><td align="left">(1/8) Array in Self-Refresh</td></tr><tr><td align="center">4</td><td align="left">(3/4) Array in Self-Refresh</td></tr><tr><td align="center">5</td><td align="left">Half array in Self-Refresh</td></tr><tr><td align="center">6</td><td align="left">Quarter Array in Self-Refresh</td></tr><tr><td align="center">7</td><td align="left">(1/8) Array in Self-Refresh</td></tr></tbody></table> | Optional  | Integer [0-7]  |

## 6. Memory Specifications

Various sample memory specifications are given in the JSON files targeting DDR2/DDR3/DDR4,LPDDR/LPDDR2/LPDDR3/LPDDR4,HBM2 and WIDE IO DRAM devices. The memory specifications are based on 1Gb DDR2, 1Gb & 2Gb DDR3, 2Gb LPDDR/LPDDR2 and 4Gb DDR4/LPDDR3 Micron datasheets and the 256Mb Wide IO SDR specifications are based on JEDEC timing specifications and circuit-level IDD measurements by TU Kaiserslautern, inplace of the as yet unavailable vendor datasheets. 4 of the memory specifications target dual-rank DDR3 DIMMs.

Note: The timing specifications in the memory specifcaton file are in clock cycles (cc). The current specifications for Reading and Writing do not include the I/O consumption. They are computed and included seperately based on Micron Power Calculator. The IDD measures associated with different power supply sources of equal measure (VDD2, VDDCA and VDDQ) for LPDDR2, LPDDR3, LPDDR4, DDR4, HBM2 and WIDE IO memories have been added up together for simplicity, since it does not impact power computation accuracy. The current measures for dual-rank DIMMs reflect only the measures for the active rank. The default state of the idle rank is assumed to be the same as the complete memory state, for background power estimation. Accordingly, in all dual-rank memory specifications, IDD2P0 has been subtracted from the active currents and all background currents have been halved. They are also accounted for seperately by the power model. Stacking multiple Wide IO DRAM dies can also be captured by the nbrOfRanks parameter.

## 7. Variation-aware Power And Energy Estimation

15 of the included datasheets reflect the impact of process-variations on DRAM currents for a selection of DDR3 memories manufactured at 50nm process technology. These memories include:
(1) MICRON_128MB_DDR3-1066_8bit - revision G
(2) MICRON_128MB_DDR3-1066_16bit - revision G
(3) MICRON_128MB_DDR3-1600_8bit - revision G
(4) MICRON_256MB_DDR3-1066_8bit - revision D
(5) MICRON_256MB_DDR3-1600_16bit - revision D

The original vendor-provided datasheet current specifications are given in JSONs
without suffixes such as _mu, _2s and _3s. JSONs including suffixes indicate that the
current measures are either: (1) typical (mu), or (2) include +2 sigma variation (2s),
or (3) include +3 sigma variation (3s). These measures are derived based on the
Monte-Carlo analysis performed on our SPICE-based DRAM cross-section.

To include these JSONs in your simulations, simply use them as the target memory.

## 8. Example Usage

An example of using this tool is provided below. Clone the repository and make.
```
make -j4
```
Once the build is successful, run with the command trace for combined bank simulation:
```
./drampower -m memspecs/MICRON_1Gb_DDR3-1066_8bit_G.json -c traces/commands.trace -r
```
The output should be something like this:

```
* Analysis start time: Fri Dec 27 13:12:41 2019
* Analyzing the input trace
* Bankwise mode: disabled
* Partial Array Self-Refresh: disabled

* Trace Details:

#ACT commands: 1502
#RD + #RDA commands: 1499
#WR + #WRA commands: 3
#PRE (+ PREA) commands: 1502
#REF commands: 36
#REFB commands: 0
#Active Cycles: 30959
  #Active Idle Cycles: 12529
  #Active Power-Up Cycles: 0
    #Auto-Refresh Active cycles during Self-Refresh Power-Up: 0
#Precharged Cycles: 84523
  #Precharged Idle Cycles: 75531
  #Precharged Power-Up Cycles: 0
    #Auto-Refresh Precharged cycles during Self-Refresh Power-Up: 0
  #Self-Refresh Power-Up Cycles: 0
Total Idle Cycles (Active + Precharged): 88060
#Power-Downs: 0
  #Active Fast-exit Power-Downs: 0
  #Active Slow-exit Power-Downs: 0
  #Precharged Fast-exit Power-Downs: 0
  #Precharged Slow-exit Power-Downs: 0
#Power-Down Cycles: 0
  #Active Fast-exit Power-Down Cycles: 0
  #Active Slow-exit Power-Down Cycles: 0
    #Auto-Refresh Active cycles during Self-Refresh: 0
  #Precharged Fast-exit Power-Down Cycles: 0
  #Precharged Slow-exit Power-Down Cycles: 0
    #Auto-Refresh Precharged cycles during Self-Refresh: 0
#Auto-Refresh Cycles: 2124
#Self-Refreshes: 0
#Self-Refresh Cycles: 0
----------------------------------------
Total Trace Length (clock cycles): 115482
----------------------------------------

* Trace Power and Energy Estimates:

ACT Cmd Energy: 1690806.75 pJ
PRE Cmd Energy: 739727.95 pJ
RD Cmd Energy: 1096829.27 pJ
WR Cmd Energy: 2363.98 pJACT Stdby Energy: 3485065.67 pJ
  Active Idle Energy: 1410394.00 pJ
  Active Power-Up Energy: 0.00 pJ
    Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0.00 pJ
PRE Stdby Energy: 8325436.21 pJ
  Precharge Idle Energy: 7439732.65 pJ
  Precharged Power-Up Energy: 0.00 pJ
    Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0.00 pJ
  Self-Refresh Power-Up Energy: 0.00 pJ
Total Idle Energy (Active + Precharged): 8850126.64 pJ
Total Power-Down Energy: 0.00 pJ
  Fast-Exit Active Power-Down Energy: 0.00 pJ
  Slow-Exit Active Power-Down Energy: 0.00 pJ
    Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0.00 pJ
  Fast-Exit Precharged Power-Down Energy: 0.00 pJ
  Slow-Exit Precharged Power-Down Energy: 0.00 pJ
    Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0.00 pJ
Auto-Refresh Energy: 717298.31 pJ
Bankwise-Refresh Energy: 0.00 pJ
Self-Refresh Energy: 0.00 pJ
----------------------------------------
Total Trace Energy: 16057528.14 pJ
Average Power: 74.11 mW
----------------------------------------
* Power Computation End time: Fri Dec 27 13:12:41 2019
* Total Simulation time: 0.015625 seconds
```

It is also worth to try the bank wise granular simulation feature with the same trace. Inorder to do so, run:
```
./drampower -m memspecs/MICRON_1Gb_DDR3-1066_8bit_G.json -c traces/commands.trace -r -b 100 100 -s 0
```
The output should be something like this:
```
* Analysis start time: Fri Dec 27 13:09:10 2019
* Analyzing the input trace
* Bankwise mode: enabled (power offset factors  ρ=100% ,σ=100% )
* Partial Array Self-Refresh: enabled
* Bankwise Details:
## @ Bank 0
  #ACT commands: 268
  #RD + #RDA commands: 268
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 268
## @ Bank 1
  #ACT commands: 205
  #RD + #RDA commands: 205
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 205
## @ Bank 2
  #ACT commands: 170
  #RD + #RDA commands: 170
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 170
## @ Bank 3
  #ACT commands: 181
  #RD + #RDA commands: 181
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 181
## @ Bank 4
  #ACT commands: 208
  #RD + #RDA commands: 205
  #WR + #WRA commands: 3
  #PRE (+ PREA) commands: 208
## @ Bank 5
  #ACT commands: 215
  #RD + #RDA commands: 215
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 215
## @ Bank 6
  #ACT commands: 106
  #RD + #RDA commands: 106
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 106
## @ Bank 7
  #ACT commands: 149
  #RD + #RDA commands: 149
  #WR + #WRA commands: 0
  #PRE (+ PREA) commands: 149

* Trace Details:

#ACT commands: 1502
#RD + #RDA commands: 1499
#WR + #WRA commands: 3
#PRE (+ PREA) commands: 1502
#REF commands: 36
#REFB commands: 0
#Active Cycles: 30959
  #Active Idle Cycles: 12529
  #Active Power-Up Cycles: 0
    #Auto-Refresh Active cycles during Self-Refresh Power-Up: 0
#Precharged Cycles: 84523
  #Precharged Idle Cycles: 75531
  #Precharged Power-Up Cycles: 0
    #Auto-Refresh Precharged cycles during Self-Refresh Power-Up: 0
  #Self-Refresh Power-Up Cycles: 0
Total Idle Cycles (Active + Precharged): 88060
#Power-Downs: 0
  #Active Fast-exit Power-Downs: 0
  #Active Slow-exit Power-Downs: 0
  #Precharged Fast-exit Power-Downs: 0
  #Precharged Slow-exit Power-Downs: 0
#Power-Down Cycles: 0
  #Active Fast-exit Power-Down Cycles: 0
  #Active Slow-exit Power-Down Cycles: 0
    #Auto-Refresh Active cycles during Self-Refresh: 0
  #Precharged Fast-exit Power-Down Cycles: 0
  #Precharged Slow-exit Power-Down Cycles: 0
    #Auto-Refresh Precharged cycles during Self-Refresh: 0
#Auto-Refresh Cycles: 2124
#Self-Refreshes: 0
#Self-Refresh Cycles: 0
----------------------------------------
Total Trace Length (clock cycles): 115482
----------------------------------------

* Bankwise Details:
## @ Bank 0
  ACT Cmd Energy: 301689 pJ
  PRE Cmd Energy: 105591 pJ
  RD Cmd Energy: 196098 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 2169352 pJ

## @ Bank 1
  ACT Cmd Energy: 230769 pJ
  PRE Cmd Energy: 80769 pJ
  RD Cmd Energy: 150000 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 2027513 pJ

## @ Bank 2
  ACT Cmd Energy: 191370 pJ
  PRE Cmd Energy: 66979 pJ
  RD Cmd Energy: 124390 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 1948714 pJ

## @ Bank 3
  ACT Cmd Energy: 203752 pJ
  PRE Cmd Energy: 71313 pJ
  RD Cmd Energy: 132439 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 1973480 pJ

## @ Bank 4
  ACT Cmd Energy: 234146 pJ
  PRE Cmd Energy: 81951 pJ
  RD Cmd Energy: 150000 pJ
  WR Cmd Energy: 2364 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 2034437 pJ

## @ Bank 5
  ACT Cmd Energy: 242026 pJ
  PRE Cmd Energy: 84709 pJ
  RD Cmd Energy: 157317 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 2050028 pJ

## @ Bank 6
  ACT Cmd Energy: 119325 pJ
  PRE Cmd Energy: 41764 pJ
  RD Cmd Energy: 77561 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 1804624 pJ

## @ Bank 7
  ACT Cmd Energy: 167730 pJ
  PRE Cmd Energy: 58705 pJ
  RD Cmd Energy: 109024 pJ
  WR Cmd Energy: 0 pJ
  Auto-Refresh Energy: 89662 pJ
  Bankwise-Refresh Energy: 0 pJ
  ACT Stdby Energy: 435633 pJ
  PRE Stdby Energy: 1040680 pJ
  Active Idle Energy: 176299 pJ
  Precharge Idle Energy: 929967 pJ
  Fast-Exit Active Power-Down Energy: 0 pJ
  Fast-Exit Precharged Power-Down Energy: 0 pJ
  Slow-Exit Active Power-Down Energy: 0 pJ
  Slow-Exit Precharged Power-Down Energy: 0 pJ
  Self-Refresh Energy: 0 pJ
  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0 pJ
  Self-Refresh Power-Up Energy: 0 pJ
  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0 pJ
  Active Power-Up Energy: 0 pJ
  Precharged Power-Up Energy: 0 pJ
  Total Energy: 1901435 pJ


* Trace Power and Energy Estimates:

ACT Cmd Energy: 1690806.75 pJ
PRE Cmd Energy: 739727.95 pJ
RD Cmd Energy: 1096829.27 pJ
WR Cmd Energy: 2363.98 pJ
RD I/O Energy: 465730.58 pJ
WR Termination Energy: 4772.98 pJ
ACT Stdby Energy: 3485065.67 pJ
  Active Idle Energy: 1410394.00 pJ
  Active Power-Up Energy: 0.00 pJ
    Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0.00 pJ
PRE Stdby Energy: 8325436.21 pJ
  Precharge Idle Energy: 7439732.65 pJ
  Precharged Power-Up Energy: 0.00 pJ
    Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: 0.00 pJ
  Self-Refresh Power-Up Energy: 0.00 pJ
Total Idle Energy (Active + Precharged): 8850126.64 pJ
Total Power-Down Energy: 0.00 pJ
  Fast-Exit Active Power-Down Energy: 0.00 pJ
  Slow-Exit Active Power-Down Energy: 0.00 pJ
    Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0.00 pJ
  Fast-Exit Precharged Power-Down Energy: 0.00 pJ
  Slow-Exit Precharged Power-Down Energy: 0.00 pJ
    Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: 0.00 pJ
Auto-Refresh Energy: 717298.31 pJ
Bankwise-Refresh Energy: 0.00 pJ
Self-Refresh Energy: 0.00 pJ
----------------------------------------
Total Trace Energy: 16380086.12 pJ
Average Power: 75.60 mW
----------------------------------------
* Power Computation End time: Fri Dec 27 13:09:11 2019
* Total Simulation time: 0 seconds

```
As can be noticed, the tool performs DRAM command scheduling and reports the number
of activates, precharges, reads, writes, refreshes, power-downs and self-refreshes
besides the number of clock cycles spent in the active and precharged states, in the
power-down (fast/slow-exit) and self-refresh states and in the idle mode. It also
reports the energy consumption of these components, besides the IO and Termination
components in pJ (pico Joules) and the average power consumption of the trace in mW.
It also reports the simulation start/end times and the total simulation time in seconds.

## 9. DRAMPower Library

The DRAMPower tool has an additional feature and can be used as a library.
In order to use the library run "make lib", include [LibDRAMPower.h](src/libdrampower/LibDRAMPower.h) in your project and
link the file src/libdrampower.a with your project.
Examples for the usage of the library are [lib_test.cc](test/libdrampowertest/lib_test.cc) and [window_example.cc](test/libdrampowertest/window_example.cc).

## 10. Authors & Acknowledgment

The tool is based on the DRAM power model developed jointly by the Computer Engineering Research Group at TU Delft and the Electronic Systems Group at TU Eindhoven
and verified by the Microelectronic System Design Research Group at TU Kaiserslautern with equivalent circuit-level simulations. This tool has been developed by
Karthik Chandrasekar with Yonghui Li under the supervision of Dr. Benny Akesson and Prof. Kees Goossens. The IO and Termination Power measures have been employed
from Micron's DRAM Power Calculator. If you decide to use DRAMPower in your research, please cite one of the following references:

**To cite the DRAMPower Tool:**
```
[1] DRAMPower: Open-source DRAM Power & Energy Estimation Tool
Karthik Chandrasekar, Christian Weis, Yonghui Li, Sven Goossens, Matthias Jung, Omar Naji, Benny Akesson, Norbert Wehn, and Kees Goossens
URL: http://www.drampower.info
```

**To cite the DRAM power model:**
```
[2] "Improved Power Modeling of DDR SDRAMs"
Karthik Chandrasekar, Benny Akesson, and Kees Goossens
In Proc. 14th Euromicro Conference on Digital System Design (DSD), 2011
```
**To cite Bank-Wise granular power model:**
```
[3] "A Bank-Wise DRAM Power Model for System Simulations"
Deepak M. Mathew, Éder F. Zulian, Subash Kannoth, Matthias Jung, Christian Weis, Norbert Wehn
RAPIDO ’17, January 23-25, 2017, Stockholm, Sweden
```
**To cite the 3D-DRAM power model:**
```
[4] "System and Circuit Level Power Modeling of Energy-Efficient 3D-Stacked Wide I/O DRAMs"
Karthik Chandrasekar, Christian Weis, Benny Akesson, Norbert Wehn, and Kees Goossens
In Proc. Design, Automation and Test in Europe (DATE), 2013
```

**To cite variation-aware DRAM power estimation:**
```
[5] "Towards Variation-Aware System-Level Power Estimation of DRAMs: An Empirical Approach"
Karthik Chandrasekar, Christian Weis, Benny Akesson, Norbert Wehn, and Kees Goossens
In Proc. Design Automation Conference (DAC), 2013
```
## 11. Contact Information

Further questions about the tool and the power model can be directed to:

Matthias Jung (matthias.jung@iese.fraunhofer.de)

Feel free to ask for updates to the tool's features and please do report any bugs
and errors you encounter. This will encourage us to continuously improve the tool.

## Disclaimer

The tool does not check the timing accuracy of the user's memory command trace
and the use of commands and memory modes. It is expected that the user employs
a valid trace generated using a DRAM memory controller or simulator, which
satisfies all memory timing constraints and other requirements. The user DOES
NOT get ANY WARRANTIES when using this tool. This software is released under the
BSD 3-Clause License. By using this software, the user implicitly agrees to the
licensing terms.
