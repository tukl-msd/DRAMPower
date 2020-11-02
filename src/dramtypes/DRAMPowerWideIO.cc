#include "DRAMPowerIF.h"
#include "DRAMPowerWideIO.h"
using namespace DRAMPower;
using namespace std;


DRAMPowerWideIO::DRAMPowerWideIO(MemSpecWideIO& memSpec, bool includeIoAndTermination):
    memSpec(memSpec),
    includeIoAndTermination(includeIoAndTermination)
{
    for (unsigned i = 0; i < memSpec.numberOfRanks; ++i){
        counters.push_back(CountersWideIO(memSpec));
        energy.push_back(Energy());
        energy[i].total_energy = 0;
        power.push_back(Power());
        cmdListPerRank.push_back(std::vector<DRAMPower::MemCommand>());
    }
    total_cycles = 0;
    allranks_energy = 0;
    allranks_avg_power = 0;
}

void DRAMPowerWideIO::calcEnergy()
{
  splitCmdList();
  if (includeIoAndTermination) io_term_power();
  for (unsigned i = 0; i < memSpec.numberOfRanks; ++i){
  updateCounters(true,i);
  if (includeIoAndTermination) calcIoTermEnergy(i);
  energy[i].clearEnergy(memSpec.numberOfBanks);
  power[i].clearIOPower();
  bankPowerCalc(i);
  }
}

void DRAMPowerWideIO::calcWindowEnergy(int64_t timestamp)
{
  for (unsigned i = 0; i < memSpec.numberOfRanks; ++i){
      doCommand(MemCommand::NOP, 0, timestamp,i);
  }
  splitCmdList();
  if (includeIoAndTermination) io_term_power();
  for (unsigned i = 0; i < memSpec.numberOfRanks; ++i){
      updateCounters(false, timestamp);
      energy[i].clearEnergy(memSpec.numberOfBanks);
      power[i].clearIOPower();
      if (includeIoAndTermination) calcIoTermEnergy(i);
      bankPowerCalc(i);
      counters[i].clearCounters(timestamp);
  }
}


double DRAMPowerWideIO::getEnergy() {
   return allranks_energy;
}

double DRAMPowerWideIO::getPower(){
    return allranks_avg_power;
}


void DRAMPowerWideIO::updateCounters(bool lastUpdate, unsigned idx, int64_t timestamp)
{
    counters[idx].getCommands(cmdListPerRank[idx], lastUpdate, timestamp);
    evaluateCommands(cmdListPerRank[idx],idx); //command list already modified
    cmdListPerRank[idx].clear();
}

// Used to analyse a given list of commands and identify command timings
// and memory state transitions
void DRAMPowerWideIO::evaluateCommands(vector<MemCommand>& cmd_list, unsigned idx)
{
  // for each command identify timestamp, type and bank
  for (auto cmd : cmd_list) {
    // For command type
    int type = cmd.getType();
    // For command bank
    unsigned bank = cmd.getBank();
    // Command Issue timestamp in clock cycles (cc)
    int64_t timestamp = cmd.getTimeInt64();

    if (bank < memSpec.numberOfBanks){
        if (type == MemCommand::ACT) {
            counters[idx].handleAct(bank, timestamp);
        } else if (type == MemCommand::RD) {
            counters[idx].handleRd(bank, timestamp);
        } else if (type == MemCommand::WR) {
            counters[idx].handleWr(bank, timestamp);
        } else if (type == MemCommand::REF) {
            counters[idx].handleRef(bank, timestamp);
        } else if (type == MemCommand::PRE) {
            counters[idx].handlePre(bank, timestamp);
        } else if (type == MemCommand::PREA) {
            counters[idx].handlePreA(bank, timestamp);
        } else if (type == MemCommand::PDN_F_ACT) {
            counters[idx].handlePdnFAct(bank, timestamp);
        } else if (type == MemCommand::PDN_F_PRE) {
            counters[idx].handlePdnFPre(bank, timestamp);
        } else if (type == MemCommand::PUP_ACT) {
            counters[idx].handlePupAct(timestamp);
        } else if (type == MemCommand::PUP_PRE) {
            counters[idx].handlePupPre(timestamp);
        } else if (type == MemCommand::SREN) {
            counters[idx].handleSREn(bank, timestamp);
        } else if (type == MemCommand::SREX) {
            counters[idx].handleSREx(bank, timestamp);
        } else if (type == MemCommand::END || type == MemCommand::NOP) {
            counters[idx].handleNopEnd(timestamp);
        } else {
            PRINTDEBUGMESSAGE("Unknown command given, exiting.", timestamp, type, bank);
            exit(-1);
        }
    }
    else PRINTDEBUGMESSAGE("Command given to non-existent bank", timestamp, type, bank);
  }
} // Counters::evaluateCommands

