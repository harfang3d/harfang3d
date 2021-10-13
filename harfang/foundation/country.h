// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>

namespace hg {

struct LocaleInfo {
	std::string continent, region, country, fips, iso2, iso3;
	int iso;
	std::string internet;
};

// FIPS 10-4: American National Standard Codes for the Representation of Names of Countries, Dependencies, and Areas of Special Sovereignty for Information Interchange.
const LocaleInfo *GetFIPSCountry(const char *);
// ISO 3166: Two-character.
const LocaleInfo *GetISO2Country(const char *);
// ISO 3166: Three-character.
const LocaleInfo *GetISO3Country(const char *);
// ISO 3166: Three-digit.
const LocaleInfo *GetISOCountry(int);

} // namespace hg
