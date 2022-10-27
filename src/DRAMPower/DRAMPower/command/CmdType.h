#ifndef DRAMPOWER_COMMAND_CMDTYPE_H
#define DRAMPOWER_COMMAND_CMDTYPE_H

#include <iostream>
#include <string>

namespace DRAMPower
{

// Taken from DRAM 4.0
// ToDo: Look in DRAMSys; Mit Lukas absprechen
//												DRAMSys  
//												\	\__>  DRAMPower
//												 \            \_v
//												  \_______> DRAMCommon	
enum class CmdType {
	NOP = 0,    // 0
	RD,     // 1
	WR,     // 2
	RDA,    // 3
	WRA,    // 4
	ACT,    // 5
	PRE,    // 6
	REFB,   // 7
	REFP2B,
	PRESB,  // 8
	REFSB,  // 9
	PREA,   // 10
	REFA,   // 11
	PDEA,   // 12
	PDEP,   // 13
	PDXA,   // 15
	PDXP,   // 16
	SREFEN, // 14
	SREFEX,  // 17
	DSMEN,
	DSMEX,
	DDR5_EXTRA_COMMAND,
	CALC_WINDOW,
	END_OF_SIMULATION,
	COUNT,
};

constexpr const char * to_string(CmdType cmd)
{
	switch (cmd)
	{
	case CmdType::ACT:
		return "ACT";
	case CmdType::PRE:
		return "PRE";
	case CmdType::PREA:
		return "PREA";
	case CmdType::REFA:
		return "REFA";
	case CmdType::REFB:
		return "REFB";
	case CmdType::RD:
		return "RD";
	case CmdType::RDA:
		return "RDA";
	case CmdType::WR:
		return "WR";
	case CmdType::WRA:
		return "WRA";
	case CmdType::REFSB:
		return "REFSB";
	case CmdType::REFP2B:
		return "REFP2B";
	case CmdType::PRESB:
		return "PRESB";
	case CmdType::PDEA:
		return "PDEA";
	case CmdType::PDEP:
		return "PDEP";
	case CmdType::PDXA:
		return "PDXA";
	case CmdType::PDXP:
		return "PDXP";
	case CmdType::SREFEN:
		return "SREFEN";
	case CmdType::SREFEX:
		return "SREFEX";
	case CmdType::DSMEN:
		return "DSMEN";
	case CmdType::DSMEX:
		return "DSMEX";
	case CmdType::END_OF_SIMULATION:
		return "END_OF_SIMULATION";
	default:
		return "to_string()";
	};
};

inline std::ostream& operator<<(std::ostream& os, CmdType cmd) {
	return os << to_string(cmd);
}

}


#endif /* DRAMPOWER_COMMAND_CMDTYPE_H */