//call the clear counters
void DRAMPowerWideIO::clearCountersWrapper()
{
    for (unsigned i = 0; i < memSpec.numberOfRanks; ++i){
        counters[i].clear();
    }
}

//Split command list in sublists for each rank
void DRAMPowerWideIO::splitCmdList(){
    for (size_t i = 0; i < cmdList.size(); ++i) {
        MemCommand& cmd = cmdList[i];
        unsigned rank = cmd.getRank();
        if (rank == 0) {
            cmdListPerRank[0].push_back(cmd);
        } else if (rank == 1 && memSpec.numberOfRanks>=2) {
            cmdListPerRank[1].push_back(cmd);
        }else if (rank == 2 && memSpec.numberOfRanks>=3) {
            cmdListPerRank[2].push_back(cmd);
        }else if (rank == 3 && memSpec.numberOfRanks==4) {
            cmdListPerRank[3].push_back(cmd);
        }
        else{
            throw std::invalid_argument("Command issued to invalid Rank. WideIO has maximum of 4 Ranks. RankIDs={0,1,2,3}");
        }
    }
}

//////////////////POWER CALCULATION////////////////////////


void DRAMPowerWideIO::Energy::clearEnergy(int64_t nbrofBanks){

      act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      read_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      write_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      refb_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      act_stdby_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      pre_stdby_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      idle_energy_act_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      idle_energy_pre_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      f_act_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      f_pre_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      sref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      sref_ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      sref_ref_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      sref_ref_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      spup_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      spup_ref_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      spup_ref_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      spup_ref_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      pup_act_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      pup_pre_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      total_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
      window_energy_per_vdd.assign(static_cast<size_t>(nbrofBanks), 0.0);

      window_energy       = 0.0;

      read_io_energy      = 0.0;
      write_term_energy   = 0.0;
      io_term_energy      = 0.0;
}


void DRAMPowerWideIO::Power::clearIOPower(){
    IO_power             = 0.0;
    WR_ODT_power         = 0.0;
}



