#ifndef DRAMPOWER_UTIL_DATABUS
#define DRAMPOWER_UTIL_DATABUS

#include <cstddef>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>

#include <DRAMUtils/config/toggling_rate.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util {

enum class DataBusMode {
    Bus = 0,
    TogglingRate
};

namespace detail {
    template <size_t N>
    struct DataBusBuilderTag {
        static constexpr size_t value = N;
    };
    struct DataBusBuilderData {
        std::optional<std::size_t> width;
        std::optional<std::size_t> dataRate;
        std::optional<util::BusIdlePatternSpec> idlePattern;
        std::optional<util::BusInitPatternSpec> initPattern;
        std::optional<DRAMUtils::Config::TogglingRateIdlePattern> togglingRateIdlePattern;
        std::optional<double> togglingRate;
        std::optional<double> dutyCycle;
        std::optional<DataBusMode> busType;
    };
} // namespace detail

/** This Builder class is used to create a DataBus.
 *  The Builder class uses a tag system to ensure that all required parameters are set exactly once.
 */
template <typename BuilderTag_t = detail::DataBusBuilderTag<0>>
class DataBusBuilder {
// Private type definitions
private:
    template <size_t N>
    using Tag_t = detail::DataBusBuilderTag<N>;
    using BuilderData_t = detail::DataBusBuilderData;

// Private member variables
private:
    BuilderData_t m_data;

public:
// Public Constructors
    DataBusBuilder(const BuilderData_t& data) : m_data(data) {}
    DataBusBuilder(BuilderData_t&& data) : m_data(std::move(data)) {}
    DataBusBuilder() = default;

// Public build member functions
    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 1)>
    >
    auto setWidth(std::size_t width) {
        m_data.width = width;
        return DataBusBuilder<Tag_t<Tag_v | 1>>{std::move(m_data)};
    }


    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 2)>
    >
    auto setDataRate(std::size_t dataRate) {
        m_data.dataRate = dataRate;
        return DataBusBuilder<Tag_t<Tag_v | 2>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 4)>
    >
    auto setIdlePattern(util::BusIdlePatternSpec idlePattern) {
        m_data.idlePattern = idlePattern;
        return DataBusBuilder<Tag_t<Tag_v | 4>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 8)>
    >
    auto setInitPattern(util::BusInitPatternSpec initPattern) {
        m_data.initPattern = initPattern;
        return DataBusBuilder<Tag_t<Tag_v | 8>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 16)>
    >
    auto setTogglingRateIdlePattern(DRAMUtils::Config::TogglingRateIdlePattern togglingRateIdlePattern) {
        m_data.togglingRateIdlePattern = togglingRateIdlePattern;
        return DataBusBuilder<Tag_t<Tag_v | 16>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 32)>
    >
    auto setTogglingRate(double togglingRate) {
        m_data.togglingRate = togglingRate;
        return DataBusBuilder<Tag_t<Tag_v | 32>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 64)>
    >
    auto setDutyCycle(double dutyCycle) {
        m_data.dutyCycle = dutyCycle;
        return DataBusBuilder<Tag_t<Tag_v | 64>>{std::move(m_data)};
    }

    template <
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<0 == (Tag_v & 128)>
    >
    auto setBusType(DataBusMode busType) {
        m_data.busType = busType;
        return DataBusBuilder<Tag_t<Tag_v | 128>>{std::move(m_data)};
    }

    template<typename T,
        std::size_t Tag_v = BuilderTag_t::value,
        typename = std::enable_if_t<255 == Tag_v>
    >
    auto build() {
        return T {
            m_data.width.value(),
            m_data.dataRate.value(),
            m_data.idlePattern.value(),
            m_data.initPattern.value(),
            m_data.togglingRateIdlePattern.value(),
            m_data.togglingRate.value(),
            m_data.dutyCycle.value(),
            m_data.busType.value()
        };
    }

    const BuilderData_t& getData() const {
        return m_data;
    }

    BuilderData_t consumeData() {
        return std::move(m_data);
    }
};

