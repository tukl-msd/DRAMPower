#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2COMMAND_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2COMMAND_H

#include "DRAMPower/Types.h"
#include "DRAMPower/command/Command.h"

#include <cstddef>
#include <cstdint>


namespace DRAMPower {

struct HBM2TargetCoordinate {
    HBM2TargetCoordinate() = default;
    HBM2TargetCoordinate(const TargetCoordinate& targetCoordinate);
    HBM2TargetCoordinate(std::size_t bank, std::size_t row, std::size_t column, std::size_t stack, std::size_t pseudoChannel)
        : bank(bank)
        , row(row)
        , column(column)
        , stack(stack)
        , pseudoChannel(pseudoChannel)
    {}

    std::size_t bank = 0;
    std::size_t row = 0;
    std::size_t column = 0;
    std::size_t stack = 0;
    std::size_t pseudoChannel = 0;
};

struct HBM2Command {
    HBM2Command() = default;
    HBM2Command(timestamp_t timestamp, CmdType type, HBM2TargetCoordinate targetCoord = {}, const uint8_t * data = nullptr, std::size_t sz_bits = 0);
    HBM2Command(const Command& command);

    timestamp_t timestamp = 0;
    CmdType type = CmdType::NOP;
    HBM2TargetCoordinate targetCoordinate{};
    const uint8_t * data = 0x00;
    std::size_t sz_bits = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2COMMAND_H */
