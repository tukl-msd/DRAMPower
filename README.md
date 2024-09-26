<img src="docs/images/logo_drampower_5_0.png" alt="DRAMPower 5.0" width="350" style="float: left;"/>  

# DRAM Power Model (DRAMPower 5.0)

- [Releases](#releases)
- [Installation of the DRAMPower library](#installation-of-the-drampower-library)
- [Installation of the DRAMPower Command Line application](#installation-of-the-drampower-command-line-application)
- [Project structure](#project-structure)
- [Dependencies](#dependencies)
- [Usage of the DRAMPower library](#usage-of-the-drampower-library)
- [Usage of the DRAMPower Command Line application](#usage-of-the-drampower-command-line-application)
- [Memory Specifications](#memory-specifications)
- [Variation-aware Power And Energy Estimation](#variation-aware-power-and-energy-estimation)
- [Authors & Acknowledgment](#authors--acknowledgment)
- [Contact Information](#contact-information)

## Releases
The last official release can be found here: https://github.com/tukl-msd/DRAMPower/releases/tag/5.0

The master branch of the repository should be regarded as the bleeding-edge version, which has all the latest features, but also all the latest bugs. Use at your own discretion.

## Installation of the DRAMPower library
CMake is required for the building of DRAMPower. 
Clone the repository, or download the zip file of the release you would like to use and use CMake to generate the build files, e.g.

```console
$ cd DRAMPower
$ cmake -S . -B build
$ cmake --build build
```

Optionally, test cases can be built by toggling the DRAMPOWER_BUILD_TESTS flag with CMake.
The command line tool can be built by setting the DRAMPOWER_BUILD_CLI flag.

## Installation of the DRAMPower Command Line application
CMake is required for the building of DRAMPower Command Line application.
Clone the repository, or download the zip file of the release you would like to use and use CMake to generate the build files, e.g.

```console
$ cd DRAMPower
$ cmake -S . -B build -D DRAMPOWER_BUILD_CLI=Y
$ cmake --build build
```

## Project structure
The project is structured in a library part, a (optional) test part and an (optional) Command Line application.
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
DRAMPower comes bundled with all necessary libraries and no installation of further system packages is required. DRAMPower uses the following libraries:
 - [DRAMUtils](https://github.com/tukl-msd/DRAMUtils)

The DRAMPower cli tool uses the following libraries:
 - DRAMPower
 - [DRAMUtils](https://github.com/tukl-msd/DRAMUtils)
 - [spdlog](https://github.com/gabime/spdlog/releases/tag/v1.9.2)
 - [CLI11 (CLI11 2.2 Copyright (c) 2017-2024 University of Cincinnati, developed by Henry Schreiner under NSF AWARD 1414736. All rights reserved.)](https://github.com/CLIUtils/CLI11/releases/tag/v2.4.2)

## Usage of the DRAMPower library

The project is [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) ready and can be easily included in any other CMake based project.

```cmake
include(FetchContent)

FetchContent_Declare(
drampower
GIT_REPOSITORY https://github.com/tukl-msd/DRAMPower
GIT_TAG master)
FetchContent_MakeAvailable(drampower)
```

The library target DRAMPower is then available to the rest of the project and can be consumed by any other target, e.g.

```cmake
add_executable(drampower_app ${SOURCE_FILES})
target_link_libraries(drampower_app PRIVATE DRAMSys::DRAMPower)
```

All constructs inside DRAMPower are exposed through the DRAMPower namespace.
Therefore all the following examples will refer to them with the implied usage of their namespace.

```cpp
using namespace DRAMPower;
```

To use the actual DRAM calculations, first a memspec has to be supplied. Some example memspecs are supplied in the [tests directory](https://github.com/tukl-msd/DRAMPower/tree/master/tests/tests_drampower/resources). The values in the memspecs are in SI units.
An example snippet to initialize a DDR4 based DRAM spec can look like this (only the DRAMPower related includes are shown):

```cpp
#include <DRAMUtils/memspec/MemSpec.h>
#include <DRAMPower/memspec/MemSpecDDR4.h>
#include <DRAMPower/standards/ddr4/DDR4.h>

// optional for simulation with toggling rates
#include <DRAMUtils/config/toggling_rate.h> 

// Use DRAMUtils library to parse the memspec
auto memspec = DRAMUtils::parse_memspec_from_file(
    std::filesystem::path("tests/tests_drampower/resources/ddr4.json")
);
if (!memspec) {
    throw std::runtime_error("Could not parse memspec");
}
// Make the DRAMPower specific memspec from the parsed memspec and initialize the DRAM
// The next line can throw std::bad_variant_access if the memspec is not a DDR4 memspec
// or an exception derived from std::exception if the json object is not valid
DDR4 dram(MemSpecDDR4::from_memspec(*memspec));
// Optionally use toggle rates if no data is available for the traces
// dram.setToggleRate(DRAMUtils::Config::ToggleRateDefinition{
//     0.5, // togglingRateRead  0.0 to 1.0
//     0.5, // togglingRateWrite 0.0 to 1.0
//     0.5, // dutyCycleRead     0.0 to 1.0
//     0.5, // dutyCycleWrite    0.0 to 1.0
//     TogglingRateIdlePattern::H, // idlePatternRead  H, L, Z
//     TogglingRateIdlePattern::H  // idlePatternWrite H, L, Z
// });
```

The created DRAM simulator then has to be fed with commands, e.g.

```cpp
#include <DRAMPower/command/Command.h>

#define SZ_BITS(x) sizeof(x)*8

uint8_t rd_data[] = {
    0, 0, 0, 0,  0, 0, 0, 1,
};

std::vector<Command> testPattern = {
    // {timestamp, commandtype, {bank_id, bank_group_id, rank_id, row_id}
    {  0, CmdType::ACT,  {0, 0, 0, 0}},
    // {timestamp, commandtype, {bank_id, bank_group_id, rank_id, row_id, column_id}, data, data_size}
    { 15, CmdType::RD ,  {0, 0, 0, 0, 16}, rd_data, SZ_BITS(rd_data)},
    // optional for simulation with toggling rates (dram.setToggleRate() was called)
    // { 15, CmdType::RD ,  {0, 0, 0, 0, 16}, nullptr, 64}, 
    { 35, CmdType::PRE,  {0, 0, 0, 0}},
    { 45, CmdType::END_OF_SIMULATION },
};

for (const auto& command : testPattern) {
    dram.doCoreInterfaceCommand(command);
};
```

The bank stats and energy calculations can then be accessed through their respective methods.
In the example energy_core holds the individual bank energy measures. total_energy_core holds the accumulated energy measures accessable through the energy type (e.g E_act, E_pre, E_RD, E_bg_act, E_bg_pre). total_core, total_interface and total_all hold the total energy measures for the core, interface and the sum of both as double values.
All values are returned in their respective SI units.

```cpp
auto stats = dram.getStats();

stats.bank[0].counter.act;        // 1;
stats.bank[0].counter.reads;      // 1;
stats.bank[0].counter.pre;        // 1;
stats.rank_total[0].cycles.act;   // 35;
stats.rank_total[0].cycles.pre;   // 10;
stats.bank[0].cycles.act;         // 35;
stats.bank[0].cycles.pre;         // 10;

// Core energy
auto energy_core = dram.calcCoreEnergy(dram.getLastCommandTime());
// Interface energy
auto energy_interface = dram.calcInterfaceEnergy(dram.getLastCommandTime());
// Total energy with core and interface energy

// Accummulated bank energy
// Note: total_core_detailed.E_bg_act holds holds the accumulated backgound activation energy plus the shared background activation energy 
auto total_energy_core = energy_core.total_energy();

energy_core.bank_energy[0].E_act;     // 1.7949519230769228e-10 J
total_energy_core.E_act;              // 1.7949519230769228e-10 J

energy_core.E_bg_act_shared;          // 1.1832692307692309e-09 J

energy_core.bank_energy[0].E_bg_act;  // 5.8052884615384527e-12 J
total_energy_core.E_bg_act;           // 1.1890745192307694e-09 J (5.8052884615384527e-12 J + 1.1832692307692309e-09 J)

total_energy_core.E_pre;              // 2.076923076923077e-10 J
energy_core.bank_energy[0].E_pre;     // 2.076923076923077e-10 J

total_energy_core.E_bg_pre;           // 3.1153846153846144e-10 J
energy_core.bank_energy[0].E_bg_pre;  // 1.9471153846153847e-11 J

total_energy_core.E_RD;               // 4.3569230769230762e-10 J
energy_core.bank_energy[0].E_RD;      // 4.3569230769230762e-10 J

double total_core = core_energy.total(); // 2.3234927884615384e-09 J
double total_interface = interface_energy.total(); // 1.1102233846153846e-10 J
double total_all = dram.getTotalEnergy(dram.getLastCommandTime()); // 2.4345151269230767e-09 J
```

## Usage of the DRAMPower Command Line application

The Command Line application can be built directly by setting the DRAMPOWER_BUILD_CLI flag with CMake (see [Installation Command Line application](#installation-command-line-application)).
The Command Line application can be used to calculate energy consumption of a DRAM memory using a command trace. The output can be printed to the console or written as a JSON file. The application needs the following arguments:

- -m, --memspec (required): The path to the memory specification file (JSON format)
- -c, --config  (required): Configuration file for the Command Line application (JSON format)
- -t, --trace   (required): The path to the command trace file (CSV format) (see [tests directory](https://github.com/tukl-msd/DRAMPower/tree/master/tests/tests_drampower/resources))
- -j, --json    (optional): The path to the output JSON file (default: print to console) (the output JSON file must exist and will be overwritten)

The configuration file has the following format:

```json
{
    "useToggleRate": false,       // true or false
    "toggleRateConfig": {
        "togglingRateRead": 0.5,  // 0.0 to 1.0
        "togglingRateWrite": 0.5, // 0.0 to 1.0
        "dutyCycleRead": 0.5,     // 0.0 to 1.0
        "dutyCycleWrite": 0.5,    // 0.0 to 1.0
        "idlePatternRead": "H",   // "H", "L" or "Z"
        "idlePatternWrite": "H"   // "H", "L" or "Z"
    }
}
```

The Command Line application can be used as follows (in the following example the executable drampower_cli is located in the DRAMPower/build/bin directory):

```console
$ cd build/bin
$ touch output.json
$ touch config.json
$ # The configuration file config.json must be filled with a valid configuration (see above)
$ ./drampower_cli -c config.json -m ../../tests/tests_drampower/resources/ddr4.json -t ../../tests/tests_drampower/resources/ddr4.csv
$ ./drampower_cli -c config.json -m ../../tests/tests_drampower/resources/ddr4.json -t ../../tests/tests_drampower/resources/ddr4.csv -j output.json
```
## Memory Specifications

Note: The timing specifications in the JSONs are in clock cycles (cc). The current specifications for Reading and Writing do not include the I/O consumption. They are computed and included seperately. The IDD measures associated with different power supply sources of equal measure (VDD2, VDDCA and VDDQ). The current measures for dual-rank DIMMs reflect only the measures for the active rank. The default state of the idle rank is assumed to be the same as the complete memory state, for background power estimation. Accordingly, in all dual-rank memory specifications, IDD2P0 has been subtracted from the active currents and all background currents have been halved. They are also accounted for seperately by the power model. Stacking multiple Wide IO DRAM dies can also be captured by the nbrOfRanks parameter.

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
