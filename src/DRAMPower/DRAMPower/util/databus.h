#ifndef DRAMPOWER_UTIL_DATABUS
#define DRAMPOWER_UTIL_DATABUS

#include <cstddef>
#include <type_traits>
#include <utility>


#include <DRAMPower/util/bus.h>
#include <DRAMPower/util/databus_types.h>
#include <DRAMPower/util/extension_manager_static.h>
#include <DRAMPower/util/databus_types.h>
#include <DRAMPower/dram/Interface.h>

#include <DRAMUtils/config/toggling_rate.h>
#include <DRAMUtils/util/types.h>

namespace DRAMPower::util {

// DataBus class
template<std::size_t max_bitset_size = 0, typename... BusExtensions>
class DataBus {

public:
    using Bus_t = util::Bus<max_bitset_size, BusExtensions...>;
    using ExtensionManager_t = typename Bus_t::ExtensionManager_t;
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
        // extensionManager.template callHook<databus_extensions::DataBusHook::onInit>([this](auto& ext) {
        //     ext.onInit(this->width);
        // });
    }
    DataBus(DataBusConfig&& config)
    : DataBus(config.width, config.dataRate, config.idlePattern, config.initPattern,
        config.togglingRateIdlePattern, config.togglingRate, config.dutyCycle, config.busType
    ) {}

private:
    void load(Bus_t &bus, TogglingHandle &togglingHandle, timestamp_t timestamp, std::size_t n_bits, const uint8_t *data = nullptr) {
        switch(busType) {
            case DataBusMode::Bus: {
                if (nullptr == data || 0 == n_bits) {
                    // No data to load, skip burst
                    return;
                }
                bus.load(timestamp, data, n_bits);
                break;
            }
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

    constexpr ExtensionManager_t& getExtensionManagerRead() {
        return busRead.getExtensionManager();
    }
    constexpr ExtensionManager_t& getExtensionManagerWrite() {
        return busWrite.getExtensionManager();
    }

    // Proxy getExtension
    template<typename Extension>
    constexpr auto& getExtensionRead() {
        return busRead.getExtensionManager().template getExtension<Extension>();
    }
    template<typename Extension>
    constexpr auto& getExtensionWrite() {
        return busWrite.getExtensionManager().template getExtension<Extension>();
    }
    template<typename Extension>
    constexpr const auto& getExtensionRead() {
        return busRead.getExtensionManager().template getExtension<Extension>();
    }
    template<typename Extension>
    constexpr const auto& getExtensionWrite() {
        return busWrite.getExtensionManager().template getExtension<Extension>();
    }

    template<typename Extension, typename Func>
    constexpr decltype(auto) withExtensionWrite(Func&& func) {
        return busWrite.getExtensionManager().template withExtension<Extension>(std::forward<Func>(func));
    }
    template<typename Extension, typename Func>
    constexpr decltype(auto) withExtensionRead(Func&& func) {
        return busRead.getExtensionManager().template withExtension<Extension>(std::forward<Func>(func));
    }

    // Proxy hasExtension
    template<typename Extension>
    constexpr static bool hasExtension() {
        return (false || ... || (std::is_same_v<Extension, BusExtensions>));
    }

private:
    Bus_t busRead;
    Bus_t busWrite;
    TogglingHandle togglingHandleRead;
    TogglingHandle togglingHandleWrite;
    DataBusMode busType;
    std::size_t dataRate;
    std::size_t width;
    ExtensionManager_t extensionManager;
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
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainer cannot be initialized. Check the DataBus type_sequence.");
};

template <typename... Ts>
class DataBusContainer<DRAMUtils::util::type_sequence<Ts...>,
    // Types must be unique and the sequence length must be greater than 0
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
    // The constructor is only valid if T is in the valid variant types Ts... / in the type sequence of the variant
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
    static_assert(DRAMUtils::util::always_false<Seq>::value, "DataBusContainerProxy cannot be initialized. Check the DataBus type_sequence.");
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

    template<typename Extension, typename Func>
    decltype(auto) withExtensionRead(Func&& func) {
        return std::visit([&func](auto && arg) {
            return arg.template withExtensionRead<Extension>(std::forward<Func>(func));
        }, m_dataBusContainer.getVariant());
    }
    template<typename Extension, typename Func>
    decltype(auto) withExtensionWrite(Func&& func) {
        return std::visit([&func](auto && arg) {
            return arg.template withExtensionWrite<Extension>(std::forward<Func>(func));
        }, m_dataBusContainer.getVariant());
    }

    // All databus types must support the given Extension for this to be true
    template<typename Extension>
    static constexpr bool hasExtension() {
        // Check if all DataBus types support the given extension
        return (Tss::template hasExtension<Extension>() && ...);
    }

    // The active databus must support the given Extension for this to be true
    template<typename Extension>
    bool hasExtensionRuntime() const {
        return std::visit([](auto && arg) {
            return arg.template hasExtension<Extension>();
        }, m_dataBusContainer.getVariant());
    }
    
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DATABUS */
