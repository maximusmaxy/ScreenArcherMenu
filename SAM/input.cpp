#include "input.h"

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/InputMap.h"

#include "constants.h"
#include "sam.h"

UInt32 inputRepeat[InputMap::kMaxMacros] = { 0 };

void SamInputHandler::OnButtonEvent(ButtonEvent* inputEvent)
{
	UInt32	keyCode;
	UInt32	deviceType = inputEvent->deviceType;
	UInt32	keyMask = inputEvent->keyMask;

	// Mouse
	if (deviceType == InputEvent::kDeviceType_Mouse)
	{
		//Need to capture mouse scroll 0x800 = up, 0x900 = down
		//if (keyMask == 0x800)
		//	keyCode = InputMap::kMaxMacros;
		//else if (keyMask == 0x900)
		//	keyCode = InputMap::kMaxMacros + 1;
		//else
		keyCode = InputMap::kMacro_MouseButtonOffset + keyMask;
	}
	// Gamepad
	else if (deviceType == InputEvent::kDeviceType_Gamepad)
		keyCode = InputMap::GamepadMaskToKeycode(keyMask);
	// Keyboard
	else
		keyCode = keyMask;

	// Valid scancode? Add two for mouse scroll
	if (keyCode >= InputMap::kMaxMacros)// + 2)
		return;

	//BSFixedString	control	= *inputEvent->GetControlID();
	float timer = inputEvent->timer;

	if (inputEvent->isDown == 1.0f) {
		if (timer == 0.0f) {
			//GFxValue isOpen;
			samManager.Invoke("root1.Menu_mc.ProcessKeyDown", nullptr, &GFxValue(keyCode), 1);
			inputRepeat[keyCode] = 0;
			//if (!isOpen.GetBool())
			//	inputEvent->handled = 2;
		}
		else {
			UInt32 repeats = ++inputRepeat[keyCode];
			if (repeats > 10) { //if (repeats > 20 && repeats % 3 == 0) {
				samManager.Invoke("root1.Menu_mc.ProcessKeyRepeat", nullptr, &GFxValue(keyCode), 1);
			}
		}
	}
	else if (inputEvent->isDown == 0.0f && timer != 0.0f) {
		samManager.Invoke("root1.Menu_mc.ProcessKeyUp", nullptr, &GFxValue(keyCode), 1);
	}
}

SamInputHandler inputHandler;