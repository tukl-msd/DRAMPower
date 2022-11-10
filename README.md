<img src="docs/images/logo_drampower_5_0.png" alt="DRAMPower 5.0" width="350" style="float: left;"/>  

# DRAM Power Model (DRAMPower 5.0)

- [Releases](#releases)
- [Installation](#installation)
- [Project structure](#project-structure)
- [Dependencies](#dependencies)
- [Memory Specifications](#memory-specifications)
- [Variation-aware Power And Energy Estimation](#variation-aware-power-and-energy-estimation)
- [Authors & Acknowledgment](#authors--acknowledgment)
- [Contact Information](#contact-information)

## Releases
The last official release can be found here: https://github.com/ravenrd/DRAMPower/releases/tag/5.0

The master branch of the repository should be regarded as the bleeding-edge version, which has all the latest features, but also all the latest bugs. Use at your own discretion.

## Installation
CMake is required for the building of DRAMPower. 
Clone the repository, or download the zip file of the release you would like to use and use CMake to generate the build files, e.g.

```
cd DRAMPower
mkdir build && cd build
cmake ../ 
make -j4 DRAMPower
```

Optionally, test cases can be built by toggling the DRAMPOWER_BUILD_TESTS flag with CMake.
The command line tool can be built by setting the DRAMPOWER_BUILD_CLI flag.

## Project structure
The project is structured in a library part and an (optional) Command Line application.
The library can be built using the CMake target DRAMPower.
Integration of DRAMPower in other projects can be easily achieved by including it as a git submodule or by using the CMake FetchContent directive.

This repository contains the following sub-directoires

     DRAMPower                  # top directory
     └── cmake                  # cmake scripts used by configuration step
     ├── lib                    # contains bundled dependencies of the project
     ├── src                    # top level directory containing the actual sources
         ├── DRAMPower          # source code of the actual DRAMPower library
         └── cli                # the optional Command Line tool
     └── tests                  # test cases used by the project

## Dependencies
DRAMPower comes bundled with all necessary libraries and no installation of further system packages is required.


## Memory Specifications

36 sample memory specifications are given in the XMLs targeting DDR2/DDR3/DDR4, LPDDR/LPDDR2/LPDDR3 and WIDE IO DRAM devices. The memory specifications are based on 1Gb DDR2, 1Gb & 2Gb DDR3, 2Gb LPDDR/LPDDR2 and 4Gb DDR4/LPDDR3 Micron datasheets and the 256Mb Wide IO SDR specifications are based on JEDEC timing specifications and circuit-level IDD measurements by TU Kaiserslautern, inplace of the as yet unavailable vendor datasheets. 4 of the memory specifications target dual-rank DDR3 DIMMs.

Note: The timing specifications in the XMLs are in clock cycles (cc). The current specifications for Reading and Writing do not include the I/O consumption. They are computed and included seperately based on Micron Power Calculator. The IDD measures associated with different power supply sources of equal measure (VDD2, VDDCA and VDDQ) for LPDDR2, LPDDR3, DDR4 and WIDE IO memories have been added up together for simplicity, since it does not impact power computation accuracy. The current measures for dual-rank DIMMs reflect only the measures for the active rank. The default state of the idle rank is assumed to be the same as the complete memory state, for background power estimation. Accordingly, in all dual-rank memory specifications, IDD2P0 has been subtracted from the active currents and all background currents have been halved. They are also accounted for seperately by the power model. Stacking multiple Wide IO DRAM dies can also be captured by the nbrOfRanks parameter.

## Variation-aware Power And Energy Estimation

15 of the included datasheets reflect the impact of process-variations on DRAM currents for a selection of DDR3 memories manufactured at 50nm process technology. These memories include:
(1) MICRON_128MB_DDR3-1066_8bit - revision G
(2) MICRON_128MB_DDR3-1066_16bit - revision G
(3) MICRON_128MB_DDR3-1600_8bit - revision G
(4) MICRON_256MB_DDR3-1066_8bit - revision D
(5) MICRON_256MB_DDR3-1600_16bit - revision D

The original vendor-provided datasheet current specifications are given in XMLs
without suffixes such as _mu, _2s and _3s. XMLs including suffixes indicate that the
current measures are either: (1) typical (mu), or (2) include +2 sigma variation (2s),
or (3) include +3 sigma variation (3s). These measures are derived based on the
Monte-Carlo analysis performed on our SPICE-based DRAM cross-section.

To include these XMLs in your simulations, simply use them as the target memory.

## Authors & Acknowledgment

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

**To cite the 3D-DRAM power model:**
```
[3] "System and Circuit Level Power Modeling of Energy-Efficient 3D-Stacked Wide I/O DRAMs"
Karthik Chandrasekar, Christian Weis, Benny Akesson, Norbert Wehn, and Kees Goossens
In Proc. Design, Automation and Test in Europe (DATE), 2013
```

**To cite variation-aware DRAM power estimation:**
```
[4] "Towards Variation-Aware System-Level Power Estimation of DRAMs: An Empirical Approach"
Karthik Chandrasekar, Christian Weis, Benny Akesson, Norbert Wehn, and Kees Goossens
In Proc. Design Automation Conference (DAC), 2013
```

## Contact Information

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
