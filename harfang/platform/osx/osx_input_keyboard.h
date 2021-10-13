// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "input_system/input_keyboard.h"

namespace gs {
namespace input {

/// Keyboard input OSX device
class KeyboardOSX : public Keyboard
{
public:
	void Update(Window::Handle handle) override;
};

} // input
} // gs
