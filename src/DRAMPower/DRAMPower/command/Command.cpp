#include "Command.h"

namespace DRAMPower {

    TargetCoordinate::TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id)
        : bank(bank_id), bankGroup(bank_group_id), rank(rank_id)
    {};

    TargetCoordinate::TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id, std::size_t row_id)
        : bank(bank_id), bankGroup(bank_group_id), rank(rank_id), row(row_id)
    {};

    TargetCoordinate::TargetCoordinate(std::size_t bank_id, std::size_t bank_group_id, std::size_t rank_id, std::size_t row_id, std::size_t column_id)
        : bank(bank_id), bankGroup(bank_group_id), rank(rank_id), row(row_id), column(column_id)
    {};

    Command::Command(timestamp_t timestamp, CmdType type, TargetCoordinate targetCoord, const uint8_t * data, std::size_t sz_bits)
        : timestamp(timestamp)
        , type(type)
        , targetCoordinate(targetCoord)
        , data(data)
        , sz_bits(sz_bits)
    {};

} // namespace DRAMPower