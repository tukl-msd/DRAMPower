#ifndef DRAMPOWER_DATA_STATS_H
#define DRAMPOWER_DATA_STATS_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <cstdint>

namespace DRAMPower
{
	struct CycleStats : public util::Serialize, public util::Deserialize
	{
		struct command_stats_t : public util::Serialize, public util::Deserialize {
			uint64_t act = 0;
			uint64_t pre = 0;
			uint64_t preSameBank = 0;
			uint64_t reads = 0;
			uint64_t writes = 0;
			uint64_t refAllBank = 0;
			uint64_t refPerBank = 0;
			uint64_t refPerTwoBanks = 0;
			uint64_t refSameBank = 0;
			uint64_t readAuto = 0;
			uint64_t writeAuto = 0;

			void serialize(std::ostream& stream) const override {
				stream.write(reinterpret_cast<const char *>(&act), sizeof(act));
				stream.write(reinterpret_cast<const char *>(&pre), sizeof(pre));
				stream.write(reinterpret_cast<const char *>(&preSameBank), sizeof(preSameBank));
				stream.write(reinterpret_cast<const char *>(&reads), sizeof(reads));
				stream.write(reinterpret_cast<const char *>(&writes), sizeof(writes));
				stream.write(reinterpret_cast<const char *>(&refAllBank), sizeof(refAllBank));
				stream.write(reinterpret_cast<const char *>(&refPerBank), sizeof(refPerBank));
				stream.write(reinterpret_cast<const char *>(&refPerTwoBanks), sizeof(refPerTwoBanks));
				stream.write(reinterpret_cast<const char *>(&refSameBank), sizeof(refSameBank));
				stream.write(reinterpret_cast<const char *>(&readAuto), sizeof(readAuto));
				stream.write(reinterpret_cast<const char *>(&writeAuto), sizeof(writeAuto));
			}
			void deserialize(std::istream& stream) override {
				stream.read(reinterpret_cast<char *>(&act), sizeof(act));
				stream.read(reinterpret_cast<char *>(&pre), sizeof(pre));
				stream.read(reinterpret_cast<char *>(&preSameBank), sizeof(preSameBank));
				stream.read(reinterpret_cast<char *>(&reads), sizeof(reads));
				stream.read(reinterpret_cast<char *>(&writes), sizeof(writes));
				stream.read(reinterpret_cast<char *>(&refAllBank), sizeof(refAllBank));
				stream.read(reinterpret_cast<char *>(&refPerBank), sizeof(refPerBank));
				stream.read(reinterpret_cast<char *>(&refPerTwoBanks), sizeof(refPerTwoBanks));
				stream.read(reinterpret_cast<char *>(&refSameBank), sizeof(refSameBank));
				stream.read(reinterpret_cast<char *>(&readAuto), sizeof(readAuto));
				stream.read(reinterpret_cast<char *>(&writeAuto), sizeof(writeAuto));
			}

			// operator ==
			bool operator==(const command_stats_t& rhs) const {
				return act == rhs.act &&
					pre == rhs.pre &&
					preSameBank == rhs.preSameBank &&
					reads == rhs.reads &&
					writes == rhs.writes &&
					refAllBank == rhs.refAllBank &&
					refPerBank == rhs.refPerBank &&
					refPerTwoBanks == rhs.refPerTwoBanks &&
					refSameBank == rhs.refSameBank &&
					readAuto == rhs.readAuto &&
					writeAuto == rhs.writeAuto;
			}
		} counter;

		struct cycles_t : public util::Serialize, public util::Deserialize {
			uint64_t act = 0;
			uint64_t pre = 0;
			uint64_t powerDownAct = 0;
			uint64_t powerDownPre = 0;
			uint64_t selfRefresh = 0;
			uint64_t deepSleepMode = 0;
			uint64_t activeTime() const { return act; };
			//uint64_t prechargeTime;

			void serialize(std::ostream& stream) const override {
				stream.write(reinterpret_cast<const char *>(&act), sizeof(act));
				stream.write(reinterpret_cast<const char *>(&pre), sizeof(pre));
				stream.write(reinterpret_cast<const char *>(&powerDownAct), sizeof(powerDownAct));
				stream.write(reinterpret_cast<const char *>(&powerDownPre), sizeof(powerDownPre));
				stream.write(reinterpret_cast<const char *>(&selfRefresh), sizeof(selfRefresh));
				stream.write(reinterpret_cast<const char *>(&deepSleepMode), sizeof(deepSleepMode));
			}
			void deserialize(std::istream& stream) override {
				stream.read(reinterpret_cast<char *>(&act), sizeof(act));
				stream.read(reinterpret_cast<char *>(&pre), sizeof(pre));
				stream.read(reinterpret_cast<char *>(&powerDownAct), sizeof(powerDownAct));
				stream.read(reinterpret_cast<char *>(&powerDownPre), sizeof(powerDownPre));
				stream.read(reinterpret_cast<char *>(&selfRefresh), sizeof(selfRefresh));
				stream.read(reinterpret_cast<char *>(&deepSleepMode), sizeof(deepSleepMode));
			}

