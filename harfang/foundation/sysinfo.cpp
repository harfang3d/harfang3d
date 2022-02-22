// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/sysinfo.h"
#include "foundation/format.h"

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <iphlpapi.h>
#endif

#include <vector>

namespace hg {

#if WIN32

//
static bool dmi_system_uuid(const BYTE *p, short ver, BYTE *out) {
	bool only0xFF = true, only0x00 = true;

	for (int i = 0; i < 16 && (only0x00 || only0xFF); i++) {
		if (p[i] != 0x00)
			only0x00 = false;
		if (p[i] != 0xFF)
			only0xFF = false;
	}

	if (only0xFF || only0x00)
		return false;

	if (ver >= 0x0206) {
		out[0] = p[3];
		out[1] = p[2];
		out[2] = p[1];
		out[3] = p[0];
		out[4] = p[5];
		out[5] = p[4];
		out[6] = p[7];
		out[7] = p[6];
		out[8] = p[8];
		out[9] = p[9];
		out[10] = p[10];
		out[11] = p[11];
		out[12] = p[12];
		out[13] = p[13];
		out[14] = p[14];
		out[15] = p[15];
	} else {
		out[0] = p[0];
		out[1] = p[1];
		out[2] = p[2];
		out[3] = p[3];
		out[4] = p[4];
		out[5] = p[5];
		out[6] = p[6];
		out[7] = p[7];
		out[8] = p[8];
		out[9] = p[9];
		out[10] = p[10];
		out[11] = p[11];
		out[12] = p[12];
		out[13] = p[13];
		out[14] = p[14];
		out[15] = p[15];
	}

	return true;
}

struct RawSMBIOSData {
	BYTE Used20CallingMethod;
	BYTE SMBIOSMajorVersion;
	BYTE SMBIOSMinorVersion;
	BYTE DmiRevision;
	DWORD Length;
	BYTE SMBIOSTableData[];
};

struct RawSMBIOSTable {
	BYTE Type;
	BYTE Length;
	WORD Handle;
};

const char *dmi_string(const RawSMBIOSTable *table, BYTE index) {
	if (index == 0)
		return "Not Specified";

	for (const char *bp = reinterpret_cast<const char *>(table) + table->Length; *bp; bp += strlen(bp) + 1)
		if (--index == 0)
			return bp;

	return "Bad Index";
}

SysInfo GetSysInfo() {
	SysInfo sysinfo;

	const auto data_size = GetSystemFirmwareTable('RSMB', 0, NULL, 0);

	if (data_size == 0)
		return {};

	std::vector<uint8_t> data;
	data.resize(data_size);

	if (GetSystemFirmwareTable('RSMB', 0, data.data(), data_size) != data_size)
		return {};

	RawSMBIOSData *smbios = reinterpret_cast<RawSMBIOSData *>(data.data());

	for (BYTE *p = smbios->SMBIOSTableData; p - smbios->SMBIOSTableData < smbios->Length; p += 2) {
		auto h = (RawSMBIOSTable *)p;

		if (h->Type == 0) {
			sysinfo.bios.vendor = dmi_string(h, p[0x4]);
			sysinfo.bios.version = dmi_string(h, p[0x5]);
			sysinfo.bios.release_date = dmi_string(h, p[0x8]);

			if (p[0x16] != 0xff && p[0x17] != 0xff) {
				sysinfo.bios.EC_version[0] = p[0x16];
				sysinfo.bios.EC_version[1] = p[0x17];
			}
		} else if (h->Type == 1) {
			sysinfo.system.manufacturer = dmi_string(h, p[0x4]);
			sysinfo.system.product = dmi_string(h, p[0x5]);
			sysinfo.system.version = dmi_string(h, p[0x6]);
			sysinfo.system.serial = dmi_string(h, p[0x7]);
			dmi_system_uuid(p + 0x8, smbios->SMBIOSMajorVersion * 0x100 + smbios->SMBIOSMinorVersion, sysinfo.system.uuid);
			sysinfo.system.sku = dmi_string(h, p[0x19]);
			sysinfo.system.family = dmi_string(h, p[0x1a]);
		} else if (h->Type == 2) {
			sysinfo.motherboard.manufacturer = dmi_string(h, p[0x4]);
			sysinfo.motherboard.product = dmi_string(h, p[0x5]);
			sysinfo.motherboard.version = dmi_string(h, p[0x6]);
			sysinfo.motherboard.serial_number = dmi_string(h, p[0x7]);
			sysinfo.motherboard.asset_tag = p[0x8];
			sysinfo.motherboard.feature_flags = p[0x9];
			sysinfo.motherboard.location_in_chassis = dmi_string(h, p[0xa]);
		} else if (h->Type == 3) {
			Processor processor;
			processor.socket = dmi_string(h, p[0x4]);
			processor.type = p[0x5];
			processor.family = p[0x6];
			processor.manufacturer = dmi_string(h, p[0x7]);
			processor.id = *reinterpret_cast<uint64_t *>(&p[0x8]);
			processor.version = dmi_string(h, p[0x10]);
			processor.voltage = p[0x11];
			processor.external_clock = *reinterpret_cast<uint16_t *>(&p[0x12]);
			processor.max_speed = *reinterpret_cast<uint16_t *>(&p[0x14]);
			processor.current_speed = *reinterpret_cast<uint16_t *>(&p[0x16]);
			processor.status = p[0x18];
			processor.serial_number = dmi_string(h, p[0x20]);
			processor.asset_tag = dmi_string(h, p[0x21]);
			processor.part_number = dmi_string(h, p[0x22]);
			processor.core_count = p[0x23];
			processor.core_enabled = p[0x24];
			processor.thread_count = p[0x25];
			processor.core_count_2 = *reinterpret_cast<uint16_t *>(&p[0x2a]);
			processor.core_enabled_2 = *reinterpret_cast<uint16_t *>(&p[0x2c]);
			processor.thread_count_2 = *reinterpret_cast<uint16_t *>(&p[0x2e]);
			sysinfo.processors.push_back(processor);
		}

		p += h->Length;
		while ((*(WORD *)p) != 0)
			p++;
	}

	return sysinfo;
}

#else

SysInfo GetSysInfo() { return {}; }

#endif

} // namespace hg
