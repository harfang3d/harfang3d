// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#if 0 // FIXME

#include "platform/win32/input_dinput.h"
#include "foundation/assert.h"
#include "foundation/cext.h"
#include "foundation/log.h"
#include "foundation/string.h"
#include "platform/input_device.h"
#include "platform/input_system.h"
#include "platform/window_system.h"
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x800
#include <dinput.h>

namespace hg {

struct DInputDeviceObjects {
	LPDIRECTINPUTEFFECT effect = nullptr;
	LPDIRECTINPUTDEVICE8 device = nullptr;
};

static DInputDeviceObjects CreateDevice(HWND hwnd, LPDIRECTINPUT8 lpdi, GUID guid) {
	DIDEVCAPS didcaps;
	didcaps.dwSize = sizeof(DIDEVCAPS);

	DInputDeviceObjects device;
	if (FAILED(lpdi->CreateDevice(guid, &device.device, nullptr)))
		return device;

	if (FAILED(device.device->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND)))
		error("Failed to set cooperative level");
	if (FAILED(device.device->SetDataFormat(&c_dfDIJoystick2)))
		error("Failed to set data format");
	/*
	if (FAILED(lpdi->GetCapabilities(&didcaps)))
		__LOG_E__ << "failed to get device capabilities.\n";
*/

	/*
		Since we will be playing force feedback effects, we should disable the
		auto-centering spring.
	*/
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = FALSE;

	if (FAILED(device.device->SetProperty(DIPROP_AUTOCENTER, &dipdw.diph)))
		warn("Failed to disable force feedback spring");

	// this application needs only one effect: applying raw forces
	DWORD rgdwAxes[2] = {DIJOFS_X, DIJOFS_Y};
	LONG rglDirection[2] = {0, 0};
	DICONSTANTFORCE cf = {0};

	DIEFFECT eff;
	ZeroMemory(&eff, sizeof(eff));
	eff.dwSize = sizeof(DIEFFECT);
	eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
	eff.dwDuration = INFINITE;
	eff.dwSamplePeriod = 0;
	eff.dwGain = DI_FFNOMINALMAX;
	eff.dwTriggerButton = DIEB_NOTRIGGER;
	eff.dwTriggerRepeatInterval = 0;
	eff.cAxes = 1;
	eff.rgdwAxes = rgdwAxes;
	eff.rglDirection = rglDirection;
	eff.lpEnvelope = 0;
	eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
	eff.lpvTypeSpecificParams = &cf;
	eff.dwStartDelay = 0;

	// create the prepared effect
	if (FAILED(device.device->CreateEffect(GUID_ConstantForce, &eff, &device.effect, nullptr)))
		warn("Failed to create device force feedback effect");

	device.device->Acquire();
	if (device.effect)
		device.effect->Start(1, 0);

	return device;
}

static void ReleaseDevice(DInputDeviceObjects &device) {
	if (device.effect) {
		device.effect->Release();
		device.effect = nullptr;
	}

	if (device.device) {
		device.device->Release();
		device.device = nullptr;
	}
}

//
struct DInputDevice : public InputDevice {
	~DInputDevice() { ReleaseDevice(device); }

	InputDeviceType GetType() const override { return InputDevicePad; }

	void Update() override {
		if (!device.device)
			return;
		auto win = GetWindowInFocus();
		auto hwnd = (HWND)GetWindowHandle(win);
		if (hwnd == NULL) {
			return;
		}

		previous_state = current_state;

		memset(&current_state, 0, sizeof(DIJOYSTATE2));
		//device.device->Acquire();
		device.device->Poll();
		if (FAILED(device.device->GetDeviceState(sizeof(DIJOYSTATE2), &current_state)))
			return;
	}