			// operator ==
			bool operator==(const cycles_t& rhs) const {
				return act == rhs.act &&
					pre == rhs.pre &&
					powerDownAct == rhs.powerDownAct &&
					powerDownPre == rhs.powerDownPre &&
					selfRefresh == rhs.selfRefresh &&
					deepSleepMode == rhs.deepSleepMode;
			}
		} cycles;

		struct prepos_t : public util::Serialize, public util::Deserialize {
			uint64_t readMerged = 0;
			uint64_t readMergedTime = 0;
			uint64_t writeMerged = 0;
			uint64_t writeMergedTime = 0;
			uint64_t readSeamless = 0;
			uint64_t writeSeamless = 0;

			void serialize(std::ostream& stream) const override {
				stream.write(reinterpret_cast<const char *>(&readMerged), sizeof(readMerged));
				stream.write(reinterpret_cast<const char *>(&readMergedTime), sizeof(readMergedTime));
				stream.write(reinterpret_cast<const char *>(&writeMerged), sizeof(writeMerged));
				stream.write(reinterpret_cast<const char *>(&writeMergedTime), sizeof(writeMergedTime));
				stream.write(reinterpret_cast<const char *>(&readSeamless), sizeof(readSeamless));
				stream.write(reinterpret_cast<const char *>(&writeSeamless), sizeof(writeSeamless));
			}
			void deserialize(std::istream& stream) override {
				stream.read(reinterpret_cast<char *>(&readMerged), sizeof(readMerged));
				stream.read(reinterpret_cast<char *>(&readMergedTime), sizeof(readMergedTime));
				stream.read(reinterpret_cast<char *>(&writeMerged), sizeof(writeMerged));
				stream.read(reinterpret_cast<char *>(&writeMergedTime), sizeof(writeMergedTime));
				stream.read(reinterpret_cast<char *>(&readSeamless), sizeof(readSeamless));
				stream.read(reinterpret_cast<char *>(&writeSeamless), sizeof(writeSeamless));
			}

			// operator ==
			bool operator==(const prepos_t& rhs) const {
				return readMerged == rhs.readMerged &&
					readMergedTime == rhs.readMergedTime &&
					writeMerged == rhs.writeMerged &&
					writeMergedTime == rhs.writeMergedTime &&
					readSeamless == rhs.readSeamless &&
					writeSeamless == rhs.writeSeamless;
			}
		} prepos;

		void serialize(std::ostream& stream) const override {
			counter.serialize(stream);
			cycles.serialize(stream);
			prepos.serialize(stream);
		}
		void deserialize(std::istream& stream) override {
			counter.deserialize(stream);
			cycles.deserialize(stream);
			prepos.deserialize(stream);
		}

		// operator ==
		bool operator==(const CycleStats& rhs) const {
			return counter == rhs.counter &&
				cycles == rhs.cycles &&
				prepos == rhs.prepos;
		}
	};

    struct TogglingStats
    {
        util::bus_stats_t read;
        util::bus_stats_t write;
    };

	struct SimulationStats
	{
		util::bus_stats_t commandBus;
		util::bus_stats_t readBus;
		util::bus_stats_t writeBus;
		util::bus_stats_t clockStats;
		util::bus_stats_t wClockStats;

		util::bus_stats_t readDBI;
		util::bus_stats_t writeDBI;

        TogglingStats togglingStats;

		util::bus_stats_t readDQSStats;
		util::bus_stats_t writeDQSStats;

		std::vector<CycleStats> bank;
		std::vector<CycleStats> rank_total;

		// Operator ==
		bool operator==(const SimulationStats& other) const {
			// Compare banks
			bool banksEqual = true;
			if (bank.size() != other.bank.size()) {
				banksEqual = false;
			} else {
				for (size_t i = 0; i < bank.size(); ++i) {
					if (!(bank[i] == other.bank[i])) {
						banksEqual = false;
						break;
					}
				}
			}
			bool rank_totalEqual = true;
			if (rank_total.size() != other.rank_total.size()) {
				rank_totalEqual = false;
			} else {
				for (size_t i = 0; i < rank_total.size(); ++i) {
					if (!(rank_total[i] == other.rank_total[i])) {
						rank_totalEqual = false;
						break;
					}
				}
			}
			return commandBus == other.commandBus &&
				readBus == other.readBus &&
				writeBus == other.writeBus &&
				clockStats == other.clockStats &&
				wClockStats == other.wClockStats &&
				readDBI == other.readDBI &&
				writeDBI == other.writeDBI &&
				togglingStats.read == other.togglingStats.read &&
				togglingStats.write == other.togglingStats.write &&
				readDQSStats == other.readDQSStats &&
				writeDQSStats == other.writeDQSStats &&
				banksEqual &&
				rank_totalEqual;
		}
	};
};


#endif /* DRAMPOWER_DATA_STATS_H */
