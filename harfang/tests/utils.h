// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <stdint.h>

namespace hg {
namespace test {

extern const std::string LoremIpsum;

std::string GetTempDirectoryName();

std::string CreateTempFilepath();

} // namespace test
} // namespace hg
