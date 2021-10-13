.title Reading Input

The input system provides access to the HID devices connected to the host machine.

## Supported Device Classes

All supported input device classes are utilized in the same manner. All devices connected to the host are identified by a unique name. There are two main functions, one to list available device names and one to read a specific device state.

Wrapper classes are available to manage current and previous states to implement high-level functions such as pressed, released, etc. When the class update function is called the current state replaces the previous state and is then replace by the current state.

The following table lists main clases and functions for all supported device classes.

Device Class  | List Devices              | Read State           | Wrapper Class
--------------|---------------------------|----------------------|--------------
Mouse         | [GetMouseNames]           | [ReadMouse]          | [Mouse]
Keyboard      | [GetKeyboardNames]        | [ReadKeyboard]       | [Keyboard]
Gamepad       | [GetGamepadNames]         | [ReadGamepad]        | [Gamepad]
VR Controller | [GetVRControllerNames]    | [ReadVRController]   | [VRController]
VR Generic Tracker | [GetVRGenericTrackerNames] | [ReadVRGenericTracker] | [VRGenericTracker]

## Reading from the Mouse and Keyboard devices

The main keyboard device connected to the host is always retrieved using the `default` name. All of the physical keyboard keys can be queried by using their corresponding [Key].

The main mouse device connected to the host is always retrieved using the `default` name.

## Headless Application

Most input device require your application to open a window and will only return a valid state when your application window has focus.

For headless applications, the device name `raw` can be used. A raw device will return a valid state even if the application does not have focus.
