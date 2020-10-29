#include "DRAMPowerIF.h"
#include "DRAMPowerDDR4.h"
using namespace DRAMPower;
using namespace std;


DRAMPowerDDR4::DRAMPowerDDR4(MemSpecDDR4& memSpec, bool includeIoAndTermination):
    memSpec(memSpec),
    counters(memSpec),
    includeIoAndTermination(includeIoAndTermination)
{
    total_cycles = 0;
    energy.total_energy = 0;
}

void DRAMPowerDDR4::calcEnergy()
{
  updateCounters(true);
  energy.clearEnergy(memSpec.numberOfBanks);
  power.clearIOPower();
  if (includeIoAndTermination) calcIoTermEnergy();
  bankPowerCalc();
}

void DRAMPowerDDR4::calcWindowEnergy(int64_t timestamp)
{
  doCommand(MemCommand::NOP, 0, timestamp);
  updateCounters(false, timestamp);
  energy.clearEnergy(memSpec.numberOfBanks);
  power.clearIOPower();
  if (includeIoAndTermination) calcIoTermEnergy();
  bankPowerCalc();
  counters.clearCounters(timestamp);
}


double DRAMPowerDDR4::getEnergy() {
   return energy.total_energy;
}

double DRAMPowerDDR4::getPower(){
    return power.average_power;
}


void DRAMPowerDDR4::updateCounters(bool lastUpdate, int64_t timestamp)
{

  cout << endl << "  entered update counters success ";
  counters.getCommands(cmdList, lastUpdate, timestamp);
  cout << endl << "  getCommands succsss ";
  evaluateCommands(cmdList); //command list already modified
  cmdList.clear();
}

// Used to analyse a given list of commands and identify command timings
// and memory state transitions
void DRAMPowerDDR4::evaluateCommands(vector<MemCommand>& cmd_list)  //change: it has acces to cmdList
{
  // for each command identify timestamp, type and bank
  for (auto cmd : cmd_list) {
    // For command type
    int type = cmd.getType();
    // For command bank
    unsigned bank = cmd.getBank();
    // Command Issue timestamp in clock cycles (cc)
    int64_t timestamp = cmd.getTimeInt64();

    if (type == MemCommand::ACT) {
      counters.handleAct(bank, timestamp);
    } else if (type == MemCommand::RD) {
      counters.handleRd(bank, timestamp);
    } else if (type == MemCommand::WR) {
      counters.handleWr(bank, timestamp);
    } else if (type == MemCommand::REF) {
      counters.handleRef(bank, timestamp);
    } else if (type == MemCommand::PRE) {
      counters.handlePre(bank, timestamp);
    } else if (type == MemCommand::PREA) {
      counters.handlePreA(bank, timestamp);
    } else if (type == MemCommand::PDN_F_ACT) {
      counters.handlePdnFAct(bank, timestamp);
    } else if (type == MemCommand::PDN_F_PRE) {
      counters.handlePdnFPre(bank, timestamp);
    } else if (type == MemCommand::PDN_S_PRE) {
      counters.handlePdnSPre(bank, timestamp);
    } else if (type == MemCommand::PUP_ACT) {
      counters.handlePupAct(timestamp);
    } else if (type == MemCommand::PUP_PRE) {
      counters.handlePupPre(timestamp);
    } else if (type == MemCommand::SREN) {
      counters.handleSREn(bank, timestamp);
    } else if (type == MemCommand::SREX) {
      counters.handleSREx(bank, timestamp);
    } else if (type == MemCommand::END || type == MemCommand::NOP) {
      counters.handleNopEnd(timestamp);
    } else {
      PRINTDEBUGMESSAGE("Unknown command given, exiting.", timestamp, type, bank);
      exit(-1);
    }
  }
} // Counters::evaluateCommands

//call the clear counters
void DRAMPowerDDR4::clearCountersWrapper()
{
  counters.clear();
}

//////////////////POWER CALCULATION////////////////////////


void DRAMPowerDDR4::Energy::clearEnergy(int64_t nbrofBanks){

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
      s_pre_pd_energy_banks.assign(static_cast<size_t>(nbrofBanks), 0.0);
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
      read_oterm_energy   = 0.0;
      write_oterm_energy  = 0.0;
      io_term_energy      = 0.0;
}


void DRAMPowerDDR4::Power::clearIOPower(){
    IO_power             = 0.0;
    WR_ODT_power         = 0.0;
    TermRD_power         = 0.0;
    TermWR_power         = 0.0;
}



