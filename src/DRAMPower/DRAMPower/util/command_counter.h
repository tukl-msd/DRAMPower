#ifndef DRAMPOWER_UTIL_COMMAND_COUNTER_H
#define DRAMPOWER_UTIL_COMMAND_COUNTER_H

#include <vector>
#include <array>
#include <cassert>

#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

namespace DRAMPower::util
{

template<typename CommandEnum>
class CommandCounter : public Serialize, public Deserialize
{
public:
	using counter_t = std::array<std::size_t, static_cast<std::size_t>(CommandEnum::COUNT)>;
private:
	counter_t counter = {0};
public:
	CommandCounter() = default;
public:
	void inc(CommandEnum cmd) {
		assert(counter.size() > static_cast<std::size_t>(cmd));
		counter[static_cast<std::size_t>(cmd)] += 1;
	};

	std::size_t get(CommandEnum cmd) const {
		assert(counter.size() > static_cast<std::size_t>(cmd));
		return counter[static_cast<std::size_t>(cmd)];
	};

	void serialize(std::ostream& stream) const override {
		const std::size_t counterSize = counter.size();
		stream.write(reinterpret_cast<const char *>(&counterSize), sizeof(counterSize));
		for (const auto &count : counter) {
			stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
		}
	}
	void deserialize(std::istream& stream) override {
		std::size_t counterSize = 0;
		stream.read(reinterpret_cast<char *>(&counterSize), sizeof(counterSize));
		assert(counterSize <= static_cast<std::size_t>(CommandEnum::COUNT));
		counter.fill(0);
		for (std::size_t i = 0; i < counterSize; i++) {
			std::size_t count = 0;
			stream.read(reinterpret_cast<char *>(&count), sizeof(count));
			counter[i] = count;
		}
	}
};

}

#endif /* DRAMPOWER_UTIL_COMMAND_COUNTER_H */
