#ifndef DRAMPOWER_DDR_INTERFACE_H
#define DRAMPOWER_DDR_INTERFACE_H

#pragma once 

#include <DRAMPower/Types.h>

#include <DRAMPower/util/bus.h>

#include <stdint.h>

namespace DRAMPower {

struct interface_stats_t 
{
	util::bus_stats_t command_bus;
	util::bus_stats_t read_bus;
	util::bus_stats_t write_bus;
};

}

#endif /* DRAMPOWER_DDR_INTERFACE_H */