void DRAMPowerDDR4::bankPowerCalc()
{
  const MemSpecDDR4::MemTimingSpec& t = memSpec.memTimingSpec;
  const MemSpecDDR4::MemArchitectureSpec& memArchSpec = memSpec.memArchSpec;
  const MemSpecDDR4::BankWiseParams& bwPowerParams = memSpec.bwParams;
  const int64_t nbrofBanks               = memSpec.numberOfBanks;
  const Counters& c = counters;

  int vddIdx=0;
  for (auto mps : memSpec.memPowerSpec) {


    int64_t burstCc = memSpec.burstLength / memSpec.dataRate;

    // Using the number of cycles that at least one bank is active here
    // But the current iDDrho is less than iDD3N1
    double iDDrho = (static_cast<double>(bwPowerParams.bwPowerFactRho) / 100.0) * (mps.iXX3N - mps.iXX2N) + mps.iXX2N;
    double esharedActStdby = static_cast<double>(c.actcycles) * t.tCK * iDDrho * mps.vXX;
    // Fixed componenent for PASR
    double iDDsigma = (static_cast<double>(bwPowerParams.bwPowerFactSigma) / 100.0) * mps.iXX6;
    double esharedPASR = static_cast<double>(c.sref_cycles) * t.tCK * iDDsigma * mps.iXX6;
    // ione is Active background current for a single bank. When a single bank is Active
    //,all the other remainig (B-1) banks will consume  a current of iDDrho (based on factor Rho)
    // So to derrive ione we add (B-1)*iDDrho to the iDD3N and distribute it to each banks.
    double ione = (mps.iXX3N + (iDDrho * (static_cast<double>(nbrofBanks - 1)))) / (static_cast<double>(nbrofBanks));
    // If memory specification does not provide  bank wise refresh current,
    // approximate it to single bank background current removed from
    // single bank active current

    //TODO: correct this, iDD5b is not burst but average current!!!
    //double iDD5Blocal = (mps.iDD5B == 0.0) ? (mps.iDD0 - ione) :(mps.iDD5B);
    //DDR4 DOESNT HAVE PB REF

    // if memory specification does not provide the REFB timing approximate it
    // to time of ACT + PRE
    //int64_t tRefBlocal = (t.REFB == 0) ? (t.tRAS + t.tRP) : (t.REFB);

    //Distribution of energy componets to each banks
    for (unsigned i = 0; i < nbrofBanks; i++) {
        energy.act_energy_banks[i] = static_cast<double>(c.numberofactsBanks[i] * t.tRAS) * t.tCK  * (mps.iXX0 - ione) * mps.vXX;
        energy.pre_energy_banks[i] = static_cast<double>(c.numberofpresBanks[i] * t.tRP) * t.tCK  * (mps.iXX0 - ione) * mps.vXX;
        energy.read_energy_banks[i] = static_cast<double>(c.numberofreadsBanks[i] * burstCc) * t.tCK  * (mps.iXX4R - mps.iXX3N) * mps.vXX;
        energy.write_energy_banks[i] = static_cast<double>(c.numberofwritesBanks[i] * burstCc) * t.tCK * (mps.iXX4W - mps.iXX3N) * mps.vXX;

        energy.ref_energy_banks[i] = static_cast<double>(c.numberofrefs * t.tRFC) * t.tCK * (mps.iXX5 - mps.iXX3N) * mps.vXX /
                                                         static_cast<double>(nbrofBanks);
        energy.pre_stdby_energy_banks[i] = static_cast<double>(c.precycles) * t.tCK * mps.iXX2N * mps.vXX/ static_cast<double>(nbrofBanks);
        energy.act_stdby_energy_banks[i] = (static_cast<double>(c.actcyclesBanks[i]) * t.tCK * (mps.iXX3N - iDDrho) * mps.vXX/
                                            static_cast<double>(nbrofBanks)) + esharedActStdby / static_cast<double>(nbrofBanks);
        energy.idle_energy_act_banks[i] = static_cast<double>(c.idlecycles_act) * t.tCK * mps.iXX3N * mps.vXX/ static_cast<double>(nbrofBanks);
        energy.idle_energy_pre_banks[i] = static_cast<double>(c.idlecycles_pre) * t.tCK * mps.iXX2N * mps.vXX/ static_cast<double>(nbrofBanks);
        energy.f_act_pd_energy_banks[i] = static_cast<double>(c.f_act_pdcycles) * t.tCK * mps.iXX3P * mps.vXX / static_cast<double>(nbrofBanks);
        energy.f_pre_pd_energy_banks[i] = static_cast<double>(c.f_pre_pdcycles) * t.tCK * mps.iXX2P * mps.vXX / static_cast<double>(nbrofBanks);
        energy.s_pre_pd_energy_banks[i] = static_cast<double>(c.s_pre_pdcycles) * t.tCK * mps.iXX2P * mps.vXX / static_cast<double>(nbrofBanks);

        energy.sref_energy_banks[i] = engy_sref_banks(c,mps, esharedPASR, i);
        energy.sref_ref_act_energy_banks[i] = static_cast<double>(c.sref_ref_act_cycles) * t.tCK * mps.iXX3P * mps.vXX / static_cast<double>(nbrofBanks);
        energy.sref_ref_pre_energy_banks[i] = static_cast<double>(c.sref_ref_pre_cycles) * t.tCK * mps.iXX2P * mps.vXX / static_cast<double>(nbrofBanks);
        energy.sref_ref_energy_banks[i] = energy.sref_ref_act_energy_banks[i] + energy.sref_ref_pre_energy_banks[i] ;//

        energy.spup_energy_banks[i] = static_cast<double>(c.spup_cycles) * t.tCK * mps.iXX2N * mps.vXX / static_cast<double>(nbrofBanks);
        energy.spup_ref_act_energy_banks[i] = static_cast<double>(c.spup_ref_act_cycles) * t.tCK * mps.iXX3N * mps.vXX / static_cast<double>(nbrofBanks);//
        energy.spup_ref_pre_energy_banks[i] = static_cast<double>(c.spup_ref_pre_cycles) * t.tCK * mps.iXX2N * mps.vXX / static_cast<double>(nbrofBanks);
        energy.spup_ref_energy_banks[i] = energy.spup_ref_act_energy_banks[i] + energy.spup_ref_pre_energy_banks[i];
        energy.pup_act_energy_banks[i] = static_cast<double>(c.pup_act_cycles) * t.tCK * mps.iXX3N * mps.vXX / static_cast<double>(nbrofBanks);
        energy.pup_pre_energy_banks[i] = static_cast<double>(c.pup_pre_cycles) * t.tCK * mps.iXX2N * mps.vXX / static_cast<double>(nbrofBanks);
    }



        // Calculate total energy per bank.
        for (unsigned i = 0; i < nbrofBanks; i++) {
            energy.total_energy_banks[i] = energy.act_energy_banks[i] + energy.pre_energy_banks[i] + energy.read_energy_banks[i]
                                          + energy.ref_energy_banks[i] + energy.write_energy_banks[i] +
                                          + static_cast<double>(memSpec.numberOfRanks) * (energy.act_stdby_energy_banks[i]
                                          + energy.pre_stdby_energy_banks[i] + energy.f_pre_pd_energy_banks[i]
                                          + energy.s_pre_pd_energy_banks[i]+ energy.sref_ref_energy_banks[i] + energy.spup_ref_energy_banks[i]);
        }

        //Energy total for vdd domain
        energy.window_energy_per_vdd[vddIdx] = sum(energy.total_energy_banks);
        vddIdx++;
      }
    // Calculate total energy for all banks.
    energy.window_energy = sum(energy.window_energy_per_vdd) + energy.io_term_energy;

    power.window_average_power = energy.window_energy / (static_cast<double>(window_cycles) * t.tCK);

    window_cycles = c.actcycles + c.precycles +
                    c.f_act_pdcycles + c.f_pre_pdcycles +
                    + c.s_pre_pdcycles + c.sref_cycles
                    + c.sref_ref_act_cycles + c.sref_ref_pre_cycles +
                    c.spup_ref_act_cycles + c.spup_ref_pre_cycles;

    total_cycles += window_cycles;

    energy.total_energy += energy.window_energy;

    // Calculate the average power consumption
    power.average_power = energy.total_energy / (static_cast<double>(total_cycles) * t.tCK);

} // DRAMPowerDDR4::bankPowerCalc






