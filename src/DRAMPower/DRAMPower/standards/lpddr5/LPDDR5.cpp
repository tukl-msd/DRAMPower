#include "LPDDR5.h"

#include <DRAMPower/command/Pattern.h>
#include <DRAMPower/standards/lpddr5/core_calculation_LPDDR5.h>
#include <DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h>
#include <iostream>


namespace DRAMPower {

    using namespace DRAMUtils::Config;

    LPDDR5::LPDDR5(const MemSpecLPDDR5 &memSpec)
        : dram_base<CmdType>(PatternEncoderOverrides{})
        , memSpec(memSpec)
        , ranks(memSpec.numberOfRanks, { (std::size_t)memSpec.numberOfBanks })
        , commandBus{7, 2, // modelled with datarate 2
            util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L}
        , readBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L}
        , writeBus{memSpec.bitWidth * memSpec.numberOfDevices, memSpec.dataRate,
            util::Bus::BusIdlePatternSpec::L, util::Bus::BusInitPatternSpec::L}
        , readDQS(memSpec.dataRate, true)
        , wck(memSpec.dataRate / memSpec.memTimingSpec.WCKtoCK, !memSpec.wckAlwaysOnMode)
    {
        this->registerPatterns();

        this->registerBankHandler<CmdType::ACT>(&LPDDR5::handleAct);
        this->registerBankHandler<CmdType::PRE>(&LPDDR5::handlePre);
        this->registerRankHandler<CmdType::PREA>(&LPDDR5::handlePreAll);
        this->registerBankHandler<CmdType::REFB>(&LPDDR5::handleRefPerBank);
        this->registerBankHandler<CmdType::RD>(&LPDDR5::handleRead);
        this->registerBankHandler<CmdType::RDA>(&LPDDR5::handleReadAuto);
        this->registerBankHandler<CmdType::WR>(&LPDDR5::handleWrite);
        this->registerBankHandler<CmdType::WRA>(&LPDDR5::handleWriteAuto);

        this->registerBankGroupHandler<CmdType::REFP2B>(&LPDDR5::handleRefPerTwoBanks);

        this->registerRankHandler<CmdType::REFA>(&LPDDR5::handleRefAll);
        this->registerRankHandler<CmdType::SREFEN>(&LPDDR5::handleSelfRefreshEntry);
        this->registerRankHandler<CmdType::SREFEX>(&LPDDR5::handleSelfRefreshExit);
        this->registerRankHandler<CmdType::PDEA>(&LPDDR5::handlePowerDownActEntry);
        this->registerRankHandler<CmdType::PDEP>(&LPDDR5::handlePowerDownPreEntry);
        this->registerRankHandler<CmdType::PDXA>(&LPDDR5::handlePowerDownActExit);
        this->registerRankHandler<CmdType::PDXP>(&LPDDR5::handlePowerDownPreExit);
        this->registerRankHandler<CmdType::DSMEN>(&LPDDR5::handleDSMEntry);
        this->registerRankHandler<CmdType::DSMEX>(&LPDDR5::handleDSMExit);

        routeCommand<CmdType::END_OF_SIMULATION>([this](const Command &cmd) { this->endOfSimulation(cmd.timestamp); });
    };

    void LPDDR5::toggling_rate_enable(timestamp_t timestamp, timestamp_t enable_timestamp, DRAMPower::util::Bus &bus, DRAMPower::TogglingHandle &togglinghandle) {
        // Change from bus to toggling rate
        assert(enable_timestamp >= timestamp);
        bus.disable(enable_timestamp);
        if ( enable_timestamp > timestamp ) {
            // Schedule toggling rate enable
            this->addImplicitCommand(enable_timestamp, [this, &togglinghandle, enable_timestamp]() {
                togglinghandle.enable(enable_timestamp);
            });
        } else {
            togglinghandle.enable(enable_timestamp);
        }
    }

    timestamp_t LPDDR5::toggling_rate_get_enable_time(timestamp_t timestamp) {
        timestamp_t busdisabletimestamp = timestamp;
        busdisabletimestamp = std::max(readBus.get_lastburst_timestamp(), busdisabletimestamp);
        busdisabletimestamp = std::max(writeBus.get_lastburst_timestamp(), busdisabletimestamp);
        return busdisabletimestamp;
    }

    timestamp_t LPDDR5::update_toggling_rate(timestamp_t timestamp, const std::optional<ToggleRateDefinition> &toggleratedefinition)
    {
        if (toggleratedefinition) {
            // Enable toggle rate
            togglingHandleRead.setWidth(memSpec.bitWidth * memSpec.numberOfDevices);
            togglingHandleWrite.setWidth(memSpec.bitWidth * memSpec.numberOfDevices);
            togglingHandleRead.setDataRate(memSpec.dataRate);
            togglingHandleWrite.setDataRate(memSpec.dataRate);
            togglingHandleRead.setTogglingRateAndDutyCycle(
                toggleratedefinition->togglingRateRead,
                toggleratedefinition->dutyCycleRead,
                toggleratedefinition->idlePatternRead
            );
            togglingHandleWrite.setTogglingRateAndDutyCycle(
                toggleratedefinition->togglingRateWrite,
                toggleratedefinition->dutyCycleWrite,
                toggleratedefinition->idlePatternWrite
            );
            // toggling rate already enabled
            if (togglingHandleRead.isEnabled() && togglingHandleWrite.isEnabled()) {
                return timestamp;
            }
            // Get next possible enable time (wait for pending bus transactions with data)
            timestamp_t enable_timestamp = toggling_rate_get_enable_time(timestamp);
            // Enable toggling rate
            toggling_rate_enable(timestamp, enable_timestamp, readBus, togglingHandleRead);
            toggling_rate_enable(timestamp, enable_timestamp, writeBus, togglingHandleWrite);
            return enable_timestamp;
        } else {
            togglingHandleRead.disable(timestamp);
            togglingHandleWrite.disable(timestamp);
            readBus.enable(timestamp);
            writeBus.enable(timestamp);
        }
        return timestamp;
    }

    uint64_t LPDDR5::getBankCount() {
        return memSpec.numberOfBanks;
    }

    uint64_t LPDDR5::getRankCount() {
        return memSpec.numberOfRanks;
    }

    uint64_t LPDDR5::getDeviceCount() {
        return memSpec.numberOfDevices;
    }

    void LPDDR5::registerPatterns() {
        using namespace pattern_descriptor;
        // LPDDR5 needs 2 commands for activation (ACT-1 and ACT-2)
        // ACT-1 must be followed by ACT-2 in almost every case (CAS, WRITE,
        // MASK WRITE and READ commands can be issued inbetween ACT-1 and ACT-2)
        // Here we consider ACT = ACT-1 + ACT-2, not considering interleaving
        commandPattern_t act_pattern = {
            // ACT-1
            // R1
            H, H, H, R14, R15, R16, R17,
            // F1
            BA0, BA1, BA2, BA3, R11, R12, R13,
            // ACT-2
            // R2
            H, H, L, R7, R8, R9, R10,
            // F2
            R0, R1, R2, R3, R4, R5, R6
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            act_pattern[9] = BG0;
            act_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            act_pattern[10] = V;
        }
        this->registerPattern<CmdType::ACT>(act_pattern);

        commandPattern_t pre_pattern = {
            // R1
            L, L, L, H, H, H, H,
            // F1
            BA0, BA1, BA2, BA3, V, V, L
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            pre_pattern[9] = BG0;
            pre_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            pre_pattern[10] = V;
        }
        this->registerPattern<CmdType::PRE>(pre_pattern);

        commandPattern_t prea_pattern = {
            // R1
            L, L, L, H, H, H, H,
            // F1
            V, V, V, V, V, V, H
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            prea_pattern[9] = BG0;
            prea_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            prea_pattern[10] = V;
        }
        this->registerPattern<CmdType::PREA>(prea_pattern);

        // For refresh commands LPDDR5 has RFM (Refresh Management)
        // Considering RFM is disabled, CA3 is V
        commandPattern_t refb_pattern = {
            // R1
            L, L, L, H, H, H, L,
            // F1
            BA0, BA1, BA2, V, V, V, L
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            refb_pattern[9] = BG0;
        }
        this->registerPattern<CmdType::REFB>(refb_pattern);

        commandPattern_t refa_pattern = {
            // R1
            L, L, L, H, H, H, L,
            // F1
            V, V, V, V, V, V, H
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            refa_pattern[9] = BG0;
        }
        this->registerPattern<CmdType::REFA>(refa_pattern);

        if (memSpec.bank_arch == MemSpecLPDDR5::MBG || memSpec.bank_arch == MemSpecLPDDR5::M16B) {
            this->registerPattern<CmdType::REFP2B>(refb_pattern);
        }

        commandPattern_t rd_pattern = {
            // R1
            H, L, L, C0, C3, C4, C5,
            // F1
            BA0, BA1, BA2, BA3, C1, C2, L
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            rd_pattern[9] = BG0;
            rd_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            rd_pattern[10] = L; // B4
        }
        this->registerPattern<CmdType::RD>(rd_pattern);

        commandPattern_t rda_pattern = {
            // R1
            H, L, L, C0, C3, C4, C5,
            // F1
            BA0, BA1, BA2, BA3, C1, C2, H
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            rda_pattern[9] = BG0;
            rda_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            rda_pattern[10] = L;
        }
        this->registerPattern<CmdType::RDA>(rda_pattern);

        commandPattern_t wr_pattern = {
            // R1
            L, H, H, C0, C3, C4, C5,
            // F1
            BA0, BA1, BA2, BA3, C1, C2, L
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            wr_pattern[9] = BG0;
            wr_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            wr_pattern[10] = V;
        }
        this->registerPattern<CmdType::WR>(wr_pattern);

        commandPattern_t wra_pattern = {
            // R1
            L, H, H, C0, C3, C4, C5,
            // F1
            BA0, BA1, BA2, BA3, C1, C2, H
        };
        if (memSpec.bank_arch == MemSpecLPDDR5::MBG) {
            wra_pattern[9] = BG0;
            wra_pattern[10] = BG1;
        } else if (memSpec.bank_arch == MemSpecLPDDR5::M8B) {
            wra_pattern[10] = V;
        }
        this->registerPattern<CmdType::WRA>(wra_pattern);

        this->registerPattern<CmdType::SREFEN>({
            // R1
            L, L, L, H, L, H, H,
            // F1
            V, V, V, V, V, L, L
        });
        this->registerPattern<CmdType::SREFEX>({
            // R1
            L, L, L, H, L, H, L,
            // F1
            V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::PDEA>({
            // R1
            L, L, L, H, L, H, H,
            // F1
            V, V, V, V, V, L, H
        });
        this->registerPattern<CmdType::PDXA>({
            // R1
            L, L, L, H, L, H, L,
            // F1
            V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::PDEP>({
            // R1
            L, L, L, H, L, H, H,
            // F1
            V, V, V, V, V, L, H
        });
        this->registerPattern<CmdType::PDXP>({
            // R1
            L, L, L, H, L, H, L,
            // F1
            V, V, V, V, V, V, V
        });
        this->registerPattern<CmdType::DSMEN>({
            // R1
            L, L, L, H, L, H, H,
            // F1
            V, V, V, V, V, H, L
        });
        this->registerPattern<CmdType::DSMEX>({
            // R1
            L, L, L, H, L, H, L,
            // F1
            V, V, V, V, V, V, V
        });
    }

    void LPDDR5::handleInterfaceOverrides(size_t length, bool /*read*/)
    {
        // Set command bus pattern overrides
        switch(length) {
            case 32:
                this->encoder.settings.updateSettings({
                    {pattern_descriptor::C0, PatternEncoderBitSpec::L},
                });
                break;
            default:
                // Pull down
                // No interface power needed for PatternEncoderBitSpec::L
                // Defaults to burst length 16
            case 16:
                this->encoder.settings.removeSetting(pattern_descriptor::C0);
                break;
        }
    }

    void LPDDR5::handle_interface_commandbus(const Command &cmd) {
        auto pattern = getCommandPattern(cmd);
        auto ca_length = getPattern(cmd.type).size() / commandBus.get_width();
        commandBus.load(cmd.timestamp, pattern, ca_length);
    }

    void LPDDR5::handle_interface_data_common(const Command &cmd, const size_t length) {
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            readDQS.start(cmd.timestamp);
            readDQS.stop(cmd.timestamp + length / memSpec.dataRate);
            if (!memSpec.wckAlwaysOnMode) {
                wck.start(cmd.timestamp);
                wck.stop(cmd.timestamp + length / memSpec.dataRate);
            }
            handleInterfaceOverrides(length, true);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
             if (!memSpec.wckAlwaysOnMode) {
                wck.start(cmd.timestamp);
                wck.stop(cmd.timestamp + length / memSpec.dataRate);
            }
            handleInterfaceOverrides(length, false);
        }
    }

    void LPDDR5::handle_interface_toggleRate(const Command &cmd) {
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            if (cmd.sz_bits == 0) {
                // Use default burst length
                this->togglingHandleRead.incCountBurstLength(cmd.timestamp, memSpec.burstLength);
            } else {
                this->togglingHandleRead.incCountBitLength(cmd.timestamp, cmd.sz_bits);
            }
            assert(cmd.sz_bits % togglingHandleRead.getWidth() == 0);
            handle_interface_data_common(cmd, cmd.sz_bits / togglingHandleRead.getWidth());
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            if (cmd.sz_bits == 0) {
                // Use default burst length
                this->togglingHandleWrite.incCountBurstLength(cmd.timestamp, memSpec.burstLength);
            } else {
                this->togglingHandleWrite.incCountBitLength(cmd.timestamp, cmd.sz_bits);
            }
            assert(cmd.sz_bits % togglingHandleWrite.getWidth() == 0);
            handle_interface_data_common(cmd, cmd.sz_bits / togglingHandleWrite.getWidth());
        }
        handle_interface_commandbus(cmd);
    }

    void LPDDR5::handle_interface(const Command &cmd) {
        size_t length = 0;
        if (cmd.type == CmdType::RD || cmd.type == CmdType::RDA) {
            length = cmd.sz_bits / readBus.get_width();
            if ( cmd.data != nullptr ) {
                readBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
            }
            handle_interface_data_common(cmd, length);
        } else if (cmd.type == CmdType::WR || cmd.type == CmdType::WRA) {
            length = cmd.sz_bits / writeBus.get_width();
            if ( cmd.data != nullptr ) {
                writeBus.load(cmd.timestamp, cmd.data, cmd.sz_bits);
            }
            handle_interface_data_common(cmd, length);
        }
        handle_interface_commandbus(cmd);
    }

    void LPDDR5::handleAct(Rank &rank, Bank &bank, timestamp_t timestamp) {
        bank.counter.act++;

        bank.cycles.act.start_interval(timestamp);

        if ( !rank.isActive(timestamp) ) {
            rank.cycles.act.start_interval(timestamp);
        }

        bank.bankState = Bank::BankState::BANK_ACTIVE;
    }

    void LPDDR5::handlePre(Rank &rank, Bank &bank, timestamp_t timestamp) {
        if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
            return;

        bank.counter.pre++;
        bank.cycles.act.close_interval(timestamp);
        bank.latestPre = timestamp;
        bank.bankState = Bank::BankState::BANK_PRECHARGED;

        if (!rank.isActive(timestamp)) {
            rank.cycles.act.close_interval(timestamp);
        }
    }

    void LPDDR5::handlePreAll(Rank &rank, timestamp_t timestamp) {
        for (auto &bank: rank.banks) {
            handlePre(rank, bank, timestamp);
        }
    }

    void LPDDR5::handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp) {
        handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
    }

    void LPDDR5::handleRefPerTwoBanks(Rank & rank, std::size_t bank_id, timestamp_t timestamp) {
        Bank & bank_1 = rank.banks[bank_id];
        Bank & bank_2 = rank.banks[(bank_id + memSpec.perTwoBankOffset)%16];
        handleRefreshOnBank(rank, bank_1, timestamp, memSpec.memTimingSpec.tRFCPB, bank_1.counter.refPerTwoBanks);
        handleRefreshOnBank(rank, bank_2, timestamp, memSpec.memTimingSpec.tRFCPB, bank_2.counter.refPerTwoBanks);
    }

    void LPDDR5::handleRefAll(Rank &rank, timestamp_t timestamp) {
        for (auto& bank : rank.banks) {
            handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
        }
        rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
    }

    void LPDDR5::handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter){
        ++counter;
        if (!rank.isActive(timestamp)) {
            rank.cycles.act.start_interval(timestamp);
        }
        bank.bankState = Bank::BankState::BANK_ACTIVE;
        auto timestamp_end = timestamp + timing;
        bank.refreshEndTime = timestamp_end;
        if (!bank.cycles.act.is_open())
            bank.cycles.act.start_interval(timestamp);

        // Execute implicit pre-charge at refresh end
        addImplicitCommand(timestamp_end, [this, &bank, &rank, timestamp_end]() {
            bank.bankState = Bank::BankState::BANK_PRECHARGED;
            bank.cycles.act.close_interval(timestamp_end);

            if (!rank.isActive(timestamp_end)) {
                rank.cycles.act.close_interval(timestamp_end);
            }
        });
    }

    void LPDDR5::handleRead(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.reads;
    }

    void LPDDR5::handleReadAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.readAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minReadActiveTime = timestamp + this->memSpec.prechargeOffsetRD;

        auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);

        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR5::handleWrite(Rank&, Bank &bank, timestamp_t) {
        ++bank.counter.writes;
    }

    void LPDDR5::handleWriteAuto(Rank &rank, Bank &bank, timestamp_t timestamp) {
        ++bank.counter.writeAuto;

        auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
        auto minWriteActiveTime = timestamp + this->memSpec.prechargeOffsetWR;

        auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);
        // Execute PRE after minimum active time
        addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
            this->handlePre(rank, bank, delayed_timestamp);
        });
    }

    void LPDDR5::handleSelfRefreshEntry(Rank &rank, timestamp_t timestamp) {
        // Issue implicit refresh
        handleRefAll(rank, timestamp);
        // Handle self-refresh entry after tRFC
        auto timestampSelfRefreshStart = timestamp + memSpec.memTimingSpec.tRFC;
        addImplicitCommand(timestampSelfRefreshStart, [this, &rank, timestampSelfRefreshStart]() {
            rank.counter.selfRefresh++;
            rank.cycles.sref.start_interval(timestampSelfRefreshStart);
            rank.memState = MemState::SREF;
        });
    }

    void LPDDR5::handleSelfRefreshExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.sref.close_interval(timestamp);
        rank.memState = MemState::NOT_IN_PD;
    }

    void LPDDR5::handlePowerDownActEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownAct.start_interval(entryTime);
            rank.memState = MemState::PDN_ACT;
            if (rank.cycles.act.is_open()) {
                rank.cycles.act.close_interval(entryTime);
            }
            for (auto & bank : rank.banks) {
                if (bank.cycles.act.is_open()) {
                    bank.cycles.act.close_interval(entryTime);
                }
            }
        });
    }

    void LPDDR5::handlePowerDownActExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownAct.close_interval(exitTime);

            bool rank_active = false;

            for (auto & bank : rank.banks) {
                if (bank.counter.act != 0 && bank.cycles.act.get_end() == rank.cycles.powerDownAct.get_start()) {
                    rank_active = true;
                    bank.cycles.act.start_interval(exitTime);
                }
            }

            if (rank_active) {
                rank.cycles.act.start_interval(exitTime);
            }

        });
    }

    void LPDDR5::handlePowerDownPreEntry(Rank &rank, timestamp_t timestamp) {
        auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank);
        auto entryTime = std::max(timestamp, earliestPossibleEntry);
        addImplicitCommand(entryTime, [this, &rank, entryTime]() {
            rank.cycles.powerDownPre.start_interval(entryTime);
            rank.memState = MemState::PDN_PRE;
        });
    }

    void LPDDR5::handlePowerDownPreExit(Rank &rank, timestamp_t timestamp) {
        // TODO: Is this computation necessary?
        auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank);
        auto exitTime = std::max(timestamp, earliestPossibleExit);

        addImplicitCommand(exitTime, [this, &rank, exitTime]() {
            rank.memState = MemState::NOT_IN_PD;
            rank.cycles.powerDownPre.close_interval(exitTime);
        });
    }

    void LPDDR5::handleDSMEntry(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::SREF);
        rank.cycles.deepSleepMode.start_interval(timestamp);
        rank.counter.deepSleepMode++;
        rank.memState = MemState::DSM;
    }

    void LPDDR5::handleDSMExit(Rank &rank, timestamp_t timestamp) {
        assert(rank.memState == MemState::DSM);
        rank.cycles.deepSleepMode.close_interval(timestamp);
        rank.memState = MemState::SREF;
    }

    void LPDDR5::endOfSimulation(timestamp_t) {
        if (this->implicitCommandCount() > 0)
			std::cout << ("[WARN] End of simulation but still implicit commands left!") << std::endl;
	}

    energy_t LPDDR5::calcCoreEnergy(timestamp_t timestamp) {
        Calculation_LPDDR5 calculation;
        return calculation.calcEnergy(timestamp, *this);
    }

    interface_energy_info_t LPDDR5::calcInterfaceEnergy(timestamp_t timestamp) {
        InterfaceCalculation_LPDDR5 calculation(memSpec);
        return calculation.calculateEnergy(getWindowStats(timestamp));
    }

    SimulationStats LPDDR5::getWindowStats(timestamp_t timestamp) {
        // If there are still implicit commands queued up, process them first
        processImplicitCommandQueue(timestamp);

        SimulationStats stats;
        stats.bank.resize(memSpec.numberOfBanks * memSpec.numberOfRanks);
        stats.rank_total.resize(memSpec.numberOfRanks);

        auto simulation_duration = timestamp;
        for (size_t i = 0; i < memSpec.numberOfRanks; ++i) {
            Rank &rank = ranks[i];
            size_t bank_offset = i * memSpec.numberOfBanks;

            for (std::size_t j = 0; j < memSpec.numberOfBanks; ++j) {
                stats.bank[bank_offset + j].counter = rank.banks[j].counter;
                stats.bank[bank_offset + j].cycles.act =
                    rank.banks[j].cycles.act.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.ref =
                    rank.banks[j].cycles.ref.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.selfRefresh =
                    rank.cycles.sref.get_count_at(timestamp) -
                    rank.cycles.deepSleepMode.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.deepSleepMode =
                    rank.cycles.deepSleepMode.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.powerDownAct =
                    rank.cycles.powerDownAct.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.powerDownPre =
                    rank.cycles.powerDownPre.get_count_at(timestamp);
                stats.bank[bank_offset + j].cycles.pre =
                    simulation_duration - (stats.bank[bank_offset + j].cycles.act +
                                           rank.cycles.powerDownAct.get_count_at(timestamp) +
                                           rank.cycles.powerDownPre.get_count_at(timestamp) +
                                           rank.cycles.sref.get_count_at(timestamp));
            }

            stats.rank_total[i].cycles.pre =
                simulation_duration - (rank.cycles.act.get_count_at(timestamp) +
                                       rank.cycles.powerDownAct.get_count_at(timestamp) +
                                       rank.cycles.powerDownPre.get_count_at(timestamp) +
                                       rank.cycles.sref.get_count_at(timestamp));

            stats.rank_total[i].cycles.act = rank.cycles.act.get_count_at(timestamp);
            stats.rank_total[i].cycles.ref = rank.cycles.act.get_count_at(
                timestamp);  // TODO: I think this counter is never updated
            stats.rank_total[i].cycles.powerDownAct =
                rank.cycles.powerDownAct.get_count_at(timestamp);
            stats.rank_total[i].cycles.powerDownPre =
                rank.cycles.powerDownPre.get_count_at(timestamp);
            stats.rank_total[i].cycles.selfRefresh =
                rank.cycles.sref.get_count_at(timestamp) -
                rank.cycles.deepSleepMode.get_count_at(timestamp);
            stats.rank_total[i].cycles.deepSleepMode =
                rank.cycles.deepSleepMode.get_count_at(timestamp);
        }

        stats.commandBus = commandBus.get_stats(timestamp);
        stats.readBus = readBus.get_stats(timestamp);
        stats.writeBus = writeBus.get_stats(timestamp);

        stats.clockStats = 2.0 * clock.get_stats_at(timestamp);
        stats.wClockStats = 2.0 * wck.get_stats_at(timestamp);
        stats.readDQSStats = 2.0 * readDQS.get_stats_at(timestamp);
        if (togglingHandleRead.isEnabled() && togglingHandleWrite.isEnabled()) {
            stats.togglingStats = {
                togglingHandleRead.get_stats(timestamp), // read
                togglingHandleWrite.get_stats(timestamp) // write
            };
        }
        else {
            stats.togglingStats = std::nullopt;
        }

        return stats;
    }

    SimulationStats LPDDR5::getStats() {
        return getWindowStats(getLastCommandTime());
    }

    timestamp_t LPDDR5::earliestPossiblePowerDownEntryTime(Rank &rank) {
        timestamp_t entryTime = 0;

        for (const auto &bank : rank.banks) {
            entryTime = std::max(
                {entryTime,
                 bank.counter.act == 0 ? 0
                                       : bank.cycles.act.get_start() + memSpec.memTimingSpec.tRCD,
                 bank.counter.pre == 0 ? 0 : bank.latestPre + memSpec.memTimingSpec.tRP,
                 bank.refreshEndTime});
        }

        return entryTime;
    };
}
