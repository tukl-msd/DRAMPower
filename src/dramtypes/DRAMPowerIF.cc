#include "DRAMPowerIF.h"

void DRAMPowerIF::doCommand(DRAMPower::MemCommand::cmds type, int bank, int64_t timestamp)
{
  DRAMPower::MemCommand cmd(type, static_cast<unsigned>(bank), timestamp);
  cmdList.push_back(cmd);
}
