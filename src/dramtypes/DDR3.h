#ifndef DDR3_H
#define DDR3_H

#include <vector>
#include <stdint.h>
#include <cmath>  // For pow


#include "./memspec/MemSpecDDR3.h"
#include "MemCommand.h"
#include "./counters/Counters.h"
#include "./counters/CountersDDR3.h"
#include "DRAMPowerIF.h"
#include "./common/DebugManager.h"

namespace DRAMPower {


class DRAMPowerDDR3 final : public DRAMPowerIF
{
public:
    DRAMPowerDDR3(MemSpecDDR3 &memSpec,  bool includeIoAndTermination);

    ~DRAMPowerDDR3(){}

    //////Interface methods
    void calcEnergy();

    void calcWindowEnergy(int64_t timestamp);

    double getEnergy();

    double getPower();

    void clearCountersWrapper();

    void powerPrint();

    void updateCounters(bool lastUpdate, int64_t timestamp = 0);
    //////

    MemSpecDDR3 memSpec;


    void bankPowerCalc();


    //  // Used to calculate self-refresh active energy
    double engy_sref_banks(const Counters &c, MemSpecDDR3::MemPowerSpec &mps, double esharedPASR, unsigned bnkIdx);

      int64_t total_cycles;

      int64_t window_cycles;

      struct Energy {
        // Total energy of all activates
        std::vector<double> act_energy_banks;

        // Total energy of all precharges
        std::vector<double> pre_energy_banks;

        // Total energy of all reads
        std::vector<double> read_energy_banks;

        // Total energy of all writes
        std::vector<double> write_energy_banks;

        // Total energy of all refreshes
        std::vector<double> ref_energy_banks;

        // Bankwise refresh energy
        std::vector<double> refb_energy_banks;

        // Total background energy of all active standby cycles
        std::vector<double> act_stdby_energy_banks;

        // Total background energy of all precharge standby cycles
        std::vector<double> pre_stdby_energy_banks;

        // Total energy of idle cycles in the active mode
        std::vector<double> idle_energy_act_banks;

        // Total energy of idle cycles in the precharge mode
        std::vector<double> idle_energy_pre_banks;

        // Total trace/pattern energy
        double total_energy;
        std::vector<double> total_energy_banks;

        //Window energy per voltage domain
        std::vector<double> window_energy_per_vdd;

        // Window energy
        double window_energy;

        // Average Power
        double average_power;

        // Energy consumed in active/precharged fast/slow-exit modes
        std::vector<double> f_act_pd_energy_banks;

        std::vector<double> f_pre_pd_energy_banks;

        std::vector<double> s_act_pd_energy_banks;

        std::vector<double> s_pre_pd_energy_banks;

        // Energy consumed in self-refresh mode
        std::vector<double> sref_energy_banks;

        // Energy consumed in auto-refresh during self-refresh mode
        std::vector<double> sref_ref_energy_banks;

        std::vector<double> sref_ref_act_energy_banks;

        std::vector<double> sref_ref_pre_energy_banks;

        // Energy consumed in powering-up from self-refresh mode
        std::vector<double> spup_energy_banks;

        // Energy consumed in auto-refresh during self-refresh power-up
        std::vector<double> spup_ref_energy_banks;

        std::vector<double> spup_ref_act_energy_banks;

        std::vector<double> spup_ref_pre_energy_banks;

        // Energy consumed in powering-up from active/precharged power-down modes
        std::vector<double> pup_act_energy_banks;

        std::vector<double> pup_pre_energy_banks;

        // Energy consumed by IO and Termination
        double read_io_energy;     // Read IO Energy
        double write_term_energy;  // Write Termination Energy
        double read_oterm_energy;  // Read Termination Energy from idle rank
        double write_oterm_energy; // Write Termination Energy from idle rank
        // Total IO and Termination Energy
        double io_term_energy;

        void clearEnergy(int64_t nbrofBanks);
      };

      struct Power {
        // Power measures corresponding to IO and Termination
        double IO_power;     // Read IO Power
        double WR_ODT_power; // Write ODT Power
        double TermRD_power; // Read Termination in idle rank (in dual-rank systems)
        double TermWR_power; // Write Termination in idle rank (in dual-rank systems)

        // Average Power
        double average_power;

        // Window Average Power
        double window_average_power;

        //Clear IO and Termination Power
        void clearIOPower();
      };

      void io_term_power();

      // To calculate IO and Termination Energy
      void calcIoTermEnergy();


      Energy energy;
      Power  power;

//TODO: ver o q precisa ser publico msm
private:  
    CountersDDR3 counters;
    bool includeIoAndTermination;
    void evaluateCommands(std::vector<MemCommand>& cmd_list);
    template <typename T> T sum(const std::vector<T> vec) const { return std::accumulate(vec.begin(), vec.end(), static_cast<T>(0)); }

};
}
#endif // DDR3_H