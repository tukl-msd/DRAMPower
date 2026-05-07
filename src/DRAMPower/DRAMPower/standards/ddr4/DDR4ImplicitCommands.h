#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4IMPLICITCOMMANDS_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4IMPLICITCOMMANDS_H

#include "DRAMPower/util/implicitCommandHelpers.h"

namespace DRAMPower {

// Forward declaration
class DDR4Core;

// Implicit commands functionality
struct DDR4ImplicitCommandsBridge {
    static void handlePreAferRef(DDR4Core& core, Container_rbt data);
    static void handlePre(DDR4Core& core, Container_rbt data);
    static void handleRefEntry(DDR4Core& core, Container_rt data);
    static void handlePDActEntry(DDR4Core& core, Container_rt data);
    static void handlePDPreEntry(DDR4Core& core, Container_rt data);
    static void handlePDExit(DDR4Core& core, Container_rt data);
};

// Map data and functionality to a type
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandPreAfterRef, DDR4ImplicitCommandsBridge::handlePreAferRef, Container_rbt)
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandPre, DDR4ImplicitCommandsBridge::handlePre, Container_rbt)
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandRefEntry, DDR4ImplicitCommandsBridge::handleRefEntry, Container_rt)
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandPDActEntry, DDR4ImplicitCommandsBridge::handlePDActEntry, Container_rt)
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandPDPreEntry, DDR4ImplicitCommandsBridge::handlePDPreEntry, Container_rt)
DEFINE_IMPLICIT_COMMAND(DDR4ImplicitCommandPDExit, DDR4ImplicitCommandsBridge::handlePDExit, Container_rt)

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4IMPLICITCOMMANDS_H */
