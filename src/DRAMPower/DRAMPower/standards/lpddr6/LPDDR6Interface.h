#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H

#include "DRAMPower/standards/lpddr6/LPDDR6Pattern.h"
#include "DRAMPower/util/bus.h"
#include "DRAMPower/util/databus_presets.h"
#include "DRAMPower/util/clock.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/standards/lpddr6/LPDDR6Command.h"
#include "DRAMPower/data/stats.h"

#include "DRAMPower/util/PatternHandler.h"
#include "DRAMPower/util/dbi.h"

#include "DRAMPower/memspec/MemSpecLPDDR6.h"

#include "DRAMPower/simconfig/simconfig.h"

#include <array>
#include <cstdint>
#include <cstddef>
#include <tuple>

namespace DRAMPower {

struct LPDDR6InterfaceMemSpec {
    LPDDR6InterfaceMemSpec(const MemSpecLPDDR6& memSpec)
        : dataRate(memSpec.dataRate)
        , burstLength(memSpec.burstLength)
        , bitWidth(memSpec.bitWidth)
        , wckAlwaysOnMode(memSpec.wckAlwaysOnMode)
    {}

    uint64_t dataRate;
    uint64_t burstLength;
    uint64_t bitWidth;
    bool wckAlwaysOnMode;
};

class LPDDR6Interface : public util::Serialize, public util::Deserialize {
// Public constants
public:
    static constexpr std::array<uint8_t, 288> dataBitMapping = {
        0,  4,  8,  12, 16, 20, 24, 28, 32, 36, 40, 44, // Beat 0
        1,  5,  9,  13, 17, 21, 25, 29, 33, 37, 41, 45, // Beat 1
        2,  6,  10, 14, 18, 22, 26, 30, 34, 38, 42, 46, // Beat 2
        3,  7,  11, 15, 19, 23, 27, 31, 35, 39, 43, 47, // Beat 3

        48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, // Beat 4
        49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, // Beat 5
        50, 54, 58, 62, 66, 70, 74, 78, 82, 86, 90, 94, // Beat 6
        51, 55, 59, 63, 67, 71, 75, 79, 83, 87, 91, 95, // Beat 7

        96, 100, 104, 108, 112, 116, 120, 124, // Beat 8
        97, 101, 105, 109, 113, 117, 121, 125, // Beat 9
        98, 102, 106, 110, 114, 118, 122, 126, // Beat 10
        99, 103, 107, 111, 115, 119, 123, 127, // Beat 11

        128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, // Beat 12
        129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, // Beat 13
        130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, // Beat 14
        131, 135, 139, 143, 147, 151, 155, 159, 163, 167, 171, 175, // Beat 15

        176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, // Beat 16
        177, 181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, // Beat 17
        178, 182, 186, 190, 194, 198, 202, 206, 210, 214, 218, 222, // Beat 18
        179, 183, 187, 191, 195, 199, 203, 207, 211, 215, 219, 223, // Beat 19

        224, 228, 232, 236, 240, 244, 248, 252, // Beat 20
        225, 229, 233, 237, 241, 245, 249, 253, // Beat 21
        226, 230, 234, 238, 242, 246, 250, 254, // Beat 22
        227, 231, 235, 239, 243, 247, 251, 255, // Beat 23

        // Padding
        0, 0, 0, 0, 0, 0, 0, 0, // DBI1
        0, 0, 0, 0, 0, 0, 0, 0, // DBI2
        0, 0, 0, 0, 0, 0, 0, 0, // MetaData1
        0, 0, 0, 0, 0, 0, 0, 0, // MetaData2
    };
    static constexpr std::size_t cmdBusWidth = 4;
    static constexpr std::size_t dataBusWidth = 12;
    static constexpr std::size_t burstLength = 24;
    static constexpr uint64_t cmdBusInitPattern = 0;
    
    static constexpr std::size_t additionalDataPerBurst = 32;
    static constexpr std::size_t dataBitsPerBurst = burstLength * dataBusWidth;
    static constexpr std::size_t dataBitsPerBurstNoMetaNoDBI = dataBitsPerBurst - additionalDataPerBurst;
    static_assert(0 == (burstLength * dataBusWidth) % 8, "Invalid dataBus configuration");
    static constexpr std::size_t dataBytesPerBurst = dataBitsPerBurst / 8;
    static_assert(0 == dataBitsPerBurst % 8, "Invalid burst length");

// Public type definitions
    class DataFormatter {
    public:
        std::tuple<const uint8_t*, std::size_t> formatData(
            const uint8_t* inputData, std::size_t n_bits,
            std::optional<std::reference_wrapper<const std::vector<bool>>> InversionState
        );
        std::vector<uint8_t> m_formatResult{};
        std::array<uint16_t, 2> m_metaData = {0, 0};
    };

public:
    using commandbus_t = util::Bus<cmdBusWidth>;
    using databus_t = util::databus_presets::databus_preset_t;
    using patternHandler_t = PatternHandler<CmdType, pattern_descriptor_LPDDR6::t, LPDDR6TargetCoordinate, LPDDR6Encoder, LPDDR6PatternExtraData>;

// Public constructor
public:
    LPDDR6Interface(const MemSpecLPDDR6 &memSpec, const config::SimConfig &simConfig, bool enabled = true);

// Public member functions
public:
// Member functions
    timestamp_t getLastCommandTime() const;
    void doCommand(const LPDDR6Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;
// Extensions
    void enable(timestamp_t timestamp);
    void disable(timestamp_t timestamp);
    bool isEnabled() const;
    void enableDBI(bool enable) {
        m_dbi.enable(enable);
    }
    void setMetaData(uint16_t metaDataB1, uint16_t metaDataB2);
    std::tuple<uint16_t, uint16_t> getMetaData() const;
    void setParityCheckMode(bool state);
    bool getParityCheckMode() const;

// Private member functions
private:
    void registerPatterns();
    std::optional<const uint8_t *> handleDBIInterface(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    void handleDQs(const LPDDR6Command& cmd, util::Clock &dqs, size_t length);
    void handleCommandBus(const LPDDR6Command& cmd);
    void handleData(const LPDDR6Command& cmd, bool read);
    void endOfSimulation(timestamp_t timestamp);

// Private members
    LPDDR6InterfaceMemSpec m_memSpec;
    commandbus_t m_commandBus;
    databus_t m_dataBus;
    util::Clock m_readDQS;
    util::Clock m_wck;
    util::Clock m_clock;
    util::DBI<uint8_t, 2, util::PinState::L, util::StaticDBI> m_dbi;
    DataFormatter m_formatter;
    patternHandler_t m_patternHandler;
    timestamp_t m_last_command_time = 0;
    bool m_enabled;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6INTERFACE_H */