void DRAMPowerWideIO::bankPowerCalc(unsigned idx)
{
  const MemSpecWideIO::MemTimingSpec& t = memSpec.memTimingSpec;
  const MemSpecWideIO::BankWiseParams& bwPowerParams = memSpec.bwParams;
  const int64_t nbrofBanks               = memSpec.numberOfBanks;
  const Counters& c = counters[idx];


  int vddIdx=0;
  for (auto mps : memSpec.memPowerSpec) {


    int64_t burstCc = memSpec.burstLength / memSpec.dataRate;

    // Using the number of cycles that at least one bank is active here
    // But the current iDDrho is less than iDD3N1
    double iDDrho = (static_cast<double>(bwPowerParams.bwPowerFactRho) / 100.0) * (mps.iDD3NX - mps.iDD2NX) + mps.iDD2NX;
    double esharedActStdby = static_cast<double>(c.actcycles) * t.tCK * iDDrho * mps.vDDX;

    double ione = (mps.iDD3NX + (iDDrho * (static_cast<double>(nbrofBanks - 1)))) / (static_cast<double>(nbrofBanks));
    // If memory specification does not provide  bank wise refresh current,
    // approximate it to single bank background current removed from
    // single bank active current

    //TODO: correct this, iDD5b is not burst but average current!!!
    //double iDD5Blocal = (mps.iDD5B == 0.0) ? (mps.iDD0 - ione) :(mps.iDD5B);
    //WideIO DOESNT HAVE PB REF

    // if memory specification does not provide the REFB timing approximate it
    // to time of ACT + PRE
    //int64_t tRefBlocal = (t.REFB == 0) ? (t.tRAS + t.tRP) : (t.REFB);

    //Distribution of energy componets to each banks
    for (unsigned i = 0; i < nbrofBanks; i++) {
        energy[idx].act_energy_banks[i] = static_cast<double>(c.numberofactsBanks[i] * t.tRAS) * t.tCK  * (mps.iDD0X - ione) * mps.vDDX;
        energy[idx].pre_energy_banks[i] = static_cast<double>(c.numberofpresBanks[i] * t.tRP) * t.tCK  * (mps.iDD0X - ione) * mps.vDDX;
        energy[idx].read_energy_banks[i] = static_cast<double>(c.numberofreadsBanks[i] * burstCc) * t.tCK  * (mps.iDD4RX - mps.iDD3NX) * mps.vDDX   ;
        energy[idx].write_energy_banks[i] = static_cast<double>(c.numberofwritesBanks[i] * burstCc) * t.tCK * (mps.iDD4WX - mps.iDD3NX) * mps.vDDX;

        energy[idx].ref_energy_banks[i] = static_cast<double>(c.numberofrefs * t.tRFC) * t.tCK * (mps.iDD5X - mps.iDD3NX) * mps.vDDX /
                                                         static_cast<double>(nbrofBanks);
        energy[idx].pre_stdby_energy_banks[i] = static_cast<double>(c.precycles) * t.tCK * mps.iDD2NX * mps.vDDX/ static_cast<double>(nbrofBanks);
        energy[idx].act_stdby_energy_banks[i] = (static_cast<double>(c.actcyclesBanks[i]) * t.tCK * (mps.iDD3NX - iDDrho) * mps.vDDX/
                                            static_cast<double>(nbrofBanks)) + esharedActStdby / static_cast<double>(nbrofBanks);
        energy[idx].idle_energy_act_banks[i] = static_cast<double>(c.idlecycles_act) * t.tCK * mps.iDD3NX * mps.vDDX/ static_cast<double>(nbrofBanks);
        energy[idx].idle_energy_pre_banks[i] = static_cast<double>(c.idlecycles_pre) * t.tCK * mps.iDD2NX * mps.vDDX/ static_cast<double>(nbrofBanks);
        energy[idx].f_act_pd_energy_banks[i] = static_cast<double>(c.f_act_pdcycles) * t.tCK * mps.iDD3PX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].f_pre_pd_energy_banks[i] = static_cast<double>(c.f_pre_pdcycles) * t.tCK * mps.iDD2PX * mps.vDDX / static_cast<double>(nbrofBanks);

        energy[idx].sref_energy_banks[i] = (((mps.iDD6X * static_cast<double>(c.sref_cycles)) + ((mps.iDD5X - mps.iDD3NX) * static_cast<double>(c.sref_ref_act_cycles
                                            + c.spup_ref_act_cycles + c.sref_ref_pre_cycles + c.spup_ref_pre_cycles)))
                                            * mps.vDDX * t.tCK) / static_cast<double>(memSpec.numberOfBanks);
        energy[idx].sref_ref_act_energy_banks[i] = static_cast<double>(c.sref_ref_act_cycles) * t.tCK * mps.iDD3PX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].sref_ref_pre_energy_banks[i] = static_cast<double>(c.sref_ref_pre_cycles) * t.tCK * mps.iDD2PX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].sref_ref_energy_banks[i] = energy[idx].sref_ref_act_energy_banks[i] + energy[idx].sref_ref_pre_energy_banks[i] ;//

        energy[idx].spup_energy_banks[i] = static_cast<double>(c.spup_cycles) * t.tCK * mps.iDD2NX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].spup_ref_act_energy_banks[i] = static_cast<double>(c.spup_ref_act_cycles) * t.tCK * mps.iDD3NX * mps.vDDX / static_cast<double>(nbrofBanks);//
        energy[idx].spup_ref_pre_energy_banks[i] = static_cast<double>(c.spup_ref_pre_cycles) * t.tCK * mps.iDD2NX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].spup_ref_energy_banks[i] = energy[idx].spup_ref_act_energy_banks[i] + energy[idx].spup_ref_pre_energy_banks[i];
        energy[idx].pup_act_energy_banks[i] = static_cast<double>(c.pup_act_cycles) * t.tCK * mps.iDD3NX * mps.vDDX / static_cast<double>(nbrofBanks);
        energy[idx].pup_pre_energy_banks[i] = static_cast<double>(c.pup_pre_cycles) * t.tCK * mps.iDD2NX * mps.vDDX / static_cast<double>(nbrofBanks);
    }



        // Calculate total energy per bank.
        for (unsigned i = 0; i < nbrofBanks; i++) {
            energy[idx].total_energy_banks[i] = energy[idx].act_energy_banks[i] + energy[idx].pre_energy_banks[i] + energy[idx].read_energy_banks[i]
                                          + energy[idx].ref_energy_banks[i] + energy[idx].write_energy_banks[i] +
                                          + (energy[idx].act_stdby_energy_banks[i]
                                          + energy[idx].pre_stdby_energy_banks[i] + energy[idx].f_pre_pd_energy_banks[i]
                                          + energy[idx].sref_ref_energy_banks[i] + energy[idx].spup_ref_energy_banks[i]);
        }

        //Energy total for vdd domain
        energy[idx].window_energy_per_vdd[vddIdx] = sum(energy[idx].total_energy_banks);
        vddIdx++;
      }
    // Calculate total energy for all banks.
    energy[idx].window_energy = sum(energy[idx].window_energy_per_vdd) + energy[idx].io_term_energy;

    power[idx].window_average_power = energy[idx].window_energy / (static_cast<double>(window_cycles) * t.tCK);

    window_cycles = c.actcycles + c.precycles +
                    c.f_act_pdcycles + c.f_pre_pdcycles +
                    + c.s_pre_pdcycles + c.sref_cycles
                    + c.sref_ref_act_cycles + c.sref_ref_pre_cycles +
                    c.spup_ref_act_cycles + c.spup_ref_pre_cycles;

    total_cycles += window_cycles;

    energy[idx].total_energy += energy[idx].window_energy;
    allranks_energy += energy[idx].window_energy;

    // Calculate the average power consumption
    power[idx].average_power = energy[idx].total_energy / (static_cast<double>(total_cycles) * t.tCK);

    allranks_avg_power = allranks_energy / (static_cast<double>(total_cycles) * t.tCK);

} // DRAMPowerWideIO::bankPowerCalc



