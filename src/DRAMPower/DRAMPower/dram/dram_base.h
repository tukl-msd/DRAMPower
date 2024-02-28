#ifndef DRAMPOWER_DRAM_DRAM_BASE_H
#define DRAMPOWER_DRAM_DRAM_BASE_H

#include <DRAMPower/command/Command.h>
#include <DRAMPower/command/Pattern.h>

#include <algorithm>
#include <cassert>
#include <deque>
#include <functional>
#include <vector>
#include <limits>

namespace DRAMPower {

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
    enum class PatternEncoderInit{
        L,
        H
    };
protected:
    uint64_t lastPattern;
private:
    enum class _PatternEncoderInit{
        L,
        H,
        CUSTOM
    };
    commandRouter_t commandRouter;
    commandPatternMap_t commandPatternMap;
    implicitCommandList_t implicitCommandList;
    timestamp_t last_command_time;
    PatternEncoder encoder;
    std::optional<uint64_t> custom_init_pattern;
    _PatternEncoderInit init_pattern;

    dram_base(PatternEncoderOverrides encoderoverrides, _PatternEncoderInit init_pattern,
        std::optional<uint64_t> custom_init_pattern = std::nullopt
    )
        : commandCount(static_cast<std::size_t>(CommandEnum::COUNT), 0)
        , commandRouter(static_cast<std::size_t>(CommandEnum::COUNT), [](const Command& cmd) {})
        , commandPatternMap(static_cast<std::size_t>(CommandEnum::COUNT), commandPattern_t {})
        , encoder(encoderoverrides)
        , lastPattern(0)
        , init_pattern(init_pattern)
        , custom_init_pattern(custom_init_pattern)
        {
            // TODO 
            // lastPattern is the same as command bus init pattern
            // if the command bus is loaded at 0 and the init pattern is overwritten
            // the lastPattern should be updated otherwise the init pattern
            // for pattern encoder and the init pattern of the bus are different
            if (init_pattern == _PatternEncoderInit::CUSTOM)
            {
                assert(custom_init_pattern.has_value());
                lastPattern = custom_init_pattern.value();
            }
            else
            {
                lastPattern = (init_pattern == _PatternEncoderInit::H ? std::numeric_limits<uint64_t>::max() : 0);
            }
        };
    

public:
    dram_base(PatternEncoderOverrides encoderoverrides, PatternEncoderInit init_pattern)
		: dram_base(encoderoverrides,
            init_pattern ==  PatternEncoderInit::H ? _PatternEncoderInit::H : _PatternEncoderInit::L)
    {};
	
	dram_base(PatternEncoderOverrides encoderoverrides, uint64_t init_pattern)
		: dram_base(encoderoverrides, _PatternEncoderInit::CUSTOM, init_pattern)
    {};

protected:
    virtual ~dram_base() = default;
    virtual void handle_interface(const Command& cmd) = 0;

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
    };

    template <CommandEnum cmd, typename Func>
    void routeCommand(Func&& func)
    {
        assert(commandRouter.size() > static_cast<std::size_t>(cmd));
        this->commandRouter[static_cast<std::size_t>(cmd)] = func;
    };

    template <CommandEnum cmd_type>
    void registerPattern(std::initializer_list<pattern_descriptor::t> pattern)
    {
        this->commandPatternMap[static_cast<std::size_t>(cmd_type)] = commandPattern_t(pattern);
    };

    template <CommandEnum cmd_type>
    void registerPattern(const commandPattern_t &pattern)
    {
        this->commandPatternMap[static_cast<std::size_t>(cmd_type)] = pattern;
    };

    const commandPattern_t& getPattern(CmdType cmd_type)
    {
        return this->commandPatternMap[static_cast<std::size_t>(cmd_type)];
    };

    const std::size_t implicitCommandCount() const { return this->implicitCommandList.size(); };

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
    void doCommand(const Command& command)
    {
        assert(commandCount.size() > static_cast<std::size_t>(command.type));
        assert(commandRouter.size() > static_cast<std::size_t>(command.type));

        // Process implicit command list
        processImplicitCommandQueue(command.timestamp);

        this->commandCount[static_cast<std::size_t>(command.type)]++;
        this->commandRouter[static_cast<std::size_t>(command.type)](command);

        this->last_command_time = command.timestamp;
    };

    void handleInterfaceCommand(const Command& command)
    {
        assert(commandCount.size() > static_cast<std::size_t>(command.type));
        assert(commandRouter.size() > static_cast<std::size_t>(command.type));

        if (command.type != CmdType::END_OF_SIMULATION)
            this->handle_interface(command);
    };

    auto getCommandCount(CommandEnum cmd) const
    {
        assert(commandCount.size() > static_cast<std::size_t>(cmd));

        return this->commandCount[static_cast<std::size_t>(cmd)];
    };

    timestamp_t getLastCommandTime() const { return this->last_command_time; };
};

}

#endif /* DRAMPOWER_DDR_DRAM_BASE_H */
