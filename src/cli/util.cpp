#include "util.hpp"


std::unique_ptr<uint8_t[]> CLIutil::hexStringToUint8Array(const csv::string_view data, size_t &size)
{
	// Check if the string has valid length
	if ( ( data.length() % 2 ) != 0)
	{
		throw std::invalid_argument("Invalid hex string length");
	}

	// String conversion
	std::string hexString;
	size = data.length() / 2;
	
	// Remove 0x or 0X prefix if present
	if (data.substr(0, 2) == "0x" || data.substr(0, 2) == "0X")
	{
		size--;
		hexString = data.substr(2);
	}
	else
	{
		hexString = data;
	}
	
	// Allocate memory for the array and fill it
	auto content = std::make_unique<uint8_t[]>(size);
	for (size_t i = 0; i < size; i++)
	{
		content[i] = static_cast<uint8_t>(std::stoi(hexString.substr(i * 2, 2), nullptr, 16));
	}
	return content;
}