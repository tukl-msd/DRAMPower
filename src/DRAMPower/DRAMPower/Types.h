#ifndef DRAMPOWER_TYPES_H
#define DRAMPOWER_TYPES_H

#include <DRAMPower/util/cycle_stats.h>

#include <stdint.h>

namespace DRAMPower {

using timestamp_t = uint64_t;
using interval_t = util::interval_counter<timestamp_t>;

}

#endif /* DRAMPOWER_TYPES_H */
