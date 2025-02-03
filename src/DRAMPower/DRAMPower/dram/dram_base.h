#ifndef DRAMPOWER_DRAM_DRAM_BASE_H
#define DRAMPOWER_DRAM_DRAM_BASE_H

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <DRAMPower/data/energy.h>
#include <DRAMPower/data/stats.h>

#include <DRAMUtils/config/toggling_rate.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <vector>
#include <limits>
#include <optional>

namespace DRAMPower {

using namespace DRAMUtils::Config;

template <typename CommandEnum>
class dram_base {
public:
    using commandEnum_t = CommandEnum;
    using commandHandler_t = std::function<void(const Command&)>;
    using commandCount_t = std::vector<std::size_t>;
    using commandRouter_t = std::vector<commandHandler_t>;
    using commandPattern_t = std::vector<pattern_descriptor::t>;
    using commandPatternMap_t = std::vector<commandPattern_t>;
    using implicitCommand_t = std::function<void(void)>;
    using implicitCommandListEntry_t = std::pair<timestamp_t, implicitCommand_t>;
    using implicitCommandList_t = std::deque<implicitCommandListEntry_t>;

public:
    commandCount_t commandCount;
private:
    commandRouter_t commandRouter;
    commandPatternMap_t commandPatternMap;
protected:
    PatternEncoder encoder;
    uint64_t lastPattern;
private:
    implicitCommandList_t implicitCommandList;
    timestamp_t last_command_time;
    std::optional<ToggleRateDefinition> toggleRateDefinition = std::nullopt;

public:
    dram_base(PatternEncoderOverrides encoderoverrides)
        : commandCount(static_cast<std::size_t>(CommandEnum::COUNT), 0)
        , commandRouter(static_cast<std::size_t>(CommandEnum::COUNT), [](const Command&) {})
        , commandPatternMap(static_cast<std::size_t>(CommandEnum::COUNT), commandPattern_t {})
        , encoder(encoderoverrides)
        , lastPattern(0)
        {
            init();
        };

private:
    void init()
    {
        this->lastPattern = getInitEncoderPattern();
    };

public:
    virtual ~dram_base() = 0;
private:
    void internal_handle_interface(const Command& cmd) 
    {
        if (this->toggleRateDefinition) {
            handle_interface_toggleRate(cmd);
        } else {
            handle_interface(cmd);
        }
    }
private:
    virtual void handle_interface(const Command& cmd) = 0;
    virtual void handle_interface_toggleRate(const Command& cmd) = 0;
    virtual timestamp_t update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleRateDefinition) = 0;
    virtual uint64_t getInitEncoderPattern()
    {
        // Default encoder init pattern
        return 0;
    };
public:
    virtual energy_t calcCoreEnergy(timestamp_t timestamp) = 0;
    virtual interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp) = 0;
    virtual SimulationStats getStats() = 0;
    virtual uint64_t getBankCount() = 0;
    virtual uint64_t getRankCount() = 0;
    virtual uint64_t getDeviceCount() = 0;

    double getTotalEnergy(timestamp_t timestamp)
    {
        return calcCoreEnergy(timestamp).total() + calcInterfaceEnergy(timestamp).total();
    };

public:
    uint64_t getCommandPattern(const Command& cmd)
    {
        const auto& pattern = commandPatternMap[static_cast<std::size_t>(cmd.type)];
        lastPattern = encoder.encode(cmd, pattern, lastPattern);
        return lastPattern;
    };

protected:
    template <typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func&& func)
    {
        auto entry = std::make_pair(timestamp, std::forward<Func>(func));

        auto upper = std::upper_bound(implicitCommandList.begin(), implicitCommandList.end(),
            entry,
            [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

        implicitCommandList.emplace(upper, entry);
    }

    template <CommandEnum cmd, typename Func>
    void routeCommand(Func&& func)
    {
        assert(commandRouter.size() > static_cast<std::size_t>(cmd));
        this->commandRouter[static_cast<std::size_t>(cmd)] = func;
    }

    template <CommandEnum cmd_type>
    void registerPattern(std::initializer_list<pattern_descriptor::t> pattern)
    {
        this->commandPatternMap[static_cast<std::size_t>(cmd_type)] = commandPattern_t(pattern);
    }

    template <CommandEnum cmd_type>
    void registerPattern(const commandPattern_t &pattern)
    {
        this->commandPatternMap[static_cast<std::size_t>(cmd_type)] = pattern;
    }

    const commandPattern_t& getPattern(CmdType cmd_type)
    {
        return this->commandPatternMap[static_cast<std::size_t>(cmd_type)];
    }

    std::size_t implicitCommandCount() const { return this->implicitCommandList.size(); };

    void processImplicitCommandQueue(timestamp_t timestamp)
    {
        // Process implicit command list
        while (!implicitCommandList.empty() && implicitCommandList.front().first <= timestamp) {
            // Execute implicit command functor
            auto& [timestamp, implicitCommand] = implicitCommandList.front();
            implicitCommand();
            this->last_command_time = timestamp;
            implicitCommandList.pop_front();
        };
    }

public:
    
    void doCoreCommand(const Command& command)
    {
        assert(commandCount.size() > static_cast<std::size_t>(command.type));
        assert(commandRouter.size() > static_cast<std::size_t>(command.type));

        // Process implicit command list
        processImplicitCommandQueue(command.timestamp);

        this->commandCount[static_cast<std::size_t>(command.type)]++;
        this->commandRouter[static_cast<std::size_t>(command.type)](command);

        this->last_command_time = command.timestamp;
    };

    void setToggleRate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleRateDefinition)
    {
        this->toggleRateDefinition = toggleRateDefinition;
        update_toggling_rate(timestamp, this->toggleRateDefinition);
    }

    void doInterfaceCommand(const Command& command)
    {
        assert(commandCount.size() > static_cast<std::size_t>(command.type));
        assert(commandRouter.size() > static_cast<std::size_t>(command.type));

        if (command.type != CmdType::END_OF_SIMULATION)
            this->internal_handle_interface(command);
        this->last_command_time = command.timestamp;
    };

    void doCoreInterfaceCommand(const Command& command)
    {
        doCoreCommand(command);
        doInterfaceCommand(command);
        this->last_command_time = command.timestamp;
    }

    auto getCommandCount(CommandEnum cmd) const
    {
        assert(commandCount.size() > static_cast<std::size_t>(cmd));

        return this->commandCount[static_cast<std::size_t>(cmd)];
    };

    timestamp_t getLastCommandTime() const { return this->last_command_time; };
};

template <typename CommandEnum>
dram_base<CommandEnum>::~dram_base() = default;

}

#endif /* DRAMPOWER_DDR_DRAM_BASE_H */
