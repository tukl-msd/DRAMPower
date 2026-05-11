#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6COMMAND_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6COMMAND_H

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"

#include <cstddef>
#include <cstdint>


namespace DRAMPower {

struct LPDDR6TargetCoordinate {
    LPDDR6TargetCoordinate() = default;
    LPDDR6TargetCoordinate(const TargetCoordinate& targetCoordinate)
        : bank(targetCoordinate.bank)
        , dbank((targetCoordinate.bank + 8) % 16)
        , bankGroup(targetCoordinate.bankGroup)
        , dbankGroup((targetCoordinate.bankGroup + 2) % 4)
        , rank(targetCoordinate.rank)
        , row(targetCoordinate.row)
        , column(targetCoordinate.column)
    {}

    std::size_t bank = 0;
    std::size_t dbank = 0;
    std::size_t bankGroup = 0;
    std::size_t dbankGroup = 0;
    std::size_t rank = 0;
    std::size_t row = 0;
    std::size_t column = 0;
    std::size_t subChannel = 0;
};

struct LPDDR6Command {
    LPDDR6Command() = default;
    LPDDR6Command(timestamp_t timestamp, CmdType type, LPDDR6TargetCoordinate targetCoord = {}, const uint8_t * data = nullptr, std::size_t sz_bits = 0);
    LPDDR6Command(const Command& command);

    timestamp_t timestamp = 0;
    CmdType type = CmdType::NOP;
    LPDDR6TargetCoordinate targetCoordinate{};
    const uint8_t * data = 0x00;
    std::size_t sz_bits = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6COMMAND_H */