// DataBus class
template<std::size_t blocksize = 64, std::size_t max_bitset_size = 0, std::size_t maxburst_length = 0>
class DataBus {

public:
    using Bus_t = util::Bus<blocksize, max_bitset_size, maxburst_length>;
    using IdlePattern_t = util::BusIdlePatternSpec;
    using InitPattern_t = util::BusInitPatternSpec;

public:
    DataBus(std::size_t width, std::size_t dataRate,
        IdlePattern_t idlePattern, InitPattern_t initPattern,
            DRAMUtils::Config::TogglingRateIdlePattern togglingRateIdlePattern = DRAMUtils::Config::TogglingRateIdlePattern::Z,
            const double togglingRate = 0.0, const double dutyCycle = 0.0,
            DataBusMode busType = DataBusMode::Bus
        )
        : busRead(width, dataRate, idlePattern, initPattern)
        , busWrite(width, dataRate, idlePattern, initPattern)
        , togglingHandleRead(width, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , togglingHandleWrite(width, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , busType(busType)
        , dataRate(dataRate)
        , width(width)
    {
        switch(busType) {
            case DataBusMode::Bus:
                busWrite.enable(0);
                busRead.enable(0);
                break;
            case DataBusMode::TogglingRate:
                togglingHandleRead.enable(0);
                togglingHandleWrite.enable(0);
                break;
        }
    }

private:
    void load(Bus_t &bus, TogglingHandle &togglingHandle, timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        switch(busType) {
            case DataBusMode::Bus:
                if (nullptr == data || 0 == n_bits) {
                    // No data to load, skip burst
                    return;
                }
                bus.load(timestamp, data, n_bits);
                break;
            case DataBusMode::TogglingRate:
                togglingHandle.incCountBitLength(timestamp, n_bits);
                break;
        }
    }

public:
    void loadWrite(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        load(busWrite, togglingHandleWrite, timestamp, n_bits, data);
    }
    void loadRead(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        load(busRead, togglingHandleRead, timestamp, n_bits, data);
    }

    void enableBus(timestamp_t timestamp) {
        busRead.enable(timestamp);
        busWrite.enable(timestamp);
        togglingHandleRead.disable(timestamp);
        togglingHandleWrite.disable(timestamp);
        busType = DataBusMode::Bus;
    }

    void enableTogglingRate(timestamp_t timestamp) {
        busRead.disable(timestamp);
        busWrite.disable(timestamp);
        togglingHandleRead.enable(timestamp);
        togglingHandleWrite.enable(timestamp);
        busType = DataBusMode::TogglingRate;
    }

    void setTogglingRateDefinition(DRAMUtils::Config::ToggleRateDefinition toggleratedefinition) {
        togglingHandleRead.setTogglingRateAndDutyCycle(toggleratedefinition.togglingRateRead, toggleratedefinition.dutyCycleRead, toggleratedefinition.idlePatternRead);
        togglingHandleWrite.setTogglingRateAndDutyCycle(toggleratedefinition.togglingRateWrite, toggleratedefinition.dutyCycleWrite, toggleratedefinition.idlePatternWrite);
    }

    timestamp_t lastBurst() const {
        switch(busType) {
            case DataBusMode::Bus:
                return std::max(busWrite.get_lastburst_timestamp(), busRead.get_lastburst_timestamp());
            case DataBusMode::TogglingRate:
                return std::max(togglingHandleRead.get_lastburst_timestamp(), togglingHandleWrite.get_lastburst_timestamp());
        }
        assert(false);
        return 0;
    }

    bool isBus() const {
        return DataBusMode::Bus == busType;
    }

    bool isTogglingRate() const {
        return DataBusMode::TogglingRate == busType;
    }

    std::size_t getWidth() const {
        return width;
    }

    std::size_t getDataRate() const {
        return dataRate;
    }

    void get_stats(timestamp_t timestamp,
        util::bus_stats_t &busReadStats,
        util::bus_stats_t &busWriteStats,
        util::bus_stats_t &togglingHandleReadStats,
        util::bus_stats_t &togglingHandleWriteStats) const
    {
        busReadStats += busRead.get_stats(timestamp);
        busWriteStats += busWrite.get_stats(timestamp);
        togglingHandleReadStats += togglingHandleRead.get_stats(timestamp);
        togglingHandleWriteStats += togglingHandleWrite.get_stats(timestamp);
    }

private:
    Bus_t busRead;
    Bus_t busWrite;
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;
    DataBusMode busType;
    std::size_t dataRate;
    std::size_t width;
};

/** DataBusContainer class
 *  This class allows the selection of multiple DataBus types.
 *  The DataBus types are defined in a type_sequence and must be unique.
 *  At least one DataBus type must be defined.
 *  Internally, the DataBusContainer uses a std::variant to store the DataBus types.
 */
template <typename Seq, typename = void>
class DataBusContainer
{
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainer cannot be initialized. Check the DataBus type_sequence and the fallback type.");
};

template <typename... Ts>
class DataBusContainer<DRAMUtils::util::type_sequence<Ts...>,
    std::enable_if_t<DRAMUtils::util::unique_types<Ts...>::value && sizeof...(Ts) != 0>
> {

// Public type definitions
public:
    using UnifiedVariantSequence_t = DRAMUtils::util::type_sequence<Ts...>;
    using UnifiedVariant_t = std::variant<Ts...>;

// Internal storage
private:
    std::size_t m_width;
    UnifiedVariant_t m_databusVariant;
    
// Constructors
public:
    template <typename T, std::enable_if_t<DRAMUtils::util::is_one_of<std::decay_t<T>, UnifiedVariantSequence_t>::value, int> = 0>
    explicit DataBusContainer(T&& databus)
        : m_width(databus.getWidth())
        , m_databusVariant(std::move(databus))
    {
    }

    // No default constructor
    DataBusContainer() = delete;

// Member functions
public:
    std::size_t getWidth() const {
        return m_width;
    }

    UnifiedVariant_t& getVariant() {
        return m_databusVariant;
    }

    const UnifiedVariant_t& getVariant() const {
        return m_databusVariant;
    }

};

template <typename Seq>
class DataBusContainerProxy
{
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainerProxy cannot be initialized. Check the DataBus type_sequence and the fallback type.");
};

template <typename... Tss>
class DataBusContainerProxy<DRAMUtils::util::type_sequence<Tss...>> {

// Public type definitions
public:
    using DataBusContainer_t = DataBusContainer<DRAMUtils::util::type_sequence<Tss...>>;

// Private member variables
private:
    DataBusContainer_t m_dataBusContainer;

public:
// Forwarding constructor
    template<typename... Args>
    DataBusContainerProxy(Args&&... args) : m_dataBusContainer(std::forward<Args>(args)...) {}

// Deleted default constructor
    DataBusContainerProxy() = delete;

// Forwarding member functions
    void loadWrite(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        std::visit([timestamp, n_bits, data](auto && arg) {
            arg.loadWrite(timestamp, n_bits, data);
        }, m_dataBusContainer.getVariant());
    }

    void loadRead(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        std::visit([timestamp, n_bits, data](auto && arg) {
            arg.loadRead(timestamp, n_bits, data);
        }, m_dataBusContainer.getVariant());
    }

    void enableTogglingRate(timestamp_t timestamp) {
        std::visit([timestamp](auto && arg) {
            arg.enableTogglingRate(timestamp);
        }, m_dataBusContainer.getVariant());
    }

    void enableBus(timestamp_t timestamp) {
        std::visit([timestamp](auto && arg) {
            arg.enableBus(timestamp);
        }, m_dataBusContainer.getVariant());
    }

    void setTogglingRateDefinition(const DRAMUtils::Config::ToggleRateDefinition &toggleRateDefinition) {
        std::visit([&toggleRateDefinition](auto && arg) {
            arg.setTogglingRateDefinition(toggleRateDefinition);
        }, m_dataBusContainer.getVariant());
    }

    bool isTogglingRate() const {
        return std::visit([](auto && arg) {
            return arg.isTogglingRate();
        }, m_dataBusContainer.getVariant());
    }

    bool isBus() const {
        return std::visit([](auto && arg) {
            return arg.isBus();
        }, m_dataBusContainer.getVariant());
    }

    timestamp_t lastBurst() const {
        return std::visit([](auto && arg) {
            return arg.lastBurst();
        }, m_dataBusContainer.getVariant());
    }

    std::size_t getWidth() const {
        return m_dataBusContainer.getWidth();
    }

    void get_stats(timestamp_t timestamp, util::bus_stats_t &busReadStats, util::bus_stats_t &busWriteStats, util::bus_stats_t &togglingReadState, util::bus_stats_t &togglingWriteState) const {
        std::visit([timestamp, &busReadStats, &busWriteStats, &togglingReadState, &togglingWriteState](auto && arg) {
            arg.get_stats(timestamp, busReadStats, busWriteStats, togglingReadState, togglingWriteState);
        }, m_dataBusContainer.getVariant());
    }

};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DATABUS */
