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
	NOP = 0,    			// 0
	RD,     				// 1
	WR,     				// 2
	RDA,    				// 3
	WRA,    				// 4
	ACT,    				// 5
	PRE,    				// 6
	REFB,   				// 7
	REFP2B,					// 8
	PRESB,  				// 9
	REFSB,  				// 10
	PREA,   				// 11
	REFA,   				// 12
	PDEA,   				// 13
	PDEP,   				// 14
	PDXA,   				// 15
	PDXP,   				// 16
	SREFEN, 				// 17
	SREFEX,  				// 18
	DSMEN,					// 19
	DSMEX,					// 20
	END_OF_SIMULATION,		// 21
	COUNT,					// 22
};

namespace CmdTypeUtil
{
	constexpr bool needs_data(CmdType cmd)
	{
		switch (cmd)
		{
		case CmdType::RD:
		case CmdType::RDA:
		case CmdType::WR:
		case CmdType::WRA:
			return true;
		default:
			return false;
		};
	};

	constexpr CmdType from_string(const std::string_view& str)
	{
		if (str == "NOP")
			return CmdType::NOP;
		if (str == "ACT")
			return CmdType::ACT;
		if (str == "PRE")
			return CmdType::PRE;
		if (str == "PREA")
			return CmdType::PREA;
		if (str == "PRESB")
			return CmdType::PRESB;
		if (str == "REFA")
			return CmdType::REFA;
		if (str == "REFB")
			return CmdType::REFB;
		if (str == "REFSB")
			return CmdType::REFSB;
		if (str == "REFP2B")
			return CmdType::REFP2B;
		if (str == "RD")
			return CmdType::RD;
		if (str == "RDA")
			return CmdType::RDA;
		if (str == "WR")
			return CmdType::WR;
		if (str == "WRA")
			return CmdType::WRA;
		if (str == "PDEA")
			return CmdType::PDEA;
		if (str == "PDEP")
			return CmdType::PDEP;
		if (str == "PDXA")
			return CmdType::PDXA;
		if (str == "PDXP")
			return CmdType::PDXP;
		if (str == "SREFEN")
			return CmdType::SREFEN;
		if (str == "SREFEX")
			return CmdType::SREFEX;
		if (str == "DSMEN")
			return CmdType::DSMEN;
		if (str == "DSMEX")
			return CmdType::DSMEX;
		if (str == "END")
			return CmdType::END_OF_SIMULATION;
		return CmdType::NOP;
	};

	constexpr const char * to_string(CmdType cmd)
	{
		switch (cmd)
		{
		case CmdType::NOP:
			return "NOP";
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
		}
	}

	inline std::ostream& operator<<(std::ostream& os, CmdType cmd) {
		return os << to_string(cmd);
	}
}
}
#endif /* DRAMPOWER_COMMAND_CMDTYPE_H */
