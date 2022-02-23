// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <string>
#include <vector>

namespace hg {

struct BIOS {
	std::string vendor;
	std::string version;
	std::string release_date;
	uint8_t EC_version[2]; // embedded controller version (DSP0134)
};

struct System {
	std::string manufacturer;
	std::string product;
	std::string version;
	std::string serial;
	uint8_t uuid[16];
	std::string sku;
	std::string family;
};

struct Motherboard {
	std::string manufacturer;
	std::string product;
	std::string version;
	std::string serial_number;
	uint8_t asset_tag;
	uint8_t feature_flags;
	std::string location_in_chassis;
};

struct Processor {
	std::string socket;
	uint8_t type;
	uint8_t family;
	std::string manufacturer;
	uint64_t id;
	std::string version;
	uint8_t voltage;
	uint16_t external_clock; // in MHz
	uint16_t max_speed; // in MHz
	uint16_t current_speed; // in MHz
	uint8_t status;
	std::string serial_number;
	std::string asset_tag;
	std::string part_number;
	uint8_t core_count;
	uint8_t core_enabled;
	uint8_t thread_count;
	uint16_t core_count_2;
	uint16_t core_enabled_2;
	uint16_t thread_count_2;
};

struct SysInfo {
	BIOS bios;
	System system;
	Motherboard motherboard;
	std::vector<Processor> processors;
};

SysInfo GetSysInfo();

} // namespace hg
