#ifndef DRAMPOWER_UTIL_DBIALGOS_H
#define DRAMPOWER_UTIL_DBIALGOS_H

#include "DRAMPower/util/dbihelpers.h"
#include "DRAMPower/util/binary_ops.h"
#include "DRAMPower/util/pin_types.h"

#include "DRAMUtils/util/types.h"

#include <optional>


namespace DRAMPower::util {

// Iterator selector for algorithms
struct AlgorithmIteratorType_SubChunk {};

// Iterator selector for dbi class
template<typename DBI, typename AlgorithmType>
struct IteratorSelector {
    static_assert(DRAMUtils::util::always_false<AlgorithmType>::value, "No matching Iterator found for AlgorithmType.");
};
template<typename DBI>
struct IteratorSelector<DBI, AlgorithmIteratorType_SubChunk> {
    using type = SubChunkView<typename DBI::DataBufferIterator_t, DBI::ChunkSize>;
};

// Algorithms
struct StaticDBI {

    using iterator_type_t = AlgorithmIteratorType_SubChunk;

    template <typename Iterator1, typename Iterator2, typename InvertCallbackFunctor>
    static void computeDBI(std::tuple<Iterator1, Iterator1> current,
        std::optional<std::tuple<Iterator2, Iterator2>>,
        PinState idlePattern,
        InvertCallbackFunctor&& invert_callback
    ) {
        auto [cur_it, cur_end] = current;

        std::size_t ones = 0;
        std::size_t zeros = 0;
        for (; cur_it != cur_end; ++cur_it) {
            auto chunk_value = *cur_it;
            ones += BinaryOps::popcount(chunk_value);
            zeros += cur_it.getDigits() - BinaryOps::popcount(chunk_value);
            if (cur_it.last()) {
                const bool invert =
                    (idlePattern == PinState::L && zeros < ones) ||
                    (idlePattern == PinState::H && ones < zeros);
                std::forward<InvertCallbackFunctor>(invert_callback)(invert, cur_it.getTotalChunkIdx());
                ones = 0;
                zeros = 0;
            }
        }
    }

};

template <std::size_t threshold>
struct DynamicDBI {

    using iterator_type_t = AlgorithmIteratorType_SubChunk;

    template <typename Iterator1, typename Iterator2, typename InvertCallbackFunctor>
    static void computeDBI(std::tuple<Iterator1, Iterator1> current,
        std::optional<std::tuple<Iterator2, Iterator2>> previous,
        PinState,
        InvertCallbackFunctor&& invert_callback
    ) {
        auto [cur_it, cur_end] = current;

        if (!previous) {
            // No previous burst -> cannot compute transitions
            for (; cur_it != cur_end; ++cur_it) {
                std::forward<InvertCallbackFunctor>(invert_callback)(
                    false, cur_it.getTotalChunkIdx());
            }
        } else {
            auto [prev_it, prev_end] = *previous;
            std::size_t costnormal = 0;
            for (; cur_it != cur_end; ++cur_it, ++prev_it) {

                // normal transition cost
                costnormal +=
                    BinaryOps::popcount(*cur_it ^ *prev_it);

                if (cur_it.last()) {
                    const bool invert = costnormal > threshold;
                    std::forward<InvertCallbackFunctor>(invert_callback)(
                        invert, cur_it.getTotalChunkIdx());
                    costnormal = 0;
                }
            }
        }
    }
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DBIALGOS_H */
