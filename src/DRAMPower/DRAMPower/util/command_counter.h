#ifndef DRAMPOWER_UTIL_COMMAND_COUNTER_H
#define DRAMPOWER_UTIL_COMMAND_COUNTER_H

#include <vector>
#include <cassert>

namespace DRAMPower::util
{

template<typename CommandEnum>
class CommandCounter
{
public:
	using counter_t = std::vector<std::size_t>;
private:
	counter_t counter;
public:
	CommandCounter()
		: counter(static_cast<std::size_t>(CommandEnum::COUNT), 0)
	{};
public:
	void inc(CommandEnum cmd) {
		assert(counter.size() > static_cast<std::size_t>(cmd));
		counter[static_cast<std::size_t>(cmd)] += 1;
	};

	std::size_t get(CommandEnum cmd) {
		assert(counter.size() > static_cast<std::size_t>(cmd));
		return counter[static_cast<std::size_t>(cmd)];
	};
};

}

#endif /* DRAMPOWER_UTIL_COMMAND_COUNTER_H */
