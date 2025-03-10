#ifndef DRAMPOWER_UTIL_DATABUS
#define DRAMPOWER_UTIL_DATABUS

#include <cstddef>

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/util/databus_helpermacros.h>

#include <DRAMUtils/config/toggling_rate.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util {

enum class DataBusMode {
    Bus = 0,
    TogglingRate
};

// DataBus class
template<std::size_t blocksize = 64, std::size_t max_bitset_size = 0, std::size_t maxburst_length = 0>
class DataBus {

public:
    using Bus_t = util::Bus<blocksize, max_bitset_size, maxburst_length>;
    using IdlePattern = util::BusIdlePatternSpec;
    using InitPattern = util::BusInitPatternSpec;

public:
    DataBus(std::size_t numberOfDevices, std::size_t width, std::size_t dataRate,
        IdlePattern idlePattern, InitPattern initPattern,
            DRAMUtils::Config::TogglingRateIdlePattern togglingRateIdlePattern = DRAMUtils::Config::TogglingRateIdlePattern::Z,
            const double togglingRate = 0.0, const double dutyCycle = 0.0,
            DataBusMode busType = DataBusMode::Bus
        )
        : busRead(width * numberOfDevices, dataRate, idlePattern, initPattern)
        , busWrite(width * numberOfDevices, dataRate, idlePattern, initPattern)
        , togglingHandleRead(width * numberOfDevices, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , togglingHandleWrite(width * numberOfDevices, dataRate, togglingRate, dutyCycle, togglingRateIdlePattern, false)
        , busType(busType)
        , numberOfDevices(numberOfDevices)
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

    std::size_t getNumberOfDevices() const {
        return numberOfDevices;
    }

    std::size_t getCombinedBusWidth() const {
        return width * numberOfDevices;
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
    std::size_t numberOfDevices;
    std::size_t dataRate;
    std::size_t width;
};

/** DataBusContainer class
 *  This class allows the selection of multiple DataBus types and a fallback type.
 *  The DataBus types are defined in a type_sequence.
 *  The fallback type is used if the DataBus type_sequence is empty.
 *  Internally, the DataBusContainer uses a std::variant to store the DataBus types.
 */
template <typename Seq, typename fallback_t = void, typename = void>
class DataBusContainer
{
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainer cannot be initialized. Check the DataBus type_sequence and the fallback type.");
};

// Ensure fallback not in type_sequence DRAMUtils::util::is_one_of<T, Seq>::value
template <typename... Ts, typename Fallback>
class DataBusContainer<DRAMUtils::util::type_sequence<Ts...>, Fallback, std::enable_if_t<!DRAMUtils::util::is_one_of<Fallback, DRAMUtils::util::type_sequence<Ts...>>::value>> {

// Public type definitions
public:
    using VariantTypeSequence_t = DRAMUtils::util::type_sequence<Ts...>;
    using UnifiedVariantSequence_t = DRAMUtils::util::type_sequence<Ts..., Fallback>;
    using UnifiedVariant_t = std::variant<Ts..., Fallback>;

// Private type definitions
private:
    using IdlePattern = util::BusIdlePatternSpec;
    using InitPattern = util::BusInitPatternSpec;
    using DataBusMode = util::DataBusMode;


// Type safe builder tag
private:
    template <size_t N>
    struct BuilderTag {
        static constexpr size_t value = N;
    };

// Builder
public:
    struct BuilderData {
        std::optional<uint64_t> numberOfDevices;
        std::optional<std::size_t> width;
        std::optional<std::size_t> dataRate;
        std::optional<IdlePattern> idlePattern;
        std::optional<InitPattern> initPattern;
        std::optional<DRAMUtils::Config::TogglingRateIdlePattern> togglingRateIdlePattern;
        std::optional<double> togglingRate;
        std::optional<double> dutyCycle;
        std::optional<DataBusMode> busType;
    };

private:
    template <typename T>
    struct BuilderResult {
        uint64_t numberOfDevices;
        std::size_t width;
        T variant;
    };

public:
    /** This Builder class is used to create a DataBusContainer.
     *  The Builder class uses a tag system to ensure that all required parameters are set exactly once.
     */
    template <typename BuilderTag_t = BuilderTag<0>>
    class Builder {
    private:
        BuilderData data;

    public:
        Builder(const BuilderData& data) : data(data) {}
        Builder(BuilderData&& data) : data(std::move(data)) {}
        Builder() = default;

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 1)>
        >
        auto setNumberOfDevices(const uint64_t numberOfDevices) {
            this->data.numberOfDevices = numberOfDevices;
            return Builder<BuilderTag<Tag | 1>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 2)>
        >
        auto setWidth(std::size_t width) {
            this->data.width = width;
            return Builder<BuilderTag<Tag | 2>>{std::move(data)};
        }


        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 4)>
        >
        auto setDataRate(std::size_t dataRate) {
            this->data.dataRate = dataRate;
            return Builder<BuilderTag<Tag | 4>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 8)>
        >
        auto setIdlePattern(IdlePattern idlePattern) {
            this->data.idlePattern = idlePattern;
            return Builder<BuilderTag<Tag | 8>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 16)>
        >
        auto setInitPattern(InitPattern initPattern) {
            this->data.initPattern = initPattern;
            return Builder<BuilderTag<Tag | 16>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 32)>
        >
        auto setTogglingRateIdlePattern(DRAMUtils::Config::TogglingRateIdlePattern togglingRateIdlePattern) {
            this->data.togglingRateIdlePattern = togglingRateIdlePattern;
            return Builder<BuilderTag<Tag | 32>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 64)>
        >
        auto setTogglingRate(double togglingRate) {
            this->data.togglingRate = togglingRate;
            return Builder<BuilderTag<Tag | 64>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 128)>
        >
        auto setDutyCycle(double dutyCycle) {
            this->data.dutyCycle = dutyCycle;
            return Builder<BuilderTag<Tag | 128>>{std::move(data)};
        }

        template <
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<0 == (Tag & 256)>
        >
        auto setBusType(DataBusMode busType) {
            this->data.busType = busType;
            return Builder<BuilderTag<Tag | 256>>{std::move(data)};
        }

        template<typename T,
            std::size_t Tag = BuilderTag_t::value,
            typename = std::enable_if_t<511 == Tag>,
            typename = std::enable_if_t<DRAMUtils::util::is_one_of<std::decay_t<T>, UnifiedVariantSequence_t>::value> // For a better error message
        >
        auto build() {
            return BuilderResult<T>{
                data.numberOfDevices.value(),
                data.width.value(),
                T {
                    data.numberOfDevices.value(),
                    data.width.value(),
                    data.dataRate.value(),
                    data.idlePattern.value(),
                    data.initPattern.value(),
                    data.togglingRateIdlePattern.value(),
                    data.togglingRate.value(),
                    data.dutyCycle.value(),
                    data.busType.value()
                }
            };
        }

        const BuilderData& getData() const {
            return data;
        }

        BuilderData consumeData() {
            return std::move(data);
        }
    };

// public Builder Types
public:
    using Builder_t = Builder<>;
    using BuilderReadyTag_t = BuilderTag<511>;
    using ReadyBuilder_t = Builder<BuilderReadyTag_t>;
    using BuilderData_t = BuilderData;

// Internal storage
private:
    UnifiedVariant_t storage;
    std::size_t numberOfDevices;
    std::size_t width;
    
// Constructors using BuilderResult
public:
    template <typename T, std::enable_if_t<DRAMUtils::util::is_one_of<std::decay_t<T>, UnifiedVariantSequence_t>::value, int> = 0>
    explicit DataBusContainer(BuilderResult<T>&& builderresult)
        : storage(T{std::move(builderresult.variant)})
        , numberOfDevices(builderresult.numberOfDevices)
        , width(builderresult.width)
    {}

