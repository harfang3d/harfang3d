// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>

namespace hg {

enum ScriptParamType { SPT_Null, SPT_Int, SPT_Float, SPT_String, SPT_Bool };

struct ScriptParam {
	ScriptParamType type;

	union {
		bool bv;
		int iv;
		float fv;
	};

	std::string sv;
};

} // namespace hg
