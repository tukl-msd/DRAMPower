#ifndef DRAMPOWER_COMMAND_COMMAND_H
#define DRAMPOWER_COMMAND_COMMAND_H

#include <DRAMPower/Types.h>
#include <DRAMPower/command/CmdType.h>

#include <optional>

namespace DRAMPower {

struct TargetCoordinate {
    std::size_t bank = 0;
    std::size_t bankGroup = 0;
    std::size_t rank = 0;
	std::size_t row = 0;
	std::size_t column = 0;

	TargetCoordinate() = default;
	TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id);
	TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id, std::size_t row_id);
	TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id, std::size_t row_id, std::size_t column_id);
};

class Command {
public:
    Command() = default;
	Command(timestamp_t timestamp, CmdType type, TargetCoordinate targetCoord = {}, const uint8_t * data = nullptr, std::size_t sz_bits = 0);

public:
    timestamp_t timestamp = 0;
    CmdType type;
    TargetCoordinate targetCoordinate;


    const uint8_t * data = 0x00;
	std::size_t sz_bits;
    //uint64_t burstLength;
public:
};

}

#endif /* DRAMPOWER_COMMAND_COMMAND_H */
