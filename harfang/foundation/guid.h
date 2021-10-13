// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <array>
#include <string>

namespace hg {

using Guid = std::array<unsigned char, 16>;

Guid MakeGuid(); // RFC4122 compliant
Guid MakeGuid(const std::string &guid);

bool IsValid(const Guid &guid);

bool operator<(const Guid &a, const Guid &b);
bool operator>(const Guid &a, const Guid &b);
bool operator<=(const Guid &a, const Guid &b);
bool operator>=(const Guid &a, const Guid &b);

std::string ToString(const Guid &guid, bool use_separator = true);

} // namespace hg
