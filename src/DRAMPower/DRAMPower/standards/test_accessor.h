#ifndef DRAMPOWER_STANDARDS_TEST_ACCESSOR_H
#define DRAMPOWER_STANDARDS_TEST_ACCESSOR_H

#include "DRAMPower/standards/ddr4/DDR4Core.h"
#include "DRAMPower/standards/ddr5/DDR5Core.h"
#include "DRAMPower/standards/lpddr4/LPDDR4Core.h"
#include "DRAMPower/standards/lpddr5/LPDDR5Core.h"

namespace DRAMPower::internal {

// Tests access to private members of DDR4, DDR5, LPDDR4, and LPDDR5 classes.
// https://github.com/google/googletest/blob/main/docs/advanced.md#testing-private-code
template<typename Core>
class TestAccessor {
public:
    static std::vector<Rank>& getRanks(Core& core) {
        return core.m_ranks;
    }
};

// Specializations for each standard
static const TestAccessor<DDR4Core> DDR4TestAccessor;
static const TestAccessor<DDR5Core> DDR5TestAccessor;
static const TestAccessor<LPDDR4Core> LPDDR4TestAccessor;
static const TestAccessor<LPDDR5Core> LPDDR5TestAccessor;

#ifndef DRAMPOWER_TESTING
#error "test-internal.h should only be included in test files"
#endif

} // namespace DRAMPower::internal

#endif /* DRAMPOWER_STANDARDS_TEST_ACCESSOR_H */