    // No default constructor
    DataBusContainer() = delete;

// Member functions
public:
    std::size_t getNumberOfDevices() const {
        return numberOfDevices;
    }

    std::size_t getWidth() const {
        return width;
    }

    UnifiedVariant_t& getVariant() {
        return storage;
    }

    const UnifiedVariant_t& getVariant() const {
        return storage;
    }

};

template <typename Seq, typename fallback_t = void, typename = void>
class DataBusContainerProxy
{
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainerProxy cannot be initialized. Check the DataBus type_sequence and the fallback type.");
};

template <typename... Tss, typename fallback_t>
class DataBusContainerProxy<DRAMUtils::util::type_sequence<Tss...>, fallback_t> {

public:
    using DataBusContainer_t = DataBusContainer<DRAMUtils::util::type_sequence<Tss...>, fallback_t>;
    using Builder_t = typename DataBusContainer_t::Builder_t;
    using UnifiedVariant_t = typename DataBusContainer_t::UnifiedVariant_t;

private:
    DataBusContainer_t dataBusContainer;

public:
// Forwarding constructor
    template<typename... Args>
    DataBusContainerProxy(Args&&... args) : dataBusContainer(std::forward<Args>(args)...) {}

// Forwarding member functions
    void loadWrite(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        std::visit([timestamp, n_bits, data](auto && arg) {
            arg.loadWrite(timestamp, n_bits, data);
        }, dataBusContainer.getVariant());
    }

