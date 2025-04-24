#include "Command.h"

namespace DRAMPower {

    Command::Command(timestamp_t timestamp, CmdType type, TargetCoordinate targetCoord, const uint8_t * data, std::size_t sz_bits)
        : timestamp(timestamp)
        , type(type)
        , targetCoordinate(targetCoord)
        , data(data)
        , sz_bits(sz_bits)
    {};

} // namespace DRAMPower