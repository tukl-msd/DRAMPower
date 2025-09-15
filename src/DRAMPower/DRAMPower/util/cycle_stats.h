#ifndef DRAMPOWER_UTIL_CYCLE_STATS_H
#define DRAMPOWER_UTIL_CYCLE_STATS_H

#include <stdint.h>
#include <optional>

#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

namespace DRAMPower::util
{

template<typename T>
class interval_counter : public Serialize, public Deserialize
{
private:
	T count{ 0 };

	std::optional<T> start;
	std::optional<T> end;
public:
	interval_counter() = default;
	interval_counter(T start) : start(start) {};

	interval_counter(const interval_counter<T>&) = default;
	interval_counter& operator=(const interval_counter<T>&) = default;

	interval_counter(interval_counter<T>&&) = default;
	interval_counter& operator=(interval_counter<T>&&) = default;
public:
	T get_start() const { return start.value_or(0); };
	T get_end() const { return end.value_or(0); };
public:
	bool is_open() const { return start && !end; };
	bool is_closed() const { return start && end; };

	T get_count() const { 
		return count;
	};

	T get_count_at(T timestamp) const { 
		if (is_open() && timestamp > *start)
			return count + timestamp - *start;

		return get_count();
	}

	void add(T value) {
		this->count += value;
	};

	uint64_t close_interval(uint64_t timestamp) { 
		if (!is_open())
			return T{ 0 };

		end = timestamp;
		auto diff = timestamp - *start;
		count += diff;

		return diff;
	}

	void reset_interval() {
		this->start.reset();
		this->end.reset();
	}

	void start_interval(T start) {
		this->start = start;
		this->end.reset();
	}

	void start_interval_if_not_running(T start) {
		if(!is_open())
			start_interval(start);
	}

	void serialize(std::ostream& stream) const override  {
		stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
		bool starthasValue = start.has_value();
		stream.write(reinterpret_cast<const char *>(&starthasValue), sizeof(starthasValue));
		if (starthasValue) {
			stream.write(reinterpret_cast<const char *>(&start), sizeof(start));
		}
		bool endhasValue = end.has_value();
		stream.write(reinterpret_cast<const char *>(&endhasValue), sizeof(endhasValue));
		if (endhasValue) {
			stream.write(reinterpret_cast<const char *>(&end), sizeof(end));
		}
	}
	void deserialize(std::istream& stream) override {
		stream.read(reinterpret_cast<char *>(&count), sizeof(count));
		bool starthasValue = false;
		stream.read(reinterpret_cast<char *>(&starthasValue), sizeof(starthasValue));
		if (starthasValue) {
			stream.read(reinterpret_cast<char *>(&start), sizeof(start));
		} else {
			start.reset();
		}
		bool endhasValue = false;
		stream.read(reinterpret_cast<char *>(&endhasValue), sizeof(endhasValue));
		if (endhasValue) {
			stream.read(reinterpret_cast<char *>(&end), sizeof(end));
		} else {
			end.reset();
		}
	}
};

}

#endif /* DRAMPOWER_UTIL_CYCLE_STATS_H */
