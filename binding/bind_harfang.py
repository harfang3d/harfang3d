# This is the Harfang Fabgen script used to generate bindings for the CPython and Lua languages.

import lang.cpython

import lib.std
import lib.stl
import lib
import copy


def check_bool_rval_lambda(gen, msg):
	return lambda rvals, ctx: 'if (!%s) {\n%s}\n' % (rvals[0], gen.proxy_call_error(msg, ctx))


def route_lambda(name):
	return lambda args: '%s(%s);' % (name, ', '.join(args))


def bind_std_vector(gen, T_conv, bound_name=None):
	if gen.get_language() == 'CPython':
		PySequence_T_type = 'PySequenceOf%s' % T_conv.bound_name
		gen.bind_type(lib.cpython.stl.PySequenceToStdVectorConverter(PySequence_T_type, T_conv))
	elif gen.get_language() == 'Lua':
		LuaTable_T_type = 'LuaTableOf%s' % T_conv.bound_name
		gen.bind_type(lib.lua.stl.LuaTableToStdVectorConverter(LuaTable_T_type, T_conv))
	elif gen.get_language() == 'Go':
		GoTable_T_type = 'GoSliceOf%s' % T_conv.bound_name
		gen.bind_type(lib.go.stl.GoSliceToStdVectorConverter(GoTable_T_type, T_conv))

	if bound_name is None:
		bound_name = '%sList' % T_conv.bound_name

	conv = gen.begin_class('std::vector<%s>' % T_conv.ctype, bound_name=bound_name, features={'sequence': lib.std.VectorSequenceFeature(T_conv)})
	if gen.get_language() == 'CPython':
		gen.bind_constructor(conv, ['?%s sequence' % PySequence_T_type])
	elif gen.get_language() == 'Lua':
		gen.bind_constructor(conv, ['?%s sequence' % LuaTable_T_type])
	elif gen.get_language() == 'Go':
		gen.bind_constructor(conv, ['?%s sequence' % GoTable_T_type])

	def validate_std_vector_at_idx(gen, var, ctx):
		out = 'if ((_self->size() == 0) || (%s >= _self->size())) {\n' % var
		out += gen.proxy_call_error("Invalid index", ctx)
		out += '}\n'
		return out

	gen.bind_method(conv, 'clear', 'void', [])
	gen.bind_method(conv, 'reserve', 'void', ['size_t size'])
	gen.bind_method(conv, 'push_back', 'void', ['%s v' % T_conv.ctype])
	gen.bind_method(conv, 'size', 'size_t', [])
	gen.bind_method(conv, 'at', repr(T_conv.ctype), ['size_t idx'], features={'validate_arg_in': [validate_std_vector_at_idx]})

	gen.end_class(conv)
	return conv


def expand_std_vector_proto(gen, protos, is_constructor_proto=False):
	prefix = {
		'CPython' : 'PySequenceOf',
		'Lua' : 'LuaTableOf',
		'Go' : 'GoSliceOf'
	}
	name_prefix = {
		'CPython' : 'SequenceOf',
		'Lua' : 'TableOf',
		'Go' : 'SliceOf'
	}

	if gen.get_language() not in prefix:
		return protos

	expanded_protos = []
	for proto in protos:
		expanded = []
		add_expanded = False

		start = 0 if is_constructor_proto else 1
		args = proto[start]
		blacklist = []

		feats = proto[start+1]
		if 'arg_out' in feats:
			blacklist = blacklist + feats['arg_out']

		for arg in args:
			is_optional = arg[0:1] == '?'
			if is_optional:
				arg = arg[1:]

			carg = gen.parse_named_ctype(arg)
			conv = gen.select_ctype_conv(carg.ctype)

			has_sequence = ('sequence' in conv._features) if conv is not None else False
			is_blacklisted = carg.name.naked_name() in blacklist

			if has_sequence and not is_blacklisted:
				add_expanded = True
				seq = conv._features['sequence']
				arg = '%s%s %s_%s' % (prefix[gen.get_language()], seq.wrapped_conv.bound_name, name_prefix[gen.get_language()], carg.name)

			if is_optional:
				arg = '?' + arg

			expanded.append(arg)

		if add_expanded:
			if is_constructor_proto:
				expanded_protos.append((expanded, proto[1]))
			else:
				expanded_protos.append((proto[0], expanded, copy.deepcopy(proto[2])))

	return protos + expanded_protos


def bind_task_system(gen):
	gen.add_include('foundation/task_system.h')

	gen.insert_binding_code('''
static void _CreateWorkers() { hg::g_task_system.get().create_workers(); }
static void _DeleteWorkers() { hg::g_task_system.get().delete_workers(); }
''')

	gen.bind_function('CreateWorkers', 'void', {'route': route_lambda('_CreateWorkers')})
	gen.bind_function('DeleteWorkers', 'void', {'route': route_lambda('_DeleteWorkers')})


def bind_log(gen):
	gen.add_include('foundation/log.h')

	gen.bind_named_enum('hg::LogLevel', ['LL_Normal', 'LL_Warning', 'LL_Error', 'LL_Debug', 'LL_All'])

	gen.bind_function('hg::set_log_level', 'void', ['hg::LogLevel log_level'], bound_name='SetLogLevel')
	gen.bind_function('hg::set_log_detailed', 'void', ['bool is_detailed'], bound_name='SetLogDetailed')

	gen.bind_function('hg::log', 'void', ['const char *msg', '?const char *details'], bound_name='Log')
	gen.bind_function('hg::warn', 'void', ['const char *msg', '?const char *details'], bound_name='Warn')
	gen.bind_function('hg::error', 'void', ['const char *msg', '?const char *details'], bound_name='Error')
	gen.bind_function('hg::debug', 'void', ['const char *msg', '?const char *details'], bound_name='Debug')


