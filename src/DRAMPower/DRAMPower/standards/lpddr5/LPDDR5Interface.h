#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H

#include "DRAMPower/util/pin.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/RegisterHelper.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/data/energy.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR5.h"

#include "DRAMUtils/config/toggling_rate.h"

#include <functional>
#include <stdint.h>
#include <cstddef>
#include <vector>

namespace DRAMPower {

class LPDDR5Interface {
// Public constants
public:
    const static std::size_t cmdBusWidth = 7;
    const static uint64_t cmdBusInitPattern = (1<<cmdBusWidth)-1;

// Public type definitions
public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using patternHandler_t = PatternHandler<CmdType>;
    using interfaceRegisterHelper_t = util::InterfaceRegisterHelper<LPDDR5Interface>;

// Public constructors and assignment operators
public:
    LPDDR5Interface() = delete; // no default constructor
    LPDDR5Interface(const LPDDR5Interface&) = default; // copy constructor
    LPDDR5Interface& operator=(const LPDDR5Interface&) = default; // copy assignment operator
    LPDDR5Interface(LPDDR5Interface&&) = default; // move constructor
    LPDDR5Interface& operator=(LPDDR5Interface&&) = default; // move assignment operator

    LPDDR5Interface(const MemSpecLPDDR5 &memSpec, implicitCommandInserter_t&& implicitCommandInserter, patternHandler_t &patternHandler);



// Private member variables
private:
    std::reference_wrapper<const MemSpecLPDDR5> m_memSpec;
    std::reference_wrapper<patternHandler_t> m_patternHandler;
    implicitCommandInserter_t m_implicitCommandInserter;
};

}

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5INTERFACE_H */