    void loadRead(timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        std::visit([timestamp, n_bits, data](auto && arg) {
            arg.loadRead(timestamp, n_bits, data);
        }, dataBusContainer.getVariant());
    }

    void enableTogglingRate(timestamp_t timestamp) {
        std::visit([timestamp](auto && arg) {
            arg.enableTogglingRate(timestamp);
        }, dataBusContainer.getVariant());
    }

    void enableBus(timestamp_t timestamp) {
        std::visit([timestamp](auto && arg) {
            arg.enableBus(timestamp);
        }, dataBusContainer.getVariant());
    }

    void setTogglingRateDefinition(const DRAMUtils::Config::ToggleRateDefinition &toggleRateDefinition) {
        std::visit([&toggleRateDefinition](auto && arg) {
            arg.setTogglingRateDefinition(toggleRateDefinition);
        }, dataBusContainer.getVariant());
    }

    bool isTogglingRate() const {
        return std::visit([](auto && arg) {
            return arg.isTogglingRate();
        }, dataBusContainer.getVariant());
    }

    bool isBus() const {
        return std::visit([](auto && arg) {
            return arg.isBus();
        }, dataBusContainer.getVariant());
    }

    timestamp_t lastBurst() const {
        return std::visit([](auto && arg) {
            return arg.lastBurst();
        }, dataBusContainer.getVariant());
    }

    std::size_t getCombinedBusWidth() const {
        return dataBusContainer.getWidth() * dataBusContainer.getNumberOfDevices();
    }

    std::size_t getWidth() const {
        return dataBusContainer.getWidth();
    }

    void get_stats(timestamp_t timestamp, util::bus_stats_t &busReadStats, util::bus_stats_t &busWriteStats, util::bus_stats_t &togglingReadState, util::bus_stats_t &togglingWriteState) const {
        std::visit([timestamp, &busReadStats, &busWriteStats, &togglingReadState, &togglingWriteState](auto && arg) {
            arg.get_stats(timestamp, busReadStats, busWriteStats, togglingReadState, togglingWriteState);
        }, dataBusContainer.getVariant());
    }

};


// Helper macros for CREATE_DATABUS_TYPESEQUENCE
#define __DRAMPOWER_DATABUS_ENTRY(INDEX, BASE) util::DataBus<0, (BASE) * (INDEX)>
#define __DRAMPOWER_DATABUS_EXPAND_ENTRIES(BASE, COUNT) \
    __DRAMPOWER_DATABUS_EXPAND_COMMA(BASE, COUNT, __DRAMPOWER_DATABUS_ENTRY)
// This macro is used to create a DataBus type sequence with a given base and number of entries.
#define DRAMPOWER_DATABUS_CREATE_TYPESEQUENCE(BASE, NUM_ENTRIES) \
    DRAMUtils::util::type_sequence< \
        __DRAMPOWER_DATABUS_EXPAND_ENTRIES(BASE, NUM_ENTRIES) \
    >

// Helper macros for DRAMPOWER_DATABUS_SWITCH
#define __DRAMPOWER_DATABUS_SWITCH_CASE_ENTRY_0(databus, builder, maxburst_length, BASE) \
    if (memSpec.bitWidth * memSpec.numberOfDevices <= (BASE)) { \
        databus = builder.build<util::DataBus<0, (BASE), (maxburst_length)>>(); \
    }
#define __DRAMPOWER_DATABUS_SWITCH_CASE_ENTRY_N(databus, builder, maxburst_length, INDEX, BASE) \
    else if ((INDEX - 1) * (BASE) < memSpec.bitWidth * memSpec.numberOfDevices && \
        memSpec.bitWidth * memSpec.numberOfDevices <= (INDEX) * (BASE)) { \
        databus = builder.build<util::DataBus<0, (INDEX) * (BASE), (maxburst_length)>>(); \
    }
#define __DRAMPOWER_DATABUS_EXPAND_SWITCH(databus, builder, maxburst_length, BASE, COUNT) \
    __DRAMPOWER_DATABUS_EXPAND(databus, builder, maxburst_length, BASE, COUNT, __DRAMPOWER_DATABUS_SWITCH_CASE_ENTRY_N)
// This macro is used to create a DataBusContainerProxy or a DataBusContainer
#define DRAMPOWER_DATABUS_SELECTOR(databus, builder, TARGET, maxburst_length, BASE, COUNT) \
    __DRAMPOWER_DATABUS_SWITCH_CASE_ENTRY_0(databus, builder, maxburst_length, BASE) \
    __DRAMPOWER_DATABUS_EXPAND_SWITCH(databus, builder, maxburst_length, BASE, COUNT)
} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_DATABUS */