// IO and Termination power calculation based on Micron Power Calculators
// Absolute power measures are obtained from Micron Power Calculator (mentioned in mW)
void DRAMPowerWideIO::io_term_power()
{

  power[0].IO_power     = memSpec.memPowerSpec[0].ioPower;    // in W
  power[0].WR_ODT_power = memSpec.memPowerSpec[0].wrOdtPower; // in W

  if (memSpec.memPowerSpec[0].capacitance != 0.0) {
    // If capacity is given, then IO Power depends on DRAM clock frequency.
    power[0].IO_power = memSpec.memPowerSpec[0].capacitance * 0.5 * pow(memSpec.memPowerSpec[0].vDDX, 2.0)
                        * memSpec.memTimingSpec.fCKMHz * 1000000;
  }
} // DRAMPowerWideIO::io_term_power


void DRAMPowerWideIO::calcIoTermEnergy(unsigned idx)
{
        const MemSpecWideIO::MemTimingSpec& t                 = memSpec.memTimingSpec;
        const Counters& c = counters[idx];

        // memSpec.width represents the number of data (dq) pins.
        // 1 DQS pin is associated with every data byte
        int64_t dqPlusDqsBits = memSpec.bitWidth + memSpec.bitWidth / 8;
        // 1 DQS and 1 DM pin is associated with every data byte
        int64_t dqPlusDqsPlusMaskBits = memSpec.bitWidth + memSpec.bitWidth / 8 + memSpec.bitWidth / 8;
        // Size of one clock period for the data bus.
        double ddtRPeriod = t.tCK / static_cast<double>(memSpec.dataRate);

        // Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
        energy[idx].read_io_energy = static_cast<double>(sum(c.numberofreadsBanks) * memSpec.burstLength)
                                * ddtRPeriod * power[0].IO_power * static_cast<double>(dqPlusDqsBits);


        // Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM
        energy[idx].write_term_energy = static_cast<double>(sum(c.numberofwritesBanks) * memSpec.burstLength)
                                   * ddtRPeriod * power[0].WR_ODT_power * static_cast<double>(dqPlusDqsPlusMaskBits);

        // Sum of all IO and termination energy
        energy[idx].io_term_energy = energy[idx].read_io_energy + energy[idx].write_term_energy;

}




