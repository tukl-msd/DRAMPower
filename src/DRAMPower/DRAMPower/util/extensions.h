#ifndef DRAMPOWER_UTIL_EXTENSIONS
#define DRAMPOWER_UTIL_EXTENSIONS

#include <DRAMPower/command/Command.h>
#include <DRAMPower/util/extension_base.h>
#include <DRAMPower/util/databus_presets.h>

namespace DRAMPower::extensions {

class DRAMPowerExtensionBase {

};

class DRAMPowerExtensionDBI : public DRAMPowerExtensionBase {
    public:
        explicit DRAMPowerExtensionDBI(DRAMPower::util::databus_presets::databus_preset_t& dataBus);

    public:
        void set(bool dbi);
        bool get() const;
    private:
        bool m_dbi = false;
        DRAMPower::util::databus_presets::databus_preset_t& m_dataBus;
};

} // namespace DRAMPower::extensions

#endif /* DRAMPOWER_UTIL_EXTENSIONS */