	bool SetEffect(InputDeviceEffect fx, float v) override {
		if (!device.effect || fx != InputDeviceConstantForce)
			return false;

		device.device->Acquire();
		LONG rglDirection[1] = {0};
		DICONSTANTFORCE cf;

		DIEFFECT eff;
		ZeroMemory(&eff, sizeof(eff));
		eff.dwSize = sizeof(DIEFFECT);
		eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
		eff.dwDuration = INFINITE;
		eff.cAxes = 1;
		eff.rglDirection = rglDirection;
		eff.lpEnvelope = 0;
		eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
		eff.lpvTypeSpecificParams = &cf;
		eff.dwStartDelay = 0;

		cf.lMagnitude = LONG(v);

		// now set the new parameters and start the effect immediately
		device.effect->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START);
		return true;
	}

	bool IsButtonDown(Button b) const override { return TestButtonState(b, current_state); }
	bool WasButtonDown(Button b) const override { return TestButtonState(b, previous_state); }

	float GetValue(AnalogInput i) const override { return GetStateInputValue(i, current_state); }
	float GetLastValue(AnalogInput i) const override { return GetStateInputValue(i, previous_state); }

	bool GetValueRange(AnalogInput i, float &min, float &max) const override {
		switch (i) {
			case InputButton0:
			case InputButton1:
				min = 0.f;
				max = 1.f;
				break;

			case InputAxisX:
			case InputAxisY:
			case InputAxisS:
			case InputAxisT:
				min = -1.f;
				max = 1.f;
				break;

			default:
				return false;
		}
		return true;
	}

	void ResetLastValues() { previous_state = current_state; };

	static float GetStateInputValue(AnalogInput i, const DIJOYSTATE2 &state) {
		switch (i) {
			case InputAxisX:
				return float(state.lX);
			case InputAxisY:
				return float(state.lY);
			case InputAxisZ:
				return float(state.lZ);

			case InputRotX:
				return float(state.lRx);
			case InputRotY:
				return float(state.lRy);
			case InputRotZ:
				return float(state.lRz);

			case InputAxisS:
				return float(state.rglSlider[0]);
			case InputAxisT:
				return float(state.rglSlider[1]);

			default:
				break;
		}
		return 0.f;
	}

	static bool TestButtonState(Button btn, const DIJOYSTATE2 &state) {
		if (btn >= Button0 && btn <= Button127)
			return asbool(float(state.rgbButtons[btn - Button0]));

		switch (btn) {
			case ButtonCrossUp:
				return asbool(state.rgdwPOV[0] == 0);
			case ButtonCrossDown:
				return asbool(state.rgdwPOV[0] == 18000);
			case ButtonCrossLeft:
				return asbool(state.rgdwPOV[0] == 27000);
			case ButtonCrossRight:
				return asbool(state.rgdwPOV[0] == 9000);

			default:
				break;
		}
		return false;
	}

	DInputDeviceObjects device;
	DIJOYSTATE2 current_state, previous_state;
};

static BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef) {
	auto &guids = *reinterpret_cast<std::map<std::string, GUID> *>(pvRef);

	char16_t guid_wstring[64];
	int len = StringFromGUID2(lpddi->guidInstance, (LPOLESTR)guid_wstring, 64);
	__ASSERT__(len);
	auto guid_string = utf16_to_utf8(guid_wstring);
	auto device_name = std::string(lpddi->tszProductName) + " " + guid_string;
	guids[device_name] = lpddi->guidInstance;
	return DIENUM_CONTINUE;
}

struct DInput8DeviceProvider : InputDeviceProvider {
	DInput8DeviceProvider() {
		if (DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A, (LPVOID *)&lpdi, nullptr) == DI_OK)
			lpdi->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, &guids, DIEDFL_ATTACHEDONLY);
	}
	~DInput8DeviceProvider() { lpdi->Release(); }

	std::vector<std::string> GetDevices() const override {
		std::vector<std::string> names;
		std::transform(guids.begin(), guids.end(), std::back_inserter(names), [](const std::pair<std::string, GUID> &i) { return i.first; });
		return names;
	}

	std::shared_ptr<InputDevice> GetDevice(const std::string &name) override {
		auto g = guids.find(name);
		if (g == guids.end())
			return nullptr; // no such device

		auto guid = g->second;
		auto hwnd = HWND(GetWindowHandle(GetWindowInFocus()));

		auto device = std::make_shared<DInputDevice>();
		device->device = CreateDevice(hwnd, lpdi, guid);
		return device;
	}

	LPDIRECTINPUT8 lpdi = nullptr;

	std::map<std::string, GUID> guids;
};

void RegisterDInput8Devices(InputSystem &system) {
	system.RegisterDeviceProvider("dinput8", std::make_unique<DInput8DeviceProvider>());
}

} // namespace hg

#endif