void DRAMPowerWideIO::powerPrint()
{
   const char eUnit[] = " pJ";
   const int64_t nbrofBanks = memSpec.numberOfBanks;
   ios_base::fmtflags flags = cout.flags();
   streamsize precision = cout.precision();
   cout.precision(0);
   for (size_t i = 0; i < counters.size(); ++i){
       const Counters& c = counters[i];
       const Energy& e = energy[i];
       const Power& pow = power[i];
       cout << endl << "* Rank " << i << " Details *" << endl << endl;
       cout << endl << "* Bankwise Details:";
       for (unsigned i = 0; i < nbrofBanks; i++) {
           cout << endl << "## @ Bank " << i << fixed
                << endl << "  #ACT commands: " << c.numberofactsBanks[i]
                   << endl << "  #RD + #RDA commands: " << c.numberofreadsBanks[i]
                      << endl << "  #WR + #WRA commands: " << c.numberofwritesBanks[i]
                         << endl << "  #PRE (+ PREA) commands: " << c.numberofpresBanks[i];
       }
       cout << endl;

       for (unsigned i = 0; i < nbrofBanks; i++) {
           cout << endl << "## @ Bank " << i << fixed
                << endl << "  ACT Cmd Energy: " << e.act_energy_banks[i] << eUnit
                << endl << "  PRE Cmd Energy: " << e.pre_energy_banks[i] << eUnit
                << endl << "  RD Cmd Energy: " << e.read_energy_banks[i] << eUnit
                << endl << "  WR Cmd Energy: " << e.write_energy_banks[i] << eUnit
                << endl << "  Auto-Refresh Energy: " << e.ref_energy_banks[i] << eUnit
                << endl << "  ACT Stdby Energy: " <<  e.act_stdby_energy_banks[i] << eUnit
                << endl << "  PRE Stdby Energy: " << e.pre_stdby_energy_banks[i] << eUnit
                << endl << "  Active Idle Energy: "<<  e.idle_energy_act_banks[i] << eUnit
                << endl << "  Precharge Idle Energy: "<<  e.idle_energy_pre_banks[i] << eUnit
                << endl << "  Fast-Exit Active Power-Down Energy: "<<  e.f_act_pd_energy_banks[i] << eUnit
                << endl << "  Fast-Exit Precharged Power-Down Energy: "<<  e.f_pre_pd_energy_banks[i] << eUnit
                << endl << "  Self-Refresh Energy: "<<  e.sref_energy_banks[i] << eUnit
                << endl << "  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: "<<  e.sref_ref_act_energy_banks[i] << eUnit
                << endl << "  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: " <<  e.sref_ref_pre_energy_banks[i] << eUnit
                << endl << "  Self-Refresh Power-Up Energy: "<< e.spup_energy_banks[i] << eUnit
                << endl << "  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<<  e.spup_ref_act_energy_banks[i] << eUnit
                << endl << "  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<<  e.spup_ref_pre_energy_banks[i] << eUnit
                << endl << "  Active Power-Up Energy: "<<  e.pup_act_energy_banks[i] << eUnit
                << endl << "  Precharged Power-Up Energy: "<< e.pup_pre_energy_banks[i] << eUnit
                << endl << "  Total Energy of Bank: " << e.total_energy_banks[i] << eUnit
                << endl;
       }
       cout << endl;
       cout << endl
            << endl << "  Rank " << i << " Energy : "<< e.total_energy << eUnit
            << endl << "  Rank " << i << " Average Power : " << pow.average_power << " mW"
            << endl << "----------------------------------------" << endl;
   }
  if (includeIoAndTermination) {
    cout << endl << "RD I/O Energy: " << energy[0].read_io_energy << eUnit << endl;
    // No Termination for LPDDR/2/3 and DDR memories
    cout << "WR Termination Energy: " << energy[0].write_term_energy << eUnit << endl;

  }

  cout << endl;
  cout << endl << "----------------------------------------"
       << endl << "  Total Trace Energy : "<< allranks_energy << eUnit
       << endl << "  Total Average Power : " << allranks_avg_power << " mW"
       << endl << "----------------------------------------" << endl;


  cout.flags(flags);
  cout.precision(precision);
}



