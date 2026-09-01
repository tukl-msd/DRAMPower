#include "DRAMPower/standards/hbm2/HBM2Command.h"

#include <cassert>


namespace DRAMPower {

HBM2TargetCoordinate::HBM2TargetCoordinate(const TargetCoordinate& targetCoordinate)
    : bank(targetCoordinate.bank)
    , row(targetCoordinate.row)
    , column(targetCoordinate.column)
    , stack(0) // TODO: cannot be computed from targetCoordinate
    , pseudoChannel(0) // TODO: cannot be computed from targetCoordinate
{}
    
HBM2Command::HBM2Command(timestamp_t timestamp, CmdType type, HBM2TargetCoordinate targetCoord, const uint8_t * data, std::size_t sz_bits)
    : timestamp(timestamp)
    , type(type)
    , targetCoordinate(targetCoord)
    , data(data)
    , sz_bits(sz_bits)
{}
HBM2Command::HBM2Command(const Command& command)
    : timestamp(command.timestamp)
    , type(static_cast<CmdType>(command.type))
    , targetCoordinate(command.targetCoordinate)
    , data(command.data)
    , sz_bits(command.sz_bits)
{
    assert(static_cast<std::size_t>(command.type) < static_cast<std::size_t>(CmdType::COUNT) && "Unsupported cmdType");
}

} // namespace DRAMPower