// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/string.h"

/// The base path to the unit test resource folder.
extern std::string unit_resource_path;

/// Return the path to a unit test resource files.
extern std::string GetResPath(const char *name);
/// Return the path to an output file for the currently running test.
extern std::string GetOutPath(const char *name);