// Self-refresh active energy estimation per banks
double DRAMPowerDDR4::engy_sref_banks(const Counters& c,MemSpecDDR4::MemPowerSpec& mps, double esharedPASR, unsigned bnkIdx)
{

    const MemSpecDDR4::BankWiseParams& bwPowerParams = memSpec.bwParams;
    const MemSpecDDR4::MemTimingSpec& t                 = memSpec.memTimingSpec;

    // Bankwise Self-refresh energy
    double sref_energy_banks;
    // Dynamic componenents for PASR energy varying based on PASR mode
    double iDDsigmaDynBanks;
    double pasr_energy_dyn;
    // This component is distributed among all banks
    double sref_energy_shared;
    //Is PASR Active
    if (bwPowerParams.flgPASR){
        sref_energy_shared = (((mps.iXX5 - mps.iXX3N) * (static_cast<double>(c.sref_ref_act_cycles
                                                          + c.spup_ref_act_cycles + c.sref_ref_pre_cycles + c.spup_ref_pre_cycles))) * mps.vXX * t.tCK)
                                                / memSpec.numberOfBanks;
        //if the bank is active under current PASR mode
        if (bwPowerParams.isBankActiveInPasr(bnkIdx)){
            // Distribute the sref energy to the active banks
            iDDsigmaDynBanks = (static_cast<double>(100 - bwPowerParams.bwPowerFactSigma) / (100.0 * static_cast<double>(memSpec.numberOfBanks))) * mps.iXX6;
            pasr_energy_dyn = mps.vXX * iDDsigmaDynBanks * static_cast<double>(c.sref_cycles);
            // Add the static components
            sref_energy_banks = sref_energy_shared + pasr_energy_dyn + (esharedPASR /static_cast<double>(memSpec.numberOfBanks));

        }else{
            sref_energy_banks = (esharedPASR /static_cast<double>(memSpec.numberOfBanks));
        }
    }
    //When PASR is not active total all the banks are in Self-Refresh. Thus total Self-Refresh energy is distributed across all banks
    else{


            sref_energy_banks = (((mps.iXX6 * static_cast<double>(c.sref_cycles)) + ((mps.iXX5 - mps.iXX3N) * static_cast<double>(c.sref_ref_act_cycles
                                                + c.spup_ref_act_cycles + c.sref_ref_pre_cycles + c.spup_ref_pre_cycles)))
                                                * mps.vXX * t.tCK)
                                                / static_cast<double>(memSpec.numberOfBanks);
    }
    return sref_energy_banks;
}


