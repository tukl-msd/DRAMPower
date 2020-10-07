#include "DRAMPowerIF.h"

void DRAMPowerIF::doCommand(DRAMPower::MemCommand::cmds type, int bank, int64_t timestamp)
{
  DRAMPower::MemCommand cmd(type, static_cast<unsigned>(bank), timestamp);
  cmdList.push_back(cmd);
}

void DRAMPowerIF::setupDebugManager(const bool debug __attribute__((unused)),
                                    const bool writeToConsole __attribute__((unused)),
                                    const bool writeToFile __attribute__((unused)),
                                    const std::string &traceName __attribute__((unused)))

{
#ifndef NDEBUG
    auto &dbg = DebugManager::getInstance();
    dbg.debug = debug;
    dbg.writeToConsole = writeToConsole;
    dbg.writeToFile = writeToFile;
    if (dbg.writeToFile && (traceName!=""))
        dbg.openDebugFile(traceName + ".txt");
#endif
}
