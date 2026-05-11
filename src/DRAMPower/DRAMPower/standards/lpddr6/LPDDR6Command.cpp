#include "DRAMPower/standards/lpddr6/LPDDR6Command.h"

#include <cassert>


namespace DRAMPower {

LPDDR6TargetCoordinate::LPDDR6TargetCoordinate(const TargetCoordinate& targetCoordinate)
    : bank(targetCoordinate.bank)
    , dbank((targetCoordinate.bank + 8) % 16) // Assuming a constant bank count of 16 and an offset of 8
    , bankGroup(targetCoordinate.bankGroup)
    , rank(targetCoordinate.rank)
    , row(targetCoordinate.row)
    , column(targetCoordinate.column)
{}
    
LPDDR6Command::LPDDR6Command(timestamp_t timestamp, CmdType type, LPDDR6TargetCoordinate targetCoord, const uint8_t * data, std::size_t sz_bits)
    : timestamp(timestamp)
    , type(type)
    , targetCoordinate(targetCoord)
    , data(data)
    , sz_bits(sz_bits)
{}
LPDDR6Command::LPDDR6Command(const Command& command)
    : timestamp(command.timestamp)
    , type(static_cast<CmdType>(command.type))
    , targetCoordinate(command.targetCoordinate)
    , data(command.data)
    , sz_bits(command.sz_bits)
{
    assert(static_cast<std::size_t>(command.type) < static_cast<std::size_t>(CmdType::COUNT) && "Unsupported cmdType");
}

} // namespace DRAMPower