// IO and Termination power calculation based on Micron Power Calculators
// Absolute power measures are obtained from Micron Power Calculator (mentioned in mW)
void DRAMPowerDDR4::io_term_power()
{

  power.IO_power     = memSpec.memPowerSpec[0].ioPower;    // in W
  power.WR_ODT_power = memSpec.memPowerSpec[0].wrOdtPower; // in W

  if (memSpec.numberOfRanks > 1) {
    power.TermRD_power = memSpec.memPowerSpec[0].termRdPower; // in W
    power.TermWR_power = memSpec.memPowerSpec[0].termWrPower; // in W
  }

  if (memSpec.memPowerSpec[0].capacitance != 0.0) {
    // If capacity is given, then IO Power depends on DRAM clock frequency.
    power.IO_power = memSpec.memPowerSpec[0].capacitance * 0.5 * pow(memSpec.memPowerSpec[0].vXX, 2.0) * memSpec.memTimingSpec.fCKMHz * 1000000;
  }
} // DRAMPowerDDR4::io_term_power


void DRAMPowerDDR4::calcIoTermEnergy()
{
        io_term_power();
        const MemSpecDDR4::MemTimingSpec& t                 = memSpec.memTimingSpec;
        const Counters& c = counters;

        // memSpec.width represents the number of data (dq) pins.
        // 1 DQS pin is associated with every data byte
        int64_t dqPlusDqsBits = memSpec.bitWidth + memSpec.bitWidth / 8;
        // 1 DQS and 1 DM pin is associated with every data byte
        int64_t dqPlusDqsPlusMaskBits = memSpec.bitWidth + memSpec.bitWidth / 8 + memSpec.bitWidth / 8;
        // Size of one clock period for the data bus.
        double ddtRPeriod = t.tCK / static_cast<double>(memSpec.dataRate);

        // Read IO power is consumed by each DQ (data) and DQS (data strobe) pin
        energy.read_io_energy = static_cast<double>(sum(c.numberofreadsBanks) * memSpec.burstLength)
                                * ddtRPeriod * power.IO_power * static_cast<double>(dqPlusDqsBits);


        // Write ODT power is consumed by each DQ (data), DQS (data strobe) and DM
        energy.write_term_energy = static_cast<double>(sum(c.numberofwritesBanks) * memSpec.burstLength)
                                   * ddtRPeriod * power.WR_ODT_power * static_cast<double>(dqPlusDqsPlusMaskBits);


        if (memSpec.numberOfRanks > 1) {
          // Termination power consumed in the idle rank during reads on the active
          // rank by each DQ (data) and DQS (data strobe) pin.
          energy.read_oterm_energy = static_cast<double>(sum(c.numberofreadsBanks) * memSpec.burstLength)
                                     * ddtRPeriod * power.TermRD_power * static_cast<double>(dqPlusDqsBits);

          // Termination power consumed in the idle rank during writes on the active
          // rank by each DQ (data), DQS (data strobe) and DM (data mask) pin.
          energy.write_oterm_energy = static_cast<double>(sum(c.numberofwritesBanks) * memSpec.burstLength)
                                      * ddtRPeriod * power.TermWR_power * static_cast<double>(dqPlusDqsPlusMaskBits);
        }

        // Sum of all IO and termination energy
        energy.io_term_energy = energy.read_io_energy + energy.write_term_energy
                                + energy.read_oterm_energy + energy.write_oterm_energy;

}




