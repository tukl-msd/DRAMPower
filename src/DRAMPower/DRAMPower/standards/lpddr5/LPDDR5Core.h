#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H

#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"

#include "DRAMPower/memspec/MemSpecLPDDR5.h"

#include <functional>
#include <vector>

namespace DRAMPower {

class LPDDR5Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelper<LPDDR5Core>;
// Public constructors
public:
    LPDDR5Core() = delete; // No default constructor
    LPDDR5Core(const LPDDR5Core&) = default; // copy constructor
    LPDDR5Core& operator=(const LPDDR5Core&) = default; // copy assignment operator
    LPDDR5Core(LPDDR5Core&&) = default; // move constructor
    LPDDR5Core& operator=(LPDDR5Core&&) = default; // move assignment operator
    LPDDR5Core(const MemSpecLPDDR5 &memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)})
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_ranks};
    }

// Publie member variables:
public:
    std::vector<Rank> m_ranks;

// Private member variables
private:
    std::reference_wrapper<const MemSpecLPDDR5> m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H */
