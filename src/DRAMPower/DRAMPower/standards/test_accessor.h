#ifndef DRAMPOWER_STANDARDS_TEST_INTERNAL_H
#define DRAMPOWER_STANDARDS_TEST_INTERNAL_H

#include "DRAMPower/standards/ddr4/DDR4.h"
#include "DRAMPower/standards/ddr5/DDR5.h"
#include "DRAMPower/standards/lpddr4/LPDDR4.h"
#include "DRAMPower/standards/lpddr5/LPDDR5.h"

namespace DRAMPower::internal {

// Tests access to private members of DDR4, DDR5, LPDDR4, and LPDDR5 classes.
// https://github.com/google/googletest/blob/main/docs/advanced.md#testing-private-code
template<typename Standard, typename Core, typename Interface>
class TestAccessor {
public:
    static Core& getCore(Standard& standard) {
        return standard.getCore();
    }
    static const Core& getCore(const Standard& standard) {
        return standard.getCore();
    }

    static Interface& getInterface(Standard& standard) {
        return standard.getInterface();
    }
    static const Interface& getInterface(const Standard& standard) {
        return standard.getInterface();
    }
};

// Specializations for each standard
static const TestAccessor<DDR4, DDR4Core, DDR4Interface> DDR4TestAccessor;
static const TestAccessor<DDR5, DDR5Core, DDR5Interface> DDR5TestAccessor;
static const TestAccessor<LPDDR4, LPDDR4Core, LPDDR4Interface> LPDDR4TestAccessor;
static const TestAccessor<LPDDR5, LPDDR5Core, LPDDR5Interface> LPDDR5TestAccessor;

#ifndef DRAMPOWER_TESTING
#error "test-internal.h should only be included in test files"
#endif

} // namespace DRAMPower::internal

#endif /* DRAMPOWER_STANDARDS_TEST_INTERNAL_H */
