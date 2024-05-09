#include "util.hpp"


inline bool CLIutil::hexCharToInt(char hexChar, uint8_t &result)
{
	if( hexChar >= '0' && hexChar <= '9' ) 
	{
		result = hexChar - '0';
		return true;
	}
	else if ( hexChar >= 'A' && hexChar <= 'F' )
	{
		result = hexChar - 'A' + 10;
		return true;
	}
	else if ( hexChar >= 'a' && hexChar <= 'f' )
	{
		result = hexChar - 'a' + 10;
		return true;
	}
	else
	{
		return false;	
	}
}

bool CLIutil::hexStringToUint8Array(const csv::string_view data, std::unique_ptr<uint8_t[]> &arr, std::size_t &arraySize)
{
	if ( ( data.length() % 2 ) != 0)
	{
		return false;
	}

	arraySize = data.length() / 2;
	arr = std::make_unique<uint8_t[]>(arraySize);

	for( std::size_t i = 0; i < data.length(); i += 2 )
	{
		uint8_t byte_upper;
		uint8_t byte_lower;
		if ( !hexCharToInt(data.at(i), byte_upper) )
		{
			return false;
		}
		byte_upper <<= 4;
		if ( !hexCharToInt(data.at(i), byte_lower) )
		{
			return false;
		}
		arr[i / 2] = byte_upper | byte_lower;

	}

	return true;
}