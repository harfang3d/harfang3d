// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "osx_input_system/osx_input_mouse.h"
#include "osx_input_system/osx_input_system.h"

namespace gs {
namespace input {

void MouseOSX::Update(Window::Handle handle)
{
	// backup current state...
	last_state = state;

	// ...and update it
}

bool MouseOSX::SetValue(InputCode, float) { return false; }

} // input
} // gs