void DRAMPowerDDR4::powerPrint()
{
   const Counters& c = counters;

  const uint64_t nRanks = static_cast<uint64_t>(memSpec.numberOfRanks);
  const char eUnit[] = " pJ";
  const int64_t nbrofBanks = memSpec.numberOfBanks;
  double nRanksDouble = static_cast<double>(nRanks);

  ios_base::fmtflags flags = cout.flags();
  streamsize precision = cout.precision();
  cout.precision(0);
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
        << endl << "  ACT Cmd Energy: " << energy.act_energy_banks[i] << eUnit
        << endl << "  PRE Cmd Energy: " << energy.pre_energy_banks[i] << eUnit
        << endl << "  RD Cmd Energy: " << energy.read_energy_banks[i] << eUnit
        << endl << "  WR Cmd Energy: " << energy.write_energy_banks[i] << eUnit
        << endl << "  Auto-Refresh Energy: " << energy.ref_energy_banks[i] << eUnit
        << endl << "  ACT Stdby Energy: " << nRanksDouble * energy.act_stdby_energy_banks[i] << eUnit
        << endl << "  PRE Stdby Energy: " << nRanksDouble * energy.pre_stdby_energy_banks[i] << eUnit
        << endl << "  Active Idle Energy: "<< nRanksDouble * energy.idle_energy_act_banks[i] << eUnit
        << endl << "  Precharge Idle Energy: "<< nRanksDouble * energy.idle_energy_pre_banks[i] << eUnit
        << endl << "  Fast-Exit Active Power-Down Energy: "<< nRanksDouble * energy.f_act_pd_energy_banks[i] << eUnit
        << endl << "  Fast-Exit Precharged Power-Down Energy: "<< nRanksDouble * energy.f_pre_pd_energy_banks[i] << eUnit
        << endl << "  Slow-Exit Precharged Power-Down Energy: "<< nRanksDouble * energy.s_pre_pd_energy_banks[i] << eUnit
        << endl << "  Self-Refresh Energy: "<< nRanksDouble * energy.sref_energy_banks[i] << eUnit
        << endl << "  Slow-Exit Active Power-Down Energy during Auto-Refresh cycles in Self-Refresh: "<< nRanksDouble * energy.sref_ref_act_energy_banks[i] << eUnit
        << endl << "  Slow-Exit Precharged Power-Down Energy during Auto-Refresh cycles in Self-Refresh: " << nRanksDouble * energy.sref_ref_pre_energy_banks[i] << eUnit
        << endl << "  Self-Refresh Power-Up Energy: "<< nRanksDouble * energy.spup_energy_banks[i] << eUnit
        << endl << "  Active Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<< nRanksDouble * energy.spup_ref_act_energy_banks[i] << eUnit
        << endl << "  Precharge Stdby Energy during Auto-Refresh cycles in Self-Refresh Power-Up: "<< nRanksDouble * energy.spup_ref_pre_energy_banks[i] << eUnit
        << endl << "  Active Power-Up Energy: "<< nRanksDouble * energy.pup_act_energy_banks[i] << eUnit
        << endl << "  Precharged Power-Up Energy: "<< nRanksDouble * energy.pup_pre_energy_banks[i] << eUnit
        << endl << "  Total Energy of Bank: " << energy.total_energy_banks[i] << eUnit
        << endl;
    }
    cout << endl;
    cout << endl << "----------------------------------------"
         << endl << "  Total Trace Energy : "<< energy.total_energy << eUnit
         << endl << "  Total Average Power : " << power.average_power << " mW"
         << endl << "----------------------------------------" << endl;

  if (includeIoAndTermination) {
    cout << endl << "RD I/O Energy: " << energy.read_io_energy << eUnit << endl;
    // No Termination for LPDDR/2/3 and DDR memories
    cout << "WR Termination Energy: " << energy.write_term_energy << eUnit << endl;

    if (nRanks > 1) {
      cout <<         "RD Termination Energy (Idle rank): " << energy.read_oterm_energy << eUnit
           << endl << "WR Termination Energy (Idle rank): " << energy.write_oterm_energy << eUnit << endl;
    }
  }

  cout.flags(flags);
  cout.precision(precision);
}