def bind_time(gen):
	gen.add_include('foundation/time.h')

	gen.typedef('hg::time_ns', 'int64_t')

	gen.bind_function('hg::time_to_sec_f', 'float', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_ms_f', 'float', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_us_f', 'float', ['hg::time_ns t'])

	gen.bind_function('hg::time_to_day', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_hour', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_min', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_sec', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_ms', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_us', 'int64_t', ['hg::time_ns t'])
	gen.bind_function('hg::time_to_ns', 'int64_t', ['hg::time_ns t'])

	gen.bind_function('hg::time_from_sec_f', 'hg::time_ns', ['float sec'])
	gen.bind_function('hg::time_from_ms_f', 'hg::time_ns', ['float ms'])
	gen.bind_function('hg::time_from_us_f', 'hg::time_ns', ['float us'])

	gen.bind_function('hg::time_from_day', 'hg::time_ns', ['int64_t day'])
	gen.bind_function('hg::time_from_hour', 'hg::time_ns', ['int64_t hour'])
	gen.bind_function('hg::time_from_min', 'hg::time_ns', ['int64_t min'])
	gen.bind_function('hg::time_from_sec', 'hg::time_ns', ['int64_t sec'])
	gen.bind_function('hg::time_from_ms', 'hg::time_ns', ['int64_t ms'])
	gen.bind_function('hg::time_from_us', 'hg::time_ns', ['int64_t us'])
	gen.bind_function('hg::time_from_ns', 'hg::time_ns', ['int64_t ns'])

	gen.bind_function('hg::time_now', 'hg::time_ns', [])

	gen.add_include('foundation/time_to_string.h')

	gen.bind_function('hg::time_to_string', 'std::string', ['hg::time_ns t'])


def bind_clock(gen):
	gen.add_include('foundation/clock.h')

	gen.bind_function('hg::reset_clock', 'void', [], bound_name='ResetClock')

	gen.bind_function('hg::tick_clock', 'hg::time_ns', [], bound_name='TickClock')
	gen.bind_function('hg::get_clock', 'hg::time_ns', [], bound_name='GetClock')
	gen.bind_function('hg::get_clock_dt', 'hg::time_ns', [], bound_name='GetClockDt')

	gen.bind_function('hg::skip_clock', 'void', [], bound_name='SkipClock')


def bind_input(gen):
	gen.add_include('platform/input_system.h')

	gen.bind_function('hg::InputInit', 'void', [])
	gen.bind_function('hg::InputShutdown', 'void', [])

	# mouse
	gen.bind_named_enum('hg::MouseButton', ['MB_0', 'MB_1', 'MB_2', 'MB_3', 'MB_4', 'MB_5', 'MB_6', 'MB_7'])

	gen.insert_binding_code('''
static int _MouseState_X(hg::MouseState *s) { return s->x; }
static int _MouseState_Y(hg::MouseState *s) { return s->y; }
static bool _MouseState_Button(hg::MouseState *s, int btn) { return s->button[btn]; }
static int _MouseState_Wheel(hg::MouseState *s) { return s->wheel; }
static int _MouseState_HWheel(hg::MouseState *s) { return s->hwheel; }
''')

	mouse_state = gen.begin_class('hg::MouseState')
	gen.bind_method(mouse_state, 'X', 'int', [], {'route': route_lambda('_MouseState_X')})
	gen.bind_method(mouse_state, 'Y', 'int', [], {'route': route_lambda('_MouseState_Y')})
	gen.bind_method(mouse_state, 'Button', 'bool', ['hg::MouseButton btn'], {'route': route_lambda('_MouseState_Button')})
	gen.bind_method(mouse_state, 'Wheel', 'int', [], {'route': route_lambda('_MouseState_Wheel')})
	gen.bind_method(mouse_state, 'HWheel', 'int', [], {'route': route_lambda('_MouseState_HWheel')})
	gen.end_class(mouse_state)

	gen.bind_function('hg::ReadMouse', 'hg::MouseState', ['?const char *name'])
	gen.bind_function('hg::GetMouseNames', 'std::vector<std::string>', [])

	mouse = gen.begin_class('hg::Mouse')
	gen.bind_constructor(mouse, ['?const char *name'])

	gen.bind_method(mouse, 'X', 'int', [])
	gen.bind_method(mouse, 'Y', 'int', [])
	gen.bind_method(mouse, 'DtX', 'int', [])
	gen.bind_method(mouse, 'DtY', 'int', [])

	gen.bind_method(mouse, 'Down', 'bool', ['int button'])
	gen.bind_method(mouse, 'Pressed', 'bool', ['int button'])
	gen.bind_method(mouse, 'Released', 'bool', ['int button'])

	gen.bind_method(mouse, 'Wheel', 'int', [])
	gen.bind_method(mouse, 'HWheel', 'int', [])

	gen.bind_method(mouse, 'Update', 'void', [])

	gen.bind_method(mouse, 'GetState', 'hg::MouseState', []) 
	gen.bind_method(mouse, 'GetOldState', 'hg::MouseState', []) 

	gen.end_class(mouse)

	# keyboard
	gen.bind_named_enum('hg::Key', [
		'K_LShift', 'K_RShift', 'K_LCtrl', 'K_RCtrl', 'K_LAlt', 'K_RAlt', 'K_LWin', 'K_RWin',
		'K_Tab', 'K_CapsLock', 'K_Space', 'K_Backspace', 'K_Insert', 'K_Suppr', 'K_Home', 'K_End', 'K_PageUp', 'K_PageDown',
		'K_Up', 'K_Down', 'K_Left', 'K_Right',
		'K_Escape',
		'K_F1', 'K_F2', 'K_F3', 'K_F4', 'K_F5', 'K_F6', 'K_F7', 'K_F8', 'K_F9', 'K_F10', 'K_F11', 'K_F12',
		'K_PrintScreen', 'K_ScrollLock', 'K_Pause', 'K_NumLock', 'K_Return',
		'K_0', 'K_1', 'K_2', 'K_3', 'K_4', 'K_5', 'K_6', 'K_7', 'K_8', 'K_9',
		'K_Numpad0', 'K_Numpad1', 'K_Numpad2', 'K_Numpad3', 'K_Numpad4', 'K_Numpad5', 'K_Numpad6', 'K_Numpad7', 'K_Numpad8', 'K_Numpad9',
		'K_Add', 'K_Sub', 'K_Mul', 'K_Div', 'K_Enter',
		'K_A', 'K_B', 'K_C', 'K_D', 'K_E', 'K_F', 'K_G', 'K_H', 'K_I', 'K_J', 'K_K', 'K_L', 'K_M', 'K_N', 'K_O', 'K_P', 'K_Q', 'K_R', 'K_S', 'K_T', 'K_U', 'K_V', 'K_W', 'K_X', 'K_Y', 'K_Z',
		'K_Plus', 'K_Comma', 'K_Minus', 'K_Period',
		'K_OEM1', 'K_OEM2', 'K_OEM3', 'K_OEM4', 'K_OEM5', 'K_OEM6', 'K_OEM7', 'K_OEM8',
		'K_BrowserBack', 'K_BrowserForward', 'K_BrowserRefresh', 'K_BrowserStop', 'K_BrowserSearch', 'K_BrowserFavorites', 'K_BrowserHome', 'K_VolumeMute', 'K_VolumeDown', 'K_VolumeUp',
		'K_MediaNextTrack', 'K_MediaPrevTrack', 'K_MediaStop', 'K_MediaPlayPause', 'K_LaunchMail', 'K_LaunchMediaSelect', 'K_LaunchApp1', 'K_LaunchApp2',
		'K_Last'
	])

	gen.insert_binding_code('''
static bool _KeyboardState_Key(hg::KeyboardState *s, hg::Key key) { return s->key[key]; }
''')

	keyboard_state = gen.begin_class('hg::KeyboardState')
	gen.bind_method(keyboard_state, 'Key', 'bool', ['hg::Key key'], {'route': route_lambda('_KeyboardState_Key')})
	gen.end_class(keyboard_state)

	gen.bind_function('hg::ReadKeyboard', 'hg::KeyboardState', ['?const char *name'])
	gen.bind_function('hg::GetKeyName', 'const char *', ['hg::Key key', '?const char *name'])
	gen.bind_function('hg::GetKeyboardNames', 'std::vector<std::string>', [])

	keyboard = gen.begin_class('hg::Keyboard')
	gen.bind_constructor(keyboard, ['?const char *name'])

	gen.bind_method(keyboard, 'Down', 'bool', ['hg::Key key'])
	gen.bind_method(keyboard, 'Pressed', 'bool', ['hg::Key key'])
	gen.bind_method(keyboard, 'Released', 'bool', ['hg::Key key'])

	gen.bind_method(keyboard, 'Update', 'void', [])

	gen.bind_method(keyboard, 'GetState', 'hg::KeyboardState', []) 
	gen.bind_method(keyboard, 'GetOldState', 'hg::KeyboardState', []) 

	gen.end_class(keyboard)

	bind_signal_T(gen, 'TextInputSignal', 'void', ['const char*'], 'TextInputCallback')
	gen.bind_variable('const hg::Signal<void(const char *)> hg::on_text_input', bound_name='OnTextInput')

	# gamepad
	gen.bind_named_enum('hg::GamepadAxes', ['GA_LeftX', 'GA_LeftY', 'GA_RightX', 'GA_RightY', 'GA_LeftTrigger', 'GA_RightTrigger', 'GA_Count'])
	gen.bind_named_enum('hg::GamepadButton', [
		'GB_ButtonA', 'GB_ButtonB', 'GB_ButtonX', 'GB_ButtonY', 'GB_LeftBumper', 'GB_RightBumper', 'GB_Back', 'GB_Start',
		'GB_Guide', 'GB_LeftThumb', 'GB_RightThumb', 'GB_DPadUp', 'GB_DPadRight', 'GB_DPadDown', 'GB_DPadLeft', 'GB_Count'
	])

	gen.insert_binding_code('''
static bool _GamepadState_IsConnected(hg::GamepadState *s) { return s->connected; }
static float _GamepadState_Axes(hg::GamepadState *s, hg::GamepadAxes idx) { return idx < 6 ? s->axes[idx] : 0.f; }
static bool _GamepadState_Button(hg::GamepadState *s, hg::GamepadButton btn) { return btn < 15 ? s->button[btn] : false; }
''')

	gamepad_state = gen.begin_class('hg::GamepadState')
	gen.bind_method(gamepad_state, 'IsConnected', 'bool', [], {'route': route_lambda('_GamepadState_IsConnected')})
	gen.bind_method(gamepad_state, 'Axes', 'float', ['hg::GamepadAxes idx'], {'route': route_lambda('_GamepadState_Axes')})
	gen.bind_method(gamepad_state, 'Button', 'bool', ['hg::GamepadButton btn'], {'route': route_lambda('_GamepadState_Button')})
	gen.end_class(gamepad_state)

	gen.bind_function('hg::ReadGamepad', 'hg::GamepadState', ['?const char *name'])
	gen.bind_function('hg::GetGamepadNames', 'std::vector<std::string>', [])

	gamepad = gen.begin_class('hg::Gamepad')
	gen.bind_constructor(gamepad, ['?const char *name'])

	gen.bind_method(gamepad, 'IsConnected', 'bool', [])
	gen.bind_method(gamepad, 'Connected', 'bool', [])
	gen.bind_method(gamepad, 'Disconnected', 'bool', [])

	gen.bind_method(gamepad, 'Axes', 'float', ['hg::GamepadAxes axis'])
	gen.bind_method(gamepad, 'DtAxes', 'float', ['hg::GamepadAxes axis'])

	gen.bind_method(gamepad, 'Down', 'bool', ['hg::GamepadButton btn'])
	gen.bind_method(gamepad, 'Pressed', 'bool', ['hg::GamepadButton btn'])
	gen.bind_method(gamepad, 'Released', 'bool', ['hg::GamepadButton btn'])

	gen.bind_method(gamepad, 'Update', 'void', [])

	gen.end_class(gamepad)
	
	# Joystick

	gen.insert_binding_code('''
static bool _JoystickState_IsConnected(hg::JoystickState *s) { return s->connected; }
static float _JoystickState_Axes(hg::JoystickState *s, int idx) { return idx < s->nbAxes ? s->axes[idx] : 0.f; }
static bool _JoystickState_Button(hg::JoystickState *s, int btn) { return btn < s->nbButtons ? s->buttons[btn] : false; }
''')

	Joystick_state = gen.begin_class('hg::JoystickState')
	gen.bind_method(Joystick_state, 'IsConnected', 'bool', [], {'route': route_lambda('_JoystickState_IsConnected')})
	gen.bind_method(Joystick_state, 'Axes', 'float', ['int idx'], {'route': route_lambda('_JoystickState_Axes')})
	gen.bind_method(Joystick_state, 'Button', 'bool', ['int btn'], {'route': route_lambda('_JoystickState_Button')})
	gen.end_class(Joystick_state)

	gen.bind_function('hg::ReadJoystick', 'hg::JoystickState', ['?const char *name'])
	gen.bind_function('hg::GetJoystickNames', 'std::vector<std::string>', [])
	gen.bind_function('hg::GetJoystickDeviceNames', 'std::vector<std::string>', [])

	Joystick = gen.begin_class('hg::Joystick')
	gen.bind_constructor(Joystick, ['?const char *name'])

	gen.bind_method(Joystick, 'IsConnected', 'bool', [])
	gen.bind_method(Joystick, 'Connected', 'bool', [])
	gen.bind_method(Joystick, 'Disconnected', 'bool', [])
	
	gen.bind_method(Joystick, 'AxesCount', 'int', [])
	gen.bind_method(Joystick, 'Axes', 'float', ['int axis'])
	gen.bind_method(Joystick, 'DtAxes', 'float', ['int axis'])
	
	gen.bind_method(Joystick, 'ButtonsCount', 'int', [])
	gen.bind_method(Joystick, 'Down', 'bool', ['int btn'])
	gen.bind_method(Joystick, 'Pressed', 'bool', ['int btn'])
	gen.bind_method(Joystick, 'Released', 'bool', ['int btn'])

	gen.bind_method(Joystick, 'Update', 'void', [])
	gen.bind_method(Joystick, 'GetDeviceName', 'std::string', [])	

	gen.end_class(Joystick)

	# VR controller
	gen.bind_named_enum('hg::VRControllerButton', [
		'VRCB_DPad_Up', 'VRCB_DPad_Down', 'VRCB_DPad_Left', 'VRCB_DPad_Right', 'VRCB_System', 'VRCB_AppMenu', 'VRCB_Grip', 'VRCB_A',
		'VRCB_ProximitySensor', 'VRCB_Axis0', 'VRCB_Axis1', 'VRCB_Axis2', 'VRCB_Axis3', 'VRCB_Axis4', 'VRCB_Count'
	])

	gen.insert_binding_code('''
static bool _VRControllerState_IsConnected(hg::VRControllerState *s) { return s->connected; }
static hg::Mat4 _VRControllerState_World(hg::VRControllerState *s) { return s->world; }
static bool _VRControllerState_Pressed(hg::VRControllerState *s, hg::VRControllerButton btn) { return s->pressed[btn]; }
static bool _VRControllerState_Touched(hg::VRControllerState *s, hg::VRControllerButton btn) { return s->touched[btn]; }
static hg::tVec2<float> _VRControllerState_Surface(hg::VRControllerState *s, int idx) { return idx < 5 ? s->surface[idx] : hg::tVec2<float>{}; }
''')

	vr_controller_state = gen.begin_class('hg::VRControllerState')
	gen.bind_method(vr_controller_state, 'IsConnected', 'bool', [], {'route': route_lambda('_VRControllerState_IsConnected')})
	gen.bind_method(vr_controller_state, 'World', 'hg::Mat4', [], {'route': route_lambda('_VRControllerState_World')})
	gen.bind_method(vr_controller_state, 'Pressed', 'bool', ['hg::VRControllerButton btn'], {'route': route_lambda('_VRControllerState_Pressed')})
	gen.bind_method(vr_controller_state, 'Touched', 'bool', ['hg::VRControllerButton btn'], {'route': route_lambda('_VRControllerState_Touched')})
	gen.bind_method(vr_controller_state, 'Surface', 'hg::tVec2<float>', ['int idx'], {'route': route_lambda('_VRControllerState_Surface')})
	gen.end_class(vr_controller_state)

	gen.bind_function('hg::ReadVRController', 'hg::VRControllerState', ['?const char *name'])
	gen.bind_function('hg::SendVRControllerHapticPulse', 'void', ['hg::time_ns duration', '?const char *name'])

	gen.bind_function('hg::GetVRControllerNames', 'std::vector<std::string>', [])

	vr_controller = gen.begin_class('hg::VRController')
	gen.bind_constructor(vr_controller, ['?const char *name'])

	gen.bind_method(vr_controller, 'IsConnected', 'bool', [])
	gen.bind_method(vr_controller, 'Connected', 'bool', [])
	gen.bind_method(vr_controller, 'Disconnected', 'bool', [])

	gen.bind_method(vr_controller, 'World', 'hg::Mat4', [])

	gen.bind_method(vr_controller, 'Down', 'bool', ['hg::VRControllerButton btn'])
	gen.bind_method(vr_controller, 'Pressed', 'bool', ['hg::VRControllerButton btn'])
	gen.bind_method(vr_controller, 'Released', 'bool', ['hg::VRControllerButton btn'])

	gen.bind_method(vr_controller, 'Touch', 'bool', ['hg::VRControllerButton btn'])
	gen.bind_method(vr_controller, 'TouchStart', 'bool', ['hg::VRControllerButton btn'])
	gen.bind_method(vr_controller, 'TouchEnd', 'bool', ['hg::VRControllerButton btn'])

	gen.bind_method(vr_controller, 'Surface', 'hg::tVec2<float>', ['int idx'])
	gen.bind_method(vr_controller, 'DtSurface', 'hg::tVec2<float>', ['int idx'])

	gen.bind_method(vr_controller, 'SendHapticPulse', 'void', ['hg::time_ns duration'])

	gen.bind_method(vr_controller, 'Update', 'void', [])

	gen.end_class(vr_controller)

	# VR generic tracker
	gen.insert_binding_code('''
static bool _VRGenericTrackerState_IsConnected(hg::VRGenericTrackerState *s) { return s->connected; }
static hg::Mat4 _VRGenericTrackerState_World(hg::VRGenericTrackerState *s) { return s->world; }
''')

	vr_generic_tracker_state = gen.begin_class('hg::VRGenericTrackerState')
	gen.bind_method(vr_generic_tracker_state, 'IsConnected', 'bool', [], {'route': route_lambda('_VRGenericTrackerState_IsConnected')})
	gen.bind_method(vr_generic_tracker_state, 'World', 'hg::Mat4', [], {'route': route_lambda('_VRGenericTrackerState_World')})
	gen.end_class(vr_generic_tracker_state)

	gen.bind_function('hg::ReadVRGenericTracker', 'hg::VRGenericTrackerState', ['?const char *name'])

	gen.bind_function('hg::GetVRGenericTrackerNames', 'std::vector<std::string>', [])

	vr_generic_tracker = gen.begin_class('hg::VRGenericTracker')
	gen.bind_constructor(vr_generic_tracker, ['?const char *name'])

	gen.bind_method(vr_generic_tracker, 'IsConnected', 'bool', [])
	gen.bind_method(vr_generic_tracker, 'World', 'hg::Mat4', [])

	gen.bind_method(vr_generic_tracker, 'Update', 'void', [])

	gen.end_class(vr_generic_tracker)


def bind_openvr(gen):
	gen.add_include('engine/openvr_api.h')

	gen.bind_function('hg::OpenVRInit', 'bool', [])
	gen.bind_function('hg::OpenVRShutdown', 'void', [])

	openvr_eye = gen.begin_class('hg::OpenVREye')
	gen.bind_members(openvr_eye, ['hg::Mat4 offset', 'hg::Mat44 projection'])
	gen.end_class(openvr_eye)

	gen.insert_binding_code('''
static bgfx::FrameBufferHandle _OpenVREyeFrameBuffer_GetHandle(hg::OpenVREyeFrameBuffer *s) { return s->fb; }
''')

	openvr_eye_fb = gen.begin_class('hg::OpenVREyeFrameBuffer')
	gen.bind_method(openvr_eye_fb, 'GetHandle', 'bgfx::FrameBufferHandle', [], {'route': route_lambda('_OpenVREyeFrameBuffer_GetHandle')})
	gen.end_class(openvr_eye_fb)

	gen.bind_named_enum('hg::OpenVRAA', ['OVRAA_None', 'OVRAA_MSAA2x', 'OVRAA_MSAA4x', 'OVRAA_MSAA8x', 'OVRAA_MSAA16x'])

	gen.bind_function('hg::OpenVRCreateEyeFrameBuffer', 'hg::OpenVREyeFrameBuffer', ['?hg::OpenVRAA aa'])
	gen.bind_function('hg::OpenVRDestroyEyeFrameBuffer', 'void', ['hg::OpenVREyeFrameBuffer &eye_fb'])

	openvr_state = gen.begin_class('hg::OpenVRState')
	gen.bind_members(openvr_state, ['hg::Mat4 body', 'hg::Mat4 head', 'hg::Mat4 inv_head', 'hg::OpenVREye left', 'hg::OpenVREye right', 'uint32_t width', 'uint32_t height'])
	gen.end_class(openvr_state)

	gen.bind_function('hg::OpenVRGetState', 'hg::OpenVRState', ['const hg::Mat4 &body', 'float znear', 'float zfar'])
	gen.bind_function('hg::OpenVRStateToViewState', 'void', ['const hg::OpenVRState &state', 'hg::ViewState &left', 'hg::ViewState &right'], {'arg_out': ['left', 'right']})

	gen.bind_function('hg::OpenVRSubmitFrame', 'void', ['const hg::OpenVREyeFrameBuffer &left', 'const hg::OpenVREyeFrameBuffer &right'])
	gen.bind_function('hg::OpenVRPostPresentHandoff', 'void', [])

	gen.bind_function('hg::OpenVRGetColorTexture', 'hg::Texture', ['const hg::OpenVREyeFrameBuffer &eye'])
	gen.bind_function('hg::OpenVRGetDepthTexture', 'hg::Texture', ['const hg::OpenVREyeFrameBuffer &eye'])

	gen.bind_function('hg::OpenVRGetFrameBufferSize', 'hg::tVec2<int>', [])


def bind_sranipal(gen):
	gen.add_include('engine/sranipal_api.h')

	gen.bind_function('hg::SRanipalInit', 'bool', [])
	gen.bind_function('hg::SRanipalShutdown', 'void', [])
	gen.bind_function('hg::SRanipalLaunchEyeCalibration', 'void', [])
	gen.bind_function('hg::SRanipalIsViveProEye', 'bool', [])
	
	sranipal_eye_state = gen.begin_class('hg::SRanipalEyeState')
	gen.bind_members(sranipal_eye_state, ['bool pupil_diameter_valid', 'hg::Vec3 gaze_origin_mm', 'hg::Vec3 gaze_direction_normalized', 'float pupil_diameter_mm', 'float eye_openness'])
	gen.end_class(sranipal_eye_state)

	sranipal_state = gen.begin_class('hg::SRanipalState')
	gen.bind_members(sranipal_state, ['hg::SRanipalEyeState right_eye', 'hg::SRanipalEyeState left_eye'])
	gen.end_class(sranipal_state)
	gen.bind_function('hg::SRanipalGetState', 'hg::SRanipalState', [])


def bind_recast_detour(gen):
	gen.add_include('engine/recast_detour.h')
	gen.add_include('DetourNavMesh.h')

	# dtNavMesh	
	nav_mesh = gen.bind_ptr('dtNavMesh *', bound_name='NavMesh')

	# dtNavMeshQuery
	nav_mesh_query = gen.bind_ptr('dtNavMeshQuery *', bound_name='NavMeshQuery')
	gen.bind_function('hg::CreateNavMeshQuery', 'dtNavMeshQuery *', ['const dtNavMesh *mesh'])
	
	gen.bind_function('hg::LoadNavMeshFromFile', 'dtNavMesh *', ['const char *path'])
	gen.bind_function('hg::LoadNavMeshFromAssets', 'dtNavMesh *', ['const char *name'])
	gen.bind_function('hg::DestroyNavMesh', 'void', ['dtNavMesh *mesh'])
	gen.bind_function('hg::DestroyNavMeshQuery', 'void', ['dtNavMeshQuery *mesh'])
	gen.bind_function('hg::DrawNavMesh', 'void', ['const dtNavMesh *mesh', 'bgfx::ViewId view_id', 'const bgfx::VertexLayout &vtx_decl', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', 'hg::RenderState state'])
	gen.bind_function('hg::FindNavigationPathTo', 'std::vector<hg::Vec3>', ['const dtNavMeshQuery *query', 'const hg::Vec3 &from', 'const hg::Vec3 &to'])

	# DetourCrowd	
	gen.add_include('DetourCrowd.h')
	
	dt_crowd_agent_params = gen.begin_class('dtCrowdAgentParams', bound_name='CrowdAgentParams')
	gen.end_class(dt_crowd_agent_params)

	gen.insert_binding_code('''
// FIXME bind type and flags once they are bound to a proper type
static dtCrowdAgentParams _ConfigureCrowdAgent(float radius, float height, float max_acceleration, float max_speed, float collision_query_range, float path_optimization_range, float separation_weight) { //, unsigned char update_flags, unsigned char obstacle_avoidance_type, unsigned char query_filter_type)
	dtCrowdAgentParams params;

	params.radius = radius;
	params.height = height;
	params.maxAcceleration = max_acceleration;
	params.maxSpeed = max_speed;
	params.collisionQueryRange = collision_query_range;
	params.pathOptimizationRange = path_optimization_range;
	params.separationWeight = separation_weight;

	return params;
}
''')

	gen.bind_function('ConfigureCrowdAgent', 'dtCrowdAgentParams', ['float radius', 'float height', 'float max_acceleration', 'float max_speed', 'float collision_query_range', 'float path_optimization_range', 'float separation_weight'], {'route': route_lambda('_ConfigureCrowdAgent')})

	# dtCrowdAgent
	gen.insert_binding_code('''
static void _dt_crowd_agent_SetPos(dtCrowdAgent *dt_crowd_agent, const hg::Vec3 &p) {
	dt_crowd_agent->npos[0] = p.x; dt_crowd_agent->npos[1] = p.y; dt_crowd_agent->npos[2] = p.z;
	dt_crowd_agent->vel[0] = 0.f; dt_crowd_agent->vel[1] = 0.f; dt_crowd_agent->vel[2] = 0.f;
	dt_crowd_agent->dvel[0] = 0.f; dt_crowd_agent->dvel[1] = 0.f; dt_crowd_agent->dvel[2] = 0.f;
}

static hg::Vec3 _dt_crowd_agent_GetPos(dtCrowdAgent *dt_crowd_agent) { return {dt_crowd_agent->npos[0], dt_crowd_agent->npos[1], dt_crowd_agent->npos[2]}; }
static hg::Vec3 _dt_crowd_agent_GetVel(dtCrowdAgent *dt_crowd_agent) { return {dt_crowd_agent->vel[0], dt_crowd_agent->vel[1], dt_crowd_agent->vel[2]}; }
''')

	dt_crowd_agent = gen.begin_class('dtCrowdAgent', bound_name='CrowdAgent', noncopyable=True)
	gen.bind_method(dt_crowd_agent, 'SetPos', 'void', ['const hg::Vec3 &pos'], {'route': route_lambda('_dt_crowd_agent_SetPos')})
	gen.bind_method(dt_crowd_agent, 'GetPos', 'hg::Vec3', [], {'route': route_lambda('_dt_crowd_agent_GetPos')})
	gen.bind_method(dt_crowd_agent, 'GetVel', 'hg::Vec3', [], {'route': route_lambda('_dt_crowd_agent_GetVel')})
	gen.end_class(dt_crowd_agent)
	
	# dtCrowd
	gen.insert_binding_code('''
static bool _dtCrowd_init(dtCrowd *dt_crowd, const int maxAgent, const float maxAgentRadius, dtNavMesh *nav) { return dt_crowd->init(maxAgent, maxAgentRadius, nav); }
static void _dtCrowd_update(dtCrowd *dt_crowd, const float dt) { dt_crowd->update(dt, nullptr); }

static int _dtCrowd_addAgent(dtCrowd *dt_crowd, const hg::Vec3 &pos, const dtCrowdAgentParams *params) {
	float p[3] = {pos.x, pos.y, pos.z};
	return dt_crowd->addAgent(p, params);
}

static bool _dtCrowd_requestMoveTarget(dtCrowd *dt_crowd, const int idx, const hg::Vec3 &pos, dtNavMeshQuery *navmesh_query) {
	dtQueryFilter filter;
	filter.setIncludeFlags(0xffff);
	filter.setExcludeFlags(0);

	static const float m_polyPickExt[3] = {2, 4, 2}; // XYZ bounding box to pick nearby polygon
	dtPolyRef polyRef = 0;
	float p[3] = {pos.x, pos.y, pos.z};
	navmesh_query->findNearestPoly(p, m_polyPickExt, &filter, &polyRef, 0);

	return dt_crowd->requestMoveTarget(idx, polyRef, p);
}
''')

	dt_crowd = gen.begin_class('dtCrowd', bound_name='Crowd', noncopyable=True)
	gen.bind_constructor(dt_crowd, [])
	gen.bind_method(dt_crowd, 'init', 'bool', ['const int max_agent', 'const float maxAgentRadius', 'dtNavMesh *mesh'], {'route': route_lambda('_dtCrowd_init')}, bound_name='Init')
	gen.bind_method(dt_crowd, 'update', 'void', ['const float dt'], {'route': route_lambda('_dtCrowd_update')}, bound_name='Update')
	gen.bind_method(dt_crowd, 'addAgent', 'int', ['const hg::Vec3 &pos', 'const dtCrowdAgentParams *params'], {'route': route_lambda('_dtCrowd_addAgent')}, bound_name='AddAgent')
	gen.bind_method(dt_crowd, 'removeAgent', 'void', ['const int idx'], bound_name='RemoveAgent')
	gen.bind_method(dt_crowd, 'getEditableAgent', 'dtCrowdAgent *', ['const int idx'], bound_name='GetEditableAgent')	
	gen.bind_method(dt_crowd, 'requestMoveTarget', 'bool', ['const int idx', 'const hg::Vec3 &pos', 'dtNavMeshQuery *query'], {'route': route_lambda('_dtCrowd_requestMoveTarget')}, bound_name='RequestMoveTarget')
	gen.end_class(dt_crowd)


def bind_platform(gen):
	gen.add_include('platform/platform.h')
	
	# hg::FileFilter
	file_filter = gen.begin_class('hg::FileFilter')
	gen.bind_members(file_filter, ['std::string name', 'std::string pattern'])
	gen.end_class(file_filter)

	bind_std_vector(gen, file_filter)

	gen.bind_function('hg::OpenFolderDialog', 'bool', ['const std::string &title', 'std::string &folder_name', '?const std::string &initial_dir'], {'arg_in_out': ['folder_name']})
	gen.bind_function('hg::OpenFileDialog', 'bool', ['const std::string &title', 'const std::vector<hg::FileFilter> &filters', 'std::string &file', '?const std::string &initial_dir'], {'arg_in_out': ['file']})
	gen.bind_function('hg::SaveFileDialog', 'bool', ['const std::string &title', 'const std::vector<hg::FileFilter> &filters', 'std::string &file', '?const std::string &initial_dir'], {'arg_in_out': ['file']})


def bind_engine(gen):
	gen.add_include('engine/init.h')
	gen.add_include('engine/engine.h')

	gen.bind_function('hg::GetExecutablePath', 'std::string', [])

	gen.bind_function('hg::EndFrame', 'void', [])

	gen.bind_function('hg::GetLastFrameDuration', 'hg::time_ns', [])
	gen.bind_function('hg::GetLastFrameDurationSec', 'float', [])
	gen.bind_function('hg::ResetLastFrameDuration', 'void', [])

	gen.bind_function('hg::_DebugHalt', 'void', [])


def bind_projection(gen):
	gen.add_include('foundation/projection.h')

	gen.bind_function('hg::ZoomFactorToFov', 'float', ['float zoom_factor'])
	gen.bind_function('hg::FovToZoomFactor', 'float', ['float fov'])

	gen.bind_function('hg::ComputeOrthographicProjectionMatrix', 'hg::Mat44', ['float znear', 'float zfar', 'float size', 'const hg::tVec2<float> &aspect_ratio', '?const hg::tVec2<float> &offset'])
	gen.bind_function('hg::ComputePerspectiveProjectionMatrix', 'hg::Mat44', ['float znear', 'float zfar', 'float zoom_factor', 'const hg::tVec2<float> &aspect_ratio', '?const hg::tVec2<float> &offset'])

	gen.bind_function('hg::ComputeAspectRatioX', 'hg::tVec2<float>', ['float width', 'float height'])
	gen.bind_function('hg::ComputeAspectRatioY', 'hg::tVec2<float>', ['float width', 'float height'])
	gen.bind_function('hg::Compute2DProjectionMatrix', 'hg::Mat44', ['float znear', 'float zfar', 'float res_x', 'float res_y', 'bool y_up'])

	gen.bind_function('hg::ExtractZoomFactorFromProjectionMatrix', 'float', ['const hg::Mat44 &m'])
	gen.bind_function('hg::ExtractZRangeFromPerspectiveProjectionMatrix', 'void', ['const hg::Mat44 &m', 'float &znear', 'float &zfar'], {'arg_out': ['znear', 'zfar']})
	gen.bind_function('hg::ExtractZRangeFromOrthographicProjectionMatrix', 'void', ['const hg::Mat44 &m', 'float &znear', 'float &zfar'], {'arg_out': ['znear', 'zfar']})
	gen.bind_function('hg::ExtractZRangeFromProjectionMatrix', 'void', ['const hg::Mat44 &m', 'float &znear', 'float &zfar'], {'arg_out': ['znear', 'zfar']})

	gen.bind_function('hg::ProjectToClipSpace', 'bool', ['const hg::Mat44 &proj', 'const hg::Vec3 &view', 'hg::Vec3 &clip'], {'arg_out': ['clip']})
	gen.bind_function('hg::ProjectOrthoToClipSpace', 'bool', ['const hg::Mat44 &proj', 'const hg::Vec3 &view', 'hg::Vec3 &clip'], {'arg_out': ['clip']})
	gen.bind_function('hg::UnprojectFromClipSpace', 'bool', ['const hg::Mat44 &inv_proj', 'const hg::Vec3 &clip', 'hg::Vec3 &view'], {'arg_out': ['view']})
	gen.bind_function('hg::UnprojectOrthoFromClipSpace', 'bool', ['const hg::Mat44 &inv_proj', 'const hg::Vec3 &clip', 'hg::Vec3 &view'], {'arg_out': ['view']})

	gen.bind_function('hg::ClipSpaceToScreenSpace', 'hg::Vec3', ['const hg::Vec3 &clip', 'const hg::tVec2<float> &resolution'])
	gen.bind_function('hg::ScreenSpaceToClipSpace', 'hg::Vec3', ['const hg::Vec3 &screen', 'const hg::tVec2<float> &resolution'])

	gen.bind_function('hg::ProjectToScreenSpace', 'bool', ['const hg::Mat44 &proj', 'const hg::Vec3 &view', 'const hg::tVec2<float> &resolution', 'hg::Vec3 &screen'], {'arg_out': ['screen']})
	gen.bind_function('hg::ProjectOrthoToScreenSpace', 'bool', ['const hg::Mat44 &proj', 'const hg::Vec3 &view', 'const hg::tVec2<float> &resolution', 'hg::Vec3 &screen'], {'arg_out': ['screen']})
	gen.bind_function('hg::UnprojectFromScreenSpace', 'bool', ['const hg::Mat44 &inv_proj', 'const hg::Vec3 &screen', 'const hg::tVec2<float> &resolution', 'hg::Vec3 &view'], {'arg_out': ['view']})
	gen.bind_function('hg::UnprojectOrthoFromScreenSpace', 'bool', ['const hg::Mat44 &inv_proj', 'const hg::Vec3 &screen', 'const hg::tVec2<float> &resolution', 'hg::Vec3 &view'], {'arg_out': ['view']})

	gen.bind_function('hg::ProjectZToClipSpace', 'float', ['float z', 'const hg::Mat44 &proj'])


def bind_plugins(gen):
	gen.bind_function_overloads('hg::LoadPlugins', [('uint32_t', [], []), ('uint32_t', ['const std::string &path'], [])])
	gen.bind_function('hg::UnloadPlugins', 'void', [])


def bind_window_system(gen):
	gen.add_include('platform/window_system.h')

	gen.bind_function('hg::WindowSystemInit', 'void', [])
	gen.bind_function('hg::WindowSystemShutdown', 'void', [])

	# hg::MonitorRotation
	gen.bind_named_enum('hg::MonitorRotation', ['MR_0', 'MR_90', 'MR_180', 'MR_270'], storage_type='uint8_t')

	# hg:MonitorMode
	monitor_mode = gen.begin_class('hg::MonitorMode')
	gen.bind_members(monitor_mode, ['std::string name', 'hg::Rect<int> rect', 'int frequency', 'hg::MonitorRotation rotation', 'uint8_t supported_rotations'])
	gen.end_class(monitor_mode)
	bind_std_vector(gen, monitor_mode)
	
	# hg::Monitor
	monitor = gen.bind_ptr('hg::Monitor *', bound_name='Monitor')
	bind_std_vector(gen, monitor)

	gen.bind_function('hg::GetMonitors', 'std::vector<hg::Monitor*>', [])
	gen.bind_function('hg::GetMonitorRect', 'hg::Rect<int>', ['const hg::Monitor * monitor'])
	gen.bind_function('hg::IsPrimaryMonitor', 'bool', ['const hg::Monitor *monitor'])
	gen.bind_function('hg::IsMonitorConnected', 'bool', ['const hg::Monitor *monitor'])
	gen.bind_function('hg::GetMonitorName', 'std::string', ['const hg::Monitor *monitor'])
	gen.bind_function('hg::GetMonitorSizeMM', 'hg::tVec2<int>', ['const hg::Monitor *monitor'])
	gen.bind_function('hg::GetMonitorModes', 'bool', ['const hg::Monitor *monitor', 'std::vector<hg::MonitorMode> &modes'], features={'arg_out': ['modes']})

	# hg::Window
	gen.bind_named_enum('hg::WindowVisibility', ['WV_Windowed', 'WV_Undecorated', 'WV_Fullscreen', 'WV_Hidden', 'WV_FullscreenMonitor1', 'WV_FullscreenMonitor2', 'WV_FullscreenMonitor3'])

	window = gen.bind_ptr('hg::Window *', bound_name='Window')

	gen.bind_function_overloads('hg::NewWindow', [
		('hg::Window *', ['int width', 'int height', '?int bpp', '?hg::WindowVisibility visibility'], []),
		('hg::Window *', ['const char *title', 'int width', 'int height', '?int bpp', '?hg::WindowVisibility visibility'], [])
	])
	gen.bind_function_overloads('hg::NewFullscreenWindow', [
		('hg::Window *', ['const hg::Monitor *monitor', 'int mode_index', '?hg::MonitorRotation rotation'], []),
		('hg::Window *', ['const char *title', 'const hg::Monitor *monitor', 'int mode_index', '?hg::MonitorRotation rotation'], [])
	])
	gen.bind_function('hg::NewWindowFrom', 'hg::Window *', ['void *handle'])

	gen.bind_function('hg::GetWindowHandle', 'void *', ['const hg::Window *window'])
	gen.bind_function('hg::UpdateWindow', 'bool', ['const hg::Window *window'])
	gen.bind_function('hg::DestroyWindow', 'bool', ['const hg::Window *window'])

	gen.bind_function('hg::GetWindowClientSize', 'bool', ['const hg::Window *window', 'int &width', 'int &height'], features={'arg_out': ['width', 'height']})
	gen.bind_function('hg::SetWindowClientSize', 'bool', ['hg::Window *window', 'int width', 'int height'])

	gen.bind_function('hg::GetWindowContentScale', 'hg::tVec2<float>', ['const hg::Window *window'])

	gen.bind_function('hg::GetWindowTitle', 'bool', ['const hg::Window *window', 'std::string &title'], features={'arg_out': ['title']})
	gen.bind_function('hg::SetWindowTitle', 'bool', ['hg::Window *window', 'const std::string &title'])

	gen.bind_function('hg::WindowHasFocus', 'bool', ['const hg::Window *window'])
	gen.bind_function('hg::GetWindowInFocus', 'hg::Window*', [])

	gen.bind_function('hg::GetWindowPos', 'hg::tVec2<int>', ['const hg::Window *window'])
	gen.bind_function('hg::SetWindowPos', 'bool', ['hg::Window *window', 'const hg::tVec2<int> position'])

	gen.bind_function('hg::IsWindowOpen', 'bool', ['const hg::Window *window'])

	gen.bind_function('hg::ShowCursor', 'void', [])
	gen.bind_function('hg::HideCursor', 'void', [])

#
def decl_get_set_method(gen, conv, type, method_suffix, var_name, features=[]):
	gen.bind_method(conv, 'Get' + method_suffix, 'const %s' % type, [], features)
	gen.bind_method(conv, 'Set' + method_suffix, 'void', ['const %s &%s' % (type, var_name)], features)


def decl_comp_get_set_method(gen, conv, comp_type, comp_var_name, type, method_suffix, var_name, features=[]):
	gen.bind_method(conv, 'Get' + method_suffix, 'const %s &' % type, ['const %s *%s' % (comp_type, comp_var_name)], features)
	gen.bind_method(conv, 'Set' + method_suffix, 'void', ['%s *%s' % (comp_type, comp_var_name), 'const %s &%s' % (type, var_name)], features)


def bind_LuaObject(gen):
	gen.add_include('script/lua_vm.h')
	gen.add_include('engine/lua_object.h')

	if gen.get_language() == 'Lua':
		class LuaObjectConverter(lang.lua.LuaTypeConverterCommon):
			def get_type_glue(self, gen, module_name):
				check = '''\
inline bool %s(lua_State *L, int idx) { return true; }
''' % self.check_func

				to_c = '''\
inline void %s(lua_State *L, int idx, void *obj) {
	lua_pushvalue(L, idx);
	*reinterpret_cast<hg::LuaObject *>(obj) = hg::LuaObject(L, luaL_ref(L, LUA_REGISTRYINDEX));
}
''' % self.to_c_func

				from_c = '''\
inline int %s(lua_State *L, void *obj, OwnershipPolicy) {
	return hg::PushForeign(L, *reinterpret_cast<hg::LuaObject *>(obj)); // PushForeign can unpack/repack certain types coming from a different VM
}
''' % self.from_c_func
				return check + to_c + from_c

		lua_object = gen.bind_type(LuaObjectConverter('hg::LuaObject'))
		lua_object._inline = True

		bind_std_vector(gen, lua_object)
	else:
		lua_object = gen.begin_class('hg::LuaObject')
		gen.end_class(lua_object)

		bind_std_vector(gen, lua_object)


def bind_signal_T(gen, name, rtype, args, cb_name):
	sig = '%s(%s)' % (rtype, ','.join(args))

	lib.stl.bind_function_T(gen, 'std::function<%s>' % sig, cb_name)

	cnx = gen.begin_class('hg::Signal<%s>::Connection' % sig, bound_name='%sConnection' % cb_name)
	gen.end_class(cnx)

	signal_T = gen.begin_class('hg::Signal<%s>' % sig, noncopyable=True)
	gen.bind_method(signal_T, 'Connect', 'hg::Signal<%s>::Connection' % sig, ['std::function<%s> listener' % sig])
	gen.bind_method(signal_T, 'Disconnect', 'void', ['hg::Signal<%s>::Connection connection' % sig])
	gen.bind_method(signal_T, 'DisconnectAll', 'void', [])

	named_args = []
	for i in range(len(args)):
		named_args.append(args[i] + ' arg%d' % i)

	gen.bind_method(signal_T, 'Emit', 'void', named_args)
	gen.bind_method(signal_T, 'GetListenerCount', 'size_t', [])
	gen.end_class(signal_T)


def bind_scene(gen):
	gen.add_include('engine/scene.h')

	# hg::SceneAnimRef
	gen.bind_named_enum('hg::AnimLoopMode', ['ALM_Once', 'ALM_Infinite', 'ALM_Loop'])

	scene_anim_ref = gen.begin_class('hg::SceneAnimRef')
	scene_anim_ref._inline = True
	gen.bind_comparison_ops(scene_anim_ref, ['==', '!='], ['const hg::SceneAnimRef &ref'])
	gen.end_class(scene_anim_ref)

	gen.bind_variable("const hg::SceneAnimRef hg::InvalidSceneAnimRef")
	bind_std_vector(gen, scene_anim_ref)

	scene_play_anim_ref = gen.begin_class('hg::ScenePlayAnimRef')
	scene_play_anim_ref._inline = True
	gen.bind_comparison_ops(scene_play_anim_ref, ['==', '!='], ['const hg::ScenePlayAnimRef &ref'])
	gen.end_class(scene_play_anim_ref)

	bind_std_vector(gen, scene_play_anim_ref)

	gen.bind_variable("const hg::time_ns hg::UnspecifiedAnimTime")

	# hg::Easing
	gen.bind_named_enum('hg::Easing', [
		'E_Linear', 'E_Step', 'E_SmoothStep', 'E_InQuad', 'E_OutQuad', 'E_InOutQuad', 'E_OutInQuad', 'E_InCubic', 'E_OutCubic', 'E_InOutCubic', 'E_OutInCubic',
		'E_InQuart', 'E_OutQuart', 'E_InOutQuart', 'E_OutInQuart', 'E_InQuint', 'E_OutQuint', 'E_InOutQuint', 'E_OutInQuint', 'E_InSine', 'E_OutSine', 'E_InOutSine',
		'E_OutInSine', 'E_InExpo', 'E_OutExpo', 'E_InOutExpo', 'E_OutInExpo', 'E_InCirc', 'E_OutCirc', 'E_InOutCirc', 'E_OutInCirc', 'E_InElastic', 'E_OutElastic',
		'E_InOutElastic', 'E_OutInElastic', 'E_InBack', 'E_OutBack', 'E_InOutBack', 'E_OutInBack', 'E_InBounce', 'E_OutBounce', 'E_InOutBounce', 'E_OutInBounce'
	], 'unsigned char')

	# hg::CollisionMesh
#	collision_mesh = gen.begin_class('hg::CollisionMesh')
#	gen.bind_comparison_ops(collision_mesh, ['==', '!='], ['const hg::CollisionMesh &c'])
#	gen.end_class(collision_mesh)
#	gen.bind_variable("const hg::CollisionMesh hg::InvalidCollisionMeshRef")

	#
	scene = gen.begin_class('hg::Scene', noncopyable=True)
	scene_view = gen.begin_class('hg::SceneView')

	node = gen.begin_class('hg::Node')
	node._inline = True

	# hg::Transform
	TRS = gen.begin_class('hg::TransformTRS')
	gen.bind_constructor(TRS, [])
	gen.bind_members(TRS, ['hg::Vec3 pos', 'hg::Vec3 rot', 'hg::Vec3 scl'])
	gen.end_class(TRS)

	transform = gen.begin_class('hg::Transform')

	gen.bind_method(transform, 'IsValid', 'bool', [])
	gen.bind_comparison_op(transform, '==', ['const hg::Transform &t'])

	gen.bind_method(transform, 'GetPos', 'hg::Vec3', [])
	gen.bind_method(transform, 'SetPos', 'void', ['const hg::Vec3 &T'])
	gen.bind_method(transform, 'GetRot', 'hg::Vec3', [])
	gen.bind_method(transform, 'SetRot', 'void', ['const hg::Vec3 &R'])
	gen.bind_method(transform, 'GetScale', 'hg::Vec3', [])
	gen.bind_method(transform, 'SetScale', 'void', ['const hg::Vec3 &S'])
	gen.bind_method(transform, 'GetTRS', 'hg::TransformTRS', [])
	gen.bind_method(transform, 'SetTRS', 'void', ['const hg::TransformTRS &TRS'])

	gen.bind_method(transform, 'GetPosRot', 'void', ['hg::Vec3 &pos', 'hg::Vec3 &rot'], {'arg_out': ['pos', 'rot']})
	gen.bind_method(transform, 'SetPosRot', 'void', ['const hg::Vec3 &pos', 'const hg::Vec3 &rot'])

	gen.insert_binding_code('static void _NodeClearParent(hg::Transform *trs) { trs->SetParentNode(hg::NullNode); }')

	gen.bind_method(transform, 'GetParentNode', 'hg::Node', [], bound_name='GetParent')
	gen.bind_method(transform, 'SetParentNode', 'void', ['const hg::Node &n'], bound_name='SetParent')
	gen.bind_method(transform, 'ClearParent', 'void', [], {'route': route_lambda('_NodeClearParent')})

	gen.bind_method(transform, 'GetWorld', 'hg::Mat4', [])
	gen.bind_method(transform, 'SetWorld', 'void', ['const hg::Mat4 &world'])
	gen.bind_method(transform, 'SetLocal', 'void', ['const hg::Mat4 &local'])

	gen.end_class(transform)

	# hg::Camera
	zrange = gen.begin_class('hg::CameraZRange')
	gen.bind_constructor(zrange, [])
	gen.bind_members(zrange, ['float znear', 'float zfar'])
	gen.end_class(zrange)

	camera = gen.begin_class('hg::Camera')

	gen.bind_method(camera, 'IsValid', 'bool', [])
	gen.bind_comparison_op(camera, '==', ['const hg::Camera &c'])

	gen.bind_method(camera, 'GetZNear', 'float', [])
	gen.bind_method(camera, 'SetZNear', 'void', ['float v'])
	gen.bind_method(camera, 'GetZFar', 'float', [])
	gen.bind_method(camera, 'SetZFar', 'void', ['float v'])
	gen.bind_method(camera, 'GetZRange', 'hg::CameraZRange', [])
	gen.bind_method(camera, 'SetZRange', 'void', ['const hg::CameraZRange &z'])
	gen.bind_method(camera, 'GetFov', 'float', [])
	gen.bind_method(camera, 'SetFov', 'void', ['float v'])
	gen.bind_method(camera, 'GetIsOrthographic', 'bool', [])
	gen.bind_method(camera, 'SetIsOrthographic', 'void', ['bool v'])
	gen.bind_method(camera, 'GetSize', 'float', [])
	gen.bind_method(camera, 'SetSize', 'void', ['float v'])

	gen.end_class(camera)

	# hg::Object
	object = gen.begin_class('hg::Object')

	gen.bind_method(object, 'IsValid', 'bool', [])
	gen.bind_comparison_op(object, '==', ['const hg::Object &o'])

	gen.bind_method(object, 'GetModelRef', 'hg::ModelRef', [])
	gen.bind_method(object, 'SetModelRef', 'void', ['const hg::ModelRef &r'])
	gen.bind_method(object, 'ClearModelRef', 'void', [])
	gen.bind_method_overloads(object, 'GetMaterial', [
		('hg::Material &', ['size_t slot_idx'], []),
		('hg::Material *', ['const std::string &name'], [])
	])
	gen.bind_method(object, 'SetMaterial', 'void', ['size_t slot_idx', 'hg::Material mat'])
	gen.bind_method(object, 'GetMaterialCount', 'size_t', [])
	gen.bind_method(object, 'SetMaterialCount', 'void', ['size_t count'])
	gen.bind_method(object, 'GetMaterialName', 'std::string', ['size_t slot_idx'])
	gen.bind_method(object, 'SetMaterialName', 'void', ['size_t slot_idx', 'const std::string &name'])
	gen.bind_method(object, 'GetMinMax', 'bool', ['const hg::PipelineResources &resources', 'hg::MinMax &minmax'], {'arg_out': ['minmax']})

	gen.bind_method(object, 'GetBoneCount', 'size_t', [])
	gen.bind_method(object, 'SetBoneCount', 'void', ['size_t count'])
	gen.bind_method(object, 'SetBoneNode', 'bool', ['size_t idx', 'const hg::Node &node'], bound_name='SetBone')
	gen.bind_method(object, 'GetBoneNode', 'hg::Node', ['size_t idx'], bound_name='GetBone')

	gen.end_class(object)

	# hg::Light
	gen.bind_named_enum('hg::LightType', ['LT_Point', 'LT_Spot', 'LT_Linear'])
	gen.bind_named_enum('hg::LightShadowType', ['LST_None', 'LST_Map'])

	light = gen.begin_class('hg::Light')

	gen.bind_method(light, 'IsValid', 'bool', [])
	gen.bind_comparison_op(light, '==', ['const hg::Light &l'])

	gen.bind_method(light, 'GetType', 'hg::LightType', [])
	gen.bind_method(light, 'SetType', 'void', ['hg::LightType v'])
	gen.bind_method(light, 'GetShadowType', 'hg::LightShadowType', [])
	gen.bind_method(light, 'SetShadowType', 'void', ['hg::LightShadowType v'])
	gen.bind_method(light, 'GetDiffuseColor', 'hg::Color', [])
	gen.bind_method(light, 'SetDiffuseColor', 'void', ['const hg::Color &v'])
	gen.bind_method(light, 'GetDiffuseIntensity', 'float', [])
	gen.bind_method(light, 'SetDiffuseIntensity', 'void', ['float v'])
	gen.bind_method(light, 'GetSpecularColor', 'hg::Color', [])
	gen.bind_method(light, 'SetSpecularColor', 'void', ['const hg::Color &v'])
	gen.bind_method(light, 'GetSpecularIntensity', 'float', [])
	gen.bind_method(light, 'SetSpecularIntensity', 'void', ['float v'])
	gen.bind_method(light, 'GetRadius', 'float', [])
	gen.bind_method(light, 'SetRadius', 'void', ['float v'])
	gen.bind_method(light, 'GetInnerAngle', 'float', [])
	gen.bind_method(light, 'SetInnerAngle', 'void', ['float v'])
	gen.bind_method(light, 'GetOuterAngle', 'float', [])
	gen.bind_method(light, 'SetOuterAngle', 'void', ['float v'])
	gen.bind_method(light, 'GetPSSMSplit', 'hg::Vec4', [])
	gen.bind_method(light, 'SetPSSMSplit', 'void', ['const hg::Vec4 &v'])
	gen.bind_method(light, 'GetPriority', 'float', [])
	gen.bind_method(light, 'SetPriority', 'void', ['float v'])

	gen.end_class(light)

	# hg::RigidBody
	gen.add_include('engine/physics.h')

	gen.bind_named_enum('hg::RigidBodyType', ['RBT_Dynamic', 'RBT_Kinematic', 'RBT_Static'], storage_type='uint8_t')
	gen.bind_named_enum('hg::CollisionEventTrackingMode', ['CETM_EventOnly', 'CETM_EventAndContacts'], storage_type='uint8_t')

	contact = gen.begin_class('hg::Contact')
	gen.bind_members(contact, ['hg::Vec3 P', 'hg::Vec3 N'])
	gen.bind_member(contact, 'float d')
	gen.end_class(contact)

	bind_std_vector(gen, contact)

	rigid_body = gen.begin_class('hg::RigidBody')

	gen.bind_method(rigid_body, 'IsValid', 'bool', [])
	gen.bind_comparison_op(rigid_body, '==', ['const hg::RigidBody &b'])

	gen.bind_method(rigid_body, 'GetType', 'hg::RigidBodyType', [])
	gen.bind_method(rigid_body, 'SetType', 'void', ['hg::RigidBodyType type'])

	gen.bind_method(rigid_body, 'GetLinearDamping', 'float', [])
	gen.bind_method(rigid_body, 'SetLinearDamping', 'void', ['float damping'])
	gen.bind_method(rigid_body, 'GetAngularDamping', 'float', [])
	gen.bind_method(rigid_body, 'SetAngularDamping', 'void', ['float damping'])
	gen.bind_method(rigid_body, 'GetRestitution', 'float', [])
	gen.bind_method(rigid_body, 'SetRestitution', 'void', ['float restitution'])
	gen.bind_method(rigid_body, 'GetFriction', 'float', [])
	gen.bind_method(rigid_body, 'SetFriction', 'void', ['float friction'])
	gen.bind_method(rigid_body, 'GetRollingFriction', 'float', [])
	gen.bind_method(rigid_body, 'SetRollingFriction', 'void', ['float rolling_friction'])

	gen.end_class(rigid_body)

	# hg::Collision
	gen.bind_named_enum('hg::CollisionType', ['CT_Sphere', 'CT_Cube', 'CT_Cone', 'CT_Capsule', 'CT_Cylinder', 'CT_Mesh'], storage_type='uint8_t')

	collision = gen.begin_class('hg::Collision')

	gen.bind_method(collision, 'IsValid', 'bool', [])
	gen.bind_comparison_op(collision, '==', ['const hg::Collision &c'])

	gen.bind_method(collision, 'GetType', 'hg::CollisionType', [])
	gen.bind_method(collision, 'SetType', 'void', ['hg::CollisionType type'])
	gen.bind_method(collision, 'GetLocalTransform', 'hg::Mat4', [])
	gen.bind_method(collision, 'SetLocalTransform', 'void', ['hg::Mat4 m'])
	gen.bind_method(collision, 'GetMass', 'float', [])
	gen.bind_method(collision, 'SetMass', 'void', ['float mass'])
	gen.bind_method(collision, 'GetRadius', 'float', [])
	gen.bind_method(collision, 'SetRadius', 'void', ['float radius'])
	gen.bind_method(collision, 'GetHeight', 'float', [])
	gen.bind_method(collision, 'SetHeight', 'void', ['float height'])
	gen.bind_method(collision, 'SetSize', 'void', ['const hg::Vec3 &size'])
	gen.bind_method(collision, 'GetCollisionResource', 'std::string', [])
	gen.bind_method(collision, 'SetCollisionResource', 'void', ['const std::string &path'])

	gen.end_class(collision)

	# hg::Instance
	instance = gen.begin_class('hg::Instance')

	gen.bind_method(instance, 'IsValid', 'bool', [])
	gen.bind_comparison_op(instance, '==', ['const hg::Instance &i'])

	gen.bind_method(instance, 'GetPath', 'std::string', [])
	gen.bind_method(instance, 'SetPath', 'void', ['const std::string &path'])

	gen.bind_method(instance, 'SetOnInstantiateAnim', 'void', ['const std::string &anim'])
	gen.bind_method(instance, 'SetOnInstantiateAnimLoopMode', 'void', ['hg::AnimLoopMode loop_mode'])
	gen.bind_method(instance, 'ClearOnInstantiateAnim', 'void', [])

	gen.bind_method(instance, 'GetOnInstantiateAnim', 'std::string', [])
	gen.bind_method(instance, 'GetOnInstantiateAnimLoopMode', 'hg::AnimLoopMode', [])
	gen.bind_method(instance, 'GetOnInstantiatePlayAnimRef', 'hg::ScenePlayAnimRef', [])

	gen.end_class(instance)

	# hg::Script
	script = gen.begin_class('hg::Script')

	gen.bind_method(script, 'IsValid', 'bool', [])
	gen.bind_comparison_op(script, '==', ['const hg::Script &s'])

	gen.bind_method(script, 'GetPath', 'std::string', [])
	gen.bind_method(script, 'SetPath', 'void', ['const std::string &path'])

	gen.end_class(script)

	bind_std_vector(gen, script)

	# hg::Node
	gen.bind_method(node, 'IsValid', 'bool', [])
	gen.bind_comparison_op(node, '==', ['const hg::Node &n'])

	gen.bind_method(node, 'GetUid', 'uint32_t', [])

	gen.bind_method(node, 'GetFlags', 'uint32_t', [])
	gen.bind_method(node, 'SetFlags', 'void', ['uint32_t flags'])

	gen.bind_method(node, 'Enable', 'void', [])
	gen.bind_method(node, 'Disable', 'void', [])

	gen.bind_method(node, 'IsEnabled', 'bool', [])
	gen.bind_method(node, 'IsItselfEnabled', 'bool', [])

	gen.bind_method(node, 'HasTransform', 'bool', [])
	gen.bind_method(node, 'GetTransform', 'hg::Transform', [])
	gen.bind_method(node, 'SetTransform', 'void', ['const hg::Transform &t'])
	gen.bind_method(node, 'RemoveTransform', 'void', [])

	gen.bind_method(node, 'HasCamera', 'bool', [])
	gen.bind_method(node, 'GetCamera', 'hg::Camera', [])
	gen.bind_method(node, 'SetCamera', 'void', ['const hg::Camera &c'])
	gen.bind_method(node, 'RemoveCamera', 'void', [])

	gen.bind_method(node, 'ComputeCameraViewState', 'hg::ViewState', ['const hg::tVec2<float> &aspect_ratio'])

	gen.bind_method(node, 'HasObject', 'bool', [])
	gen.bind_method(node, 'GetObject', 'hg::Object', [])
	gen.bind_method(node, 'SetObject', 'void', ['const hg::Object &o'])
	gen.bind_method(node, 'RemoveObject', 'void', [])

	gen.bind_method(node, 'GetMinMax', 'bool', ['const hg::PipelineResources &resources', 'hg::MinMax &minmax'], {'arg_out': ['minmax']})

	gen.bind_method(node, 'HasLight', 'bool', [])
	gen.bind_method(node, 'GetLight', 'hg::Light', [])
	gen.bind_method(node, 'SetLight', 'void', ['const hg::Light &l'])
	gen.bind_method(node, 'RemoveLight', 'void', [])

	gen.bind_method(node, 'HasRigidBody', 'bool', [])
	gen.bind_method(node, 'GetRigidBody', 'hg::RigidBody', [])
	gen.bind_method(node, 'SetRigidBody', 'void', ['const hg::RigidBody &b'])
	gen.bind_method(node, 'RemoveRigidBody', 'void', [])

	gen.bind_method(node, 'GetCollisionCount', 'size_t', [])
	gen.bind_method(node, 'GetCollision', 'hg::Collision', ['size_t slot'])
	gen.bind_method(node, 'SetCollision', 'void', ['size_t slot', 'const hg::Collision &c'])
	gen.bind_method_overloads(node, 'RemoveCollision', [
		('void', ['const hg::Collision &c'], {}),
		('void', ['size_t slot'], {})
	])

	gen.bind_method(node, 'GetName', 'std::string', [])
	gen.bind_method(node, 'SetName', 'void', ['const std::string &name'])

	gen.bind_method(node, 'GetScriptCount', 'size_t', [])
	gen.bind_method(node, 'GetScript', 'hg::Script', ['size_t idx'])
	gen.bind_method(node, 'SetScript', 'void', ['size_t idx', 'const hg::Script &s'])
	gen.bind_method_overloads(node, 'RemoveScript', [
		('void', ['const hg::Script &s'], {}),
		('void', ['size_t slot'], {})
	])

	gen.bind_method(node, 'HasInstance', 'bool', [])
	gen.bind_method(node, 'GetInstance', 'hg::Instance', [])
	gen.bind_method(node, 'SetInstance', 'void', ['const hg::Instance &instance'])

	gen.bind_method(node, 'SetupInstanceFromFile', 'bool', ['hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}})
	gen.bind_method(node, 'SetupInstanceFromAssets', 'bool', ['hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}})
	gen.bind_method(node, 'DestroyInstance', 'void', [])

	gen.bind_method(node, 'IsInstantiatedBy', 'hg::Node', [])

	gen.bind_method(node, 'GetInstanceSceneView', 'hg::SceneView', [])
	gen.bind_method(node, 'GetInstanceSceneAnim', 'hg::SceneAnimRef', ['const std::string &path'])

	gen.bind_method(node, 'StartOnInstantiateAnim', 'void', [])
	gen.bind_method(node, 'StopOnInstantiateAnim', 'void', [])

	gen.bind_method(node, 'GetWorld', 'hg::Mat4', [])
	gen.bind_method(node, 'SetWorld', 'void', ['const hg::Mat4 &world'])
	gen.bind_method(node, 'ComputeWorld', 'hg::Mat4', [])

	gen.end_class(node)

	bind_std_vector(gen, node)

	gen.bind_variable("const hg::Node hg::NullNode")

	# hg::SceneView
	gen.bind_method(scene_view, 'GetNodes', 'std::vector<hg::Node>', ['const hg::Scene &scene'])
	gen.bind_method(scene_view, 'GetNode', 'hg::Node', ['const hg::Scene &scene', 'const std::string &name'])

	gen.end_class(scene_view)

	# hg::RaycastOut
	raycast_out = gen.begin_class('hg::RaycastOut')
	gen.bind_members(raycast_out, ['hg::Vec3 P', 'hg::Vec3 N', 'hg::Node node', 'float t'], ['copy_obj'])
	gen.end_class(raycast_out)

	bind_std_vector(gen, raycast_out)

	#
	gen.bind_named_enum('hg::NodeComponentIdx', ['NCI_Transform', 'NCI_Camera', 'NCI_Object', 'NCI_Light', 'NCI_RigidBody'])

	# hg::Scene
	bind_signal_T(gen, 'TimeSignal', 'void', ['hg::time_ns'], 'TimeCallback')

	gen.bind_constructor(scene, [])

	gen.bind_method(scene, 'GetNode', 'hg::Node', ['const std::string &name'])
	gen.bind_method(scene, 'GetNodeEx', 'hg::Node', ['const std::string &path'])

	gen.bind_method(scene, 'GetNodes', 'std::vector<hg::Node>', [])
	gen.bind_method(scene, 'GetAllNodes', 'std::vector<hg::Node>', [])
	gen.bind_method(scene, 'GetNodesWithComponent', 'std::vector<hg::Node>', ['hg::NodeComponentIdx idx'])
	gen.bind_method(scene, 'GetAllNodesWithComponent', 'std::vector<hg::Node>', ['hg::NodeComponentIdx idx'])

	gen.bind_method(scene, 'GetNodeCount', 'size_t', [])
	gen.bind_method(scene, 'GetAllNodeCount', 'size_t', [])

	gen.bind_method(scene, 'GetNodeChildren', 'std::vector<hg::Node>', ['const hg::Node &node'])

	gen.bind_method(scene, 'IsChildOf', 'bool', ['const hg::Node &node', 'const hg::Node &parent'])
	gen.bind_method(scene, 'IsRoot', 'bool', ['const hg::Node &node'])

	#
	gen.bind_method(scene, 'ReadyWorldMatrices', 'void', [])
	gen.bind_method(scene, 'ComputeWorldMatrices', 'void', [])

	gen.bind_method(scene, 'Update', 'void', ['hg::time_ns dt'])

	#
	gen.bind_method(scene, 'GetSceneAnims', 'std::vector<hg::SceneAnimRef>', [])
	gen.bind_method(scene, 'GetSceneAnim', 'hg::SceneAnimRef', ['const char *name'])

	gen.bind_method(scene, 'PlayAnim', 'hg::ScenePlayAnimRef', ['hg::SceneAnimRef ref', '?hg::AnimLoopMode loop_mode', '?hg::Easing easing', '?hg::time_ns t_start', '?hg::time_ns t_end', '?bool paused', '?float t_scale'])

	gen.bind_method(scene, 'IsPlaying', 'bool', ['hg::ScenePlayAnimRef ref'])
	gen.bind_method(scene, 'StopAnim', 'void', ['hg::ScenePlayAnimRef ref'])
	gen.bind_method(scene, 'StopAllAnims', 'void', [])

	#
	gen.bind_method(scene, 'GetPlayingAnimNames', 'std::vector<std::string>', [])
	gen.bind_method(scene, 'GetPlayingAnimRefs', 'std::vector<hg::ScenePlayAnimRef>', [])
	gen.bind_method(scene, 'UpdatePlayingAnims', 'void', ['hg::time_ns dt'])

	#
	gen.bind_method(scene, 'HasKey', 'bool', ['const std::string &key'])
	gen.bind_method(scene, 'GetKeys', 'std::vector<std::string>', [])
	gen.bind_method(scene, 'RemoveKey', 'void', ['const std::string &key'])

	gen.bind_method(scene, 'GetValue', 'std::string', ['const std::string &key'])
	gen.bind_method(scene, 'SetValue', 'void', ['const std::string &key', 'const std::string &value'])

	#
	gen.bind_method(scene, 'GarbageCollect', 'size_t', [])
	gen.bind_method(scene, 'Clear', 'void', [])

	#
	gen.bind_method(scene, 'ReserveNodes', 'void', ['size_t count'])
	gen.bind_method(scene, 'CreateNode', 'hg::Node', ['?std::string name'])
	gen.bind_method(scene, 'DestroyNode', 'void', ['const hg::Node &node'])

	#
	gen.bind_method(scene, 'ReserveTransforms', 'void', ['size_t count'])
	gen.bind_method_overloads(scene, 'CreateTransform', [
		('hg::Transform', [], []),
		('hg::Transform', ['const hg::Vec3 &T', '?const hg::Vec3 &R', '?const hg::Vec3 &S'], [])
	])
	gen.bind_method(scene, 'DestroyTransform', 'void', ['const hg::Transform &transform'])

	#
	gen.bind_method(scene, 'ReserveCameras', 'void', ['size_t count'])
	gen.bind_method_overloads(scene, 'CreateCamera', [
		('hg::Camera', [], []),
		('hg::Camera', ['float znear', 'float zfar', '?float fov'], [])
	])
	gen.bind_method(scene, 'CreateOrthographicCamera', 'hg::Camera', ['float znear', 'float zfar', '?float size'])
	gen.bind_method(scene, 'DestroyCamera', 'void', ['const hg::Camera &camera'])

	gen.bind_method(scene, 'ComputeCurrentCameraViewState', 'hg::ViewState', ['const hg::tVec2<float> &aspect_ratio'])

	#
	gen.bind_method(scene, 'ReserveObjects', 'void', ['size_t count'])
	protos = [
		('hg::Object', [], []),
		('hg::Object', ['const hg::ModelRef &model', 'const std::vector<hg::Material> &materials'], [])
	]
	gen.bind_method_overloads(scene, 'CreateObject', expand_std_vector_proto(gen, protos))
	gen.bind_method(scene, 'DestroyObject', 'void', ['const hg::Object &object'])

	#
	gen.bind_method(scene, 'ReserveLights', 'void', ['size_t count'])
	gen.bind_method(scene, 'CreateLight', 'hg::Light', [])
	gen.bind_method(scene, 'DestroyLight', 'void', ['const hg::Light &light'])

	gen.bind_method_overloads(scene, 'CreateLinearLight', [
		('hg::Light', ['const hg::Color &diffuse', 'float diffuse_intensity', 'const hg::Color &specular', 'float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias', '?const hg::Vec4 &pssm_split'], []),
		('hg::Light', ['const hg::Color &diffuse', 'const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias', '?const hg::Vec4 &pssm_split'], [])
	])
	gen.bind_method_overloads(scene, 'CreatePointLight', [
		('hg::Light', ['float radius', 'const hg::Color &diffuse', 'float diffuse_intensity', 'const hg::Color &specular', 'float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], []),
		('hg::Light', ['float radius', 'const hg::Color &diffuse', 'const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], [])
	])
	gen.bind_method_overloads(scene, 'CreateSpotLight', [
		('hg::Light', ['float radius', 'float inner_angle', 'float outer_angle', 'const hg::Color &diffuse', 'float diffuse_intensity', 'const hg::Color &specular', 'float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], []),
		('hg::Light', ['float radius', 'float inner_angle', 'float outer_angle', 'const hg::Color &diffuse', 'const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], [])
	])

	#
	gen.bind_method(scene, 'ReserveScripts', 'void', ['size_t count'])
	gen.bind_method(scene, 'CreateScript', 'hg::Script', ['?const std::string &path'])
	gen.bind_method(scene, 'DestroyScript', 'void', ['const hg::Script &script'])

	#
	gen.bind_method(scene, 'GetScriptCount', 'size_t', [])
	gen.bind_method(scene, 'SetScript', 'void', ['size_t slot_idx', 'const hg::Script &script'])
	gen.bind_method(scene, 'GetScript', 'hg::Script', ['size_t slot_idx'])

	#
	gen.bind_method(scene, 'CreateRigidBody', 'hg::RigidBody', [])
	gen.bind_method(scene, 'DestroyRigidBody', 'void', ['const hg::RigidBody &rigid_body'])

	#
	gen.bind_method(scene, 'CreateCollision', 'hg::Collision', [])
	gen.bind_method(scene, 'DestroyCollision', 'void', ['const hg::Collision &collision'])
    
	#
	gen.bind_method(scene, 'CreateInstance', 'hg::Instance', [])
	gen.bind_method(scene, 'DestroyInstance', 'void', ['const hg::Instance &Instance'])

	#
	canvas = gen.begin_class('hg::Scene::Canvas')
	gen.bind_members(canvas, ['bool clear_z', 'bool clear_color', 'hg::Color color'])
	gen.end_class(canvas)

	gen.bind_member(scene, 'hg::Scene::Canvas canvas')

	#
	environment = gen.begin_class('hg::Scene::Environment')
	gen.bind_members(environment, ['hg::Color ambient', 'hg::Color fog_color', 'float fog_near', 'float fog_far', 'hg::TextureRef brdf_map'])
	gen.end_class(environment)

	gen.bind_member(scene, 'hg::Scene::Environment environment')

	#
	gen.bind_method(scene, 'GetCurrentCamera', 'hg::Node', [])
	gen.bind_method(scene, 'SetCurrentCamera', 'void', ['const hg::Node &camera'])

	#
	gen.bind_method(scene, 'GetMinMax', 'bool', ['const hg::PipelineResources &resources', 'hg::MinMax &minmax'], {'arg_out': ['minmax']})

	gen.end_class(scene)

	# helpers
	gen.bind_function('hg::CreateSceneRootNode', 'hg::Node', ['hg::Scene &scene', 'std::string name', 'const hg::Mat4 &mtx'])

	gen.bind_function('hg::CreateCamera', 'hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float znear', 'float zfar', '?float fov'])
	gen.bind_function('hg::CreateOrthographicCamera', 'hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float znear', 'float zfar', '?float size'])

	gen.bind_function_overloads('hg::CreatePointLight', [
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float radius', '?const hg::Color &diffuse', '?const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], {}),
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float radius', 'const hg::Color &diffuse', 'float diffuse_intensity', '?const hg::Color &specular', '?float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], {})
	])
	gen.bind_function_overloads('hg::CreateSpotLight', [
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float radius', 'float inner_angle', 'float outer_angle', '?const hg::Color &diffuse', '?const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], {}),
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'float radius', 'float inner_angle', 'float outer_angle', 'const hg::Color &diffuse', 'float diffuse_intensity', '?const hg::Color &specular', '?float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias'], {})
	])
	gen.bind_function_overloads('hg::CreateLinearLight', [
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', '?const hg::Color &diffuse', '?const hg::Color &specular', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias', 'const hg::Vec4 &pssm_split'], {}),
		('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'const hg::Color &diffuse', 'float diffuse_intensity', '?const hg::Color &specular', '?float specular_intensity', '?float priority', '?hg::LightShadowType shadow_type', '?float shadow_bias', 'const hg::Vec4 &pssm_split'], {})
	])

	protos = [('hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'const hg::ModelRef &model', 'const std::vector<hg::Material> &materials'], [])]
	gen.bind_function_overloads('hg::CreateObject', expand_std_vector_proto(gen, protos))

	gen.bind_function('hg::CreateInstanceFromFile', 'hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'const std::string &name', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', 'bool &success', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}, 'arg_out': ['success']})
	gen.bind_function('hg::CreateInstanceFromAssets', 'hg::Node', ['hg::Scene &scene', 'const hg::Mat4 &mtx', 'const std::string &name', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', 'bool &success', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}, 'arg_out': ['success']})

	gen.bind_function('hg::CreateScript', 'hg::Node', ['hg::Scene &scene', '?const std::string &path'])

	protos = [
		('hg::Node', ['hg::Scene &scene', 'float radius', 'const hg::Mat4 &mtx', 'const hg::ModelRef &model_ref', 'std::vector<hg::Material> &materials'], {}),
		('hg::Node', ['hg::Scene &scene', 'float radius', 'const hg::Mat4 &mtx', 'const hg::ModelRef &model_ref', 'std::vector<hg::Material> &materials', 'float mass'], {}),
	]
	gen.bind_function_overloads('hg::CreatePhysicSphere', expand_std_vector_proto(gen, protos))

	protos = [
		('hg::Node', ['hg::Scene &scene', 'const hg::Vec3 &size', 'const hg::Mat4 &mtx', 'const hg::ModelRef &model_ref', 'std::vector<hg::Material> &materials'], {}),
		('hg::Node', ['hg::Scene &scene', 'const hg::Vec3 &size', 'const hg::Mat4 &mtx', 'const hg::ModelRef &model_ref', 'std::vector<hg::Material> &materials', 'float mass'], {}),
	]
	gen.bind_function_overloads('hg::CreatePhysicCube', expand_std_vector_proto(gen, protos))

	# load/save scene
	gen.bind_constants('uint32_t', [
		("LSSF_Nodes", "hg::LSSF_Nodes"),
		("LSSF_Scene", "hg::LSSF_Scene"),
		("LSSF_Anims", "hg::LSSF_Anims"),
		("LSSF_KeyValues", "hg::LSSF_KeyValues"),
		("LSSF_Physics", "hg::LSSF_Physics"),
		("LSSF_Scripts", "hg::LSSF_Scripts"),
		("LSSF_All", "hg::LSSF_All"),
		("LSSF_QueueTextureLoads", "hg::LSSF_QueueTextureLoads"),
		("LSSF_FreezeMatrixToTransformOnSave", "hg::LSSF_FreezeMatrixToTransformOnSave"),
		("LSSF_QueueModelLoads", "hg::LSSF_QueueModelLoads"),
		("LSSF_DoNotChangeCurrentCameraIfValid", "hg::LSSF_DoNotChangeCurrentCameraIfValid"),
	], 'LoadSaveSceneFlags')

	gen.bind_function('hg::SaveSceneJsonToFile', 'bool', ['const char *path', 'const hg::Scene &scene', 'const hg::PipelineResources &resources', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}})
	gen.bind_function('hg::SaveSceneBinaryToFile', 'bool', ['const char *path', 'const hg::Scene &scene', 'const hg::PipelineResources &resources', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}})
	gen.bind_function('hg::SaveSceneBinaryToData', 'bool', ['hg::Data &data', 'const hg::Scene &scene', 'const hg::PipelineResources &resources', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}})

	gen.insert_binding_code('''
static bool _LoadSceneBinaryFromFile(const char *path, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneBinaryFromFile(path, scene, resources, pipeline, ctx, flags);
}
static bool _LoadSceneBinaryFromAssets(const char *name, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneBinaryFromAssets(name, scene, resources, pipeline, ctx, flags);
}
static bool _LoadSceneBinaryFromData(const hg::Data &data, const char *name, hg::Scene &scene, const hg::Reader &deps_ir, const hg::ReadProvider &deps_ip, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneBinaryFromData(data, name, scene, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

#include "foundation/data_rw_interface.h"
#include "foundation/file_rw_interface.h"

#include "engine/assets_rw_interface.h"

static bool _LoadSceneBinaryFromDataAndFile(const hg::Data &data, const char *name, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	return _LoadSceneBinaryFromData(data, name, scene, hg::g_file_reader, hg::g_file_read_provider, resources, pipeline, flags);
}
static bool _LoadSceneBinaryFromDataAndAssets(const hg::Data &data, const char *name, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	return _LoadSceneBinaryFromData(data, name, scene, hg::g_assets_reader, hg::g_assets_read_provider, resources, pipeline, flags);
}

static bool _LoadSceneJsonFromFile(const char *path, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneJsonFromFile(path, scene, resources, pipeline, ctx, flags);
}
static bool _LoadSceneJsonFromAssets(const char *name, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneJsonFromAssets(name, scene, resources, pipeline, ctx, flags);
}
static bool _LoadSceneJsonFromData(const hg::Data &data, const char *name, hg::Scene &scene, const hg::Reader &deps_ir, const hg::ReadProvider &deps_ip, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneJsonFromData(data, name, scene, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

static bool _LoadSceneFromFile(const char *path, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneFromFile(path, scene, resources, pipeline, ctx, flags);
}
static bool _LoadSceneFromAssets(const char *name, hg::Scene &scene, hg::PipelineResources &resources, const hg::PipelineInfo &pipeline, uint32_t flags = LSSF_All) {
	hg::LoadSceneContext ctx;
	return hg::LoadSceneFromAssets(name, scene, resources, pipeline, ctx, flags);
}
''')

	gen.bind_function('_LoadSceneBinaryFromFile', 'bool', ['const char *path', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneBinaryFromFile')
	gen.bind_function('_LoadSceneBinaryFromAssets', 'bool', ['const char *name', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneBinaryFromAssets')
	gen.bind_function('_LoadSceneJsonFromFile', 'bool', ['const char *path', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneJsonFromFile')
	gen.bind_function('_LoadSceneJsonFromAssets', 'bool', ['const char *name', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneJsonFromAssets')

	gen.bind_function('_LoadSceneBinaryFromDataAndFile', 'bool', ['const hg::Data &data', 'const char *name', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneBinaryFromDataAndFile')
	gen.bind_function('_LoadSceneBinaryFromDataAndAssets', 'bool', ['const hg::Data &data', 'const char *name', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneBinaryFromDataAndAssets')

	gen.bind_function('_LoadSceneFromFile', 'bool', ['const char *path', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneFromFile')
	gen.bind_function('_LoadSceneFromAssets', 'bool', ['const char *name', 'hg::Scene &scene', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline', '?uint32_t flags'], {'constants_group': {'flags': 'LoadSaveSceneFlags'}}, bound_name = 'LoadSceneFromAssets')

	# duplicate
	gen.bind_function('hg::DuplicateNodesFromFile', 'std::vector<hg::Node>', ['hg::Scene &scene', 'const std::vector<hg::Node> &nodes', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodesFromAssets', 'std::vector<hg::Node>', ['hg::Scene &scene', 'const std::vector<hg::Node> &nodes', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodesAndChildrenFromFile', 'std::vector<hg::Node>', ['hg::Scene &scene', 'const std::vector<hg::Node> &nodes', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodesAndChildrenFromAssets', 'std::vector<hg::Node>', ['hg::Scene &scene', 'const std::vector<hg::Node> &nodes', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])

	gen.bind_function('hg::DuplicateNodeFromFile', 'hg::Node', ['hg::Scene &scene', 'hg::Node node', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodeFromAssets', 'hg::Node', ['hg::Scene &scene', 'hg::Node node', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodeAndChildrenFromFile', 'std::vector<hg::Node>', ['hg::Scene &scene', 'hg::Node node', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])
	gen.bind_function('hg::DuplicateNodeAndChildrenFromAssets', 'std::vector<hg::Node>', ['hg::Scene &scene', 'hg::Node node', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'])

	# scene forward pipeline
	gen.add_include('engine/scene_forward_pipeline.h')

	gen.bind_function('GetSceneForwardPipelineFog', 'hg::ForwardPipelineFog', ['const hg::Scene &scene'])

	gen.insert_binding_code('''\
static std::vector<hg::ForwardPipelineLight> _GetSceneForwardPipelineLights(const hg::Scene &scene) {
	std::vector<hg::ForwardPipelineLight> lights;
	hg::GetSceneForwardPipelineLights(scene, lights);
	return lights;
}
''')

	gen.bind_function('GetSceneForwardPipelineLights', 'std::vector<hg::ForwardPipelineLight>', ['const hg::Scene &scene'], {'route': route_lambda('_GetSceneForwardPipelineLights')})

	#

	# SceneForwardPipelinePass
	gen.bind_named_enum('hg::SceneForwardPipelinePass', ['SFPP_Opaque', 'SFPP_Transparent', 'SFPP_Slot0LinearSplit0', 'SFPP_Slot0LinearSplit1', 'SFPP_Slot0LinearSplit2', 'SFPP_Slot0LinearSplit3', 'SFPP_Slot1Spot', 'SFPP_DepthPrepass'])

	sfppo = gen.begin_class('hg::SceneForwardPipelinePassViewId')
	gen.bind_constructor(sfppo, [])
	gen.end_class(sfppo)

	gen.bind_function('hg::GetSceneForwardPipelinePassViewId', 'bgfx::ViewId', ['const hg::SceneForwardPipelinePassViewId &views', 'hg::SceneForwardPipelinePass pass'])

	#
	render_data = gen.begin_class('hg::SceneForwardPipelineRenderData')
	gen.bind_constructor(render_data, [])
	gen.end_class(render_data)

	gen.bind_function('hg::PrepareSceneForwardPipelineCommonRenderData', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'hg::SceneForwardPipelineRenderData &render_data',
	'const hg::ForwardPipeline &pipeline', 'const hg::PipelineResources &resources', 'hg::SceneForwardPipelinePassViewId &views', '?const char *debug_name'], {'arg_in_out': ['view_id','views']})

	gen.bind_function('hg::PrepareSceneForwardPipelineViewDependentRenderData', 'void', ['bgfx::ViewId &view_id', 'const hg::ViewState &view_state', 'const hg::Scene &scene', 'hg::SceneForwardPipelineRenderData &render_data',
	'const hg::ForwardPipeline &pipeline', 'const hg::PipelineResources &resources', 'hg::SceneForwardPipelinePassViewId &views', '?const char *debug_name'], {'arg_in_out': ['view_id','views']})

	gen.bind_function('hg::SubmitSceneToForwardPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'const hg::ViewState &view_state',
	'const hg::ForwardPipeline &pipeline', 'const hg::SceneForwardPipelineRenderData &render_data', 'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', '?bgfx::FrameBufferHandle frame_buffer',
	'?const char *debug_name'], {'arg_in_out': ['view_id'], 'arg_out': ['views']})

	gen.bind_function('hg::SubmitSceneToPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'const hg::ViewState &view_state', 'const hg::ForwardPipeline &pipeline',
	'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', '?bgfx::FrameBufferHandle fb', '?const char *debug_name'], {'arg_in_out': ['view_id'], 'arg_out': ['views']})

	gen.bind_function('hg::SubmitSceneToPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'bool fov_axis_is_horizontal', 'const hg::ForwardPipeline &pipeline',
	'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', '?bgfx::FrameBufferHandle fb', '?const char *debug_name'], {'arg_in_out': ['view_id'], 'arg_out': ['views']})

	# reverse bind
	gen.rbind_function('OnCollision', 'void', ['const hg::Node &a', 'const hg::Node &b', 'const std::vector<hg::Contact> &contacts'])

	gen.rbind_function('OnUpdate_NodeCtx', 'void', ['hg::Node &node', 'hg::time_ns dt'])
	gen.rbind_function('OnAttachToNode', 'void', ['hg::Node &node'])
	gen.rbind_function('OnDetachFromNode', 'void', ['hg::Node &node'])

	gen.rbind_function('OnUpdate_SceneCtx', 'void', ['hg::Scene &scene', 'hg::time_ns dt'])
	gen.rbind_function('OnAttachToScene', 'void', ['hg::Scene &scene'])
	gen.rbind_function('OnDetachFromScene', 'void', ['hg::Scene &scene'])

	gen.rbind_function('OnDestroy', 'void', [])

	gen.insert_code('#include "engine/scene_forward_pipeline.h"\n')

	gen.rbind_function('OnSubmitSceneToForwardPipeline', 'void', ['bgfx::ViewId view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'const hg::ViewState &view_state', 'const hg::ForwardPipeline &pipeline', 'const hg::PipelineResources &resources', 'bgfx::FrameBufferHandle fb'])

	# ForwardPipelineAAA
	gen.bind_named_enum('hg::ForwardPipelineAAADebugBuffer', ['FPAAADB_None', 'FPAAADB_SSGI', 'FPAAADB_SSR'])
		
	forward_pipeline_aaa_config = gen.begin_class('hg::ForwardPipelineAAAConfig')
	gen.bind_constructor(forward_pipeline_aaa_config, [])
	gen.bind_members(forward_pipeline_aaa_config, [
		'float temporal_aa_weight',
		'int sample_count', 'float max_distance', 'float z_thickness',
		'float bloom_threshold', 'float bloom_bias', 'float bloom_intensity',
		'float motion_blur',
		'float exposure', 'float gamma'
	])
	gen.end_class(forward_pipeline_aaa_config)

	gen.bind_function('hg::LoadForwardPipelineAAAConfigFromFile', 'bool', ['const char *path', 'hg::ForwardPipelineAAAConfig &config'])
	gen.bind_function('hg::LoadForwardPipelineAAAConfigFromAssets', 'bool', ['const char *path', 'hg::ForwardPipelineAAAConfig &config'])
	gen.bind_function('hg::SaveForwardPipelineAAAConfigToFile', 'bool', ['const char *path', 'const hg::ForwardPipelineAAAConfig &config'])

	forward_pipeline_aaa = gen.begin_class('hg::ForwardPipelineAAA')
	gen.bind_method(forward_pipeline_aaa, 'Flip', 'void', ['const hg::ViewState &view_state'])
	gen.end_class(forward_pipeline_aaa)

	gen.bind_function('hg::CreateForwardPipelineAAAFromFile', 'hg::ForwardPipelineAAA', ['const char *path', 'const hg::ForwardPipelineAAAConfig &config', '?bgfx::BackbufferRatio::Enum ssgi_ratio', '?bgfx::BackbufferRatio::Enum ssr_ratio'])
	gen.bind_function('hg::CreateForwardPipelineAAAFromAssets', 'hg::ForwardPipelineAAA', ['const char *path', 'const hg::ForwardPipelineAAAConfig &config', '?bgfx::BackbufferRatio::Enum ssgi_ratio', '?bgfx::BackbufferRatio::Enum ssr_ratio'])
	gen.bind_function('hg::DestroyForwardPipelineAAA', 'void', ['hg::ForwardPipelineAAA &pipeline'])
	gen.bind_function('hg::IsValid', 'bool', ['const hg::ForwardPipelineAAA &pipeline'])

	gen.bind_function('hg::SubmitSceneToForwardPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'const hg::ViewState &view_state',
	'const hg::ForwardPipeline &pipeline', 'const hg::SceneForwardPipelineRenderData &render_data', 'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', 
	'const hg::ForwardPipelineAAA &aaa', 'const hg::ForwardPipelineAAAConfig &aaa_config', 'int frame', '?bgfx::FrameBufferHandle frame_buffer', '?const char *debug_name'], {'arg_in_out': ['view_id']})

	#
	gen.bind_function('hg::SubmitSceneToPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'const hg::ViewState &view_state',
	'const hg::ForwardPipeline &pipeline', 'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', 
	'hg::ForwardPipelineAAA &aaa', 'const hg::ForwardPipelineAAAConfig &aaa_config', 'int frame', '?bgfx::FrameBufferHandle frame_buffer', '?const char *debug_name'], {'arg_in_out': ['view_id'], 'arg_out': ['views']})

	gen.bind_function('hg::SubmitSceneToPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Scene &scene', 'const hg::Rect<int> &rect', 'bool fov_axis_is_horizontal',
	'const hg::ForwardPipeline &pipeline', 'const hg::PipelineResources &resources', 'const hg::SceneForwardPipelinePassViewId &views', 
	'hg::ForwardPipelineAAA &aaa', 'const hg::ForwardPipelineAAAConfig &aaa_config', 'int frame', '?bgfx::FrameBufferHandle frame_buffer', '?const char *debug_name'], {'arg_in_out': ['view_id'], 'arg_out': ['views']})

	#
	gen.add_include('engine/debugger.h')
	gen.bind_function('hg::DebugSceneExplorer', 'void', ['hg::Scene &scene', 'const char *name'])


def bind_bullet3_physics(gen):
	gen.add_include('engine/scene_bullet3_physics.h')

	node_node_contacts = gen.begin_class('hg::NodePairContacts')
	gen.end_class(node_node_contacts)

	bullet = gen.begin_class('hg::SceneBullet3Physics', noncopyable=True)

	gen.bind_constructor(bullet, ['?int thread_count'])

	gen.bind_method(bullet, 'SceneCreatePhysicsFromFile', 'void', ['const hg::Scene &scene'])
	gen.bind_method(bullet, 'SceneCreatePhysicsFromAssets', 'void', ['const hg::Scene &scene'])

	gen.bind_method(bullet, 'NodeCreatePhysicsFromFile', 'void', ['const hg::Node &node'])
	gen.bind_method(bullet, 'NodeCreatePhysicsFromAssets', 'void', ['const hg::Node &node'])

	gen.bind_method(bullet, 'NodeStartTrackingCollisionEvents', 'void', ['const hg::Node &node', '?hg::CollisionEventTrackingMode mode'])
	gen.bind_method(bullet, 'NodeStopTrackingCollisionEvents', 'void', ['const hg::Node &node'])

	gen.bind_method(bullet, 'NodeDestroyPhysics', 'void', ['const hg::Node &node'])

	gen.bind_method(bullet, 'NodeHasBody', 'bool', ['const hg::Node &node'])

	gen.bind_method(bullet, 'StepSimulation', 'void', ['hg::time_ns display_dt', '?hg::time_ns step_dt', '?int max_step'])

	gen.bind_method(bullet, 'CollectCollisionEvents', 'void', ['const hg::Scene &scene', 'hg::NodePairContacts &node_pair_contacts'])

	gen.bind_method(bullet, 'SyncTransformsFromScene', 'void', ['const hg::Scene &scene'])
	gen.bind_method(bullet, 'SyncTransformsToScene', 'void', ['hg::Scene &scene'])

	gen.bind_method(bullet, 'GarbageCollect', 'size_t', ['const hg::Scene &scene'])
	gen.bind_method(bullet, 'GarbageCollectResources', 'size_t', [])

	gen.bind_method(bullet, 'ClearNodes', 'void', [])
	gen.bind_method(bullet, 'Clear', 'void', [])

	#
	gen.bind_method(bullet, 'NodeWake', 'void', ['const hg::Node &node'])

	gen.bind_method(bullet, 'NodeSetDeactivation', 'void', ['const hg::Node &node', 'bool enable'])
	gen.bind_method(bullet, 'NodeGetDeactivation', 'bool', ['const hg::Node &node'])

	gen.bind_method(bullet, 'NodeResetWorld', 'void', ['const hg::Node &node', 'const hg::Mat4 &world'])
	gen.bind_method(bullet, 'NodeTeleport', 'void', ['const hg::Node &node', 'const hg::Mat4 &world'])

	gen.bind_method(bullet, 'NodeAddForce', 'void', ['const hg::Node &node', 'const hg::Vec3 &F', '?const hg::Vec3 &world_pos'])
	gen.bind_method(bullet, 'NodeAddImpulse', 'void', ['const hg::Node &node', 'const hg::Vec3 &dt_velocity', '?const hg::Vec3 &world_pos'])
	gen.bind_method(bullet, 'NodeAddTorque', 'void', ['const hg::Node &node', 'const hg::Vec3 &T'])
	gen.bind_method(bullet, 'NodeAddTorqueImpulse', 'void', ['const hg::Node &node', 'const hg::Vec3 &dt_angular_velocity'])
	gen.bind_method(bullet, 'NodeGetPointVelocity', 'hg::Vec3', ['const hg::Node &node', 'const hg::Vec3 &world_pos'])

	gen.bind_method(bullet, 'NodeGetLinearVelocity', 'hg::Vec3', ['const hg::Node &node'])
	gen.bind_method(bullet, 'NodeSetLinearVelocity', 'void', ['const hg::Node &node', 'const hg::Vec3 &V'])
	gen.bind_method(bullet, 'NodeGetAngularVelocity', 'hg::Vec3', ['const hg::Node &node'])
	gen.bind_method(bullet, 'NodeSetAngularVelocity', 'void', ['const hg::Node &node', 'const hg::Vec3 &W'])

	gen.bind_method(bullet, 'NodeGetLinearFactor', 'hg::Vec3', ['const hg::Node &node'])
	gen.bind_method(bullet, 'NodeSetLinearFactor', 'void', ['const hg::Node &node', 'const hg::Vec3 &k'])
	gen.bind_method(bullet, 'NodeGetAngularFactor', 'hg::Vec3', ['const hg::Node &node'])
	gen.bind_method(bullet, 'NodeSetAngularFactor', 'void', ['const hg::Node &node', 'const hg::Vec3 &k'])

	#
	gen.bind_method(bullet, 'NodeCollideWorld', 'hg::NodePairContacts', ['const hg::Node &node', 'const hg::Mat4 &world', '?int max_contact'])

	gen.bind_function('GetNodesInContact', 'std::vector<hg::Node>', ['const hg::Scene &scene', 'const hg::Node with', 'const hg::NodePairContacts &node_pair_contacts'])
	gen.bind_function('GetNodePairContacts', 'std::vector<hg::Contact>', ['const hg::Node &first', 'const hg::Node &second', 'const hg::NodePairContacts &node_pair_contacts'])

	#
	gen.bind_method(bullet, 'RaycastFirstHit', 'hg::RaycastOut', ['const hg::Scene &scene', 'const hg::Vec3 &p0', 'const hg::Vec3 &p1'])
	gen.bind_method(bullet, 'RaycastAllHits', 'std::vector<hg::RaycastOut>', ['const hg::Scene &scene', 'const hg::Vec3 &p0', 'const hg::Vec3 &p1'])

	#
	gen.bind_method(bullet, 'RenderCollision', 'void', ['bgfx::ViewId view_id', 'const bgfx::VertexLayout &vtx_layout', 'bgfx::ProgramHandle prg', 'hg::RenderState render_state', 'uint32_t depth'])

	gen.end_class(bullet)


def bind_lua_scene_vm(gen):
	gen.add_include('engine/scene_lua_vm.h')

	vm = gen.begin_class('hg::SceneLuaVM')
	gen.bind_constructor(vm, [])

	gen.bind_method(vm, 'CreateScriptFromSource', 'bool', ['hg::Scene &scene', 'const hg::Script &script', 'const std::string &src'])

	gen.bind_method(vm, 'CreateScriptFromFile', 'bool', ['hg::Scene &scene', 'const hg::Script &script'])
	gen.bind_method(vm, 'CreateScriptFromAssets', 'bool', ['hg::Scene &scene', 'const hg::Script &script'])

	gen.bind_method(vm, 'CreateNodeScriptsFromFile', 'std::vector<hg::Script>', ['hg::Scene &scene', 'const hg::Node &node'])
	gen.bind_method(vm, 'CreateNodeScriptsFromAssets', 'std::vector<hg::Script>', ['hg::Scene &scene', 'const hg::Node &node'])

	gen.insert_binding_code('''
static std::vector<hg::Script> __LuaVM_SceneCreateScriptsFromFile(hg::SceneLuaVM *vm, hg::Scene &scene) {
	const auto refs = vm->SceneCreateScriptsFromFile(scene);

	std::vector<hg::Script> scripts;
	scripts.reserve(refs.size());

	for (auto ref : refs)
		scripts.push_back(scene.GetScript(ref));

	return scripts;
}

static std::vector<hg::Script> __LuaVM_SceneCreateScriptsFromAssets(hg::SceneLuaVM *vm, hg::Scene &scene) {
	const auto refs = vm->SceneCreateScriptsFromAssets(scene);

	std::vector<hg::Script> scripts;
	scripts.reserve(refs.size());

	for (auto ref : refs)
		scripts.push_back(scene.GetScript(ref));

	return scripts;
}

static std::vector<hg::Script> __LuaVM_GarbageCollect(hg::SceneLuaVM *vm, const hg::Scene &scene) {
	const auto refs = vm->GarbageCollect(scene);

	std::vector<hg::Script> scripts;
	scripts.reserve(refs.size());

	for (auto ref : refs)
		scripts.push_back(scene.GetScript(ref));

	return scripts;
}

static void __LuaVM_DestroyScripts(hg::SceneLuaVM *vm, const std::vector<hg::Script> &scripts) {
	std::vector<hg::ComponentRef> refs;
	refs.reserve(scripts.size());

	for (auto &script : scripts)
		refs.push_back(script.ref);

	vm->DestroyScripts(refs);
}
''')

	gen.bind_method(vm, 'SceneCreateScriptsFromFile', 'std::vector<hg::Script>', ['hg::Scene &scene'], {'route': route_lambda('__LuaVM_SceneCreateScriptsFromFile')})
	gen.bind_method(vm, 'SceneCreateScriptsFromAssets', 'std::vector<hg::Script>', ['hg::Scene &scene'], {'route': route_lambda('__LuaVM_SceneCreateScriptsFromAssets')})
	gen.bind_method(vm, 'GarbageCollect', 'std::vector<hg::Script>', ['const hg::Scene &scene'], {'route': route_lambda('__LuaVM_GarbageCollect')})
	gen.bind_method(vm, 'DestroyScripts', 'void', ['const std::vector<hg::Script> &scripts'], {'route': route_lambda('__LuaVM_DestroyScripts')})

	gen.bind_method(vm, 'GetScriptInterface', 'std::vector<std::string>', ['const hg::Script &script'])
	gen.bind_method(vm, 'GetScriptCount', 'size_t', [])

	gen.insert_binding_code('''
static hg::LuaObject __LuaVM_GetScriptEnv(hg::SceneLuaVM *vm, const hg::Script &script) { return vm->GetScriptEnv(script.ref); }

static hg::LuaObject __LuaVM_GetScriptValue(hg::SceneLuaVM *vm, const hg::Script &script, const std::string &name) { return vm->GetScriptValue(script.ref, name); }
static bool __LuaVM_SetScriptValue(hg::SceneLuaVM *vm, const hg::Script &script, const std::string &name, const hg::LuaObject &value) { return vm->SetScriptValue(script.ref, name, value); }
''')

	gen.bind_method(vm, 'GetScriptEnv', 'hg::LuaObject', ['const hg::Script &script'], {'route': route_lambda('__LuaVM_GetScriptEnv')})

	gen.bind_method(vm, 'GetScriptValue', 'hg::LuaObject', ['const hg::Script &script', 'const std::string &name'], {'route': route_lambda('__LuaVM_GetScriptValue')})
	gen.bind_method(vm, 'SetScriptValue', 'bool', ['const hg::Script &script', 'const std::string &name', 'const hg::LuaObject &value'], {'route': route_lambda('__LuaVM_SetScriptValue')})

	gen.bind_method_overloads(vm, 'Call', expand_std_vector_proto(gen, [
		('bool', ['const hg::Script &script', 'const std::string &function', 'const std::vector<hg::LuaObject> &args', 'std::vector<hg::LuaObject> *ret_vals'], {'arg_out': ['ret_vals']})
	]))

	gen.bind_method(vm, 'MakeLuaObject', 'hg::LuaObject', [])

	#
	if gen.get_language() == 'CPython':
		gen.insert_binding_code('''
extern "C" {
#include "lua.h"
}

// from bind_Lua.h
struct hg_lua_type_info {
	uint32_t type_tag;
	const char *c_type;
	const char *bound_name;

	bool (*check)(lua_State *L, int index);
	void (*to_c)(lua_State *L, int index, void *out);
	int (*from_c)(lua_State *L, void *obj, OwnershipPolicy policy);
};

// return a type info from its type tag
hg_lua_type_info *hg_lua_get_bound_type_info(uint32_t type_tag);
// return a type info from its type name
hg_lua_type_info *hg_lua_get_c_type_info(const char *type);
// returns the typetag of a userdata object on the stack, nullptr if not a Fabgen object
uint32_t hg_lua_get_wrapped_object_type_tag(lua_State *L, int idx);

static hg::LuaObject __PyObjectToLuaObject(hg::SceneLuaVM *vm, PyObject *o) {
	lua_State *L = vm->GetL();

	if (PyLong_CheckExact(o)) {
		lua_pushinteger(L, PyLong_AsLong(o));
	} else if (PyFloat_CheckExact(o)) {
		lua_pushnumber(L, PyLong_AsDouble(o));
	} else if (o == Py_False) {
		lua_pushboolean(L, false);
	} else if (o == Py_True) {
		lua_pushboolean(L, true);
	} else if (PyUnicode_Check(o)) {
		PyObject *utf8_pyobj = PyUnicode_AsUTF8String(o);
		const std::string v = PyBytes_AsString(utf8_pyobj);
		Py_DECREF(utf8_pyobj);
		lua_pushstring(L, v.data());
	} else if (auto w = cast_to_wrapped_Object_safe(o)) {
		const auto py_info = gen_get_bound_type_info(w->type_tag);
		const auto lua_info = hg_lua_get_bound_type_info(w->type_tag);

		if (py_info && lua_info) {
			void *obj;
			py_info->to_c(o, &obj);
			__ASSERT__(obj != nullptr);
			lua_info->from_c(L, obj, Copy);
		} else {
			lua_pushnil(L);
			PyErr_SetString(PyExc_TypeError, "Cannot transfer this object type from CPython to Lua");
		}
	} else {
		lua_pushnil(L);
		PyErr_SetString(PyExc_TypeError, "Cannot transfer this object type from CPython to Lua");
	}

	return hg::Pop(L);
}

static PyObject *__LuaObjectToPyObject(hg::SceneLuaVM *vm, const hg::LuaObject &lua_obj) {
	auto L = lua_obj.L();

	lua_obj.Push();

	if (lua_isinteger(L, -1)) {
		const auto v = lua_tointeger(L, -1);
		lua_pop(L, 1);
		return PyLong_FromLongLong(v);
	}

	if (lua_isnumber(L, -1)) {
		const auto v = lua_tonumber(L, -1);
		lua_pop(L, 1);
		return PyFloat_FromDouble(v);
	}

	if (lua_isboolean(L, -1)) {
		const auto v = lua_toboolean(L, -1);
		lua_pop(L, 1);
		if (v == 1)
			Py_RETURN_TRUE;
		Py_RETURN_FALSE;
	}

	if (lua_isstring(L, -1)) {
		const std::string v = lua_tostring(L, -1);
		lua_pop(L, 1);
		return PyUnicode_DecodeUTF8(v.c_str(), v.size(), nullptr);
	}

	if (auto type_tag = hg_lua_get_wrapped_object_type_tag(L, -1)) {
		const auto py_info = gen_get_bound_type_info(type_tag);
		const auto lua_info = hg_lua_get_bound_type_info(type_tag);

		if (py_info && lua_info) {
			void *obj;
			lua_info->to_c(L, -1, &obj);
			__ASSERT__(obj != nullptr);
			return py_info->from_c(obj, Copy);
		} else {
			PyErr_SetString(PyExc_TypeError, "Cannot transfer this object type from Lua to CPython");
		}
	}

	Py_RETURN_NONE;
}
''')

		gen.bind_method(vm, 'Pack', 'hg::LuaObject', ['PyObject *o'], {'route': route_lambda('__PyObjectToLuaObject')})
		gen.bind_method(vm, 'Unpack', 'PyObject *', ['const hg::LuaObject &o'], {'route': route_lambda('__LuaObjectToPyObject')})

	gen.end_class(vm)


def bind_scene_systems(gen):
	gen.add_include('engine/scene_systems.h')

	scene_clocks = gen.begin_class('hg::SceneClocks')
	gen.bind_constructor(scene_clocks, [])
	gen.end_class(scene_clocks)

	scene_sync_to_systems_from_file_protos = [
		('void', ['hg::Scene &scene', 'hg::SceneLuaVM &vm'], []),
	]
	scene_sync_to_systems_from_assets_protos = [
		('void', ['hg::Scene &scene', 'hg::SceneLuaVM &vm'], []),
	]
	scene_update_systems_protos = [
		('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt'], []),
		('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt', 'hg::SceneLuaVM &vm'], []),
	]
	scene_garbage_collect_systems_protos = [
		('size_t', ['hg::Scene &scene'], []),
		('size_t', ['hg::Scene &scene', 'hg::SceneLuaVM &vm'], []),
	]
	scene_clear_systems_protos = [
		('void', ['hg::Scene &scene'], []),
		('void', ['hg::Scene &scene', 'hg::SceneLuaVM &vm'], []),
	]

	if gen.defined('HG_ENABLE_BULLET3_SCENE_PHYSICS'):
		scene_sync_to_systems_from_file_protos.extend([
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics'], []),
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics', 'hg::SceneLuaVM &vm'], []),
		])
		scene_sync_to_systems_from_assets_protos.extend([
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics'], []),
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics', 'hg::SceneLuaVM &vm'], []),
		])
		scene_update_systems_protos.extend([
			('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt', 'hg::SceneBullet3Physics &physics', 'hg::time_ns step', 'int max_physics_step'], []),
			('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt', 'hg::SceneBullet3Physics &physics', 'hg::time_ns step', 'int max_physics_step', 'hg::SceneLuaVM &vm'], []),
			('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt', 'hg::SceneBullet3Physics &physics', 'hg::NodePairContacts &contacts', 'hg::time_ns step', 'int max_physics_step'], []),
			('void', ['hg::Scene &scene', 'hg::SceneClocks &clocks', 'hg::time_ns dt', 'hg::SceneBullet3Physics &physics', 'hg::NodePairContacts &contacts', 'hg::time_ns step', 'int max_physics_step', 'hg::SceneLuaVM &vm'], []),
		])
		scene_garbage_collect_systems_protos.extend([
			('size_t', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics'], []),
			('size_t', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics', 'hg::SceneLuaVM &vm'], []),
		])
		scene_clear_systems_protos.extend([
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics'], []),
			('void', ['hg::Scene &scene', 'hg::SceneBullet3Physics &physics', 'hg::SceneLuaVM &vm'], []),
		])

	gen.bind_function_overloads('hg::SceneSyncToSystemsFromFile', scene_sync_to_systems_from_file_protos)
	gen.bind_function_overloads('hg::SceneSyncToSystemsFromAssets', scene_sync_to_systems_from_assets_protos)
	gen.bind_function_overloads('hg::SceneUpdateSystems', scene_update_systems_protos)
	gen.bind_function_overloads('hg::SceneGarbageCollectSystems', scene_garbage_collect_systems_protos)
	gen.bind_function_overloads('hg::SceneClearSystems', scene_clear_systems_protos)


def bind_font(gen):
	gen.add_include('engine/font.h')
	
	font = gen.begin_class('hg::Font')
	gen.end_class(font)

	gen.bind_function('hg::LoadFontFromFile', 'hg::Font', ['const char *path', '?float size', '?uint16_t resolution', '?int padding', '?const char *glyphs'])
	gen.bind_function('hg::LoadFontFromAssets', 'hg::Font', ['const char *name', '?float size', '?uint16_t resolution', '?int padding', '?const char *glyphs'])

	gen.bind_named_enum('hg::DrawTextHAlign', ['DTHA_Left', 'DTHA_Center', 'DTHA_Right'])
	gen.bind_named_enum('hg::DrawTextVAlign', ['DTVA_Top', 'DTVA_Center', 'DTVA_Bottom'])

	gen.bind_function_overloads('hg::DrawText', expand_std_vector_proto(gen, [
		('void', ['bgfx::ViewId view_id', 'const hg::Font &font', 'const char *text', 'bgfx::ProgramHandle prg', 'const char *page_uniform',
		'uint8_t page_stage', 'const hg::Mat4 &mtx'], []),

		('void', ['bgfx::ViewId view_id', 'const hg::Font &font', 'const char *text', 'bgfx::ProgramHandle prg', 'const char *page_uniform',
		'uint8_t page_stage', 'const hg::Mat4 &mtx', 'hg::Vec3 pos'], []),

		('void', ['bgfx::ViewId view_id', 'const hg::Font &font', 'const char *text', 'bgfx::ProgramHandle prg', 'const char *page_uniform',
		'uint8_t page_stage', 'const hg::Mat4 &mtx', 'hg::Vec3 pos', 'hg::DrawTextHAlign halign', 'hg::DrawTextVAlign valign'], []),

		('void', ['bgfx::ViewId view_id', 'const hg::Font &font', 'const char *text', 'bgfx::ProgramHandle prg', 'const char *page_uniform',
		'uint8_t page_stage', 'const hg::Mat4 &mtx', 'hg::Vec3 pos', 'hg::DrawTextHAlign halign', 'hg::DrawTextVAlign valign', 'const std::vector<hg::UniformSetValue> &values',
		'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState state', '?uint32_t depth'], [])
	]))

	gen.bind_function('hg::ComputeTextRect', 'hg::Rect<float>', ['const hg::Font &font', 'const char *text', '?float xpos', '?float ypos'])
	gen.bind_function('hg::ComputeTextHeight', 'float', ['const hg::Font &font', 'const char *text'])


def bind_meta(gen):
	gen.add_include('engine/meta.h')
	gen.add_include('json.hpp')

	json = gen.begin_class('hg::json', bound_name='JSON')
	gen.bind_constructor(json, [])
	gen.end_class(json)

	gen.bind_function('hg::LoadJsonFromFile', 'hg::json', ['const char *path'])
	gen.bind_function('hg::LoadJsonFromAssets', 'hg::json', ['const char *name'])

	gen.bind_function('hg::SaveJsonToFile', 'bool', ['const hg::json &js', 'const char *path'])

	gen.bind_function('GetJsonString', 'bool', ['const hg::json &js', 'const std::string &key', 'std::string &value'], {'arg_out': ['value'], 'route': route_lambda('hg::GetJsonValue')})
	gen.bind_function('GetJsonBool', 'bool', ['const hg::json &js', 'const std::string &key', 'bool &value'], {'arg_out': ['value'], 'route': route_lambda('hg::GetJsonValue')})
	gen.bind_function('GetJsonInt', 'bool', ['const hg::json &js', 'const std::string &key', 'int &value'], {'arg_out': ['value'], 'route': route_lambda('hg::GetJsonValue')})
	gen.bind_function('GetJsonFloat', 'bool', ['const hg::json &js', 'const std::string &key', 'float &value'], {'arg_out': ['value'], 'route': route_lambda('hg::GetJsonValue')})

	gen.bind_function_overloads('hg::SetJsonValue', [
		('void', ['hg::json &js', 'const std::string &key', 'const std::string &value'], {}),
		('void', ['hg::json &js', 'const std::string &key', 'bool value'], {}),
		('void', ['hg::json &js', 'const std::string &key', 'int value'], {}),
		('void', ['hg::json &js', 'const std::string &key', 'float value'], {})
	])


def bind_render(gen):
	gen.add_include('engine/render_pipeline.h')

	gen.bind_named_enum('bgfx::RendererType::Enum', [
		'Noop', 'Direct3D9', 'Direct3D11', 'Direct3D12', 'Gnm', 'Metal', 'Nvn', 'OpenGLES', 'OpenGL', 'Vulkan', 'Count'
	], bound_name='RendererType', prefix='RT_')

	gen.bind_function_overloads('hg::RenderInit', [
		('bool', ['hg::Window *window'], []),
		('bool', ['hg::Window *window', 'bgfx::RendererType::Enum type'], []),
		('hg::Window *', ['int width', 'int height', 'uint32_t reset_flags', '?bgfx::TextureFormat::Enum format', '?uint32_t debug_flags'], {'constants_group': {'reset_flags': 'ResetFlags', 'debug_flags': 'DebugFlags'}}),
		('hg::Window *', ['int width', 'int height', 'bgfx::RendererType::Enum type', '?uint32_t reset_flags', '?bgfx::TextureFormat::Enum format', '?uint32_t debug_flags'], {'constants_group': {'reset_flags': 'ResetFlags', 'debug_flags': 'DebugFlags'}}),
		('hg::Window *', ['const char *window_title', 'int width', 'int height', 'uint32_t reset_flags', '?bgfx::TextureFormat::Enum format', '?uint32_t debug_flags'], {'constants_group': {'reset_flags': 'ResetFlags', 'debug_flags': 'DebugFlags'}}),
		('hg::Window *', ['const char *window_title', 'int width', 'int height', 'bgfx::RendererType::Enum type', '?uint32_t reset_flags', '?bgfx::TextureFormat::Enum format', '?uint32_t debug_flags'], {'constants_group': {'reset_flags': 'ResetFlags', 'debug_flags': 'DebugFlags'}})
	])
	gen.bind_function('hg::RenderShutdown', 'void', [])

	gen.bind_function('hg::RenderResetToWindow', 'bool', ['hg::Window *win', 'int &width', 'int &height', '?uint32_t reset_flags'], {'arg_in_out': ['width', 'height']})

	gen.bind_named_enum('bgfx::TextureFormat::Enum', [
		'BC1', 'BC2', 'BC3', 'BC4', 'BC5', 'BC6H', 'BC7', 'ETC1', 'ETC2', 'ETC2A', 'ETC2A1', 'PTC12', 'PTC14', 'PTC12A', 'PTC14A', 'PTC22', 'PTC24',
		'ATC', 'ATCE', 'ATCI', 'ASTC4x4', 'ASTC5x5', 'ASTC6x6', 'ASTC8x5', 'ASTC8x6', 'ASTC10x5',
		'Unknown',
		'R1', 'A8', 'R8', 'R8I', 'R8U', 'R8S', 'R16', 'R16I', 'R16U', 'R16F', 'R16S', 'R32I', 'R32U', 'R32F', 'RG8', 'RG8I', 'RG8U', 'RG8S',
		'RG16', 'RG16I', 'RG16U', 'RG16F', 'RG16S', 'RG32I', 'RG32U', 'RG32F', 'RGB8', 'RGB8I', 'RGB8U', 'RGB8S', 'RGB9E5F', 'BGRA8', 'RGBA8',
		'RGBA8I', 'RGBA8U', 'RGBA8S', 'RGBA16', 'RGBA16I', 'RGBA16U', 'RGBA16F', 'RGBA16S', 'RGBA32I', 'RGBA32U', 'RGBA32F', 'R5G6B5', 'RGBA4',
		'RGB5A1', 'RGB10A2', 'RG11B10F',
		'UnknownDepth',
		'D16', 'D24', 'D24S8', 'D32', 'D16F', 'D24F', 'D32F', 'D0S8',
	], bound_name='TextureFormat', prefix='TF_')

	gen.bind_constants('uint32_t', [
		("RF_None", "BGFX_RESET_NONE"),
		("RF_MSAA2X", "BGFX_RESET_MSAA_X2"),
		("RF_MSAA4X", "BGFX_RESET_MSAA_X4"),
		("RF_MSAA8X", "BGFX_RESET_MSAA_X8"),
		("RF_MSAA16X", "BGFX_RESET_MSAA_X16"),
		("RF_VSync", "BGFX_RESET_VSYNC"),
		("RF_MaxAnisotropy", "BGFX_RESET_MAXANISOTROPY"),
		("RF_Capture", "BGFX_RESET_CAPTURE"),
		("RF_FlushAfterRender", "BGFX_RESET_FLUSH_AFTER_RENDER"),
		("RF_FlipAfterRender", "BGFX_RESET_FLIP_AFTER_RENDER"),
		("RF_SRGBBackBuffer", "BGFX_RESET_SRGB_BACKBUFFER"),
		("RF_HDR10", "BGFX_RESET_HDR10"),
		("RF_HiDPI", "BGFX_RESET_HIDPI"),
		("RF_DepthClamp", "BGFX_RESET_DEPTH_CLAMP"),
		("RF_Suspend", "BGFX_RESET_SUSPEND"),
	], 'ResetFlags')

	gen.bind_named_enum('bgfx::BackbufferRatio::Enum', ['Equal', 'Half', 'Quarter', 'Eighth', 'Sixteenth', 'Double'], bound_name='BackbufferRatio', prefix='BR_')
	
	gen.bind_function('bgfx::reset', 'void', ['uint32_t width', 'uint32_t height', '?uint32_t flags', '?bgfx::TextureFormat::Enum format'], {'constants_group': {'flags': 'ResetFlags'}}, bound_name='RenderReset')

	gen.bind_constants('uint32_t', [
		("DF_IFH", "BGFX_DEBUG_IFH"),
		("DF_Profiler", "BGFX_DEBUG_PROFILER"),
		("DF_Stats", "BGFX_DEBUG_STATS"),
		("DF_Text", "BGFX_DEBUG_TEXT"),
		("DF_Wireframe", "BGFX_DEBUG_WIREFRAME"),
	], 'DebugFlags')

	gen.bind_function('bgfx::setDebug', 'void', ['uint32_t flags'], {'constants_group': {'flags': 'DebugFlags'}}, bound_name='SetRenderDebug')

	# frame
	fbh = gen.begin_class('bgfx::FrameBufferHandle')
	gen.end_class(fbh)
	
	gen.bind_variable("const bgfx::FrameBufferHandle hg::InvalidFrameBufferHandle")

	gen.bind_constants('uint16_t', [
		("CF_None", "BGFX_CLEAR_NONE"),
		("CF_Color", "BGFX_CLEAR_COLOR"),
		("CF_Depth", "BGFX_CLEAR_DEPTH"),
		("CF_Stencil", "BGFX_CLEAR_STENCIL"),
		("CF_DiscardColor0", "BGFX_CLEAR_DISCARD_COLOR_0"),
		("CF_DiscardColor1", "BGFX_CLEAR_DISCARD_COLOR_1"),
		("CF_DiscardColor2", "BGFX_CLEAR_DISCARD_COLOR_2"),
		("CF_DiscardColor3", "BGFX_CLEAR_DISCARD_COLOR_3"),
		("CF_DiscardColor4", "BGFX_CLEAR_DISCARD_COLOR_4"),
		("CF_DiscardColor5", "BGFX_CLEAR_DISCARD_COLOR_5"),
		("CF_DiscardColor6", "BGFX_CLEAR_DISCARD_COLOR_6"),
		("CF_DiscardColor7", "BGFX_CLEAR_DISCARD_COLOR_7"),
		("CF_DiscardDepth", "BGFX_CLEAR_DISCARD_DEPTH"),
		("CF_DiscardStencil", "BGFX_CLEAR_DISCARD_STENCIL"),
		("CF_DiscardColorAll", "BGFX_CLEAR_DISCARD_COLOR_MASK"),
		("CF_DiscardAll", "BGFX_CLEAR_DISCARD_MASK"),
	], 'ClearFlags')

	gen.bind_named_enum('bgfx::ViewMode::Enum', ['Default', 'Sequential', 'DepthAscending', 'DepthDescending'], bound_name='ViewMode', prefix='VM_')

	gen.insert_binding_code('''\
static void _SetViewClear(bgfx::ViewId view_id, uint16_t clear_flags, const hg::Color &col, float depth = 1.f, uint8_t stencil = 0) {
	bgfx::setViewClear(view_id, clear_flags, hg::ColorToABGR32(col), depth, stencil);
}
''')

	gen.bind_function_overloads('bgfx::setViewClear', [
		('void', ['bgfx::ViewId view_id', 'uint16_t flags', '?uint32_t rgba', '?float depth', '?uint8_t stencil'], {'constants_group': {'flags': 'ClearFlags'}}),
		('void', ['bgfx::ViewId view_id', 'uint16_t flags', 'const hg::Color &col', '?float depth', '?uint8_t stencil'], {'route': route_lambda('_SetViewClear'), 'constants_group': {'flags': 'ClearFlags'}})
	], bound_name='SetViewClear')

	gen.bind_function('bgfx::setViewRect', 'void', ['bgfx::ViewId view_id', 'uint16_t x', 'uint16_t y', 'uint16_t w', 'uint16_t h'], bound_name='SetViewRect')
	gen.bind_function('bgfx::setViewFrameBuffer', 'void', ['bgfx::ViewId view_id', 'bgfx::FrameBufferHandle handle'], bound_name='SetViewFrameBuffer')
	gen.bind_function('bgfx::setViewMode', 'void', ['bgfx::ViewId view_id', 'bgfx::ViewMode::Enum mode'], bound_name='SetViewMode')

	gen.bind_function('bgfx::touch', 'void', ['bgfx::ViewId view_id'], bound_name='Touch')
	gen.bind_function('bgfx::frame', 'uint32_t', [], bound_name='Frame')

	gen.insert_binding_code('''\
static void _SetViewTransform(bgfx::ViewId view_id, const hg::Mat4 &view, const hg::Mat44 &proj) {
	bgfx::setViewTransform(view_id, hg::to_bgfx(view).data(), hg::to_bgfx(proj).data());
}
''')
	gen.bind_function('_SetViewTransform', 'void', ['bgfx::ViewId view_id', 'const hg::Mat4 &view', 'const hg::Mat44 &proj'], bound_name='SetViewTransform')

	gen.bind_function_overloads('hg::SetView2D', [
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'float znear', 'float zfar'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'float znear', 'float zfar', 'uint16_t flags', 'const hg::Color &color', 'float depth', 'uint8_t stencil', '?bool y_up'], {'constants_group': {'flags': 'ClearFlags'}})
	])
	gen.bind_function_overloads('hg::SetViewPerspective', [
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar', 'float zoom_factor'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar', 'float zoom_factor', 'uint16_t flags', 'const hg::Color &color', 'float depth', 'uint8_t stencil'], {'constants_group': {'flags': 'ClearFlags'}})
	])
	gen.bind_function_overloads('hg::SetViewOrthographic', [
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar', 'float size'], {}),
		('void', ['bgfx::ViewId id', 'int x', 'int y', 'int res_x', 'int res_y', 'const hg::Mat4 &world', 'float znear', 'float zfar', 'float size', 'uint16_t flags', 'const hg::Color &color', 'float depth', 'uint8_t stencil'], {'constants_group': {'flags': 'ClearFlags'}})
	])

	# vertex declaration
	gen.bind_named_enum('bgfx::Attrib::Enum', ['Position', 'Normal', 'Tangent', 'Bitangent', 'Color0', 'Color1', 'Color2', 'Color3', 'Indices', 'Weight',
						'TexCoord0', 'TexCoord1', 'TexCoord2', 'TexCoord3', 'TexCoord4', 'TexCoord5', 'TexCoord6', 'TexCoord7'], bound_name='Attrib', prefix='A_')
	gen.bind_named_enum('bgfx::AttribType::Enum', ['Uint8', 'Uint10', 'Int16', 'Half', 'Float'], bound_name='AttribType', prefix='AT_')

	vtx_decl = gen.begin_class('bgfx::VertexLayout')
	gen.bind_constructor(vtx_decl, [])
	gen.bind_method(vtx_decl, 'begin', 'bgfx::VertexLayout &', [], bound_name='Begin')
	gen.bind_method(vtx_decl, 'add', 'bgfx::VertexLayout &', ['bgfx::Attrib::Enum attrib', 'uint8_t count', 'bgfx::AttribType::Enum type', '?bool normalized', '?bool as_int'], bound_name='Add')
	gen.bind_method(vtx_decl, 'skip', 'bgfx::VertexLayout &', ['uint8_t size'], bound_name='Skip')
	gen.bind_method(vtx_decl, 'end', 'void', [], bound_name='End')
	gen.bind_method(vtx_decl, 'has', 'bool', ['bgfx::Attrib::Enum attrib'], bound_name='Has')
	gen.bind_method(vtx_decl, 'getOffset', 'uint16_t', ['bgfx::Attrib::Enum attrib'], bound_name='GetOffset')
	gen.bind_method(vtx_decl, 'getStride', 'uint16_t', [], bound_name='GetStride')
	gen.bind_method(vtx_decl, 'getSize', 'uint32_t', ['uint32_t count'], bound_name='GetSize')
	gen.end_class(vtx_decl)

	gen.bind_function('hg::VertexLayoutPosFloatNormFloat', 'bgfx::VertexLayout', [])
	gen.bind_function('hg::VertexLayoutPosFloatNormUInt8', 'bgfx::VertexLayout', [])
	gen.bind_function('hg::VertexLayoutPosFloatColorFloat', 'bgfx::VertexLayout', [])
	gen.bind_function('hg::VertexLayoutPosFloatColorUInt8', 'bgfx::VertexLayout', [])
	gen.bind_function('hg::VertexLayoutPosFloatTexCoord0UInt8', 'bgfx::VertexLayout', [])
	gen.bind_function('hg::VertexLayoutPosFloatNormUInt8TexCoord0UInt8', 'bgfx::VertexLayout', [])

	# program
	bgfx_program_handle = gen.begin_class('bgfx::ProgramHandle')
	bgfx_program_handle._inline = True
	gen.end_class(bgfx_program_handle)

	gen.bind_function_overloads('hg::LoadProgramFromFile', [
		('bgfx::ProgramHandle', ['const char *path'], []),
		('bgfx::ProgramHandle', ['const char *vertex_shader_path', 'const char *fragment_shader_path'], [])
	])
	gen.bind_function_overloads('hg::LoadProgramFromAssets', [
		('bgfx::ProgramHandle', ['const char *name'], []),
		('bgfx::ProgramHandle', ['const char *vertex_shader_name', 'const char *fragment_shader_name'], [])
	])
	
	gen.insert_binding_code('static void _DestroyProgram(bgfx::ProgramHandle h) { bgfx::destroy(h); }')
	gen.bind_function('DestroyProgram', 'void', ['bgfx::ProgramHandle h'], {'route': route_lambda('_DestroyProgram')})

	# TextureInfo
	texture_info = gen.begin_class('bgfx::TextureInfo')
	gen.bind_members(texture_info, ['bgfx::TextureFormat::Enum format', 'uint32_t storageSize',
		'uint16_t width', 'uint16_t height', 'uint16_t depth', 'uint16_t numLayers',
		'uint8_t numMips', 'uint8_t bitsPerPixel', 'bool cubeMap'
	])
	gen.end_class(texture_info)

	# texture flags
	gen.bind_constants('uint64_t', [
		('TF_UMirror', 'BGFX_SAMPLER_U_MIRROR'),
		('TF_UClamp', 'BGFX_SAMPLER_U_CLAMP'),
		('TF_UBorder', 'BGFX_SAMPLER_U_BORDER'),
		('TF_VMirror', 'BGFX_SAMPLER_V_MIRROR'),
		('TF_VClamp', 'BGFX_SAMPLER_V_CLAMP'),
		('TF_VBorder', 'BGFX_SAMPLER_V_BORDER'),
		('TF_WMirror', 'BGFX_SAMPLER_W_MIRROR'),
		('TF_WClamp', 'BGFX_SAMPLER_W_CLAMP'),
		('TF_WBorder', 'BGFX_SAMPLER_W_BORDER'),
		('TF_SamplerMinPoint', 'BGFX_SAMPLER_MIN_POINT'),
		('TF_SamplerMinAnisotropic', 'BGFX_SAMPLER_MIN_ANISOTROPIC'),
		('TF_SamplerMagPoint', 'BGFX_SAMPLER_MAG_POINT'),
		('TF_SamplerMagAnisotropic', 'BGFX_SAMPLER_MAG_ANISOTROPIC'),
		('TF_BlitDestination', 'BGFX_TEXTURE_BLIT_DST'),
		('TF_ReadBack', 'BGFX_TEXTURE_READ_BACK'),
		('TF_RenderTarget', 'BGFX_TEXTURE_RT')
	], 'TextureFlags')

	gen.bind_function('hg::LoadTextureFlagsFromFile', 'uint64_t', ['const std::string &path'], {'rval_constants_group': 'TextureFlags'})
	gen.bind_function('hg::LoadTextureFlagsFromAssets', 'uint64_t', ['const std::string &name'], {'rval_constants_group': 'TextureFlags'})

	# texture
	gen.bind_function('hg::CreateTexture', 'hg::Texture', ['int width', 'int height', 'const char *name', 'uint64_t flags', '?bgfx::TextureFormat::Enum format'], {'constants_group': {'flags': 'TextureFlags'}})
	gen.bind_function('hg::CreateTextureFromPicture', 'hg::Texture', ['const hg::Picture& pic', 'const char *name', 'uint64_t flags', '?bgfx::TextureFormat::Enum format'], {'constants_group': {'flags': 'TextureFlags'}})

	gen.bind_function('hg::UpdateTextureFromPicture', 'void', ['hg::Texture &tex', 'const hg::Picture &pic'])

	gen.bind_function('hg::LoadTextureFromFile', 'hg::Texture', ['const char *path', 'uint64_t flags', 'bgfx::TextureInfo *info'], {'arg_out': ['info'], 'constants_group': {'flags': 'TextureFlags'}})
	gen.bind_function('hg::LoadTextureFromAssets', 'hg::Texture', ['const char *path', 'uint64_t flags', 'bgfx::TextureInfo *info'], {'arg_out': ['info'], 'constants_group': {'flags': 'TextureFlags'}})

	gen.insert_binding_code('static void _DestroyTexture(const hg::Texture &tex) { bgfx::destroy(tex.handle); }')
	gen.bind_function('DestroyTexture', 'void', ['const hg::Texture &tex'], {'route': route_lambda('_DestroyTexture')})

	gen.bind_function('hg::ProcessTextureLoadQueue', 'size_t', ['hg::PipelineResources &res', '?hg::time_ns t_budget'])

	gen.bind_function('hg::ProcessModelLoadQueue', 'size_t', ['hg::PipelineResources &res', '?hg::time_ns t_budget'])
	gen.bind_function('hg::ProcessLoadQueues', 'size_t', ['hg::PipelineResources &res', '?hg::time_ns t_budget'])

	# ModelRef/TextureRef/MaterialRef/PipelineProgramRef
	model_ref = gen.begin_class('hg::ModelRef')
	model_ref._inline = True
	gen.bind_comparison_ops(model_ref, ['==', '!='], ['const hg::ModelRef &m'])
	gen.end_class(model_ref)
	gen.bind_variable("const hg::ModelRef hg::InvalidModelRef")

	texture_ref = gen.begin_class('hg::TextureRef')
	texture_ref._inline = True
	gen.bind_comparison_ops(texture_ref, ['==', '!='], ['const hg::TextureRef &t'])
	gen.end_class(texture_ref)
	gen.bind_variable("const hg::TextureRef hg::InvalidTextureRef")

	material_ref = gen.begin_class('hg::MaterialRef')
	material_ref._inline = True
	gen.bind_comparison_ops(material_ref, ['==', '!='], ['const hg::MaterialRef &m'])
	gen.end_class(material_ref)
	gen.bind_variable("const hg::MaterialRef hg::InvalidMaterialRef")

	pprogram_ref = gen.begin_class('hg::PipelineProgramRef')
	pprogram_ref._inline = True
	gen.bind_comparison_ops(pprogram_ref, ['==', '!='], ['const hg::PipelineProgramRef &p'])
	gen.end_class(pprogram_ref)
	gen.bind_variable("const hg::PipelineProgramRef hg::InvalidPipelineProgramRef")

	gen.bind_function('CaptureTexture', 'uint32_t', ['const hg::PipelineResources &resources', 'const hg::TextureRef &tex', 'hg::Picture &pic'])

	# Texture with pipeline
	gen.bind_function('hg::LoadTextureFromFile', 'hg::TextureRef', ['const char *path', 'uint32_t flags', 'hg::PipelineResources &resources'])
	gen.bind_function('hg::LoadTextureFromAssets', 'hg::TextureRef', ['const char *path', 'uint32_t flags', 'hg::PipelineResources &resources'])

	# Texture
	texture_with_flags = gen.begin_class('hg::Texture')
	texture_with_flags._inline = True
	gen.end_class(texture_with_flags)

	gen.insert_binding_code('static bool _IsValid(const hg::Texture &t) { return bgfx::isValid(t.handle); }')
	gen.bind_function('hg::IsValid', 'bool', ['const hg::Texture &t'], {'route': route_lambda('_IsValid')})

	# UniformSetValue
	uniform_set_value = gen.begin_class('hg::UniformSetValue')
	gen.end_class(uniform_set_value)

	bind_std_vector(gen, uniform_set_value)

	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'float v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::tVec2<float> &v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::Vec3 &v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::Vec4 &v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::Mat3 &v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::Mat4 &v'])
	gen.bind_function('hg::MakeUniformSetValue', 'hg::UniformSetValue', ['const char *name', 'const hg::Mat44 &v'])

	# UniformSetTexture
	uniform_set_texture = gen.begin_class('hg::UniformSetTexture')
	gen.end_class(uniform_set_texture)

	bind_std_vector(gen, uniform_set_texture)

	gen.bind_function('hg::MakeUniformSetTexture', 'hg::UniformSetTexture', ['const char *name', 'const hg::Texture &texture', 'uint8_t stage'])

	# PipelineProgram
	pipeline_program = gen.begin_class('hg::PipelineProgram')
	gen.end_class(pipeline_program)

	gen.bind_function('hg::LoadPipelineProgramFromFile', 'hg::PipelineProgram', ['const char *path', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'], [])
	gen.bind_function('hg::LoadPipelineProgramFromAssets', 'hg::PipelineProgram', ['const char *name', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'], [])

	gen.bind_function('hg::LoadPipelineProgramRefFromFile', 'hg::PipelineProgramRef', ['const char *path', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'], [])
	gen.bind_function('hg::LoadPipelineProgramRefFromAssets', 'hg::PipelineProgramRef', ['const char *name', 'hg::PipelineResources &resources', 'const hg::PipelineInfo &pipeline'], [])

	# PipelineDisplayList
	#pipeline_display_list = gen.begin_class('hg::PipelineDisplayList')
	#gen.end_class(pipeline_display_list)

	#bind_std_vector(gen, pipeline_display_list)

	# ViewState
	view_state = gen.begin_class('hg::ViewState')
	gen.bind_members(view_state, ['hg::Frustum frustum', 'hg::Mat44 proj', 'hg::Mat4 view'])
	gen.end_class(view_state)

	gen.bind_function('hg::ComputeOrthographicViewState', 'hg::ViewState', ['const hg::Mat4 &world', 'float size', 'float znear', 'float zfar', 'const hg::tVec2<float> &aspect_ratio'])
	gen.bind_function('hg::ComputePerspectiveViewState', 'hg::ViewState', ['const hg::Mat4 &world', 'float fov', 'float znear', 'float zfar', 'const hg::tVec2<float> &aspect_ratio'])

	# Material
	material = gen.begin_class('hg::Material')
	gen.bind_constructor(material, [])
	gen.end_class(material)

	bind_std_vector(gen, material)

	gen.bind_function('hg::SetMaterialProgram', 'void', ['hg::Material &mat', 'hg::PipelineProgramRef program'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'float v'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::tVec2<float> &v'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::Vec3 &v'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::Vec4 &v'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::Mat3 &m'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::Mat4 &m'])
	gen.bind_function('hg::SetMaterialValue', 'void', ['hg::Material &mat', 'const char *name', 'const hg::Mat44 &m'])
	gen.bind_function('hg::SetMaterialTexture', 'void', ['hg::Material &mat', 'const char *name', 'hg::TextureRef texture', 'uint8_t stage'])
	gen.bind_function('hg::SetMaterialTextureRef', 'void', ['hg::Material &mat', 'const char *name', 'hg::TextureRef texture'])

	gen.bind_function('hg::GetMaterialTexture', 'hg::TextureRef', ['hg::Material &mat', 'const char *name'])
	gen.bind_function('hg::GetMaterialTextures', 'std::vector<std::string>', ['hg::Material &mat'])
	gen.bind_function('hg::GetMaterialValues', 'std::vector<std::string>', ['hg::Material &mat'])
	
	gen.bind_named_enum('hg::FaceCulling', ['FC_Disabled', 'FC_Clockwise', 'FC_CounterClockwise'])
	gen.bind_function('hg::GetMaterialFaceCulling', 'hg::FaceCulling', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialFaceCulling', 'void', ['hg::Material &mat', 'hg::FaceCulling culling'])

	gen.bind_named_enum('hg::DepthTest', ['DT_Less', 'DT_LessEqual', 'DT_Equal', 'DT_GreaterEqual', 'DT_Greater', 'DT_NotEqual', 'DT_Never', 'DT_Always', 'DT_Disabled'])
	gen.bind_function('hg::GetMaterialDepthTest', 'hg::DepthTest', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialDepthTest', 'void', ['hg::Material &mat', 'hg::DepthTest test'])

	gen.bind_named_enum('hg::BlendMode', ['BM_Additive', 'BM_Alpha', 'BM_Darken', 'BM_Lighten', 'BM_Multiply', 'BM_Opaque', 'BM_Screen', 'BM_LinearBurn', 'BM_Undefined'])
	gen.bind_function('hg::GetMaterialBlendMode', 'hg::BlendMode', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialBlendMode', 'void', ['hg::Material &mat', 'hg::BlendMode mode'])

	gen.bind_function('hg::GetMaterialWriteRGBA', 'void', ['const hg::Material &mat', 'bool &write_r', 'bool &write_g', 'bool &write_b', 'bool &write_a'], {'arg_out': ['write_r', 'write_g', 'write_b', 'write_a']})
	gen.bind_function('hg::SetMaterialWriteRGBA', 'void', ['hg::Material &mat', 'bool write_r', 'bool write_g', 'bool write_b', 'bool write_a'])

	gen.bind_function('hg::GetMaterialNormalMapInWorldSpace', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialNormalMapInWorldSpace', 'void', ['hg::Material &mat', 'bool enable'])

	gen.bind_function('hg::GetMaterialWriteZ', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialWriteZ', 'void', ['hg::Material &mat', 'bool enable'])

	gen.bind_function('hg::GetMaterialDiffuseUsesUV1', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialDiffuseUsesUV1', 'void', ['hg::Material &mat', 'bool enable'])
	gen.bind_function('hg::GetMaterialSpecularUsesUV1', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialSpecularUsesUV1', 'void', ['hg::Material &mat', 'bool enable'])
	gen.bind_function('hg::GetMaterialAmbientUsesUV1', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialAmbientUsesUV1', 'void', ['hg::Material &mat', 'bool enable'])

	gen.bind_function('hg::GetMaterialSkinning', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialSkinning', 'void', ['hg::Material &mat', 'bool enable'])
	gen.bind_function('hg::GetMaterialAlphaCut', 'bool', ['const hg::Material &mat'])
	gen.bind_function('hg::SetMaterialAlphaCut', 'void', ['hg::Material &mat', 'bool enable'])

	gen.bind_function('hg::CreateMaterial', 'hg::Material', ['hg::PipelineProgramRef prg'])
	gen.bind_function('hg::CreateMaterial', 'hg::Material', ['hg::PipelineProgramRef prg', 'const char *value_name', 'const hg::Vec4 &value'])
	gen.bind_function('hg::CreateMaterial', 'hg::Material', ['hg::PipelineProgramRef prg', 'const char *value_name_0', 'const hg::Vec4 &value_0', 'const char *value_name_1', 'const hg::Vec4 &value_1'])

	# RenderState
	render_state = gen.begin_class('hg::RenderState')
	gen.end_class(render_state)

	gen.bind_function_overloads('hg::ComputeRenderState', [
		('hg::RenderState', ['hg::BlendMode blend', '?hg::DepthTest depth_test', '?hg::FaceCulling culling', '?bool write_z', '?bool write_r', '?bool write_g', '?bool write_b', '?bool write_a'], {}),
		('hg::RenderState', ['hg::BlendMode blend', 'bool write_z', '?bool write_r', '?bool write_g', '?bool write_b', '?bool write_a'], {})
	])

	# sort key
	gen.bind_function('hg::ComputeSortKey', 'uint32_t', ['float view_depth'])
	gen.bind_function('hg::ComputeSortKeyFromWorld', 'uint32_t', ['const hg::Vec3 &T', 'const hg::Mat4 &view'])
	gen.bind_function('hg::ComputeSortKeyFromWorld', 'uint32_t', ['const hg::Vec3 &T', 'const hg::Mat4 &view', 'const hg::Mat4 &model'])

	# model
	gen.add_include('engine/render_pipeline.h')
	gen.add_include('engine/create_geometry.h')

	mdl = gen.begin_class('hg::Model')
	gen.end_class(mdl)

	gen.bind_function('hg::LoadModelFromFile', 'hg::Model', ['const char *path'])
	gen.bind_function('hg::LoadModelFromAssets', 'hg::Model', ['const char *name'])

	gen.bind_function('hg::CreateCubeModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float x', 'float y', 'float z'])
	gen.bind_function('hg::CreateSphereModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float radius', 'int subdiv_x', 'int subdiv_y'])
	gen.bind_function('hg::CreatePlaneModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float width', 'float length', 'int subdiv_x', 'int subdiv_z'])
	gen.bind_function('hg::CreateCylinderModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float radius', 'float height', 'int subdiv_x'])
	gen.bind_function('hg::CreateConeModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float radius', 'float height', 'int subdiv_x'])
	gen.bind_function('hg::CreateCapsuleModel', 'hg::Model', ['const bgfx::VertexLayout &decl', 'float radius', 'float height', 'int subdiv_x', 'int subdiv_y'])

	lib.stl.bind_function_T(gen, 'std::function<void(uint16_t)>', 'SetDrawStatesCallback')

	gen.insert_binding_code('''
static void _DrawModel(bgfx::ViewId view_id, const hg::Model &mdl, bgfx::ProgramHandle prg, const std::vector<hg::UniformSetValue>& values, const std::vector<hg::UniformSetTexture>& textures, const hg::Mat4 &matx, hg::RenderState state = {}, uint32_t depth = 0) {
	hg::DrawModel(view_id, mdl, prg, values, textures, &matx, 1, state, depth);
}

static void _DrawModel(bgfx::ViewId view_id, const hg::Model &mdl, bgfx::ProgramHandle prg, const std::vector<hg::UniformSetValue>& values, const std::vector<hg::UniformSetTexture>& textures, std::vector<hg::Mat4> &matx, hg::RenderState state = {}, uint32_t depth = 0) {
	hg::DrawModel(view_id, mdl, prg, values, textures, matx.data(), matx.size(), state, depth);
}
	''')

	gen.bind_function_overloads('hg::DrawModel', expand_std_vector_proto(gen, [
		('void', ['bgfx::ViewId view_id', 'const hg::Model &mdl', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue>& values', 'const std::vector<hg::UniformSetTexture>& textures', 'const hg::Mat4 &matrix', '?hg::RenderState render_state', '?uint32_t depth'], {'route': route_lambda('_DrawModel')}),
		('void', ['bgfx::ViewId view_id', 'const hg::Model &mdl', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue>& values', 'const std::vector<hg::UniformSetTexture>& textures', 'std::vector<hg::Mat4> &matrices', '?hg::RenderState render_state', '?uint32_t depth'], {'route': route_lambda('_DrawModel')})
	]))

	# PipelineResources
	gen.insert_binding_code('''
static hg::TextureRef _PipelineResources_AddTexture(hg::PipelineResources *res, const char *name, const hg::Texture &tex) { return res->textures.Add(name, tex); }
static hg::ModelRef _PipelineResources_AddModel(hg::PipelineResources *res, const char *name, const hg::Model &mdl) { return res->models.Add(name, mdl); }
static hg::PipelineProgramRef _PipelineResources_AddProgram(hg::PipelineResources *res, const char *name, const hg::PipelineProgram &prg) { return res->programs.Add(name, prg); }

static hg::TextureRef _PipelineResources_HasTexture(hg::PipelineResources *res, const char *name) { return res->textures.Has(name); }
static hg::ModelRef _PipelineResources_HasModel(hg::PipelineResources *res, const char *name) { return res->models.Has(name); }
static hg::PipelineProgramRef _PipelineResources_HasProgram(hg::PipelineResources *res, const char *name) { return res->programs.Has(name); }

static void _PipelineResources_UpdateTexture(hg::PipelineResources *res, hg::TextureRef ref, const hg::Texture &tex) { return res->textures.Update(ref, tex); }
static void _PipelineResources_UpdateModel(hg::PipelineResources *res, hg::ModelRef ref, const hg::Model &mdl) { return res->models.Update(ref, mdl); }
static void _PipelineResources_UpdateProgram(hg::PipelineResources *res, hg::PipelineProgramRef ref, const hg::PipelineProgram &prg) { return res->programs.Update(ref, prg); }

static hg::Texture& _PipelineResources_GetTexture(hg::PipelineResources *res, hg::TextureRef ref) { return res->textures.Get(ref); }
static hg::Model& _PipelineResources_GetModel(hg::PipelineResources *res, hg::ModelRef ref) { return res->models.Get(ref); }
static hg::PipelineProgram& _PipelineResources_GetProgram(hg::PipelineResources *res, hg::PipelineProgramRef ref) { return res->programs.Get(ref); }

static std::string _PipelineResources_GetTextureName(hg::PipelineResources *res, hg::TextureRef ref) { return res->textures.GetName(ref); }
static std::string _PipelineResources_GetModelName(hg::PipelineResources *res, hg::ModelRef ref) { return res->models.GetName(ref); }
static std::string _PipelineResources_GetProgramName(hg::PipelineResources *res, hg::PipelineProgramRef ref) { return res->programs.GetName(ref); }

static void _PipelineResources_DestroyAllTextures(hg::PipelineResources *res) { return res->textures.DestroyAll(); }
static void _PipelineResources_DestroyAllModels(hg::PipelineResources *res) { return res->models.DestroyAll(); }
static void _PipelineResources_DestroyAllPrograms(hg::PipelineResources *res) { return res->programs.DestroyAll(); }

static void _PipelineResources_DestroyTexture(hg::PipelineResources *res, hg::TextureRef ref) { return res->textures.Destroy(ref); }
static void _PipelineResources_DestroyModel(hg::PipelineResources *res, hg::ModelRef ref) { return res->models.Destroy(ref); }
static void _PipelineResources_DestroyProgram(hg::PipelineResources *res, hg::PipelineProgramRef ref) { return res->programs.Destroy(ref); }

static bool _PipelineResources_HasTextureInfo(hg::PipelineResources *res, hg::TextureRef ref) { return res->texture_infos.find(ref.ref) != std::end(res->texture_infos); }

static bgfx::TextureInfo _PipelineResources_GetTextureInfo(hg::PipelineResources *res, hg::TextureRef ref) {
	auto i = res->texture_infos.find(ref.ref);
	return i == std::end(res->texture_infos) ? bgfx::TextureInfo{} : i->second;
}
''')

	pipe_res = gen.begin_class('hg::PipelineResources')
	gen.bind_constructor(pipe_res, [])

	gen.bind_method(pipe_res, 'AddTexture', 'hg::TextureRef', ['const char *name', 'const hg::Texture &tex'], {'route': route_lambda('_PipelineResources_AddTexture')})
	gen.bind_method(pipe_res, 'AddModel', 'hg::ModelRef', ['const char *name', 'const hg::Model &mdl'], {'route': route_lambda('_PipelineResources_AddModel')})
	gen.bind_method(pipe_res, 'AddProgram', 'hg::PipelineProgramRef', ['const char *name', 'const hg::PipelineProgram &prg'], {'route': route_lambda('_PipelineResources_AddProgram')})

	gen.bind_method(pipe_res, 'HasTexture', 'hg::TextureRef', ['const char *name'], {'route': route_lambda('_PipelineResources_HasTexture')})
	gen.bind_method(pipe_res, 'HasModel', 'hg::ModelRef', ['const char *name'], {'route': route_lambda('_PipelineResources_HasModel')})
	gen.bind_method(pipe_res, 'HasProgram', 'hg::PipelineProgramRef', ['const char *name'], {'route': route_lambda('_PipelineResources_HasProgram')})

	gen.bind_method(pipe_res, 'UpdateTexture', 'void', ['hg::TextureRef ref', 'const hg::Texture &tex'], {'route': route_lambda('_PipelineResources_UpdateTexture')})
	gen.bind_method(pipe_res, 'UpdateModel', 'void', ['hg::ModelRef ref', 'const hg::Model &mdl'], {'route': route_lambda('_PipelineResources_UpdateModel')})
	gen.bind_method(pipe_res, 'UpdateProgram', 'void', ['hg::PipelineProgramRef ref', 'const hg::PipelineProgram &prg'], {'route': route_lambda('_PipelineResources_UpdateProgram')})
    
	gen.bind_method(pipe_res, 'GetTexture', 'hg::Texture&', ['hg::TextureRef ref'], {'route': route_lambda('_PipelineResources_GetTexture')})
	gen.bind_method(pipe_res, 'GetModel', 'hg::Model&', ['hg::ModelRef ref'], {'route': route_lambda('_PipelineResources_GetModel')})
	gen.bind_method(pipe_res, 'GetProgram', 'hg::PipelineProgram&', ['hg::PipelineProgramRef ref'], {'route': route_lambda('_PipelineResources_GetProgram')})
	
	gen.bind_method(pipe_res, 'GetTextureName', 'std::string', ['hg::TextureRef ref'], {'route': route_lambda('_PipelineResources_GetTextureName')})
	gen.bind_method(pipe_res, 'GetModelName', 'std::string', ['hg::ModelRef ref'], {'route': route_lambda('_PipelineResources_GetModelName')})
	gen.bind_method(pipe_res, 'GetProgramName', 'std::string', ['hg::PipelineProgramRef ref'], {'route': route_lambda('_PipelineResources_GetProgramName')})

	gen.bind_method(pipe_res, 'DestroyAllTextures', 'void', [], {'route': route_lambda('_PipelineResources_DestroyAllTextures')})
	gen.bind_method(pipe_res, 'DestroyAllModels', 'void', [], {'route': route_lambda('_PipelineResources_DestroyAllModels')})
	gen.bind_method(pipe_res, 'DestroyAllPrograms', 'void', [], {'route': route_lambda('_PipelineResources_DestroyAllPrograms')})

	gen.bind_method(pipe_res, 'DestroyTexture', 'void', ['hg::TextureRef ref'], {'route': route_lambda('_PipelineResources_DestroyTexture')})
	gen.bind_method(pipe_res, 'DestroyModel', 'void', ['hg::ModelRef ref'], {'route': route_lambda('_PipelineResources_DestroyModel')})
	gen.bind_method(pipe_res, 'DestroyProgram', 'void', ['hg::PipelineProgramRef ref'], {'route': route_lambda('_PipelineResources_DestroyProgram')})

	gen.bind_method(pipe_res, 'HasTextureInfo', 'bool', ['hg::TextureRef ref'], {'route': route_lambda('_PipelineResources_HasTextureInfo')})
	gen.bind_method(pipe_res, 'GetTextureInfo', 'bgfx::TextureInfo', ['hg::TextureRef ref'], {'route': route_lambda('_PipelineResources_GetTextureInfo')})

	gen.end_class(pipe_res)

	#
	gen.bind_function('hg::UpdateMaterialPipelineProgramVariant', 'void', ['hg::Material &mat', 'const hg::PipelineResources &resources'])

	gen.bind_function('hg::CreateMissingMaterialProgramValuesFromFile', 'void', ['hg::Material &mat', 'const hg::PipelineResources &resources'])
	gen.bind_function('hg::CreateMissingMaterialProgramValuesFromAssets', 'void', ['hg::Material &mat', 'const hg::PipelineResources &resources'])

	#
	frame_buffer = gen.begin_class('hg::FrameBuffer')
	gen.bind_member(frame_buffer, 'bgfx::FrameBufferHandle handle')
	gen.end_class(frame_buffer)

	gen.bind_function_overloads('hg::CreateFrameBuffer', [
		('hg::FrameBuffer', ['const hg::Texture &color', 'const hg::Texture &depth', 'const char *name'], []),
		('hg::FrameBuffer', ['bgfx::TextureFormat::Enum color_format', 'bgfx::TextureFormat::Enum depth_format', 'int aa', 'const char *name'], []),
		('hg::FrameBuffer', ['int width', 'int height', 'bgfx::TextureFormat::Enum color_format', 'bgfx::TextureFormat::Enum depth_format', 'int aa', 'const char *name'], [])
	])

	gen.bind_function('hg::GetColorTexture', 'hg::Texture', ['hg::FrameBuffer &frameBuffer'])
	gen.bind_function('hg::GetDepthTexture', 'hg::Texture', ['hg::FrameBuffer &frameBuffer'])
	
	gen.insert_binding_code('''
static void _FrameBuffer_GetTextures(hg::FrameBuffer &framebuffer, hg::Texture &color, hg::Texture &depth) {
	color = hg::GetColorTexture(framebuffer);
	depth = hg::GetDepthTexture(framebuffer);
}
static bool _IsValid(const hg::FrameBuffer &fb) { return bgfx::isValid(fb.handle); }
''')	
	gen.bind_function('GetTextures', 'void', ['hg::FrameBuffer &framebuffer', 'hg::Texture &color', 'hg::Texture &depth'], {'route': route_lambda('_FrameBuffer_GetTextures'),  'arg_out': ['color', 'depth']})
	gen.bind_function('hg::DestroyFrameBuffer', 'void', ['hg::FrameBuffer &frameBuffer'])
	gen.bind_function('hg::IsValid', 'bool', ['const hg::FrameBuffer &fb'], {'route': route_lambda('_IsValid')})
	#
	vertices = gen.begin_class('hg::Vertices')
	gen.bind_constructor(vertices, ['const bgfx::VertexLayout &decl', 'size_t count'])

	gen.bind_method(vertices, 'GetDecl', 'const bgfx::VertexLayout &', [])

	gen.bind_method(vertices, 'Begin', 'hg::Vertices &', ['size_t vertex_index'])
	gen.bind_method(vertices, 'SetPos', 'hg::Vertices &', ['const hg::Vec3 &pos'])
	gen.bind_method(vertices, 'SetNormal', 'hg::Vertices &', ['const hg::Vec3 &normal'])
	gen.bind_method(vertices, 'SetTangent', 'hg::Vertices &', ['const hg::Vec3 &tangent'])
	gen.bind_method(vertices, 'SetBinormal', 'hg::Vertices &', ['const hg::Vec3 &binormal'])
	gen.bind_method(vertices, 'SetTexCoord0', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord1', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord2', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord3', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord4', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord5', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord6', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetTexCoord7', 'hg::Vertices &', ['const hg::tVec2<float> &uv'])
	gen.bind_method(vertices, 'SetColor0', 'hg::Vertices &', ['const hg::Color &color'])
	gen.bind_method(vertices, 'SetColor1', 'hg::Vertices &', ['const hg::Color &color'])
	gen.bind_method(vertices, 'SetColor2', 'hg::Vertices &', ['const hg::Color &color'])
	gen.bind_method(vertices, 'SetColor3', 'hg::Vertices &', ['const hg::Color &color'])
	gen.bind_method(vertices, 'End', 'void', ['?bool validate'])

	gen.bind_method(vertices, 'Clear', 'void', [])

	gen.bind_method(vertices, 'Reserve', 'void', ['size_t count'])
	gen.bind_method(vertices, 'Resize', 'void', ['size_t count'])

	gen.bind_method(vertices, 'GetData', 'const void *', [])

	gen.bind_method(vertices, 'GetSize', 'size_t', [])
	gen.bind_method(vertices, 'GetCount', 'size_t', [])
	gen.bind_method(vertices, 'GetCapacity', 'size_t', [])

	gen.end_class(vertices)

	#
	gen.bind_function('hg::SetTransform', 'void', ['const hg::Mat4 &mtx'])

	gen.bind_function_overloads('hg::DrawLines',
		expand_std_vector_proto(gen, [
		('void', ['bgfx::ViewId view_id', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', '?hg::RenderState render_state', '?uint32_t depth'], {}),
		('void', ['bgfx::ViewId view_id', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState render_state', '?uint32_t depth'], {}),
		('void', ['bgfx::ViewId view_id', 'const std::vector<uint16_t> &idx', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState render_state', '?uint32_t depth'], {})
		])
	)

	gen.bind_function_overloads('hg::DrawTriangles',
		expand_std_vector_proto(gen, [
		('void', ['bgfx::ViewId view_id', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', '?hg::RenderState state', '?uint32_t depth'], {}),
		('void', ['bgfx::ViewId view_id', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState state', '?uint32_t depth'], {}),
		('void', ['bgfx::ViewId view_id', 'const std::vector<uint16_t> &idx', 'const hg::Vertices &vtx', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState state', '?uint32_t depth'], {})
		])
	)

	gen.bind_function_overloads('hg::DrawSprites',
		expand_std_vector_proto(gen, [
		('void', ['bgfx::ViewId view_id', 'const hg::Mat3 &inv_view_R', 'bgfx::VertexLayout &vtx_layout', 'const std::vector<hg::Vec3> &pos', 'const hg::tVec2<float> &size', 'bgfx::ProgramHandle prg', '?hg::RenderState state', '?uint32_t depth'], {}),
		('void', ['bgfx::ViewId view_id', 'const hg::Mat3 &inv_view_R', 'bgfx::VertexLayout &vtx_layout', 'const std::vector<hg::Vec3> &pos', 'const hg::tVec2<float> &size', 'bgfx::ProgramHandle prg', 'const std::vector<hg::UniformSetValue> &values', 'const std::vector<hg::UniformSetTexture> &textures', '?hg::RenderState state', '?uint32_t depth'], {})
		])
	)

	# pipeline
	pipeline = gen.begin_class('hg::Pipeline')
	gen.end_class(pipeline)

	# PipelineInfo
	pipeline_info = gen.begin_class('hg::PipelineInfo')
	gen.bind_member(pipeline_info, 'std::string name')
	gen.end_class(pipeline_info)

	# forward pipeline info
	gen.bind_function('hg::GetForwardPipelineInfo', 'const hg::PipelineInfo &', [])


def bind_vertex(gen):
	gen.add_include('engine/vertex.h')

	vertex = gen.begin_class('hg::Vertex')
	gen.bind_constructor(vertex, [])
	gen.bind_members(vertex, ['hg::Vec3 pos', 'hg::Vec3 normal', 'hg::Vec3 tangent', 'hg::Vec3 binormal',
		'hg::tVec2<float> uv0', 'hg::tVec2<float> uv1', 'hg::tVec2<float> uv2', 'hg::tVec2<float> uv3',
		'hg::tVec2<float> uv4', 'hg::tVec2<float> uv5', 'hg::tVec2<float> uv6', 'hg::tVec2<float> uv7',
		'hg::Color color0', 'hg::Color color1', 'hg::Color color2', 'hg::Color color3'])
	gen.end_class(vertex)

	gen.bind_function('hg::MakeVertex', 'hg::Vertex', ['const hg::Vec3 &pos', '?const hg::Vec3 &nrm', '?const hg::tVec2<float> &uv0', '?const hg::Color &color0'])


def bind_geometry_builder(gen):
	gen.add_include('engine/geometry_builder.h')

	geometry = gen.begin_class('hg::Geometry')
	gen.end_class(geometry)

	gen.bind_function('hg::SaveGeometryToFile','bool',['const char *path', 'const hg::Geometry &geo'])
	
	geometry_builder = gen.begin_class('hg::GeometryBuilder')
	gen.bind_constructor(geometry_builder, [])
	gen.bind_method(geometry_builder, 'AddVertex', 'void', ['hg::Vertex &vtx'])

	protos = [('void', ['const std::vector<uint32_t> &idxs', 'uint16_t material'], [])]
	gen.bind_method_overloads(geometry_builder, 'AddPolygon', expand_std_vector_proto(gen, protos))
	gen.bind_method(geometry_builder, 'AddTriangle', 'void',['uint32_t a', 'uint32_t b', 'uint32_t c', 'uint32_t material'])
	gen.bind_method(geometry_builder, 'AddQuad', 'void',['uint32_t a', 'uint32_t b', 'uint32_t c', 'uint32_t d', 'uint32_t material'])
	gen.bind_method(geometry_builder, 'Make', 'hg::Geometry', [])
	gen.bind_method(geometry_builder, 'Clear', 'void', [])
	gen.end_class(geometry_builder)

	
def bind_model_builder(gen):    
	gen.add_include('engine/model_builder.h')

	# ModelBuilder
	model_builder = gen.begin_class('hg::ModelBuilder')
	gen.bind_constructor(model_builder, [])
	gen.bind_method(model_builder, 'AddVertex', 'uint32_t', ['const hg::Vertex &vtx'])
	gen.bind_method(model_builder, 'AddTriangle', 'void', ['uint32_t a', 'uint32_t b', 'uint32_t c'])
	gen.bind_method(model_builder, 'AddQuad', 'void', ['uint32_t a', 'uint32_t b', 'uint32_t c', 'uint32_t d'])
	gen.bind_method(model_builder, 'AddPolygon', 'void', ['const std::vector<uint32_t> &idxs'])
	gen.bind_method(model_builder, 'GetCurrentListIndexCount', 'size_t', [])
	gen.bind_method(model_builder, 'EndList', 'void', ['uint16_t material'])
	gen.bind_method(model_builder, 'Clear', 'void', [])
	gen.bind_method(model_builder, 'MakeModel', 'hg::Model', ['const bgfx::VertexLayout &decl'])
	gen.end_class(model_builder)


def bind_iso_surface(gen):
	gen.add_include('engine/iso_surface.h')

	iso_surface = gen.begin_class('hg::IsoSurface')
	gen.end_class(iso_surface)

	gen.bind_function('hg::NewIsoSurface', 'hg::IsoSurface', ['int width', 'int height', 'int depth'])

	gen.bind_function('hg::IsoSurfaceSphere', 'void', ['hg::IsoSurface &surface', 'int width', 'int height', 'int depth', 'float x', 'float y', 'float z', 'float radius', '?float value', '?float exponent'])
	#gen.bind_function('hg::ConvoluteIsoSurface', 'hg::IsoSurface', ['const hg::IsoSurface &surface', 'int width', 'int height', 'int depth', ''])
	gen.bind_function('hg::GaussianBlurIsoSurface', 'hg::IsoSurface', ['const hg::IsoSurface &surface', 'int width', 'int height', 'int depth'])

	gen.bind_function_overloads('hg::IsoSurfaceToModel', [
		('bool', ['hg::ModelBuilder &builder', 'const hg::IsoSurface &surface', 'int width', 'int height', 'int depth', '?uint16_t material', '?float isolevel'], {}),
		('bool', ['hg::ModelBuilder &builder', 'const hg::IsoSurface &surface', 'int width', 'int height', 'int depth', 'uint16_t material', 'float isolevel', 'float scale_x', 'float scale_y', 'float scale_z'], {})
	])


def bind_fps_controller(gen):
	gen.add_include('engine/fps_controller.h')

	gen.bind_function('hg::FpsController', 'void', ['bool key_up', 'bool key_down', 'bool key_left', 'bool key_right', 'bool btn', 'float dx', 'float dy', 'hg::Vec3 &pos', 'hg::Vec3 &rot', 'float speed', 'hg::time_ns dt_t'])
	gen.bind_function('hg::FpsController', 'void', ['const hg::Keyboard &keyboard', 'const hg::Mouse &mouse', 'hg::Vec3 &pos', 'hg::Vec3 &rot', 'float speed', 'hg::time_ns dt'])


def bind_forward_pipeline(gen):
	gen.add_include('engine/forward_pipeline.h')
	
	# ForwardPipeline (inherits Pipeline)
	forward_pipeline = gen.begin_class('hg::ForwardPipeline')
	gen.add_base(forward_pipeline, gen.get_conv('hg::Pipeline'))
	gen.end_class(forward_pipeline)

	gen.bind_function('hg::CreateForwardPipeline', 'hg::ForwardPipeline', ['?int shadow_map_resolution', '?bool spot_16bit_shadow_map'])
	gen.bind_function('hg::DestroyForwardPipeline', 'void', ['hg::ForwardPipeline &pipeline'])

	# ForwardPipelineLight
	gen.bind_named_enum('hg::ForwardPipelineLightType', ['FPLT_Point', 'FPLT_Spot', 'FPLT_Linear'])

	forward_pipeline_light = gen.begin_class('hg::ForwardPipelineLight')
	gen.bind_constructor(forward_pipeline_light, [])
	gen.bind_members(forward_pipeline_light, [
		'hg::ForwardPipelineLightType type', 'hg::Mat4 world', 'hg::Color diffuse', 'hg::Color specular',
		'float radius', 'float inner_angle', 'float outer_angle', 'hg::Vec4 pssm_split', 'float priority'
	])
	gen.end_class(forward_pipeline_light)

	bind_std_vector(gen, forward_pipeline_light)

	gen.bind_named_enum('hg::ForwardPipelineShadowType', ['FPST_None', 'FPST_Map'])

	gen.bind_function('hg::MakeForwardPipelinePointLight', 'hg::ForwardPipelineLight', ['const hg::Mat4 &world', 'const hg::Color &diffuse', 'const hg::Color &specular', '?float radius', '?float priority', '?hg::ForwardPipelineShadowType shadow_type', '?float shadow_bias'])
	gen.bind_function('hg::MakeForwardPipelineSpotLight', 'hg::ForwardPipelineLight', ['const hg::Mat4 &world', 'const hg::Color &diffuse', 'const hg::Color &specular', '?float radius', '?float inner_angle', '?float outer_angle', '?float priority', '?hg::ForwardPipelineShadowType shadow_type', '?float shadow_bias'])
	gen.bind_function('hg::MakeForwardPipelineLinearLight', 'hg::ForwardPipelineLight', ['const hg::Mat4 &world', 'const hg::Color &diffuse', 'const hg::Color &specular', '?const hg::Vec4& pssm_split', '?float priority', '?hg::ForwardPipelineShadowType shadow_type', '?float shadow_bias'])

	#
	forward_pipeline_lights = gen.begin_class('hg::ForwardPipelineLights')
	gen.end_class(forward_pipeline_lights)

	gen.bind_function_overloads('hg::PrepareForwardPipelineLights', expand_std_vector_proto(gen, [('hg::ForwardPipelineLights', ['const std::vector<hg::ForwardPipelineLight> &lights'], [])]))

	# Fog
	fog = gen.begin_class('hg::ForwardPipelineFog')
	gen.bind_constructor(fog, [])
	gen.bind_members(fog, ['float near', 'float far', 'hg::Color color'])
	gen.end_class(fog)

	# submit model to forward pipeline stage
	gen.bind_function('hg::SubmitModelToForwardPipeline', 'void', ['bgfx::ViewId &view_id', 'const hg::Model &mdl', 'const hg::ForwardPipeline &pipeline', 'const hg::PipelineProgram &prg', 'uint32_t prg_variant',
	'uint8_t pipeline_stage', 'const hg::Color &ambient', 'const hg::ForwardPipelineLights &lights', 'const hg::ForwardPipelineFog &fog', 'const hg::Mat4 &mtx'], {'arg_in_out': ['view_id']})
	
def bind_file(gen):
	gen.add_include('foundation/file.h')

	gen.bind_named_enum('hg::SeekMode', ['SM_Start', 'SM_Current', 'SM_End'])

	file = gen.begin_class('hg::File')
	gen.end_class(file)

	gen.bind_function('hg::Open', 'hg::File', ['const char *path'])
	gen.bind_function('hg::OpenText', 'hg::File', ['const char *path'])
	gen.bind_function('hg::OpenWrite', 'hg::File', ['const char *path'])
	gen.bind_function('hg::OpenWriteText', 'hg::File', ['const char *path'])
	gen.bind_function('hg::OpenTemp', 'hg::File', ['const char *template_path'])
	gen.bind_function('hg::Close', 'bool', ['hg::File file'])

	gen.bind_function('hg::IsValid', 'bool', ['hg::File file'])
	gen.bind_function('hg::IsEOF', 'bool', ['hg::File file'])

	gen.bind_function('hg::GetSize', 'size_t', ['hg::File file'])

	gen.bind_function('hg::Seek', 'bool', ['hg::File file', 'int64_t offset', 'hg::SeekMode mode'])
	gen.bind_function('hg::Tell', 'size_t', ['hg::File file'])

	gen.bind_function('hg::Rewind', 'void', ['hg::File file'])

	gen.bind_function('hg::IsFile', 'bool', ['const char *path'])
	gen.bind_function('hg::Unlink', 'bool', ['const char *path'])

	gen.bind_function('hg::Read<uint8_t>', 'uint8_t', ['hg::File file'], bound_name='ReadUInt8')
	gen.bind_function('hg::Read<uint16_t>', 'uint16_t', ['hg::File file'], bound_name='ReadUInt16')
	gen.bind_function('hg::Read<uint32_t>', 'uint32_t', ['hg::File file'], bound_name='ReadUInt32')
	gen.bind_function('hg::Read<float>', 'float', ['hg::File file'], bound_name='ReadFloat')
	gen.bind_function('hg::Write<uint8_t>', 'bool', ['hg::File file', 'uint8_t value'], bound_name='WriteUInt8')
	gen.bind_function('hg::Write<uint16_t>', 'bool', ['hg::File file', 'uint16_t value'], bound_name='WriteUInt16')
	gen.bind_function('hg::Write<uint32_t>', 'bool', ['hg::File file', 'uint32_t value'], bound_name='WriteUInt32')
	gen.bind_function('hg::Write<float>', 'bool', ['hg::File file', 'float value'], bound_name='WriteFloat')

	gen.bind_function('hg::ReadString', 'std::string', ['hg::File file'])
	gen.bind_function('hg::WriteString', 'bool', ['hg::File file', 'const std::string &value'])

	gen.bind_function('hg::CopyFile', 'bool', ['const char *src', 'const char *dst'])

	gen.bind_function('hg::FileToString', 'std::string', ['const char *path'])
	gen.bind_function('hg::StringToFile', 'bool', ['const char *path', 'const char *value'])


def bind_path_tools(gen):
	gen.add_include('foundation/path_tools.h')

	gen.bind_function('hg::IsPathAbsolute', 'bool', ['const std::string &path'])
	gen.bind_function('hg::PathToDisplay', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::NormalizePath', 'std::string', ['const std::string &path'])

	gen.bind_function('hg::FactorizePath', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::CleanPath', 'std::string', ['const std::string &path'])

	gen.bind_function('hg::CutFilePath', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::CutFileName', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::CutFileExtension', 'std::string', ['const std::string &path'])

	gen.bind_function('hg::GetFilePath', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::GetFileName', 'std::string', ['const std::string &path'])
	gen.bind_function('hg::GetFileExtension', 'std::string', ['const std::string &path'])

	gen.bind_function('hg::HasFileExtension', 'bool', ['const std::string &path'])

	gen.bind_function('hg::PathStartsWith', 'bool', ['const std::string &path', 'const std::string &with'])

	gen.bind_function('hg::PathStripPrefix', 'std::string', ['const std::string &path', 'const std::string &prefix'])
	gen.bind_function('hg::PathStripSuffix', 'std::string', ['const std::string &path', 'const std::string &suffix'])
	gen.bind_function('hg::PathJoin', 'std::string', ['const std::vector<std::string> &elements'])

	gen.bind_function('hg::SwapFileExtension', 'std::string', ['const std::string &path', 'const std::string &ext'])

	gen.bind_function('hg::GetCurrentWorkingDirectory', 'std::string', [])
	gen.bind_function('hg::GetUserFolder', 'std::string', [])


def bind_data(gen):
	gen.add_include('foundation/data.h')

	data = gen.begin_class('hg::Data')
	gen.bind_constructor(data, [])
	gen.bind_method(data, 'GetSize', 'size_t', [])
	gen.bind_method(data, 'Rewind', 'void', [])
	gen.end_class(data)

	gen.bind_function('hg::LoadDataFromFile', 'bool', ['const char *path', 'hg::Data &data'])
	gen.bind_function('hg::SaveDataToFile', 'bool', ['const char *path', 'const hg::Data &data'])


def bind_dir(gen):
	gen.add_include('foundation/dir.h')

	gen.bind_named_enum('hg::DirEntryType', ['DE_File', 'DE_Dir', 'DE_Link', 'DE_All'])

	dir_entry = gen.begin_class('hg::DirEntry')
	gen.bind_members(dir_entry, ['int type', 'std::string name'])
	gen.end_class(dir_entry)

	bind_std_vector(gen, dir_entry)

	gen.bind_function('hg::ListDir', 'std::vector<hg::DirEntry>', ['const char *path', 'hg::DirEntryType type'])
	gen.bind_function('hg::ListDirRecursive', 'std::vector<hg::DirEntry>', ['const char *path', 'hg::DirEntryType type'])

	gen.bind_function('hg::MkDir', 'bool', ['const char *path', '?int permissions'])
	gen.bind_function('hg::RmDir', 'bool', ['const char *path'])

	gen.bind_function('hg::MkTree', 'bool', ['const char *path', '?int permissions'])
	gen.bind_function('hg::RmTree', 'bool', ['const char *path'])

	gen.bind_function('hg::Exists', 'bool', ['const char *path'])
	gen.bind_function('hg::IsDir', 'bool', ['const char *path'])

	gen.bind_function('hg::CopyDir', 'bool', ['const char *src', 'const char *dst'])
	gen.bind_function('hg::CopyDirRecursive', 'bool', ['const char *src', 'const char *dst'])


def bind_assets(gen):
	gen.add_include('engine/assets.h')

	gen.bind_function('hg::AddAssetsFolder', 'bool', ['const char *path'])
	gen.bind_function('hg::RemoveAssetsFolder', 'void', ['const char *path'])

	gen.bind_function('hg::AddAssetsPackage', 'bool', ['const char *path'])
	gen.bind_function('hg::RemoveAssetsPackage', 'void', ['const char *path'])

	gen.bind_function('hg::IsAssetFile', 'bool', ['const char *name'])


def bind_color(gen):
	gen.add_include('foundation/color.h')

	color = gen.begin_class('hg::Color')
	color._inline = True  # use inline alloc where possible

	gen.bind_static_members(color, ['const hg::Color Zero', 'const hg::Color One', 'const hg::Color White', 'const hg::Color Grey', 'const hg::Color Black', 'const hg::Color Red', 'const hg::Color Green', 'const hg::Color Blue', 'const hg::Color Yellow', 'const hg::Color Orange', 'const hg::Color Purple', 'const hg::Color Transparent'])
	gen.bind_members(color, ['float r', 'float g', 'float b', 'float a'])

	gen.bind_constructor_overloads(color, [
		([], []),
		(['const hg::Color &color'], []),
		(['float r', 'float g', 'float b'], []),
		(['float r', 'float g', 'float b', 'float a'], [])
	])

	gen.bind_arithmetic_ops_overloads(color, ['+', '-', '/', '*'], [('hg::Color', ['const hg::Color &color'], []), ('hg::Color', ['float k'], [])])
	gen.bind_inplace_arithmetic_ops_overloads(color, ['+=', '-=', '*=', '/='], [
		(['hg::Color &color'], []),
		(['float k'], [])
	])
	gen.bind_comparison_ops(color, ['==', '!='], ['const hg::Color &color'])

	gen.end_class(color)

	gen.bind_function('hg::ColorToGrayscale', 'float', ['const hg::Color &color'])

	gen.bind_function('hg::ColorToRGBA32', 'uint32_t', ['const hg::Color &color'])
	gen.bind_function('hg::ColorFromRGBA32', 'hg::Color', ['uint32_t rgba32'])
	gen.bind_function('hg::ColorToABGR32', 'uint32_t', ['const hg::Color &color'])
	gen.bind_function('hg::ColorFromABGR32', 'hg::Color', ['uint32_t rgba32'])

	gen.bind_function('hg::ARGB32ToRGBA32', 'uint32_t', ['uint32_t argb'])

	gen.bind_function('hg::RGBA32', 'uint32_t', ['uint8_t r', 'uint8_t g', 'uint8_t b', '?uint8_t a'])
	gen.bind_function('hg::ARGB32', 'uint32_t', ['uint8_t r', 'uint8_t g', 'uint8_t b', '?uint8_t a'])

	#inline float Dist2(const Color &i, const Color &j) { return (j.r - i.r) * (j.r - i.r) + (j.g - i.g) * (j.g - i.g) + (j.b - i.b) * (j.b - i.b) + (j.a - i.a) * (j.a - i.a); }
	#inline float Dist(const Color &i, const Color &j) { return Sqrt(Dist2(i, j)); }

	#inline bool AlmostEqual(const Color &a, const Color &b, float epsilon)

	gen.bind_function('hg::ChromaScale', 'hg::Color', ['const hg::Color &color', 'float k'])
	gen.bind_function('hg::AlphaScale', 'hg::Color', ['const hg::Color &color', 'float k'])

	gen.bind_function('hg::Clamp', 'hg::Color', ['const hg::Color &color', 'float min', 'float max'])
	gen.bind_function('hg::Clamp', 'hg::Color', ['const hg::Color &color', 'const hg::Color &min', 'const hg::Color &max'])

	gen.bind_function('hg::ColorFromVector3', 'hg::Color', ['const hg::Vec3 &v'])
	gen.bind_function('hg::ColorFromVector4', 'hg::Color', ['const hg::Vec4 &v'])

	gen.bind_function('hg::ColorI', 'hg::Color', ['int r', 'int g', 'int b', '?int a'])

	gen.bind_function('hg::ToHLS', 'hg::Color', ['const hg::Color &color'])
	gen.bind_function('hg::FromHLS', 'hg::Color', ['const hg::Color &color'])

	gen.bind_function('hg::SetSaturation', 'hg::Color', ['const hg::Color &color', 'float saturation'])

	bind_std_vector(gen, color)


def bind_picture(gen):
	gen.add_include('engine/picture.h')

	gen.bind_named_enum('hg::PictureFormat', ['PF_RGB24', 'PF_RGBA32', 'PF_RGBA32F'])

	# hg::Picture
	picture = gen.begin_class('hg::Picture')

	gen.bind_constructor_overloads(picture, [
		([], []),
		(['const hg::Picture &picture'], []),
		(['uint16_t width', 'uint16_t height', 'hg::PictureFormat format'], []),
		(['void *data', 'uint16_t width', 'uint16_t height', 'hg::PictureFormat format'], [])
	])
	
	gen.bind_method(picture, 'GetWidth', 'uint32_t', [], [])
	gen.bind_method(picture, 'GetHeight', 'uint32_t', [], [])
	gen.bind_method(picture, 'GetFormat', 'hg::PictureFormat', [], [])

	gen.insert_binding_code('''
static intptr_t _Picture_GetData(hg::Picture *picture) {
	return reinterpret_cast<intptr_t>(picture->GetData());
}
static hg::Color _Picture_GetPixelRGBA(hg::Picture *picture, uint16_t x, uint16_t y) {
	return hg::GetPixelRGBA(*picture, x, y);
}
static void _Picture_SetPixelRGBA(hg::Picture *picture, uint16_t x, uint16_t y, const hg::Color &col) {
	hg::SetPixelRGBA(*picture, x, y, col);
}
''')	
	gen.bind_method(picture, 'GetData', 'intptr_t', [], {'route': route_lambda('_Picture_GetData')})	
	gen.bind_method(picture, 'SetData', 'void', ['void *data', 'uint16_t width', 'uint16_t height', 'hg::PictureFormat format'], [])
	gen.bind_method(picture, 'CopyData', 'void', ['const void *data', 'uint16_t width', 'uint16_t height', 'hg::PictureFormat format'], [])
	
	gen.bind_method(picture, 'GetPixelRGBA', 'hg::Color', ['uint16_t x', 'uint16_t y'], {'route': route_lambda('_Picture_GetPixelRGBA') })	
	gen.bind_method(picture, 'SetPixelRGBA', 'void', ['uint16_t x', 'uint16_t y', 'const hg::Color &col'], {'route': route_lambda('_Picture_SetPixelRGBA') })	

	gen.end_class(picture)
	
	# I/O
	gen.bind_function('LoadJPG', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('LoadPNG', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('LoadGIF', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('LoadPSD', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('LoadTGA', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('LoadBMP', 'bool', ['hg::Picture &pict', 'const char *path'])

	gen.bind_function('LoadPicture', 'bool', ['hg::Picture &pict', 'const char *path'])

	gen.bind_function('SavePNG', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('SaveTGA', 'bool', ['hg::Picture &pict', 'const char *path'])
	gen.bind_function('SaveBMP', 'bool', ['hg::Picture &pict', 'const char *path'])


def bind_math(gen):
	gen.begin_class('hg::Vec3')
	gen.begin_class('hg::Vec4')
	gen.begin_class('hg::Mat3')
	gen.begin_class('hg::Mat4')
	gen.begin_class('hg::Mat44')
	gen.begin_class('hg::Quaternion')

	# math
	gen.add_include('foundation/rect.h')
	gen.add_include('foundation/math.h')

	gen.bind_named_enum('hg::RotationOrder', ['RO_ZYX', 'RO_YZX', 'RO_ZXY', 'RO_XZY', 'RO_YXZ', 'RO_XYZ', 'RO_XY', 'RO_Default'], storage_type='uint8_t')
	gen.bind_named_enum('hg::Axis', ['A_X', 'A_Y', 'A_Z', 'A_RotX', 'A_RotY', 'A_RotZ', 'A_Last'], storage_type='uint8_t')

	gen.bind_function('hg::LinearInterpolate<float>', 'float', ['float y0', 'float y1', 'float t'], bound_name='LinearInterpolate')
	gen.bind_function('hg::CosineInterpolate<float>', 'float', ['float y0', 'float y1', 'float t'], bound_name='CosineInterpolate')
	gen.insert_binding_code('''
static const float _CubicInterpolateImpl(float y0, float y1, float y2, float y3, float t) { return hg::CubicInterpolate<float>(y0, y1, y2, y3, t); }
static const hg::Vec3 _CubicInterpolateImpl(const hg::Vec3& v0, const hg::Vec3& v1, const hg::Vec3& v2, const hg::Vec3& v3, float t) { return hg::CubicInterpolate<hg::Vec3>(v0, v1, v2, v3, t); }	
''')
	gen.bind_function_overloads('_CubicInterpolateImpl', [
		('float', ['float y0', 'float y1', 'float y2', 'float y3', 'float t'], []),
        ('hg::Vec3', ['const hg::Vec3& v0', 'const hg::Vec3& v1', 'const hg::Vec3& v2', 'const hg::Vec3& v3', 'float t'], [])
	], bound_name='CubicInterpolate')
	gen.bind_function('hg::HermiteInterpolate<float>', 'float', ['float y0', 'float y1', 'float y2', 'float y3', 'float t', 'float tension', 'float bias'], bound_name='HermiteInterpolate')

	gen.bind_function('hg::ReverseRotationOrder', 'hg::RotationOrder', ['hg::RotationOrder rotation_order'])

	# hg::MinMax
	gen.add_include('foundation/minmax.h')

	minmax = gen.begin_class('hg::MinMax')
	#minmax._inline = True

	gen.bind_members(minmax, ['hg::Vec3 mn', 'hg::Vec3 mx'])
	gen.bind_constructor_overloads(minmax, [
		([], []),
		(['const hg::Vec3 &min', 'const hg::Vec3 &max'], [])
	])

	gen.bind_arithmetic_op(minmax, '*', 'hg::MinMax', ['const hg::Mat4 &m'])
	gen.bind_comparison_ops(minmax, ['==', '!='], ['const hg::MinMax &minmax'])

	gen.end_class(minmax)

	gen.bind_function('GetArea', 'float', ['const hg::MinMax &minmax'])
	gen.bind_function('GetCenter', 'hg::Vec3', ['const hg::MinMax &minmax'])

	gen.bind_function('hg::ComputeMinMaxBoundingSphere', 'void', ['const hg::MinMax &minmax', 'hg::Vec3 &origin', 'float &radius'], {'arg_out': ['origin', 'radius']})

	gen.bind_function_overloads('hg::Overlap', [
		('bool', ['const hg::MinMax &minmax_a', 'const hg::MinMax &minmax_b'], []),
		('bool', ['const hg::MinMax &minmax_a', 'const hg::MinMax &minmax_b', 'hg::Axis axis'], [])
	])
	gen.bind_function('hg::Contains', 'bool', ['const hg::MinMax &minmax', 'const hg::Vec3 &position'])

	gen.bind_function_overloads('hg::Union', [
		('hg::MinMax', ['const hg::MinMax &minmax_a', 'const hg::MinMax &minmax_b'], []),
		('hg::MinMax', ['const hg::MinMax &minmax', 'const hg::Vec3 &position'], [])
	])

	gen.bind_function('hg::IntersectRay', 'bool', ['const hg::MinMax &minmax', 'const hg::Vec3 &origin', 'const hg::Vec3 &direction', 'float &t_min', 'float &t_max'], {'arg_out': ['t_min', 't_max']})

	gen.bind_function('hg::ClassifyLine', 'bool', ['const hg::MinMax &minmax', 'const hg::Vec3 &position', 'const hg::Vec3 &direction', 'hg::Vec3 &intersection', 'hg::Vec3 *normal'], {'arg_out': ['intersection', 'normal']})
	gen.bind_function('hg::ClassifySegment', 'bool', ['const hg::MinMax &minmax', 'const hg::Vec3 &p0', 'const hg::Vec3 &p1', 'hg::Vec3 &intersection', 'hg::Vec3 *normal'], {'arg_out': ['intersection', 'normal']})

	gen.bind_function('MinMaxFromPositionSize', 'hg::MinMax', ['const hg::Vec3 &position', 'const hg::Vec3 &size'])

	# hg::Vec2<T>
	gen.add_include('foundation/vector2.h')

	def bind_vector2_T(T, bound_name):
		vector2 = gen.begin_class('hg::tVec2<%s>'%T, bound_name=bound_name)
		vector2._inline = True

		gen.bind_static_members(vector2, ['const hg::tVec2<%s> Zero'%T, 'const hg::tVec2<%s> One'%T])

		gen.bind_members(vector2, ['%s x'%T, '%s y'%T])

		gen.bind_constructor_overloads(vector2, [
			([], []),
			(['%s x'%T, '%s y'%T], []),
			(['const hg::tVec2<%s> &v'%T], []),
			(['const hg::Vec3 &v'], []),
			(['const hg::Vec4 &v'], [])
		])

		gen.bind_arithmetic_ops_overloads(vector2, ['+', '-', '/'], [
			('hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &v'%T], []),
			('hg::tVec2<%s>'%T, ['const %s k'%T], [])
		])
		gen.bind_arithmetic_op_overloads(vector2, '*', [
			('hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &v'%T], []),
			('hg::tVec2<%s>'%T, ['const %s k'%T], []),
			('hg::tVec2<%s>'%T, ['const hg::Mat3 &m'], [])
		])
		gen.bind_inplace_arithmetic_ops_overloads(vector2, ['+=', '-=', '*=', '/='], [
			(['const hg::tVec2<%s> &v'%T], []),
			(['const %s k'%T], [])
		])

		gen.insert_binding_code('static void _Vector2_%s_Set(hg::tVec2<%s> *v, %s x, %s y) { v->x = x; v->y = y; }'%(T, T, T, T))
		gen.bind_method(vector2, 'Set', 'void', ['%s x'%T, '%s y'%T], {'route': route_lambda('_Vector2_%s_Set'%T)})

		gen.end_class(vector2)

		gen.bind_function('hg::Min', 'hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &a'%T, 'const hg::tVec2<%s> &b'%T])
		gen.bind_function('hg::Max', 'hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &a'%T, 'const hg::tVec2<%s> &b'%T])

		gen.bind_function('hg::Len2', T, ['const hg::tVec2<%s> &v'%T])
		gen.bind_function('hg::Len', T, ['const hg::tVec2<%s> &v'%T])

		gen.bind_function('hg::Dot', T, ['const hg::tVec2<%s> &a'%T, 'const hg::tVec2<%s> &b'%T])

		gen.bind_function('hg::Normalize', 'hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &v'%T])
		gen.bind_function('hg::Reverse', 'hg::tVec2<%s>'%T, ['const hg::tVec2<%s> &a'%T])

		gen.bind_function('hg::Dist2', T, ['const hg::tVec2<%s> &a'%T, 'const hg::tVec2<%s> &b'%T])
		gen.bind_function('hg::Dist', T, ['const hg::tVec2<%s> &a'%T, 'const hg::tVec2<%s> &b'%T])

		bind_std_vector(gen, vector2)

		return vector2

	vector2 = bind_vector2_T('float', 'Vec2')
	ivector2 = bind_vector2_T('int', 'iVec2')

	# hg::Vec4
	gen.add_include('foundation/vector4.h')

	vector4 = gen.begin_class('hg::Vec4')
	vector4._inline = True
	gen.bind_members(vector4, ['float x', 'float y', 'float z', 'float w'])

	gen.bind_constructor_overloads(vector4, [
		([], []),
		(['float x', 'float y', 'float z', '?float w'], []),
		(['const hg::tVec2<float> &v'], []),
		(['const hg::tVec2<int> &v'], []),
		(['const hg::Vec3 &v'], []),
		(['const hg::Vec4 &v'], [])
	])

	gen.bind_arithmetic_ops_overloads(vector4, ['+', '-', '/'], [
		('hg::Vec4', ['hg::Vec4 &v'], []),
		('hg::Vec4', ['float k'], [])
	])
	gen.bind_arithmetic_ops_overloads(vector4, ['*'], [
		('hg::Vec4', ['hg::Vec4 &v'], []),
		('hg::Vec4', ['float k'], [])
	])

	gen.bind_inplace_arithmetic_ops_overloads(vector4, ['+=', '-=', '*=', '/='], [
		(['hg::Vec4 &v'], []),
		(['float k'], [])
	])

	gen.insert_binding_code('static void _Vector4_Set(hg::Vec4 *v, float x, float y, float z, float w = 1.f) { v->x = x; v->y = y; v->z = z; v->w = w; }')
	gen.bind_method(vector4, 'Set', 'void', ['float x', 'float y', 'float z', '?float w'], {'route': route_lambda('_Vector4_Set')})

	gen.end_class(vector4)

	gen.bind_function('hg::Abs', 'hg::Vec4', ['const hg::Vec4 &v'])

	gen.bind_function('hg::Normalize', 'hg::Vec4', ['const hg::Vec4 &v'])
		
	gen.bind_function_overloads('hg::RandomVec4', [
		('hg::Vec4', ['float min', 'float max'], []),
		('hg::Vec4', ['const hg::Vec4 &min', 'const hg::Vec4 &max'], [])
	])

	bind_std_vector(gen, vector4)

	# hg::Quaternion
	gen.add_include('foundation/quaternion.h')

	quaternion = gen.begin_class('hg::Quaternion')
	quaternion._inline = True

	gen.bind_members(quaternion, ['float x', 'float y', 'float z', 'float w'])

	gen.bind_constructor_overloads(quaternion, [
		([], []),
		(['float x', 'float y', 'float z', 'float w'], []),
		(['const hg::Quaternion &q'], [])
	])

	#gen.bind_comparison_ops(quaternion, ['==', '!='], ['const hg::Quaternion &q'])
	gen.bind_arithmetic_ops_overloads(quaternion, ['+', '-', '*'], [
		('hg::Quaternion', ['float v'], []),
		('hg::Quaternion', ['hg::Quaternion &q'], [])
	])
	gen.bind_arithmetic_op(quaternion, '/', 'hg::Quaternion', ['float v'])
	gen.bind_inplace_arithmetic_ops_overloads(quaternion, ['+=', '-=', '*='], [
		(['float v'], []),
		(['const hg::Quaternion &q'], [])
	])
	gen.bind_inplace_arithmetic_op(quaternion, '/=', ['float v'])

	gen.end_class(quaternion)

	gen.bind_function('hg::Normalize', 'hg::Quaternion', ['const hg::Quaternion &q'])
	gen.bind_function('hg::Inverse', 'hg::Quaternion', ['const hg::Quaternion &q'])

	gen.bind_function('hg::Len2', 'float', ['const hg::Quaternion &q'])
	gen.bind_function('hg::Len', 'float', ['const hg::Quaternion &q'])

	gen.bind_function('hg::Dist', 'float', ['const hg::Quaternion &a', 'const hg::Quaternion &b'])
	gen.bind_function('hg::Slerp', 'hg::Quaternion', ['const hg::Quaternion &a', 'const hg::Quaternion &b', 'float t'])

	gen.bind_function_overloads('hg::QuaternionFromEuler', [
		('hg::Quaternion', ['float x', 'float y', 'float z', '?hg::RotationOrder rotation_order'], []),
		('hg::Quaternion', ['hg::Vec3 &euler', '?hg::RotationOrder rotation_order'], [])
	])
	gen.bind_function('hg::QuaternionLookAt', 'hg::Quaternion', ['const hg::Vec3 &at'])
	gen.bind_function('hg::QuaternionFromMatrix3', 'hg::Quaternion', ['const hg::Mat3 &m'])
	gen.bind_function('hg::QuaternionFromAxisAngle', 'hg::Quaternion', ['float angle', 'const hg::Vec3 &axis'])

	gen.bind_function('hg::ToMatrix3', 'hg::Mat3', ['const hg::Quaternion &q'])
	gen.bind_function('hg::ToEuler', 'hg::Vec3', ['const hg::Quaternion &q', '?hg::RotationOrder rotation_order'])

	# hg::Mat3
	gen.add_include('foundation/matrix3.h')

	matrix3 = gen.begin_class('hg::Mat3')
	gen.bind_static_members(matrix3, ['const hg::Mat3 Zero', 'const hg::Mat3 Identity'])

	gen.bind_constructor_overloads(matrix3, [
		([], []),
		(['const hg::Mat4 &m'], []),
		(['const hg::Vec3 &x', 'const hg::Vec3 &y', 'const hg::Vec3 &z'], [])
	])

	gen.bind_comparison_ops(matrix3, ['==', '!='], ['const hg::Mat3 &m'])

	gen.bind_arithmetic_ops(matrix3, ['+', '-'], 'hg::Mat3', ['hg::Mat3 &m'])
	gen.bind_arithmetic_op_overloads(matrix3, '*', [
		('hg::Mat3', ['const float v'], []),
		('hg::tVec2<float>', ['const hg::tVec2<float> &v'], []),
		('hg::Vec3', ['const hg::Vec3 &v'], []),
		('hg::Vec4', ['const hg::Vec4 &v'], []),
		('hg::Mat3', ['const hg::Mat3 &m'], [])
	])
	gen.bind_inplace_arithmetic_ops(matrix3, ['+=', '-='], ['const hg::Mat3 &m'])
	gen.bind_inplace_arithmetic_op_overloads(matrix3, '*=', [
		(['const float k'], []),
		(['const hg::Mat3 &m'], [])
	])

	gen.end_class(matrix3)

	gen.bind_function('hg::Det', 'float', ['const hg::Mat3 &m'])
	gen.bind_function('hg::Inverse', 'bool', ['const hg::Mat3 &m', 'hg::Mat3 &I'], {'arg_out': ['I']})

	gen.bind_function('hg::Transpose', 'hg::Mat3', ['const hg::Mat3 &m'])

	gen.bind_function('hg::GetRow', 'hg::Vec3', ['const hg::Mat3 &m', 'uint32_t n'])
	gen.bind_function('hg::GetColumn', 'hg::Vec3', ['const hg::Mat3 &m', 'uint32_t n'])
	gen.bind_function('hg::SetRow', 'void', ['hg::Mat3 &m', 'uint32_t n', 'const hg::Vec3 &row'])
	gen.bind_function('hg::SetColumn', 'void', ['hg::Mat3 &m', 'uint32_t n', 'const hg::Vec3 &column'])

	gen.bind_function('hg::GetX', 'hg::Vec3', ['const hg::Mat3 &m'])
	gen.bind_function('hg::GetY', 'hg::Vec3', ['const hg::Mat3 &m'])
	gen.bind_function('hg::GetZ', 'hg::Vec3', ['const hg::Mat3 &m'])
	gen.bind_function('hg::GetTranslation', 'hg::Vec3', ['const hg::Mat3 &m'])
	gen.bind_function('hg::GetScale', 'hg::Vec3', ['const hg::Mat3 &m'])

	gen.bind_function('hg::SetX', 'void', ['hg::Mat3 &m', 'const hg::Vec3 &X'])
	gen.bind_function('hg::SetY', 'void', ['hg::Mat3 &m', 'const hg::Vec3 &Y'])
	gen.bind_function('hg::SetZ', 'void', ['hg::Mat3 &m', 'const hg::Vec3 &Z'])
	gen.bind_function_overloads('hg::SetTranslation', [
		('void', ['hg::Mat3 &m', 'const hg::Vec3 &T'], []),
		('void', ['hg::Mat3 &m', 'const hg::tVec2<float> &T'], [])
	])
	gen.bind_function('hg::SetScale', 'void', ['hg::Mat3 &m', 'const hg::Vec3 &S'])
	gen.bind_function('hg::SetAxises', 'void', ['hg::Mat3 &m', 'const hg::Vec3 &X', 'const hg::Vec3 &Y', 'const hg::Vec3 &Z'])

	gen.bind_function('hg::Normalize', 'hg::Mat3', ['const hg::Mat3 &m'])
	gen.bind_function('hg::Orthonormalize', 'hg::Mat3', ['const hg::Mat3 &m'])
	gen.bind_function_overloads('hg::ToEuler', [
		('hg::Vec3', ['const hg::Mat3 &m'], []),
		('hg::Vec3', ['const hg::Mat3 &m', 'hg::RotationOrder rotation_order'], [])
	])

	gen.bind_function('hg::VectorMat3', 'hg::Mat3', ['const hg::Vec3 &V'])
	gen.bind_function_overloads('hg::TranslationMat3', [
		('hg::Mat3', ['const hg::tVec2<float> &T'], []),
		('hg::Mat3', ['const hg::Vec3 &T'], [])
	])
	gen.bind_function_overloads('hg::ScaleMat3', [
		('hg::Mat3', ['const hg::tVec2<float> &S'], []),
		('hg::Mat3', ['const hg::Vec3 &S'], [])
	])
	gen.bind_function('hg::CrossProductMat3', 'hg::Mat3', ['const hg::Vec3 &V'])

	gen.bind_function('hg::RotationMatX', 'hg::Mat3', ['float angle'])
	gen.bind_function('hg::RotationMatY', 'hg::Mat3', ['float angle'])
	gen.bind_function('hg::RotationMatZ', 'hg::Mat3', ['float angle'])

	gen.bind_function('hg::RotationMat2D', 'hg::Mat3', ['float angle', 'const hg::tVec2<float> &pivot'])

	gen.bind_function_overloads('hg::RotationMat3', [
		('hg::Mat3', ['float x', 'float y', 'float z'], []),
		('hg::Mat3', ['float x', 'float y', 'float z', 'hg::RotationOrder rotation_order'], []),
		('hg::Mat3', ['const hg::Vec3 &euler'], []),
		('hg::Mat3', ['const hg::Vec3 &euler', 'hg::RotationOrder rotation_order'], [])
	])

	gen.bind_function('hg::Mat3LookAt', 'hg::Mat3', ['const hg::Vec3 &front', '?const hg::Vec3 &up'])

	gen.bind_function('hg::RotationMatXZY', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatZYX', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatXYZ', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatZXY', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatYZX', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatYXZ', 'hg::Mat3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::RotationMatXY', 'hg::Mat3', ['float x', 'float y'])

	# hg::Mat4
	gen.add_include('foundation/matrix4.h')

	matrix4 = gen.begin_class('hg::Mat4')
	gen.bind_static_members(matrix4, ['const hg::Mat4 Zero', 'const hg::Mat4 Identity'])
	
	gen.insert_binding_code('static hg::Mat4 *_Mat4_Copy(const hg::Mat4 &m) { return new hg::Mat4(m); }')

	gen.bind_constructor_overloads(matrix4, [
		([], []),
		(['const hg::Mat4 &m'], {'route': route_lambda('_Mat4_Copy')}),
		(['float m00', 'float m10', 'float m20', 'float m01', 'float m11', 'float m21', 'float m02', 'float m12', 'float m22', 'float m03', 'float m13', 'float m23'], []),
		(['const hg::Mat3 &m'], [])
	])

	gen.bind_comparison_ops(matrix4, ['==', '!='], ['const hg::Mat4 &m'])

	gen.bind_arithmetic_ops(matrix4, ['+', '-'], 'hg::Mat4', ['hg::Mat4 &m'])
	gen.bind_arithmetic_op_overloads(matrix4, '*', [
		('hg::Mat4', ['const float v'], []),
		('hg::Mat4', ['const hg::Mat4 &m'], []),
		('hg::Vec3', ['const hg::Vec3 &v'], []),
		('hg::Vec4', ['const hg::Vec4 &v'], []),
		('hg::Mat44', ['const hg::Mat44 &m'], [])
	])

	gen.end_class(matrix4)

	gen.bind_function('hg::GetRow', 'hg::Vec4', ['const hg::Mat4 &m', 'unsigned int n'])
	gen.bind_function('hg::GetColumn', 'hg::Vec3', ['const hg::Mat4 &m', 'unsigned int n'])
	gen.bind_function('hg::SetRow', 'void', ['const hg::Mat4 &m', 'unsigned int n', 'const hg::Vec4 &v'])
	gen.bind_function('hg::SetColumn', 'void', ['const hg::Mat4 &m', 'unsigned int n', 'const hg::Vec3 &v'])

	gen.bind_function('hg::GetX', 'hg::Vec3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetY', 'hg::Vec3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetZ', 'hg::Vec3', ['const hg::Mat4 &m'])

	gen.bind_function('hg::GetT', 'hg::Vec3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetTranslation', 'hg::Vec3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetR', 'hg::Vec3', ['const hg::Mat4 &m', '?hg::RotationOrder rotation_order'])
	gen.bind_function('hg::GetRotation', 'hg::Vec3', ['const hg::Mat4 &m', '?hg::RotationOrder rotation_order'])
	gen.bind_function('hg::GetRMatrix', 'hg::Mat3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetRotationMatrix', 'hg::Mat3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetS', 'hg::Vec3', ['const hg::Mat4 &m'])
	gen.bind_function('hg::GetScale', 'hg::Vec3', ['const hg::Mat4 &m'])

	gen.bind_function('hg::SetX', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &X'])
	gen.bind_function('hg::SetY', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &Y'])
	gen.bind_function('hg::SetZ', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &Z'])

	gen.bind_function('hg::SetT', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &T'])
	gen.bind_function('hg::SetTranslation', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &T'])
	gen.bind_function('hg::SetS', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &scale'])
	gen.bind_function('hg::SetScale', 'void', ['const hg::Mat4 &m', 'const hg::Vec3 &scale'])

	gen.bind_function('hg::Inverse', 'bool', ['const hg::Mat4 &m', 'hg::Mat4 &I'], {'arg_out': ['I']})
	gen.bind_function('hg::InverseFast', 'hg::Mat4', ['const hg::Mat4 &m'])

	gen.bind_function('hg::Orthonormalize', 'hg::Mat4', ['const hg::Mat4 &m'])
	gen.bind_function('hg::LerpAsOrthonormalBase', 'hg::Mat4', ['const hg::Mat4 &from', 'const hg::Mat4 &to', 'float k', '?bool fast'])

	gen.bind_function('hg::Decompose', 'void', ['const hg::Mat4 &m', 'hg::Vec3 *position', 'hg::Vec3 *rotation', 'hg::Vec3 *scale', '?hg::RotationOrder rotation_order'], {'arg_out': ['position', 'rotation', 'scale']})

	gen.bind_function('hg::Mat4LookAt', 'hg::Mat4', ['const hg::Vec3 &position', 'const hg::Vec3 &at', '?const hg::Vec3 &scale'])
	gen.bind_function('hg::Mat4LookAtUp', 'hg::Mat4', ['const hg::Vec3 &position', 'const hg::Vec3 &at', 'const hg::Vec3 &up', '?const hg::Vec3 &scale'])

	gen.bind_function('hg::Mat4LookToward', 'hg::Mat4', ['const hg::Vec3 &position', 'const hg::Vec3 &direction', '?const hg::Vec3 &scale'])
	gen.bind_function('hg::Mat4LookTowardUp', 'hg::Mat4', ['const hg::Vec3 &position', 'const hg::Vec3 &direction', 'const hg::Vec3 &up', '?const hg::Vec3 &scale'])

	gen.bind_function('hg::TranslationMat4', 'hg::Mat4', ['const hg::Vec3 &t'])
	gen.bind_function_overloads('hg::RotationMat4', [
		('hg::Mat4', ['const hg::Vec3 &euler'], []),
		('hg::Mat4', ['const hg::Vec3 &euler', 'hg::RotationOrder order'], [])
	])
	gen.bind_function_overloads('hg::ScaleMat4', [
		('hg::Mat4', ['const hg::Vec3 &scale'], []),
		('hg::Mat4', ['float scale'], [])
	])
	gen.bind_function_overloads('hg::TransformationMat4', [
		('hg::Mat4', ['const hg::Vec3 &pos', 'const hg::Vec3 &rot', '?const hg::Vec3 &scale'], []),
		('hg::Mat4', ['const hg::Vec3 &pos', 'const hg::Mat3 &rot', '?const hg::Vec3 &scale'], [])
	])

	bind_std_vector(gen, matrix4)
	
	# hg::Mat44
	gen.add_include('foundation/matrix44.h')

	matrix44 = gen.begin_class('hg::Mat44')
	gen.bind_static_members(matrix44, ['const hg::Mat44 Zero', 'const hg::Mat44 Identity'])
	
	gen.bind_constructor_overloads(matrix44, [
		([], []),
		(['float m00', 'float m10', 'float m20', 'float m30',
		'float m01', 'float m11', 'float m21', 'float m31',
		'float m02', 'float m12', 'float m22', 'float m32',
		'float m03', 'float m13', 'float m23', 'float m33'], [])
	])

	gen.bind_arithmetic_op_overloads(matrix44, '*', [
		('hg::Mat44', ['const hg::Mat4 &m'], []),
		('hg::Mat44', ['const hg::Mat44 &m'], []),
		('hg::Vec3', ['const hg::Vec3 &v'], []),
		('hg::Vec4', ['const hg::Vec4 &v'], [])
	])

	gen.end_class(matrix44)

	gen.bind_function('hg::Inverse', 'hg::Mat44', ['const hg::Mat44 &m', 'bool &result'], {'arg_out': ['result']})

	gen.bind_function('hg::GetRow', 'hg::Vec4', ['const hg::Mat44 &m', 'uint32_t idx'])
	gen.bind_function('hg::GetColumn', 'hg::Vec4', ['const hg::Mat44 &m', 'uint32_t idx'])
	gen.bind_function('hg::SetRow', 'void', ['const hg::Mat44 &m', 'uint32_t idx', 'const hg::Vec4 &v'])
	gen.bind_function('hg::SetColumn', 'void', ['const hg::Mat44 &m', 'uint32_t idx', 'const hg::Vec4 &v'])

	# hg::Vec3
	gen.add_include('foundation/vector3.h')

	vector3 = gen.begin_class('hg::Vec3')
	vector3._inline = True

	gen.bind_static_members(vector3, ['const hg::Vec3 Zero', 'const hg::Vec3 One', 'const hg::Vec3 Left', 'const hg::Vec3 Right', 'const hg::Vec3 Up', 'const hg::Vec3 Down', 'const hg::Vec3 Front', 'const hg::Vec3 Back'])
	gen.bind_members(vector3, ['float x', 'float y', 'float z'])

	gen.bind_constructor_overloads(vector3, [
		([], []),
		(['float x', 'float y', 'float z'], []),
		(['const hg::tVec2<float> &v'], []),
		(['const hg::tVec2<int> &v'], []),
		(['const hg::Vec3 &v'], []),
		(['const hg::Vec4 &v'], [])
	])

	gen.bind_function('hg::MakeVec3', 'hg::Vec3', ['const hg::Vec4 &v'])

	gen.bind_arithmetic_ops_overloads(vector3, ['+', '-', '/'], [('hg::Vec3', ['hg::Vec3 &v'], []), ('hg::Vec3', ['float k'], [])])
	gen.bind_arithmetic_ops_overloads(vector3, ['*'], [
		('hg::Vec3', ['const hg::Vec3 &v'], []),
		('hg::Vec3', ['float k'], [])
	])

	gen.bind_inplace_arithmetic_ops_overloads(vector3, ['+=', '-=', '*=', '/='], [
		(['hg::Vec3 &v'], []),
		(['float k'], [])
	])
	gen.bind_comparison_ops(vector3, ['==', '!='], ['const hg::Vec3 &v'])

	gen.insert_binding_code('static void _Vector3_Set(hg::Vec3 *v, float x, float y, float z) { v->x = x; v->y = y; v->z = z; }')
	gen.bind_method(vector3, 'Set', 'void', ['float x', 'float y', 'float z'], {'route': route_lambda('_Vector3_Set')})

	gen.end_class(vector3)

	gen.bind_function_overloads('hg::RandomVec3', [
		('hg::Vec3', ['float min', 'float max'], []),
		('hg::Vec3', ['const hg::Vec3 &min', 'const hg::Vec3 &max'], [])
	])

	gen.bind_function('hg::BaseToEuler', 'hg::Vec3', ['const hg::Vec3 &z'])
	gen.bind_function('hg::BaseToEuler', 'hg::Vec3', ['const hg::Vec3 &z', 'const hg::Vec3 &y'])

	gen.bind_function('hg::Dist2', 'float', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])
	gen.bind_function('hg::Dist', 'float', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])

	gen.bind_function('hg::Len2', 'float', ['const hg::Vec3 &v'])
	gen.bind_function('hg::Len', 'float', ['const hg::Vec3 &v'])

	gen.bind_function('hg::Min', 'hg::Vec3', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])
	gen.bind_function('hg::Max', 'hg::Vec3', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])

	gen.bind_function('hg::Dot', 'float', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])
	gen.bind_function('hg::Cross', 'hg::Vec3', ['const hg::Vec3 &a', 'const hg::Vec3 &b'])

	gen.bind_function('hg::Reverse', 'hg::Vec3', ['const hg::Vec3 &v'])
	gen.bind_function('hg::Inverse', 'hg::Vec3', ['const hg::Vec3 &v'])

	gen.bind_function('hg::Normalize', 'hg::Vec3', ['const hg::Vec3 &v'])

	gen.bind_function('hg::Clamp', 'hg::Vec3', ['const hg::Vec3 &v', 'float min', 'float max'])
	gen.bind_function('hg::Clamp', 'hg::Vec3', ['const hg::Vec3 &v', 'const hg::Vec3 &min', 'const hg::Vec3 &max'])
	gen.bind_function('hg::ClampLen', 'hg::Vec3', ['const hg::Vec3 &v', 'float min', 'float max'])

	gen.bind_function('hg::Abs', 'hg::Vec3', ['const hg::Vec3 &v'])
	gen.bind_function('hg::Sign', 'hg::Vec3', ['const hg::Vec3 &v'])

	gen.bind_function('hg::Reflect', 'hg::Vec3', ['const hg::Vec3 &v', 'const hg::Vec3 &n'])
	gen.bind_function('hg::Refract', 'hg::Vec3', ['const hg::Vec3 &v', 'const hg::Vec3 &n', '?float k_in', '?float k_out'])

	gen.bind_function('hg::Floor', 'hg::Vec3', ['const hg::Vec3 &v'])
	gen.bind_function('hg::Ceil', 'hg::Vec3', ['const hg::Vec3 &v'])

	gen.bind_function('hg::FaceForward', 'hg::Vec3', ['const hg::Vec3 &v', 'const hg::Vec3 &d'])

	gen.bind_function('hg::Deg3', 'hg::Vec3', ['float x', 'float y', 'float z'])
	gen.bind_function('hg::Rad3', 'hg::Vec3', ['float x', 'float y', 'float z'])

	gen.bind_function('hg::Vec3I', 'hg::Vec3', ['int x', 'int y', 'int z'])
	gen.bind_function('hg::Vec4I', 'hg::Vec4', ['int x', 'int y', 'int z', '?int w'])

	bind_std_vector(gen, vector3)

	# hg::Rect<T>
	def bind_rect_T(T, bound_name):
		rect = gen.begin_class('hg::Rect<%s>'%T, bound_name=bound_name)
		rect._inline = True

		gen.bind_members(rect, ['%s sx'%T, '%s sy'%T, '%s ex'%T, '%s ey'%T])

		gen.bind_constructor_overloads(rect, [
			([], []),
			(['%s x'%T, '%s y'%T], []),
			(['%s sx'%T, '%s sy'%T, '%s ex'%T, '%s ey'%T], []),
			(['const hg::Rect<%s> &rect'%T], [])
		])

		gen.end_class(rect)

		gen.bind_function('hg::GetX', T, ['const hg::Rect<%s> &rect' % T])
		gen.bind_function('hg::GetY', T, ['const hg::Rect<%s> &rect' % T])
		gen.bind_function('hg::SetX', 'void', ['hg::Rect<%s> &rect' % T, '%s x' % T])
		gen.bind_function('hg::SetY', 'void', ['hg::Rect<%s> &rect' % T, '%s y' % T])

		gen.bind_function('hg::GetWidth', T, ['const hg::Rect<%s> &rect' % T])
		gen.bind_function('hg::GetHeight', T, ['const hg::Rect<%s> &rect' % T])
		gen.bind_function('hg::SetWidth', 'void', ['hg::Rect<%s> &rect' % T, '%s width' % T])
		gen.bind_function('hg::SetHeight', 'void', ['hg::Rect<%s> &rect' % T, '%s height' % T])

		gen.bind_function('hg::GetSize', 'hg::tVec2<%s>'%T, ['const hg::Rect<%s> &rect' % T])

		gen.bind_function('hg::Inside', 'bool', ['const hg::Rect<%s> &rect' % T, 'hg::tVec2<int> &v'])
		gen.bind_function('hg::Inside', 'bool', ['const hg::Rect<%s> &rect' % T, 'hg::tVec2<float> &v'])
		gen.bind_function('hg::Inside', 'bool', ['const hg::Rect<%s> &rect' % T, 'hg::Vec3 &v'])
		gen.bind_function('hg::Inside', 'bool', ['const hg::Rect<%s> &rect' % T, 'hg::Vec4 &v'])

		gen.bind_function('hg::FitsInside', 'bool', ['const hg::Rect<%s> &a' % T, 'const hg::Rect<%s> &b' % T])
		gen.bind_function('hg::Intersects', 'bool', ['const hg::Rect<%s> &a' % T, 'const hg::Rect<%s> &b' % T])

		gen.bind_function('hg::Intersection', 'hg::Rect<%s>'%T, ['const hg::Rect<%s> &a' % T, 'const hg::Rect<%s> &b' % T])

		gen.bind_function('hg::Grow', 'hg::Rect<%s>'%T, ['const hg::Rect<%s> &rect' % T, '%s border'%T])

		gen.bind_function('hg::Offset', 'hg::Rect<%s>'%T, ['const hg::Rect<%s> &rect' % T, '%s x'%T, '%s y'%T])
		gen.bind_function('hg::Crop', 'hg::Rect<%s>'%T, ['const hg::Rect<%s> &rect' % T, '%s left'%T, '%s top'%T, '%s right'%T, '%s bottom'%T])

		gen.bind_function('hg::MakeRectFromWidthHeight', 'hg::Rect<%s>'%T, ['%s x'%T, '%s y'%T, '%s w'%T, '%s h'%T])

	bind_rect_T('float', 'Rect')
	bind_rect_T('int', 'IntRect')

	gen.bind_function('hg::ToFloatRect', 'hg::Rect<float>', ['const hg::Rect<int> &rect'])
	gen.bind_function('hg::ToIntRect', 'hg::Rect<int>', ['const hg::Rect<float> &rect'])

	# hg::Plane
	gen.add_include('foundation/plane.h')

	gen.typedef('hg::Plane', 'hg::Vec4')

	gen.bind_function('hg::MakePlane', 'hg::Plane', ['const hg::Vec3 &p', 'const hg::Vec3 &n', '?const hg::Mat4 &m'])
	gen.bind_function('hg::DistanceToPlane', 'float', ['const hg::Plane &plane', 'const hg::Vec3 &p'])

	# math std::vector
	#bind_std_vector(gen, vector2)
	#bind_std_vector(gen, ivector2)
	#bind_std_vector(gen, vector3)
	#bind_std_vector(gen, vector4)
	#bind_std_vector(gen, matrix3)
	#bind_std_vector(gen, matrix4)
	#bind_std_vector(gen, matrix44)

	
	gen.bind_function('hg::Abs', 'float', ['float v'])
	gen.bind_function('hg::Abs', 'int', ['int v'])

	gen.bind_function('hg::Min', 'float', ['float a', 'float b'])
	gen.bind_function('hg::Min', 'int', ['int a', 'int b'])

	gen.bind_function('hg::Max', 'float', ['float a', 'float b'])
	gen.bind_function('hg::Max', 'int', ['int a', 'int b'])

	gen.bind_function('hg::Clamp', 'float', ['float v', 'float min', 'float max'])
	gen.bind_function('hg::Clamp', 'int', ['int v', 'int min', 'int max'])

	gen.bind_function('hg::Wrap', 'float', ['float v', 'float start', 'float end'])
	gen.bind_function('hg::Wrap', 'int', ['int v', 'int start', 'int end'])

	gen.bind_function('hg::Lerp', 'int', ['int a', 'int b', 'float t'])
	gen.bind_function('hg::Lerp', 'float', ['float a', 'float b', 'float t'])
	gen.bind_function('hg::Lerp', 'hg::Vec3', ['const hg::Vec3 &a', 'const hg::Vec3 &b', 'float t'])
	gen.bind_function('hg::Lerp', 'hg::Vec4', ['const hg::Vec4 &a', 'const hg::Vec4 &b', 'float t'])

	gen.bind_function('hg::Quantize', 'float', ['float v', 'float q'])
	gen.bind_function('hg::IsFinite', 'bool', ['float v'])

	gen.bind_function('hg::Deg', 'float', ['float degrees'])
	gen.bind_function('hg::Rad', 'float', ['float radians'])

	gen.bind_function('hg::DegreeToRadian', 'float', ['float degrees'])
	gen.bind_function('hg::RadianToDegree', 'float', ['float radians'])

	gen.bind_function('hg::Sec', 'float', ['float seconds'])
	gen.bind_function('hg::Ms', 'float', ['float milliseconds'])

	gen.bind_function('hg::Km', 'float', ['float km'])
	gen.bind_function('hg::Mtr', 'float', ['float m'])
	gen.bind_function('hg::Cm', 'float', ['float cm'])
	gen.bind_function('hg::Mm', 'float', ['float mm'])
	gen.bind_function('hg::Inch', 'float', ['float inch'])


def bind_rand(gen):
	gen.add_include('foundation/rand.h')

	gen.bind_function('hg::Seed', 'void', ['uint32_t seed'])
	gen.bind_function('hg::Rand', 'uint32_t', ['?uint32_t range'])

	gen.bind_function('hg::FRand', 'float', ['?float range'])
	gen.bind_function('hg::FRRand', 'float', ['?float range_start', '?float range_end'])


def bind_frustum(gen):
	gen.add_include('foundation/frustum.h')

	gen.insert_binding_code('''
static const hg::Vec4 &_Frustum_GetTop(hg::Frustum *fs) { return (*fs)[hg::FP_Top]; }
static void _Frustum_SetTop(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Top] = plane; }
static const hg::Vec4 &_Frustum_GetBottom(hg::Frustum *fs) { return (*fs)[hg::FP_Bottom]; }
static void _Frustum_SetBottom(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Bottom] = plane; }
static const hg::Vec4 &_Frustum_GetLeft(hg::Frustum *fs) { return (*fs)[hg::FP_Left]; }
static void _Frustum_SetLeft(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Left] = plane; }
static const hg::Vec4 &_Frustum_GetRight(hg::Frustum *fs) { return (*fs)[hg::FP_Right]; }
static void _Frustum_SetRight(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Right] = plane; }
static const hg::Vec4 &_Frustum_GetNear(hg::Frustum *fs) { return (*fs)[hg::FP_Near]; }
static void _Frustum_SetNear(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Near] = plane; }
static const hg::Vec4 &_Frustum_GetFar(hg::Frustum *fs) { return (*fs)[hg::FP_Far]; }
static void _Frustum_SetFar(hg::Frustum *fs, const hg::Vec4 &plane) { (*fs)[hg::FP_Far] = plane; }
''')

	frustum = gen.begin_class('hg::Frustum')
	gen.bind_method(frustum, 'GetTop', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetTop')})
	gen.bind_method(frustum, 'SetTop', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetTop')})
	gen.bind_method(frustum, 'GetBottom', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetBottom')})
	gen.bind_method(frustum, 'SetBottom', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetBottom')})
	gen.bind_method(frustum, 'GetLeft', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetLeft')})
	gen.bind_method(frustum, 'SetLeft', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetLeft')})
	gen.bind_method(frustum, 'GetRight', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetRight')})
	gen.bind_method(frustum, 'SetRight', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetRight')})
	gen.bind_method(frustum, 'GetNear', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetNear')})
	gen.bind_method(frustum, 'SetNear', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetNear')})
	gen.bind_method(frustum, 'GetFar', 'hg::Vec4', [], {'route': route_lambda('_Frustum_GetFar')})
	gen.bind_method(frustum, 'SetFar', 'void', ['const hg::Vec4 &plane'], {'route': route_lambda('_Frustum_SetFar')})
	gen.end_class(frustum)

	gen.bind_function('hg::MakeFrustum', 'hg::Frustum', ['const hg::Mat44 &projection', '?const hg::Mat4 &mtx'])

	gen.bind_function('hg::TransformFrustum', 'hg::Frustum', ['const hg::Frustum &frustum', 'const hg::Mat4 &mtx'])

	gen.bind_named_enum('hg::Visibility', ['V_Outside', 'V_Inside', 'V_Clipped'], 'uint8_t')
	gen.bind_function('hg::TestVisibility', 'hg::Visibility', ['const hg::Frustum &frustum', 'uint32_t count', 'const hg::Vec3 *points', '?float distance'])
	gen.bind_function('hg::TestVisibility', 'hg::Visibility', ['const hg::Frustum &frustum', 'const hg::Vec3 &origin', 'float radius'])
	gen.bind_function('hg::TestVisibility', 'hg::Visibility', ['const hg::Frustum &frustum', 'const hg::MinMax &minmax'])


def bind_imgui(gen):
	gen.add_include('foundation/format.h')
	gen.add_include('engine/dear_imgui.h')
	gen.add_include('imgui/imgui.h', True)

	dear_imgui_context = gen.begin_class('hg::DearImguiContext')
	gen.end_class(dear_imgui_context)

	gen.bind_named_enum('ImGuiWindowFlags', [
		'ImGuiWindowFlags_NoTitleBar', 'ImGuiWindowFlags_NoResize', 'ImGuiWindowFlags_NoMove', 'ImGuiWindowFlags_NoScrollbar', 'ImGuiWindowFlags_NoScrollWithMouse',
		'ImGuiWindowFlags_NoCollapse', 'ImGuiWindowFlags_AlwaysAutoResize', 'ImGuiWindowFlags_NoSavedSettings', 'ImGuiWindowFlags_NoInputs',
		'ImGuiWindowFlags_MenuBar', 'ImGuiWindowFlags_HorizontalScrollbar', 'ImGuiWindowFlags_NoFocusOnAppearing', 'ImGuiWindowFlags_NoBringToFrontOnFocus',
		'ImGuiWindowFlags_AlwaysVerticalScrollbar', 'ImGuiWindowFlags_AlwaysHorizontalScrollbar', 'ImGuiWindowFlags_AlwaysUseWindowPadding', 'ImGuiWindowFlags_NoDocking'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiPopupFlags', [
		'ImGuiPopupFlags_None',
		'ImGuiPopupFlags_MouseButtonLeft', 'ImGuiPopupFlags_MouseButtonRight', 'ImGuiPopupFlags_MouseButtonMiddle',
		'ImGuiPopupFlags_NoOpenOverExistingPopup', 'ImGuiPopupFlags_NoOpenOverItems', 'ImGuiPopupFlags_AnyPopupId', 'ImGuiPopupFlags_AnyPopupLevel', 'ImGuiPopupFlags_AnyPopup'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiCond', ['ImGuiCond_Always', 'ImGuiCond_Once', 'ImGuiCond_FirstUseEver', 'ImGuiCond_Appearing'], 'int', namespace='')

	gen.bind_named_enum('ImGuiMouseButton', [
		'ImGuiPopupFlags_None',
		'ImGuiPopupFlags_MouseButtonLeft', 'ImGuiPopupFlags_MouseButtonRight', 'ImGuiPopupFlags_MouseButtonMiddle',
		'ImGuiPopupFlags_NoOpenOverExistingPopup', 'ImGuiPopupFlags_NoOpenOverItems',
		'ImGuiPopupFlags_AnyPopupId', 'ImGuiPopupFlags_AnyPopupLevel', 'ImGuiPopupFlags_AnyPopup'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiHoveredFlags', [
		'ImGuiHoveredFlags_None', 'ImGuiHoveredFlags_ChildWindows', 'ImGuiHoveredFlags_RootWindow', 'ImGuiHoveredFlags_AnyWindow', 'ImGuiHoveredFlags_AllowWhenBlockedByPopup',
		'ImGuiHoveredFlags_AllowWhenBlockedByActiveItem', 'ImGuiHoveredFlags_AllowWhenOverlapped', 'ImGuiHoveredFlags_AllowWhenDisabled', 'ImGuiHoveredFlags_RectOnly', 'ImGuiHoveredFlags_RootAndChildWindows'
	], 'int', namespace='')
	gen.bind_named_enum('ImGuiFocusedFlags', ['ImGuiFocusedFlags_ChildWindows', 'ImGuiFocusedFlags_RootWindow', 'ImGuiFocusedFlags_RootAndChildWindows'], 'int', namespace='')

	gen.bind_named_enum('ImGuiColorEditFlags', [
		'ImGuiColorEditFlags_None', 'ImGuiColorEditFlags_NoAlpha', 'ImGuiColorEditFlags_NoPicker', 'ImGuiColorEditFlags_NoOptions', 'ImGuiColorEditFlags_NoSmallPreview', 'ImGuiColorEditFlags_NoInputs',
		'ImGuiColorEditFlags_NoTooltip', 'ImGuiColorEditFlags_NoLabel', 'ImGuiColorEditFlags_NoSidePreview', 'ImGuiColorEditFlags_NoDragDrop', 'ImGuiColorEditFlags_AlphaBar', 'ImGuiColorEditFlags_AlphaPreview',
		'ImGuiColorEditFlags_AlphaPreviewHalf', 'ImGuiColorEditFlags_HDR', 'ImGuiColorEditFlags_DisplayRGB', 'ImGuiColorEditFlags_DisplayHSV', 'ImGuiColorEditFlags_DisplayHex', 'ImGuiColorEditFlags_Uint8',
		'ImGuiColorEditFlags_Float', 'ImGuiColorEditFlags_PickerHueBar', 'ImGuiColorEditFlags_PickerHueWheel', 'ImGuiColorEditFlags_InputRGB', 'ImGuiColorEditFlags_InputHSV'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiInputTextFlags', [
		'ImGuiInputTextFlags_CharsDecimal', 'ImGuiInputTextFlags_CharsHexadecimal', 'ImGuiInputTextFlags_CharsUppercase', 'ImGuiInputTextFlags_CharsNoBlank',
		'ImGuiInputTextFlags_AutoSelectAll', 'ImGuiInputTextFlags_EnterReturnsTrue', 'ImGuiInputTextFlags_CallbackCompletion', 'ImGuiInputTextFlags_CallbackHistory',
		'ImGuiInputTextFlags_CallbackAlways', 'ImGuiInputTextFlags_CallbackCharFilter', 'ImGuiInputTextFlags_AllowTabInput', 'ImGuiInputTextFlags_CtrlEnterForNewLine',
		'ImGuiInputTextFlags_NoHorizontalScroll', 'ImGuiInputTextFlags_AlwaysOverwrite', 'ImGuiInputTextFlags_ReadOnly', 'ImGuiInputTextFlags_Password'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiTreeNodeFlags', [
		'ImGuiTreeNodeFlags_Selected', 'ImGuiTreeNodeFlags_Framed', 'ImGuiTreeNodeFlags_AllowItemOverlap', 'ImGuiTreeNodeFlags_NoTreePushOnOpen',
		'ImGuiTreeNodeFlags_NoAutoOpenOnLog', 'ImGuiTreeNodeFlags_DefaultOpen', 'ImGuiTreeNodeFlags_OpenOnDoubleClick', 'ImGuiTreeNodeFlags_OpenOnArrow',
		'ImGuiTreeNodeFlags_Leaf', 'ImGuiTreeNodeFlags_Bullet', 'ImGuiTreeNodeFlags_CollapsingHeader'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiSelectableFlags', ['ImGuiSelectableFlags_DontClosePopups', 'ImGuiSelectableFlags_SpanAllColumns', 'ImGuiSelectableFlags_AllowDoubleClick'], 'int', namespace='')

	gen.bind_named_enum('ImGuiCol', [
		'ImGuiCol_Text', 'ImGuiCol_TextDisabled', 'ImGuiCol_WindowBg', 'ImGuiCol_ChildBg', 'ImGuiCol_PopupBg', 'ImGuiCol_Border', 'ImGuiCol_BorderShadow', 'ImGuiCol_FrameBg', 'ImGuiCol_FrameBgHovered', 
		'ImGuiCol_FrameBgActive', 'ImGuiCol_TitleBg', 'ImGuiCol_TitleBgActive', 'ImGuiCol_TitleBgCollapsed', 'ImGuiCol_MenuBarBg', 'ImGuiCol_ScrollbarBg', 'ImGuiCol_ScrollbarGrab', 'ImGuiCol_ScrollbarGrabHovered', 
		'ImGuiCol_ScrollbarGrabActive', 'ImGuiCol_CheckMark', 'ImGuiCol_SliderGrab', 'ImGuiCol_SliderGrabActive', 'ImGuiCol_Button', 'ImGuiCol_ButtonHovered', 'ImGuiCol_ButtonActive', 'ImGuiCol_Header', 
		'ImGuiCol_HeaderHovered', 'ImGuiCol_HeaderActive', 'ImGuiCol_Separator', 'ImGuiCol_SeparatorHovered', 'ImGuiCol_SeparatorActive', 'ImGuiCol_ResizeGrip', 'ImGuiCol_ResizeGripHovered', 'ImGuiCol_ResizeGripActive', 
		'ImGuiCol_PlotLines', 'ImGuiCol_PlotLinesHovered', 'ImGuiCol_PlotHistogram', 'ImGuiCol_PlotHistogramHovered', 'ImGuiCol_TextSelectedBg', 'ImGuiCol_DragDropTarget', 'ImGuiCol_NavHighlight', 
		'ImGuiCol_NavWindowingHighlight', 'ImGuiCol_NavWindowingDimBg', 'ImGuiCol_ModalWindowDimBg'
	], 'int', namespace='')

	gen.bind_named_enum('ImGuiStyleVar', [
		'ImGuiStyleVar_Alpha', 'ImGuiStyleVar_WindowPadding', 'ImGuiStyleVar_WindowRounding', 'ImGuiStyleVar_WindowMinSize', 'ImGuiStyleVar_ChildRounding', 'ImGuiStyleVar_FramePadding',
		'ImGuiStyleVar_FrameRounding', 'ImGuiStyleVar_ItemSpacing', 'ImGuiStyleVar_ItemInnerSpacing', 'ImGuiStyleVar_IndentSpacing', 'ImGuiStyleVar_GrabMinSize', 'ImGuiStyleVar_ButtonTextAlign'
	], 'int', namespace='')

	gen.bind_named_enum('ImDrawFlags', [
		'ImDrawFlags_RoundCornersNone',
		'ImDrawFlags_RoundCornersTopLeft', 'ImDrawFlags_RoundCornersTopRight', 'ImDrawFlags_RoundCornersBottomLeft', 'ImDrawFlags_RoundCornersBottomRight',
		'ImDrawFlags_RoundCornersAll'
	], 'int', namespace='')

	#gen.bind_function('ImGui::GetIO', 'ImGuiIO &', [], bound_name='ImGuiGetIO')
	#gen.bind_function('ImGui::GetStyle', 'ImGuiStyle &', [], bound_name='ImGuiGetStyle')

	imfont = gen.begin_class('ImFont')
	gen.end_class(imfont)

	gen.typedef('ImU32', 'unsigned int')

	imdrawlist = gen.begin_class('ImDrawList')

	gen.bind_method(imdrawlist, 'PushClipRect', 'void', ['hg::tVec2<float> clip_rect_min', 'hg::tVec2<float> clip_rect_max', '?bool intersect_with_curent_clip_rect'])
	gen.bind_method(imdrawlist, 'PushClipRectFullScreen', 'void', [])
	gen.bind_method(imdrawlist, 'PopClipRect', 'void', [])
	gen.bind_method(imdrawlist, 'PushTextureID', 'void', ['const hg::Texture &tex'], {'route': route_lambda('ImGui::PushTextureID')})
	gen.bind_method(imdrawlist, 'PopTextureID', 'void', [])
	gen.bind_method(imdrawlist, 'GetClipRectMin', 'hg::tVec2<float>', [])
	gen.bind_method(imdrawlist, 'GetClipRectMax', 'hg::tVec2<float>', [])
	
	gen.bind_method(imdrawlist, 'AddLine', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'ImU32 col', '?float thickness'])
	gen.bind_method(imdrawlist, 'AddRect', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'ImU32 col', '?float rounding', '?int rounding_corner_flags', '?float thickness'])
	gen.bind_method(imdrawlist, 'AddRectFilled', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'ImU32 col', '?float rounding', '?int rounding_corner_flags'])
	gen.bind_method(imdrawlist, 'AddRectFilledMultiColor', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'ImU32 col_upr_left', 'ImU32 col_upr_right', 'ImU32 col_bot_right', 'ImU32 col_bot_left'])
	gen.bind_method(imdrawlist, 'AddQuad', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'const hg::tVec2<float> &d', 'ImU32 col', '?float thickness'])
	gen.bind_method(imdrawlist, 'AddQuadFilled', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'const hg::tVec2<float> &d', 'ImU32 col'])
	gen.bind_method(imdrawlist, 'AddTriangle', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'ImU32 col', '?float thickness'])
	gen.bind_method(imdrawlist, 'AddTriangleFilled', 'void', ['const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'ImU32 col'])
	gen.bind_method(imdrawlist, 'AddCircle', 'void', ['const hg::tVec2<float> &centre', 'float radius', 'ImU32 col', '?int num_segments', '?float thickness'])
	gen.bind_method(imdrawlist, 'AddCircleFilled', 'void', ['const hg::tVec2<float> &centre', 'float radius', 'ImU32 col', '?int num_segments'])

	gen.insert_binding_code('''static void _ImDrawList_AddText(ImDrawList *self, const ImFont *font, float font_size, const hg::tVec2<float> &pos, ImU32 col, const char *text, float wrap_width = 0.0f, const hg::Vec4 *cpu_fine_clip_rect = nullptr) {
	self->AddText(font, font_size, pos, col, text, nullptr, wrap_width, reinterpret_cast<const ImVec4 *>(cpu_fine_clip_rect));
}''')

	gen.bind_method_overloads(imdrawlist, 'AddText', [
		('void', ['const hg::tVec2<float> &pos', 'ImU32 col', 'const char *text'], []),
		('void', ['const ImFont *font', 'float font_size', 'const hg::tVec2<float> &pos', 'ImU32 col', 'const char *text', '?float wrap_width', '?const hg::Vec4 *cpu_fine_clip_rect'], {'route': route_lambda('_ImDrawList_AddText')})
	])

	gen.bind_method_overloads(imdrawlist, 'AddImage', [
		('void', ['const hg::Texture &tex', 'const hg::tVec2<float> &a', 'const hg::tVec2<float> &b'], {'route': route_lambda('ImGui::AddImage')}),
		('void', ['const hg::Texture &tex', 'const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &uv_a', 'const hg::tVec2<float> &uv_b', '?ImU32 col'], {'route': route_lambda('ImGui::AddImage')})
	])
	gen.bind_method_overloads(imdrawlist, 'AddImageQuad', [
		('void', ['const hg::Texture &tex', 'const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'const hg::tVec2<float> &d'], {'route': route_lambda('ImGui::AddImageQuad')}),
		('void', ['const hg::Texture &tex', 'const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &c', 'const hg::tVec2<float> &d', 'const hg::tVec2<float> &uv_a', 'const hg::tVec2<float> &uv_b', 'const hg::tVec2<float> &uv_c', 'const hg::tVec2<float> &uv_d', '?ImU32 col'], {'route': route_lambda('ImGui::AddImageQuad')})
	])
	gen.bind_method(imdrawlist, 'AddImageRounded', 'void', ['const hg::Texture &tex', 'const hg::tVec2<float> &a', 'const hg::tVec2<float> &b', 'const hg::tVec2<float> &uv_a', 'const hg::tVec2<float> &uv_b', 'ImU32 col', 'float rounding', '?ImDrawFlags flags'], {'route': route_lambda('ImGui::AddImageRounded')})

	gen.insert_binding_code('''
static void _ImDrawList_AddPolyline(ImDrawList *self, const std::vector<hg::tVec2<float>> &points, ImU32 col, bool closed, float thickness) { self->AddPolyline(reinterpret_cast<const ImVec2 *>(points.data()), points.size(), col, closed, thickness); }
static void _ImDrawList_AddConvexPolyFilled(ImDrawList *self, const std::vector<hg::tVec2<float>> &points, ImU32 col) { self->AddConvexPolyFilled(reinterpret_cast<const ImVec2 *>(points.data()), points.size(), col); }
''')
	gen.bind_method(imdrawlist, 'AddPolyline', 'void', ['const std::vector<hg::tVec2<float>> &points', 'ImU32 col', 'bool closed', 'float thickness'], {'route': route_lambda('_ImDrawList_AddPolyline')})
	gen.bind_method(imdrawlist, 'AddConvexPolyFilled', 'void', ['const std::vector<hg::tVec2<float>> &points', 'ImU32 col'], {'route': route_lambda('_ImDrawList_AddConvexPolyFilled')})
	gen.bind_method(imdrawlist, 'AddBezierCubic', 'void', ['const hg::tVec2<float> &pos0', 'const hg::tVec2<float> &cp0', 'const hg::tVec2<float> &cp1', 'const hg::tVec2<float> &pos1', 'ImU32 col', 'float thickness', '?int num_segments'])

	gen.bind_method(imdrawlist, 'PathClear', 'void', [])

	gen.bind_method(imdrawlist, 'PathLineTo', 'void', ['const hg::tVec2<float> &pos'])
	gen.bind_method(imdrawlist, 'PathLineToMergeDuplicate', 'void', ['const hg::tVec2<float> &pos'])
	gen.bind_method(imdrawlist, 'PathFillConvex', 'void', ['ImU32 col'])
	gen.bind_method(imdrawlist, 'PathStroke', 'void', ['ImU32 col', 'bool closed', '?float thickness'])

	gen.bind_method(imdrawlist, 'PathArcTo', 'void', ['const hg::tVec2<float> &centre', 'float radius', 'float a_min', 'float a_max', '?int num_segments'])
	gen.bind_method(imdrawlist, 'PathArcToFast', 'void', ['const hg::tVec2<float> &centre', 'float radius', 'int a_min_of_12', 'int a_max_of_12'])
	gen.bind_method(imdrawlist, 'PathBezierCubicCurveTo', 'void', ['const hg::tVec2<float> &p1', 'const hg::tVec2<float> &p2', 'const hg::tVec2<float> &p3', '?int num_segments'])
	gen.bind_method(imdrawlist, 'PathRect', 'void', ['const hg::tVec2<float> &rect_min', 'const hg::tVec2<float> &rect_max', '?float rounding', '?ImDrawFlags flags'])

	gen.bind_method(imdrawlist, 'ChannelsSplit', 'void', ['int channels_count'])
	gen.bind_method(imdrawlist, 'ChannelsMerge', 'void', [])
	gen.bind_method(imdrawlist, 'ChannelsSetCurrent', 'void', ['int channel_index'])
	
	gen.end_class(imdrawlist)

	#
	gen.bind_function('ImGui::NewFrame', 'void', [], bound_name='ImGuiNewFrame')
	gen.bind_function('ImGui::Render', 'void', [], bound_name='ImGuiRender')

	gen.bind_function_overloads('ImGui::Begin', [
		('bool', ['const char *name'], []),
		('bool', ['const char *name', 'bool *open', 'ImGuiWindowFlags flags'], {'arg_in_out': ['open']})
	], bound_name='ImGuiBegin')
	gen.bind_function('ImGui::End', 'void', [], bound_name='ImGuiEnd')

	gen.bind_function('ImGui::BeginChild', 'bool', ['const char *id', '?const hg::tVec2<float> &size', '?bool border', '?ImGuiWindowFlags flags'], bound_name='ImGuiBeginChild')
	gen.bind_function('ImGui::EndChild', 'void', [], bound_name='ImGuiEndChild')

	gen.bind_function('ImGui::GetContentRegionMax', 'hg::tVec2<float>', [], bound_name='ImGuiGetContentRegionMax')
	gen.bind_function('ImGui::GetContentRegionAvail', 'hg::tVec2<float>', [], bound_name='ImGuiGetContentRegionAvail')
	gen.bind_function('ImGui::GetContentRegionAvailWidth', 'float', [], bound_name='ImGuiGetContentRegionAvailWidth')
	gen.bind_function('ImGui::GetWindowContentRegionMin', 'hg::tVec2<float>', [], bound_name='ImGuiGetWindowContentRegionMin')
	gen.bind_function('ImGui::GetWindowContentRegionMax', 'hg::tVec2<float>', [], bound_name='ImGuiGetWindowContentRegionMax')
	gen.bind_function('ImGui::GetWindowContentRegionWidth', 'float', [], bound_name='ImGuiGetWindowContentRegionWidth')
	gen.bind_function('ImGui::GetWindowDrawList', 'ImDrawList *', [], bound_name='ImGuiGetWindowDrawList')
	gen.bind_function('ImGui::GetWindowPos', 'hg::tVec2<float>', [], bound_name='ImGuiGetWindowPos')
	gen.bind_function('ImGui::GetWindowSize', 'hg::tVec2<float>', [], bound_name='ImGuiGetWindowSize')
	gen.bind_function('ImGui::GetWindowWidth', 'float', [], bound_name='ImGuiGetWindowWidth')
	gen.bind_function('ImGui::GetWindowHeight', 'float', [], bound_name='ImGuiGetWindowHeight')
	gen.bind_function('ImGui::IsWindowCollapsed', 'bool', [], bound_name='ImGuiIsWindowCollapsed')
	gen.bind_function('ImGui::SetWindowFontScale', 'void', ['float scale'], bound_name='ImGuiSetWindowFontScale')

	gen.bind_function('ImGui::SetNextWindowPos', 'void', ['const hg::tVec2<float> &pos', '?ImGuiCond condition'], bound_name='ImGuiSetNextWindowPos')
	gen.bind_function('ImGui::SetNextWindowPosCenter', 'void', ['?ImGuiCond condition'], bound_name='ImGuiSetNextWindowPosCenter')
	gen.bind_function('ImGui::SetNextWindowSize', 'void', ['const hg::tVec2<float> &size', '?ImGuiCond condition'], bound_name='ImGuiSetNextWindowSize')
	gen.bind_function('ImGui::SetNextWindowSizeConstraints', 'void', ['const hg::tVec2<float> &size_min', 'const hg::tVec2<float> &size_max'], bound_name='ImGuiSetNextWindowSizeConstraints')
	gen.bind_function('ImGui::SetNextWindowContentSize', 'void', ['const hg::tVec2<float> &size'], bound_name='ImGuiSetNextWindowContentSize')
	gen.bind_function('ImGui::SetNextWindowContentWidth', 'void', ['float width'], bound_name='ImGuiSetNextWindowContentWidth')
	gen.bind_function('ImGui::SetNextWindowCollapsed', 'void', ['bool collapsed', 'ImGuiCond condition'], bound_name='ImGuiSetNextWindowCollapsed')
	gen.bind_function('ImGui::SetNextWindowFocus', 'void', [], bound_name='ImGuiSetNextWindowFocus')
	gen.bind_function('ImGui::SetWindowPos', 'void', ['const char *name', 'const hg::tVec2<float> &pos', '?ImGuiCond condition'], bound_name='ImGuiSetWindowPos')
	gen.bind_function('ImGui::SetWindowSize', 'void', ['const char *name', 'const hg::tVec2<float> &size', '?ImGuiCond condition'], bound_name='ImGuiSetWindowSize')
	gen.bind_function('ImGui::SetWindowCollapsed', 'void', ['const char *name', 'bool collapsed', '?ImGuiCond condition'], bound_name='ImGuiSetWindowCollapsed')
	gen.bind_function('ImGui::SetWindowFocus', 'void', ['const char *name'], bound_name='ImGuiSetWindowFocus')

	gen.bind_function('ImGui::GetScrollX', 'float', [], bound_name='ImGuiGetScrollX')
	gen.bind_function('ImGui::GetScrollY', 'float', [], bound_name='ImGuiGetScrollY')
	gen.bind_function('ImGui::GetScrollMaxX', 'float', [], bound_name='ImGuiGetScrollMaxX')
	gen.bind_function('ImGui::GetScrollMaxY', 'float', [], bound_name='ImGuiGetScrollMaxY')
	gen.bind_function('ImGui::SetScrollX', 'void', ['float scroll_x'], bound_name='ImGuiSetScrollX')
	gen.bind_function('ImGui::SetScrollY', 'void', ['float scroll_y'], bound_name='ImGuiSetScrollY')
	gen.bind_function('ImGui::SetScrollHereY', 'void', ['?float center_y_ratio'], bound_name='ImGuiSetScrollHereY')
	gen.bind_function('ImGui::SetScrollFromPosY', 'void', ['float pos_y', '?float center_y_ratio'], bound_name='ImGuiSetScrollFromPosY')
	gen.bind_function('ImGui::SetKeyboardFocusHere', 'void', ['?int offset'], bound_name='ImGuiSetKeyboardFocusHere')

	gen.bind_function('ImGui::PushFont', 'void', ['ImFont *font'], bound_name='ImGuiPushFont')
	gen.bind_function('ImGui::PopFont', 'void', [], bound_name='ImGuiPopFont')
	gen.bind_function('ImGui::PushStyleColor', 'void', ['ImGuiCol idx', 'const hg::Color &color'], bound_name='ImGuiPushStyleColor')
	gen.bind_function('ImGui::PopStyleColor', 'void', ['?int count'], bound_name='ImGuiPopStyleColor')
	gen.bind_function_overloads('ImGui::PushStyleVar', [
		('void', ['ImGuiStyleVar idx', 'float value'], []),
		('void', ['ImGuiStyleVar idx', 'const hg::tVec2<float> &value'], [])
	], bound_name='ImGuiPushStyleVar')
	gen.bind_function('ImGui::PopStyleVar', 'void', ['?int count'], bound_name='ImGuiPopStyleVar')
	gen.bind_function('ImGui::GetFont', 'ImFont *', [], bound_name='ImGuiGetFont')
	gen.bind_function('ImGui::GetFontSize', 'float', [], bound_name='ImGuiGetFontSize')
	gen.bind_function('ImGui::GetFontTexUvWhitePixel', 'hg::tVec2<float>', [], bound_name='ImGuiGetFontTexUvWhitePixel')
	gen.bind_function_overloads('ImGui::GetColorU32', [
		('uint32_t', ['ImGuiCol idx', '?float alpha_multiplier'], []),
		('uint32_t', ['const hg::Color &color'], [])
	], bound_name='ImGuiGetColorU32')

	gen.bind_function('ImGui::PushItemWidth', 'void', ['float item_width'], bound_name='ImGuiPushItemWidth')
	gen.bind_function('ImGui::PopItemWidth', 'void', [], bound_name='ImGuiPopItemWidth')
	gen.bind_function('ImGui::CalcItemWidth', 'float', [], bound_name='ImGuiCalcItemWidth')
	gen.bind_function('ImGui::PushTextWrapPos', 'void', ['?float wrap_pos_x'], bound_name='ImGuiPushTextWrapPos')
	gen.bind_function('ImGui::PopTextWrapPos', 'void', [], bound_name='ImGuiPopTextWrapPos')
	gen.bind_function('ImGui::PushAllowKeyboardFocus', 'void', ['bool v'], bound_name='ImGuiPushAllowKeyboardFocus')
	gen.bind_function('ImGui::PopAllowKeyboardFocus', 'void', [], bound_name='ImGuiPopAllowKeyboardFocus')
	gen.bind_function('ImGui::PushButtonRepeat', 'void', ['bool repeat'], bound_name='ImGuiPushButtonRepeat')
	gen.bind_function('ImGui::PopButtonRepeat', 'void', [], bound_name='ImGuiPopButtonRepeat')

	gen.bind_function('ImGui::Separator', 'void', [], bound_name='ImGuiSeparator')
	gen.bind_function('ImGui::SameLine', 'void', ['?float pos_x', '?float spacing_w'], bound_name='ImGuiSameLine')
	gen.bind_function('ImGui::NewLine', 'void', [], bound_name='ImGuiNewLine')
	gen.bind_function('ImGui::Spacing', 'void', [], bound_name='ImGuiSpacing')
	gen.bind_function('ImGui::Dummy', 'void', ['const hg::tVec2<float> &size'], bound_name='ImGuiDummy')
	gen.bind_function('ImGui::Indent', 'void', ['?float width'], bound_name='ImGuiIndent')
	gen.bind_function('ImGui::Unindent', 'void', ['?float width'], bound_name='ImGuiUnindent')
	gen.bind_function('ImGui::BeginGroup', 'void', [], bound_name='ImGuiBeginGroup')
	gen.bind_function('ImGui::EndGroup', 'void', [], bound_name='ImGuiEndGroup')
	gen.bind_function('ImGui::GetCursorPos', 'hg::tVec2<float>', [], bound_name='ImGuiGetCursorPos')
	gen.bind_function('ImGui::GetCursorPosX', 'float', [], bound_name='ImGuiGetCursorPosX')
	gen.bind_function('ImGui::GetCursorPosY', 'float', [], bound_name='ImGuiGetCursorPosY')
	gen.bind_function('ImGui::SetCursorPos', 'void', ['const hg::tVec2<float> &local_pos'], bound_name='ImGuiSetCursorPos')
	gen.bind_function('ImGui::SetCursorPosX', 'void', ['float x'], bound_name='ImGuiSetCursorPosX')
	gen.bind_function('ImGui::SetCursorPosY', 'void', ['float y'], bound_name='ImGuiSetCursorPosY')
	gen.bind_function('ImGui::GetCursorStartPos', 'hg::tVec2<float>', [], bound_name='ImGuiGetCursorStartPos')
	gen.bind_function('ImGui::GetCursorScreenPos', 'hg::tVec2<float>', [], bound_name='ImGuiGetCursorScreenPos')
	gen.bind_function('ImGui::SetCursorScreenPos', 'void', ['const hg::tVec2<float> &pos'], bound_name='ImGuiSetCursorScreenPos')
	gen.bind_function('ImGui::AlignTextToFramePadding', 'void', [], bound_name='ImGuiAlignTextToFramePadding')
	gen.bind_function('ImGui::GetTextLineHeight', 'float', [], bound_name='ImGuiGetTextLineHeight')
	gen.bind_function('ImGui::GetTextLineHeightWithSpacing', 'float', [], bound_name='ImGuiGetTextLineHeightWithSpacing')
	gen.bind_function('ImGui::GetFrameHeightWithSpacing', 'float', [], bound_name='ImGuiGetFrameHeightWithSpacing')

	gen.bind_function('ImGui::Columns', 'void', ['?int count', '?const char *id', '?bool with_border'], bound_name='ImGuiColumns')
	gen.bind_function('ImGui::NextColumn', 'void', [], bound_name='ImGuiNextColumn')
	gen.bind_function('ImGui::GetColumnIndex', 'int', [], bound_name='ImGuiGetColumnIndex')
	gen.bind_function('ImGui::GetColumnOffset', 'float', ['?int column_index'], bound_name='ImGuiGetColumnOffset')
	gen.bind_function('ImGui::SetColumnOffset', 'void', ['int column_index', 'float offset_x'], bound_name='ImGuiSetColumnOffset')
	gen.bind_function('ImGui::GetColumnWidth', 'float', ['?int column_index'], bound_name='ImGuiGetColumnWidth')
	gen.bind_function('ImGui::SetColumnWidth', 'void', ['int column_index', 'float width'], bound_name='ImGuiSetColumnWidth')
	gen.bind_function('ImGui::GetColumnsCount', 'int', [], bound_name='ImGuiGetColumnsCount')

	gen.typedef('ImGuiID', 'unsigned int')

	gen.bind_function_overloads('ImGui::PushID', [
		('void', ['const char *id'], []),
		('void', ['int id'], [])
	], bound_name='ImGuiPushID')
	gen.bind_function('ImGui::PopID', 'void', [], bound_name='ImGuiPopID')
	gen.bind_function('ImGui::GetID', 'ImGuiID', ['const char *id'], bound_name='ImGuiGetID')

	#gen.bind_function('ImGui::time_ns_Edit', 'bool', ['const char *label', 'hg::time_ns &t'], {'arg_in_out': ['t']}, 'ImGuiInput_time_ns')

	gen.bind_function('ImGui::Text', 'void', ['const char *text'], bound_name='ImGuiText')
	gen.bind_function('ImGui::TextColored', 'void', ['const hg::Color &color', 'const char *text'], bound_name='ImGuiTextColored')
	gen.bind_function('ImGui::TextDisabled', 'void', ['const char *text'], bound_name='ImGuiTextDisabled')
	gen.bind_function('ImGui::TextWrapped', 'void', ['const char *text'], bound_name='ImGuiTextWrapped')
	gen.bind_function('ImGui::TextUnformatted', 'void', ['const char *text'], bound_name='ImGuiTextUnformatted')
	gen.bind_function('ImGui::LabelText', 'void', ['const char *label', 'const char *text'], bound_name='ImGuiLabelText')
	gen.bind_function('ImGui::Bullet', 'void', [], bound_name='ImGuiBullet')
	gen.bind_function('ImGui::BulletText', 'void', ['const char *label'], bound_name='ImGuiBulletText')
	gen.bind_function('ImGui::Button', 'bool', ['const char *label', '?const hg::tVec2<float> &size'], bound_name='ImGuiButton')
	gen.bind_function('ImGui::SmallButton', 'bool', ['const char *label'], bound_name='ImGuiSmallButton')
	gen.bind_function('ImGui::InvisibleButton', 'bool', ['const char *text', 'const hg::tVec2<float> &size'], bound_name='ImGuiInvisibleButton')

	gen.bind_function('ImGui::Image', 'void', ['const hg::Texture &tex', 'const hg::tVec2<float> &size', '?const hg::tVec2<float> &uv0', '?const hg::tVec2<float> &uv1', '?const hg::Color &tint_col', '?const hg::Color &border_col'], bound_name='ImGuiImage')
	gen.bind_function('ImGui::ImageButton', 'bool', ['const hg::Texture &tex', 'const hg::tVec2<float> &size', '?const hg::tVec2<float> &uv0', '?const hg::tVec2<float> &uv1', '?int frame_padding', '?const hg::Color &bg_col', '?const hg::Color &tint_col'], bound_name='ImGuiImageButton')

	gen.insert_binding_code('''\
static bool _InputText(const char *label, const char *text, size_t max_size, std::string &out, ImGuiInputTextFlags flags = 0) {
	out = text;
	out.reserve(max_size + 1);
	return ImGui::InputText(label, const_cast<char *>(out.c_str()), max_size, flags);
}
''')
	gen.bind_function('ImGui::InputText', 'bool', ['const char *label', 'const char *text', 'size_t max_size', 'std::string &out', '?ImGuiInputTextFlags flags'], {'route': route_lambda('_InputText'), 'arg_out': ['out']}, bound_name='ImGuiInputText')
	#
	gen.bind_function('ImGui::Checkbox', 'bool', ['const char *label', 'bool *value'], {'arg_in_out': ['value']}, 'ImGuiCheckbox')
	#IMGUI_API bool          CheckboxFlags(const char* label, unsigned int* flags, unsigned int flags_value);
	
	gen.bind_function_overloads('ImGui::RadioButton', [
		('bool', ['const char *label', 'bool active'], []),
		('bool', ['const char *label', 'int* v', 'int v_button'], {'arg_in_out': ['v']})
	], bound_name='ImGuiRadioButton')

	gen.bind_named_enum('ImGuiComboFlags', [
		'ImGuiComboFlags_PopupAlignLeft', 'ImGuiComboFlags_HeightSmall', 'ImGuiComboFlags_HeightRegular', 'ImGuiComboFlags_HeightLarge', 'ImGuiComboFlags_HeightLargest'
	], 'int', namespace='')

	gen.bind_function('ImGui::BeginCombo', 'bool', ['const char* label', 'const char* preview_value', '?ImGuiComboFlags flags'], bound_name='ImGuiBeginCombo')
	gen.bind_function('ImGui::EndCombo', 'void', [], bound_name='ImGuiEndCombo')
    
	gen.insert_binding_code('''\
static bool _ImGuiCombo(const char *label, int *current_item, const std::vector<std::string> &items, int height_in_items = -1) {
	auto item_cb = [](void *data, int idx, const char **out_text) -> bool {
		auto &items = *(const std::vector<std::string> *)data;
		if (size_t(idx) >= items.size())
			return false;
		*out_text = items[idx].c_str();
		return true;
	};
	return ImGui::Combo(label, current_item, item_cb, (void *)&items, hg::numeric_cast<int>(items.size()), height_in_items);
}

static bool _ImGuiColorButton(const char *id, hg::Color &color, ImGuiColorEditFlags flags = 0, const hg::tVec2<float> &size = hg::tVec2<float>(0, 0)) { return ImGui::ColorButton(id, *(hg::Vec4 *)&color, flags, size); }
static bool _ImGuiColorEdit(const char *label, hg::Color &color, ImGuiColorEditFlags flags = 0) { return ImGui::ColorEdit4(label, &color.r, flags); }
static void _ImGuiProgressBar(float fraction, const hg::tVec2<float> &size = hg::tVec2<float>(-1, 0), const char *overlay = nullptr) { ImGui::ProgressBar(fraction, size, overlay); }
''')

	protos = [('bool', ['const char *label', 'int *current_item', 'const std::vector<std::string> &items', '?int height_in_items'], {'arg_in_out': ['current_item']})]
	gen.bind_function_overloads('_ImGuiCombo', expand_std_vector_proto(gen, protos), bound_name='ImGuiCombo')

	gen.bind_function('_ImGuiColorButton', 'bool', ['const char *id', 'hg::Color &color', '?ImGuiColorEditFlags flags', '?const hg::tVec2<float> &size'], bound_name='ImGuiColorButton')
	gen.bind_function('_ImGuiColorEdit', 'bool', ['const char *label', 'hg::Color &color', '?ImGuiColorEditFlags flags'], {'arg_in_out': ['color']}, bound_name='ImGuiColorEdit')
	gen.bind_function('_ImGuiProgressBar', 'void', ['float fraction', '?const hg::tVec2<float> &size', '?const char *overlay'], bound_name='ImGuiProgressBar')

	gen.insert_binding_code('''\
static bool _ImGuiDragiVec2(const char *label, hg::tVec2<int> &v, float v_speed = 1.f, int v_min = 0, int v_max = 0) { return ImGui::DragInt2(label, &v.x, v_speed, v_min, v_max); }

static bool _ImGuiDragVec2(const char *label, hg::tVec2<float> &v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f) { return ImGui::DragFloat2(label, &v.x, v_speed, v_min, v_max); }
static bool _ImGuiDragVec3(const char *label, hg::Vec3 &v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f) { return ImGui::DragFloat3(label, &v.x, v_speed, v_min, v_max); }
static bool _ImGuiDragVec4(const char *label, hg::Vec4 &v, float v_speed = 1.f, float v_min = 0.f, float v_max = 0.f) { return ImGui::DragFloat4(label, &v.x, v_speed, v_min, v_max); }
''')

	gen.bind_function_overloads('ImGui::DragFloat', [
		('bool', ['const char *label', 'float *v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'float *v', 'float v_speed'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'float *v', 'float v_speed', 'float v_min', 'float v_max'], {'arg_in_out': ['v']})
	], bound_name='ImGuiDragFloat')
	gen.bind_function_overloads('_ImGuiDragVec2', [
		('bool', ['const char *label', 'hg::tVec2<float> &v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::tVec2<float> &v', 'float v_speed'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::tVec2<float> &v', 'float v_speed', 'float v_min', 'float v_max'], {'arg_in_out': ['v']})
	], bound_name='ImGuiDragVec2')
	gen.bind_function_overloads('_ImGuiDragVec3', [
		('bool', ['const char *label', 'hg::Vec3 &v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::Vec3 &v', 'float v_speed'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::Vec3 &v', 'float v_speed', 'float v_min', 'float v_max'], {'arg_in_out': ['v']})
	], bound_name='ImGuiDragVec3')
	gen.bind_function_overloads('_ImGuiDragVec4', [
		('bool', ['const char *label', 'hg::Vec4 &v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::Vec4 &v', 'float v_speed'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::Vec4 &v', 'float v_speed', 'float v_min', 'float v_max'], {'arg_in_out': ['v']})
	], bound_name='ImGuiDragVec4')

	gen.bind_function_overloads('_ImGuiDragiVec2', [
		('bool', ['const char *label', 'hg::tVec2<int> &v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::tVec2<int> &v', 'float v_speed'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'hg::tVec2<int> &v', 'float v_speed', 'int v_min', 'int v_max'], {'arg_in_out': ['v']})
	], bound_name='ImGuiDragIntVec2')

	#IMGUI_API bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);
	#IMGUI_API bool InputTextMultiline(const char* label, char* buf, size_t buf_size, const hg::tVec2<float>& size = hg::tVec2<float>(0,0), ImGuiInputTextFlags flags = 0, ImGuiTextEditCallback callback = NULL, void* user_data = NULL);

	gen.insert_binding_code('''\
static bool _ImGuiInputiVec2(const char *label, hg::tVec2<int> &v, ImGuiInputTextFlags flags = 0) { return ImGui::InputInt2(label, &v.x, flags); }

static bool _ImGuiInputFloat(const char *label, float &v, float step = 0.f, float step_fast = 0.f, int decimal_precision = 4, ImGuiInputTextFlags flags = 0) { return ImGui::InputFloat(label, &v, step, step_fast, hg::format("%.%1f").arg(decimal_precision).c_str(), flags); }
static bool _ImGuiInputVec2(const char *label, hg::tVec2<float> &v, int decimal_precision = 4, ImGuiInputTextFlags flags = 0) { return ImGui::InputFloat2(label, &v.x, hg::format("%.%1f").arg(decimal_precision).c_str(), flags); }
static bool _ImGuiInputVec3(const char *label, hg::Vec3 &v, int decimal_precision = 4, ImGuiInputTextFlags flags = 0) { return ImGui::InputFloat3(label, &v.x, hg::format("%.%1f").arg(decimal_precision).c_str(), flags); }
static bool _ImGuiInputVec4(const char *label, hg::Vec4 &v, int decimal_precision = 4, ImGuiInputTextFlags flags = 0) { return ImGui::InputFloat4(label, &v.x, hg::format("%.%1f").arg(decimal_precision).c_str(), flags); }
''')

	gen.bind_function_overloads('ImGui::InputInt', [
		('bool', ['const char *label', 'int *v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'int *v', 'int step', 'int step_fast', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputInt')
	gen.bind_function_overloads('_ImGuiInputFloat', [
		('bool', ['const char *label', 'float &v'], {'arg_in_out': ['v']}),
		('bool', ['const char *label', 'float &v', 'float step', 'float step_fast', '?int decimal_precision', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputFloat')
	gen.bind_function_overloads('_ImGuiInputVec2', [
		('bool', ['const char *label', 'hg::tVec2<float> &v', '?int decimal_precision', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputVec2')
	gen.bind_function_overloads('_ImGuiInputVec3', [
		('bool', ['const char *label', 'hg::Vec3 &v', '?int decimal_precision', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputVec3')
	gen.bind_function_overloads('_ImGuiInputVec4', [
		('bool', ['const char *label', 'hg::Vec4 &v', '?int decimal_precision', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputVec4')
	gen.bind_function_overloads('_ImGuiInputiVec2', [
		('bool', ['const char *label', 'hg::tVec2<int> &v', '?ImGuiInputTextFlags flags'], {'arg_in_out': ['v']})
	], bound_name='ImGuiInputIntVec2')

	gen.insert_binding_code('''\
static bool _ImGuiSliderInt(const char *label, int &v, int v_min, int v_max, const char *format="%.0f") { return ImGui::SliderInt(label, &v, v_min, v_max, format); }
static bool _ImGuiSlideriVec2(const char *label, hg::tVec2<int> &v, int v_min, int v_max, const char *format="%.0f") { return ImGui::SliderInt2(label, &v.x, v_min, v_max, format); }

static bool _ImGuiSliderFloat(const char *label, float &v, float v_min, float v_max, const char *format="%.3f") { return ImGui::SliderFloat(label, &v, v_min, v_max, format); }
static bool _ImGuiSliderVec2(const char *label, hg::tVec2<float> &v, float v_min, float v_max, const char *format="%.3f") { return ImGui::SliderFloat2(label, &v.x, v_min, v_max, format); }
static bool _ImGuiSliderVec3(const char *label, hg::Vec3 &v, float v_min, float v_max, const char *format="%.3f") { return ImGui::SliderFloat3(label, &v.x, v_min, v_max, format); }
static bool _ImGuiSliderVec4(const char *label, hg::Vec4 &v, float v_min, float v_max, const char *format="%.3f") { return ImGui::SliderFloat4(label, &v.x, v_min, v_max, format); }
''')

	gen.bind_function_overloads('_ImGuiSliderInt', [
		('bool', ['const char *label', 'int &v', 'int v_min', 'int v_max', '?const char *format'], {'arg_in_out': ['v']})
	], bound_name='ImGuiSliderInt')
	gen.bind_function_overloads('_ImGuiSlideriVec2', [
		('bool', ['const char *label', 'hg::tVec2<int> &v', 'int v_min', 'int v_max', '?const char *format'], {'arg_in_out': ['v']})
	], bound_name='ImGuiSliderIntVec2')

	gen.bind_function_overloads('_ImGuiSliderFloat', [
		('bool', ['const char *label', 'float &v', 'float v_min', 'float v_max', '?const char *format'], {'arg_in_out': ['v']})
	], bound_name='ImGuiSliderFloat')	
	gen.bind_function_overloads('_ImGuiSliderVec2', [
        ('bool', ['const char *label', 'hg::tVec2<float> &v', 'float v_min', 'float v_max', '?const char *format'], {'arg_in_out': ['v']}),
    ], bound_name='ImGuiSliderVec2')
	gen.bind_function_overloads('_ImGuiSliderVec3', [
        ('bool', ['const char *label', 'hg::Vec3 &v', 'float v_min', 'float v_max', '?const char *format'], {'arg_in_out': ['v']}),
    ], bound_name='ImGuiSliderVec3')
	gen.bind_function_overloads('_ImGuiSliderVec4', [
        ('bool', ['const char *label', 'hg::Vec4 &v', 'float v_min', 'float v_max', '?const char *format'], {'arg_in_out': ['v']}),
    ], bound_name='ImGuiSliderVec4')

	gen.bind_function('ImGui::TreeNode', 'bool', ['const char *label'], bound_name='ImGuiTreeNode')
	gen.bind_function('ImGui::TreeNodeEx', 'bool', ['const char *label', 'ImGuiTreeNodeFlags flags'], bound_name='ImGuiTreeNodeEx')
	gen.bind_function('ImGui::TreePush', 'void', ['const char *id'], bound_name='ImGuiTreePush')
	gen.bind_function('ImGui::TreePop', 'void', [], bound_name='ImGuiTreePop')
	gen.bind_function('ImGui::GetTreeNodeToLabelSpacing', 'float', [], bound_name='ImGuiGetTreeNodeToLabelSpacing')
	gen.bind_function('ImGui::SetNextItemOpen', 'void', ['bool is_open', '?ImGuiCond condition'], bound_name='ImGuiSetNextItemOpen')
	gen.bind_function_overloads('ImGui::CollapsingHeader', [
		('bool', ['const char *label', '?ImGuiTreeNodeFlags flags'], []),
		('bool', ['const char *label', 'bool *p_open', '?ImGuiTreeNodeFlags flags'], {'arg_in_out': ['p_open']})
	], bound_name='ImGuiCollapsingHeader')

	gen.insert_binding_code('''\
static bool _ImGuiSelectable(const char *label, bool selected = false, ImGuiSelectableFlags flags = 0, const hg::tVec2<float> &size = hg::tVec2<float>(0.f, 0.f)) { return ImGui::Selectable(label, selected, flags, hg::tVec2<float>(size)); }

static bool _ImGuiListBox(const char *label, int *current_item, const std::vector<std::string> &items, int height_in_items = -1) {
	auto cb = [](void *data, int idx, const char **out) -> bool {
		auto &items = *(const std::vector<std::string> *)data;
		if (size_t(idx) >= items.size())
			return false;
		*out = items[idx].c_str();
		return true;
	};
	return ImGui::ListBox(label, current_item, cb, (void *)&items, hg::numeric_cast<int>(items.size()), height_in_items);
}
''')

	gen.bind_function('_ImGuiSelectable', 'bool', ['const char *label', '?bool selected', '?ImGuiSelectableFlags flags', '?const hg::tVec2<float> &size'], bound_name='ImGuiSelectable')
	gen.bind_function_overloads('_ImGuiListBox', expand_std_vector_proto(gen, [
		('bool', ['const char *label', 'int *current_item', 'const std::vector<std::string> &items'], {'arg_in_out': ['current_item']}),
		('bool', ['const char *label', 'int *current_item', 'const std::vector<std::string> &items', 'int height_in_items'], {'arg_in_out': ['current_item']})
	]), bound_name='ImGuiListBox')

	gen.bind_function('ImGui::SetTooltip', 'void', ['const char *text'], bound_name='ImGuiSetTooltip')
	gen.bind_function('ImGui::BeginTooltip', 'void', [], bound_name='ImGuiBeginTooltip')
	gen.bind_function('ImGui::EndTooltip', 'void', [], bound_name='ImGuiEndTooltip')

	gen.bind_function('ImGui::BeginMainMenuBar', 'bool', [], bound_name='ImGuiBeginMainMenuBar')
	gen.bind_function('ImGui::EndMainMenuBar', 'void', [], bound_name='ImGuiEndMainMenuBar')
	gen.bind_function('ImGui::BeginMenuBar', 'bool', [], bound_name='ImGuiBeginMenuBar')
	gen.bind_function('ImGui::EndMenuBar', 'void', [], bound_name='ImGuiEndMenuBar')
	gen.bind_function('ImGui::BeginMenu', 'bool', ['const char *label', '?bool enabled'], bound_name='ImGuiBeginMenu')
	gen.bind_function('ImGui::EndMenu', 'void', [], bound_name='ImGuiEndMenu')
	gen.bind_function('ImGui::MenuItem', 'bool', ['const char *label', '?const char *shortcut', '?bool selected', '?bool enabled'], bound_name='ImGuiMenuItem')

	gen.bind_function('ImGui::OpenPopup', 'void', ['const char *id'], bound_name='ImGuiOpenPopup')
	gen.bind_function('ImGui::BeginPopup', 'bool', ['const char *id'], bound_name='ImGuiBeginPopup')
	gen.bind_function('ImGui::BeginPopupModal', 'bool', ['const char *name', '?bool *open', '?ImGuiWindowFlags flags'], bound_name='ImGuiBeginPopupModal')
	gen.bind_function('ImGui::BeginPopupContextItem', 'bool', ['const char *id', '?int mouse_button'], bound_name='ImGuiBeginPopupContextItem')
	gen.bind_function('ImGui::BeginPopupContextWindow', 'bool', ['?const char *id', '?ImGuiPopupFlags flags'], bound_name='ImGuiBeginPopupContextWindow')
	gen.bind_function('ImGui::BeginPopupContextVoid', 'bool', ['?const char *id', '?int mouse_button'], bound_name='ImGuiBeginPopupContextVoid')
	gen.bind_function('ImGui::EndPopup', 'void', [], bound_name='ImGuiEndPopup')
	gen.bind_function('ImGui::CloseCurrentPopup', 'void', [], bound_name='ImGuiCloseCurrentPopup')

	gen.insert_binding_code('''\
static void _ImGuiPushClipRect(const hg::tVec2<float> &clip_rect_min, const hg::tVec2<float> &clip_rect_max, bool intersect_with_current_clip_rect) {
	ImGui::PushClipRect(hg::tVec2<float>(clip_rect_min), hg::tVec2<float>(clip_rect_max), intersect_with_current_clip_rect);
}

static hg::tVec2<float> _ImGuiGetItemRectMin() { return hg::tVec2<float>(ImGui::GetItemRectMin()); }
static hg::tVec2<float> _ImGuiGetItemRectMax() { return hg::tVec2<float>(ImGui::GetItemRectMax()); }
static hg::tVec2<float> _ImGuiGetItemRectSize() { return hg::tVec2<float>(ImGui::GetItemRectSize()); }

static bool _ImGuiIsRectVisible(const hg::tVec2<float> &size) { return ImGui::IsRectVisible(size); }
static bool _ImGuiIsRectVisible(const hg::tVec2<float> &min, const hg::tVec2<float> &max) { return ImGui::IsRectVisible(min, max); }

static hg::Vec2 _ImGuiCalcTextSize(const char *text, bool hide_text_after_double_dash = false, float wrap_width = -1.f) { return ImGui::CalcTextSize(text, NULL, hide_text_after_double_dash, wrap_width); }
''')
	gen.bind_function('_ImGuiPushClipRect', 'void', ['const hg::tVec2<float> &clip_rect_min', 'const hg::tVec2<float> &clip_rect_max', 'bool intersect_with_current_clip_rect'], bound_name='ImGuiPushClipRect')
	gen.bind_function('ImGui::PopClipRect', 'void', [], bound_name='ImGuiPopClipRect')

	gen.bind_function('ImGui::IsItemHovered', 'bool', ['?ImGuiHoveredFlags flags'], bound_name='ImGuiIsItemHovered')
	gen.bind_function('ImGui::IsItemActive', 'bool', [], bound_name='ImGuiIsItemActive')
	gen.bind_function('ImGui::IsItemClicked', 'bool', ['?int mouse_button'], bound_name='ImGuiIsItemClicked')
	gen.bind_function('ImGui::IsItemVisible', 'bool', [], bound_name='ImGuiIsItemVisible')
	gen.bind_function('ImGui::IsAnyItemHovered', 'bool', [], bound_name='ImGuiIsAnyItemHovered')
	gen.bind_function('ImGui::IsAnyItemActive', 'bool', [], bound_name='ImGuiIsAnyItemActive')
	gen.bind_function('_ImGuiGetItemRectMin', 'hg::tVec2<float>', [], bound_name='ImGuiGetItemRectMin')
	gen.bind_function('_ImGuiGetItemRectMax', 'hg::tVec2<float>', [], bound_name='ImGuiGetItemRectMax')
	gen.bind_function('_ImGuiGetItemRectSize', 'hg::tVec2<float>', [], bound_name='ImGuiGetItemRectSize')
	gen.bind_function('ImGui::SetItemAllowOverlap', 'void', [], bound_name='ImGuiSetItemAllowOverlap')
	gen.bind_function('ImGui::SetItemDefaultFocus', 'void', [], bound_name='ImGuiSetItemDefaultFocus')
	gen.bind_function('ImGui::IsWindowHovered', 'bool', ['?ImGuiHoveredFlags flags'], bound_name='ImGuiIsWindowHovered')
	gen.bind_function('ImGui::IsWindowFocused', 'bool', ['?ImGuiFocusedFlags flags'], bound_name='ImGuiIsWindowFocused')
	gen.bind_function_overloads('ImGui::IsRectVisible', [
		('bool', ['const hg::tVec2<float> &size'], []),
		('bool', ['const hg::tVec2<float> &rect_min', 'const hg::tVec2<float> &rect_max'], [])
	], bound_name='ImGuiIsRectVisible')
	gen.bind_function('ImGui::GetTime', 'float', [], bound_name='ImGuiGetTime')
	gen.bind_function('ImGui::GetFrameCount', 'int', [], bound_name='ImGuiGetFrameCount')
	#IMGUI_API const char*   GetStyleColName(ImGuiCol idx);
	gen.bind_function('_ImGuiCalcTextSize', 'hg::tVec2<float>', ['const char *text', '?bool hide_text_after_double_dash', '?float wrap_width'], bound_name='ImGuiCalcTextSize')

	#IMGUI_API bool          BeginChildFrame(ImGuiID id, const hg::tVec2<float>& size, ImGuiWindowFlags flags = 0);	// helper to create a child window / scrolling region that looks like a normal widget frame
	#IMGUI_API void          EndChildFrame();

	#gen.bind_function('ImGui::GetKeyIndex', 'int', [], bound_name='ImGuiGetKeyIndex')
	gen.bind_function('ImGui::IsKeyDown', 'bool', ['int key_index'], bound_name='ImGuiIsKeyDown')
	gen.bind_function('ImGui::IsKeyPressed', 'bool', ['int key_index', '?bool repeat'], bound_name='ImGuiIsKeyPressed')
	gen.bind_function('ImGui::IsKeyReleased', 'bool', ['int key_index'], bound_name='ImGuiIsKeyReleased')
	gen.bind_function('ImGui::IsMouseDown', 'bool', ['int button'], bound_name='ImGuiIsMouseDown')
	gen.bind_function('ImGui::IsMouseClicked', 'bool', ['int button', '?bool repeat'], bound_name='ImGuiIsMouseClicked')
	gen.bind_function('ImGui::IsMouseDoubleClicked', 'bool', ['int button'], bound_name='ImGuiIsMouseDoubleClicked')
	gen.bind_function('ImGui::IsMouseReleased', 'bool', ['int button'], bound_name='ImGuiIsMouseReleased')
	#gen.bind_function('ImGui::IsWindowRectHovered', 'bool', [], bound_name='ImGuiIsWindowRectHovered')
	#gen.bind_function('ImGui::IsAnyWindowHovered', 'bool', [], bound_name='ImGuiIsAnyWindowHovered')
	gen.bind_function('ImGui::IsMouseHoveringRect', 'bool', ['const hg::tVec2<float> &rect_min', 'const hg::tVec2<float> &rect_max', '?bool clip'], bound_name='ImGuiIsMouseHoveringRect')
	gen.bind_function('ImGui::IsMouseDragging', 'bool', ['ImGuiMouseButton button', '?float lock_threshold'], bound_name='ImGuiIsMouseDragging')
	gen.bind_function('ImGui::GetMousePos', 'hg::tVec2<float>', [], bound_name='ImGuiGetMousePos')
	gen.bind_function('ImGui::GetMousePosOnOpeningCurrentPopup', 'hg::tVec2<float>', [], bound_name='ImGuiGetMousePosOnOpeningCurrentPopup')
	gen.bind_function('ImGui::GetMouseDragDelta', 'hg::tVec2<float>', ['?ImGuiMouseButton button', '?float lock_threshold'], bound_name='ImGuiGetMouseDragDelta')
	gen.bind_function('ImGui::ResetMouseDragDelta', 'void', ['?ImGuiMouseButton button'], bound_name='ImGuiResetMouseDragDelta')
	#IMGUI_API ImGuiMouseCursor GetMouseCursor();                                                // get desired cursor type, reset in ImGui::NewFrame(), this updated during the frame. valid before Render(). If you use software rendering by setting io.MouseDrawCursor ImGui will render those for you
	#IMGUI_API void          SetMouseCursor(ImGuiMouseCursor type);                              // set desired cursor type
	gen.bind_function('ImGui::CaptureKeyboardFromApp', 'void', ['bool capture'], bound_name='ImGuiCaptureKeyboardFromApp')
	gen.bind_function('ImGui::CaptureMouseFromApp', 'void', ['bool capture'], bound_name='ImGuiCaptureMouseFromApp')

	gen.insert_binding_code('static bool _ImGuiWantCaptureMouse() { return ImGui::GetIO().WantCaptureMouse; }')    
	gen.bind_function('_ImGuiWantCaptureMouse', 'bool', [], bound_name='ImGuiWantCaptureMouse')
	
	gen.insert_binding_code('static void _ImGuiMouseDrawCursor(const bool &draw_cursor) { ImGui::GetIO().MouseDrawCursor = draw_cursor; }')    
	gen.bind_function('_ImGuiMouseDrawCursor', 'void', ['const bool &draw_cursor'], bound_name='ImGuiMouseDrawCursor')

	gen.bind_function('hg::ImGuiInit', 'void', ['float font_size', 'bgfx::ProgramHandle imgui_program', 'bgfx::ProgramHandle imgui_image_program'])
	gen.bind_function('hg::ImGuiInitContext', 'hg::DearImguiContext *', ['float font_size', 'bgfx::ProgramHandle imgui_program', 'bgfx::ProgramHandle imgui_image_program'])
	gen.bind_function('hg::ImGuiShutdown', 'void', [])
	gen.bind_function('hg::ImGuiBeginFrame', 'void', ['int width', 'int height', 'hg::time_ns dt_clock', 'const hg::MouseState &mouse', 'const hg::KeyboardState &keyboard'])
	gen.bind_function('hg::ImGuiBeginFrame', 'void', ['hg::DearImguiContext &ctx', 'int width', 'int height', 'hg::time_ns dt_clock', 'const hg::MouseState &mouse', 'const hg::KeyboardState &keyboard'])
	gen.bind_function('hg::ImGuiEndFrame', 'void', ['const hg::DearImguiContext& ctx', '?bgfx::ViewId view_id'])
	gen.bind_function('hg::ImGuiEndFrame', 'void', ['?bgfx::ViewId view_id'])
	gen.bind_function('hg::ImGuiClearInputBuffer', 'void', [])


def bind_extras(gen):
	gen.add_include('thread', True)
	gen.add_include('foundation/time_chrono.h')

	gen.insert_binding_code('static void SleepThisThread(hg::time_ns duration) { std::this_thread::sleep_for(hg::time_to_chrono(duration)); }\n\n')
	gen.bind_function('SleepThisThread', 'void', ['hg::time_ns duration'], bound_name='Sleep')


def bind_audio(gen):
	gen.add_include('engine/audio_stream_interface.h')
	
	gen.bind_named_enum('AudioFrameFormat', ['AFF_LPCM_44KHZ_S16_Mono', 'AFF_LPCM_48KHZ_S16_Mono', 'AFF_LPCM_44KHZ_S16_Stereo', 'AFF_LPCM_48KHZ_S16_Stereo'])

	gen.typedef('AudioStreamRef', 'int')
	gen.bind_variable('const AudioStreamRef InvalidAudioStreamRef')

	gen.add_include('engine/audio.h')
	
	gen.bind_function('hg::AudioInit', 'bool', [])
	gen.bind_function('hg::AudioShutdown', 'void', [])

	gen.typedef('hg::SoundRef', 'int')
	gen.bind_constants('int', [("SND_Invalid", "hg::InvalidSoundRef")], 'SoundRef')
	gen.typedef('hg::SourceRef', 'int')
	gen.bind_constants('int', [("SRC_Invalid", "hg::InvalidSourceRef")], 'SourceRef')

	gen.bind_function('hg::LoadWAVSoundFile', 'hg::SoundRef', ['const char *path'], {'rval_constants_group': 'SoundRef'})
	gen.bind_function('hg::LoadWAVSoundAsset', 'hg::SoundRef', ['const char *name'], {'rval_constants_group': 'SoundRef'})

	gen.bind_function('hg::LoadOGGSoundFile', 'hg::SoundRef', ['const char *path'], {'rval_constants_group': 'SoundRef'})
	gen.bind_function('hg::LoadOGGSoundAsset', 'hg::SoundRef', ['const char *name'], {'rval_constants_group': 'SoundRef'})

	gen.bind_function('hg::UnloadSound', 'void', ['hg::SoundRef snd'], {'constants_group': {'snd': 'SoundRef'}})


	gen.bind_function('hg::SetListener', 'void', ['const hg::Mat4 &world', 'const hg::Vec3 &velocity'])

	gen.bind_named_enum('hg::SourceRepeat', ['SR_Once', 'SR_Loop'])

	gen.insert_binding_code('''
static hg::StereoSourceState *__ConstructStereoSourceState(float volume = 1.f, hg::SourceRepeat repeat = hg::SR_Once, float panning = 0.f) {
	return new hg::StereoSourceState{volume, repeat, panning};
}

static hg::SpatializedSourceState *__ConstructSpatializedSourceState(hg::Mat4 mtx = hg::Mat4::Identity, float volume = 1.f, hg::SourceRepeat repeat = hg::SR_Once, const hg::Vec3 &vel = {}) {
	return new hg::SpatializedSourceState{mtx, volume, repeat, vel};
}
''')

	stereo_source_state = gen.begin_class('hg::StereoSourceState')
	gen.bind_members(stereo_source_state, ['float volume', 'hg::SourceRepeat repeat', 'float panning'])
	gen.bind_constructor(stereo_source_state, ['?float volume', '?hg::SourceRepeat repeat', '?float panning'], {'route': route_lambda('__ConstructStereoSourceState')})
	gen.end_class(stereo_source_state)

	spatialized_source_state = gen.begin_class('hg::SpatializedSourceState')
	gen.bind_members(spatialized_source_state, ['hg::Mat4 mtx', 'float volume', 'hg::SourceRepeat repeat', 'hg::Vec3 vel'])
	gen.bind_constructor(spatialized_source_state, ['?hg::Mat4 mtx', '?float volume', '?hg::SourceRepeat repeat', '?const hg::Vec3 &vel'], {'route': route_lambda('__ConstructSpatializedSourceState')})
	gen.end_class(spatialized_source_state)
	
	gen.bind_function('hg::PlayStereo', 'hg::SourceRef', ['hg::SoundRef snd', 'const hg::StereoSourceState &state'], {'rval_constants_group': 'SourceRef', 'constants_group': {'snd': 'SoundRef'}})
	gen.bind_function('hg::PlaySpatialized', 'hg::SourceRef', ['hg::SoundRef snd', 'const hg::SpatializedSourceState &state'], {'rval_constants_group': 'SourceRef', 'constants_group': {'snd': 'SoundRef'}})

	gen.bind_function('hg::StreamWAVFileStereo', 'hg::SourceRef', ['const char *path', 'const hg::StereoSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamWAVAssetStereo', 'hg::SourceRef', ['const char *name', 'const hg::StereoSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamWAVFileSpatialized', 'hg::SourceRef', ['const char *path', 'const hg::SpatializedSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamWAVAssetSpatialized', 'hg::SourceRef', ['const char *name', 'const hg::SpatializedSourceState &state'], {'rval_constants_group': 'SourceRef'})

	gen.bind_function('hg::StreamOGGFileStereo', 'hg::SourceRef', ['const char *path', 'const hg::StereoSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamOGGAssetStereo', 'hg::SourceRef', ['const char *name', 'const hg::StereoSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamOGGFileSpatialized', 'hg::SourceRef', ['const char *path', 'const hg::SpatializedSourceState &state'], {'rval_constants_group': 'SourceRef'})
	gen.bind_function('hg::StreamOGGAssetSpatialized', 'hg::SourceRef', ['const char *name', 'const hg::SpatializedSourceState &state'], {'rval_constants_group': 'SourceRef'})

	gen.bind_function('hg::GetSourceDuration', 'hg::time_ns', ['hg::SourceRef source'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::GetSourceTimecode', 'hg::time_ns', ['hg::SourceRef source'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::SetSourceTimecode', 'bool', ['hg::SourceRef source', 'hg::time_ns t'], {'constants_group': {'source': 'SourceRef'}})

	gen.bind_function('hg::SetSourceVolume', 'void', ['hg::SourceRef source', 'float volume'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::SetSourcePanning', 'void', ['hg::SourceRef source', 'float panning'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::SetSourceRepeat', 'void', ['hg::SourceRef source', 'hg::SourceRepeat repeat'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::SetSourceTransform', 'void', ['hg::SourceRef source', 'const hg::Mat4 &world', 'const hg::Vec3 &velocity'], {'constants_group': {'source': 'SourceRef'}})

	gen.bind_named_enum('hg::SourceState', [
		'SS_Initial', 'SS_Playing', 'SS_Paused', 'SS_Stopped', 'SS_Invalid'
	])

	gen.bind_function('hg::GetSourceState', 'hg::SourceState', ['hg::SourceRef source'], {'constants_group': {'source': 'SourceRef'}})

	gen.bind_function('hg::PauseSource', 'void', ['hg::SourceRef source'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::StopSource', 'void', ['hg::SourceRef source'], {'constants_group': {'source': 'SourceRef'}})
	gen.bind_function('hg::StopAllSources', 'void', [])


def bind_bloom(gen):
	gen.add_include('engine/bloom.h')

	bloom = gen.begin_class('hg::Bloom')
	gen.end_class(bloom)

	gen.bind_function('hg::CreateBloomFromFile', 'hg::Bloom', ['const char *path', 'bgfx::BackbufferRatio::Enum ratio'])
	gen.bind_function('hg::CreateBloomFromAssets', 'hg::Bloom', ['const char *path', 'bgfx::BackbufferRatio::Enum ratio'])
	gen.bind_function('hg::DestroyBloom', 'void', ['hg::Bloom &bloom'])
	gen.bind_function('hg::ApplyBloom', 'void', ['bgfx::ViewId &view_id', 'const hg::Rect<int> &rect', 'const hg::Texture &input', 'bgfx::FrameBufferHandle output', 'hg::Bloom &bloom', 'float threshold', 'float smoothness', 'float intensity'], {'arg_in_out': ['view_id']})


def bind_sao(gen):
	gen.add_include('engine/sao.h')

	sao = gen.begin_class('hg::SAO')
	gen.end_class(sao)

	"""
	SAO CreateSAOFromFile(const char *path, bgfx::BackbufferRatio::Enum ratio, bool blur = true);
	SAO CreateSAOFromAssets(const char *path, bgfx::BackbufferRatio::Enum ratio, bool blur = true);

	void DestroySAO(SAO &sao);

	/// @note input depth buffer must be in linear depth
	void ComputeSAO(bgfx::ViewId &view_id, const iRect &rect, const Texture &attr0, const Texture &attr1, const Texture &noise, bgfx::FrameBufferHandle output,
		const SAO &sao, const Mat44 &projection, float bias, float radius, int sample_count, float sharpness);
	"""

	gen.bind_function_overloads('hg::CreateSAOFromFile', [
		('hg::SAO', ['const char *path', 'bgfx::BackbufferRatio::Enum ratio'], [])
	])
	gen.bind_function_overloads('hg::CreateSAOFromAssets', [
		('hg::SAO', ['const char *path', 'bgfx::BackbufferRatio::Enum ratio'], [])
	])
	gen.bind_function('hg::DestroySAO', 'void', ['hg::SAO &sao'])
	gen.bind_function('hg::ComputeSAO', 'void', ['bgfx::ViewId &view_id', 'const hg::Rect<int> &rect', 'const hg::Texture &attr0', 'const hg::Texture &attr1', 'const hg::Texture &noise', 'bgfx::FrameBufferHandle output', 'const hg::SAO &sao', 'const hg::Mat44 &projection', 'float bias', 'float radius', 'int sample_count', 'float sharpness'])

	
def bind_video_stream(gen):
	gen.add_include('engine/video_stream.h')
	
	gen.bind_named_enum('VideoFrameFormat', ['VFF_UNKNOWN', 'VFF_YUV422', 'VFF_RGB24'])

	gen.typedef('VideoStreamHandle', 'intptr_t')
	gen.typedef('time_ns', 'int64_t')
	
	video_streamer = gen.begin_class('IVideoStreamer')
	gen.bind_method(video_streamer, 'Startup', 'int', [])
	gen.bind_method(video_streamer, 'Shutdown', 'void', [])
	gen.bind_method(video_streamer, 'Open', 'VideoStreamHandle', ['const char *name'])
	gen.bind_method(video_streamer, 'Play', 'int', ['VideoStreamHandle h'])
	gen.bind_method(video_streamer, 'Pause', 'int', ['VideoStreamHandle h'])
	gen.bind_method(video_streamer, 'Close', 'int', ['VideoStreamHandle h'])
	gen.bind_method(video_streamer, 'Seek', 'int', ['VideoStreamHandle h', 'time_ns t'])
	gen.bind_method(video_streamer, 'GetDuration', 'time_ns', ['VideoStreamHandle h'])
	gen.bind_method(video_streamer, 'GetTimeStamp', 'time_ns', ['VideoStreamHandle h'])
	gen.bind_method(video_streamer, 'IsEnded', 'int', ['VideoStreamHandle h'])

	gen.insert_binding_code('''
static int _VideoStreamer_GetFrame(IVideoStreamer *streamer, VideoStreamHandle h, intptr_t& ptr, int &width, int &height, int &pitch, VideoFrameFormat &format) {
	return streamer->GetFrame(h, (const void **)&ptr, &width, &height, &pitch, &format);
}
''')	
	gen.bind_method(video_streamer, 'GetFrame', 'int', ['VideoStreamHandle h', 'intptr_t& ptr', 'int &width', 'int &height', 'int &pitch', 'VideoFrameFormat &format'], {'route': route_lambda('_VideoStreamer_GetFrame'), 'arg_in_out': ['ptr', 'width', 'height', 'pitch', 'format']})

	gen.bind_method(video_streamer, 'FreeFrame', 'int', ['VideoStreamHandle h', 'int frame'])
		
	gen.end_class(video_streamer)

	gen.bind_function('hg::MakeVideoStreamer', 'IVideoStreamer', ['const char *module_path'])
	gen.bind_function('hg::IsValid', 'bool', ['IVideoStreamer &streamer'])
	gen.bind_function('hg::UpdateTexture', 'bool', ['IVideoStreamer &streamer', 'VideoStreamHandle &handle', 'hg::Texture &texture', 'hg::tVec2<int> &size', 'bgfx::TextureFormat::Enum &format', '?bool destroy'], {'arg_in_out': ['texture', 'size', 'format']})
	
def bind_profiler(gen):
	gen.add_include('foundation/profiler.h')

	gen.typedef('hg::ProfilerSectionIndex', 'size_t')

	profiler_frame = gen.begin_class('hg::ProfilerFrame')
	gen.end_class(profiler_frame)

	gen.bind_function('hg::BeginProfilerSection', 'hg::ProfilerSectionIndex', ['const std::string &name', '?const std::string &section_details'])
	gen.bind_function('hg::EndProfilerSection', 'void', ['hg::ProfilerSectionIndex section_idx'])

	gen.bind_function('hg::EndProfilerFrame', 'hg::ProfilerFrame', [])
	gen.bind_function('hg::CaptureProfilerFrame', 'hg::ProfilerFrame', [])

	gen.bind_function('hg::PrintProfilerFrame', 'void', ['const hg::ProfilerFrame &profiler_frame'])


def insert_non_embedded_setup_free_code(gen):
	if gen.get_language() == 'CPython':
		gen.insert_binding_code('''
#include "foundation/log.h"
#include <iostream>

static void OnHarfangLog(const char *msg, int mask, const char *details, void *user) {
	if (mask & hg::LL_Error)
		PyErr_SetString(PyExc_RuntimeError, msg);
	else if (mask & hg::LL_Warning)
		PyErr_WarnEx(PyExc_Warning, msg, 1);
	else
		std::cout << msg << std::endl;
}

static void InstallLogHook() { hg::set_log_hook(OnHarfangLog, nullptr); }
''')
	elif gen.get_language() == 'Lua':
		gen.insert_binding_code('''
''')

	gen.insert_binding_code('''
#include "foundation/build_info.h"

static void OutputLicensingTerms(const char *lang) {
	hg::log(
		hg::format("Harfang %1 for %2 on %3 (build %4 %5 %6)")
		.arg(hg::get_version_string()).arg(lang).arg(hg::get_target_string())
		.arg(hg::get_build_sha()).arg(__DATE__).arg(__TIME__)
	);
	hg::log("See https://www.harfang3d.com/license for licensing terms");
}
''')

	if gen.get_language() == 'Lua':
		gen.add_custom_init_code('OutputLicensingTerms("Lua 5.3");\n')
	elif gen.get_language() == 'CPython':
		gen.add_custom_init_code('''
InstallLogHook();
OutputLicensingTerms("CPython 3.2+");
''')

	gen.add_custom_free_code('\n')

def bind(gen):
	gen.start('harfang')

	lib.bind_defaults(gen)

	gen.add_include('foundation/cext.h')

	if not gen.embedded:
		insert_non_embedded_setup_free_code(gen)

	void_ptr = gen.bind_ptr('void *', bound_name='VoidPointer')
	gen.insert_binding_code('static void * _int_to_VoidPointer(intptr_t ptr) { return reinterpret_cast<void *>(ptr); }')
	gen.bind_function('int_to_VoidPointer', 'void *', ['intptr_t ptr'], {'route': route_lambda('_int_to_VoidPointer')})
		
	gen.typedef('bgfx::ViewId', 'uint16_t')

	#bind_std_vector(gen, gen.get_conv('char'))
	#bind_std_vector(gen, gen.get_conv('int'))
	#bind_std_vector(gen, gen.get_conv('int8_t'))
	#bind_std_vector(gen, gen.get_conv('int16_t'))
	#bind_std_vector(gen, gen.get_conv('int32_t'))
	#bind_std_vector(gen, gen.get_conv('int64_t'))
	#bind_std_vector(gen, gen.get_conv('uint8_t'))
	bind_std_vector(gen, gen.get_conv('uint16_t'))
	bind_std_vector(gen, gen.get_conv('uint32_t'))
	#bind_std_vector(gen, gen.get_conv('uint64_t'))
	#bind_std_vector(gen, gen.get_conv('float'))
	#bind_std_vector(gen, gen.get_conv('double'))

	bind_std_vector(gen, gen.get_conv('std::string'), 'StringList')

	bind_log(gen)
	bind_time(gen)
	bind_clock(gen)
	bind_file(gen)
	bind_data(gen)
	bind_dir(gen)
	bind_path_tools(gen)
	bind_assets(gen)
	bind_math(gen)
	bind_rand(gen)
	bind_projection(gen)
	bind_frustum(gen)
	bind_window_system(gen)
	bind_color(gen)
	bind_picture(gen)
	bind_render(gen)
	bind_forward_pipeline(gen)
	bind_font(gen)
	bind_meta(gen)
	bind_LuaObject(gen)
	bind_scene(gen)
	if gen.defined('HG_ENABLE_BULLET3_SCENE_PHYSICS'):
		bind_bullet3_physics(gen)
	bind_lua_scene_vm(gen)
	bind_scene_systems(gen)
	bind_input(gen)
	bind_imgui(gen)
	bind_platform(gen)
	bind_fps_controller(gen)
	bind_extras(gen)
	bind_audio(gen)
	bind_openvr(gen)
	bind_sranipal(gen)
	bind_vertex(gen)
	bind_model_builder(gen)
	bind_geometry_builder(gen)
	bind_iso_surface(gen)
	if gen.defined('HG_ENABLE_RECAST_DETOUR_API'):
		bind_recast_detour(gen)
	bind_bloom(gen)
	bind_sao(gen)
	bind_profiler(gen)

	bind_video_stream(gen)

	gen.finalize()
