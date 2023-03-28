#include "input.h"

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/InputMap.h"

#include "constants.h"
#include "sam.h"

UInt32 inputRepeat[InputMap::kMaxMacros] = { 0 };

void SamInputHandler::OnButtonEvent(ButtonEvent* inputEvent)
{
	UInt32 keyCode = 0;
	UInt32 deviceType = inputEvent->deviceType;
	UInt32 keyMask = inputEvent->keyMask;

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
	else {
		keyCode = keyMask;
	}

	if (!keyCode)
		return;

	// Valid scancode? Add two for mouse scroll
	//if (keyCode >= InputMap::kMaxMacros)// + 2)
	//	return;

	//BSFixedString	control	= *inputEvent->GetControlID();
	float timer = inputEvent->timer;

	if (inputEvent->isDown == 1.0f) {
		if (timer == 0.0f) {
			samManager.Invoke("root1.Menu_mc.ProcessKeyDown", nullptr, &GFxValue(keyCode), 1);
			inputRepeat[keyCode] = 0;
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

RelocPtr<UInt64*> inputEnableManager(0x58D0780);
RelocPtr<UInt64> debugNameFunctor(0x1B23EB0);

struct InputEnableNameFunctor {
	UInt64** functor;
	const char*** name;
};

struct BSInputEnableLayer {
	UInt32 index;
	UInt32 state;
};

BSInputEnableLayer* inputEnableLayer { nullptr };
std::mutex inputEnableLayerMutex;

typedef bool (*_AllocateNewLayer)(UInt64* manager, BSInputEnableLayer** layerOut, InputEnableNameFunctor* functor);
RelocAddr<_AllocateNewLayer> AllocateNewLayer(0x1B22180);

//Need to dec ref the inputenablelayer manually to make sure the resources are released
typedef void (*_InputEnableLayerDecRef)(BSInputEnableLayer* InputEnableLayer);
RelocAddr<_InputEnableLayerDecRef> InputEnableLayerDecRef(0x1B22460);

typedef bool (*_EnableUserEvent)(BSInputEnableLayer* layer, UInt32 flags, bool unk1, UInt32 unk2);
RelocAddr<_EnableUserEvent> EnableUserEvent(0x241DD0);

typedef bool (*_EnableOtherEvent)(BSInputEnableLayer* layer, UInt32 flags, bool unk1, UInt32 unk2);
RelocAddr<_EnableOtherEvent> EnableOtherEvent(0x516B30);

void SetInputEnableLayer(bool enable) {
	std::lock_guard lock(inputEnableLayerMutex);

	//check if updated
	if (enable == (!!inputEnableLayer))
		return;

	if (enable) {
		const char* name = "SamInputEnableLayer";
		const char** namename = &name;
		UInt64* functor = debugNameFunctor;
		InputEnableNameFunctor nameFunctor{ &functor, &namename };
		if (!AllocateNewLayer(*inputEnableManager, &inputEnableLayer, &nameFunctor)) {
			inputEnableLayer = nullptr;
			return;
		}
		
		static UInt32 userFlags =
			1 << 0 |	//movement
			1 << 1 |	//looking
			1 << 6 |	//fighting
			1 << 7 |	//sneaking
			1 << 8 |	//menu
			1 << 10;	//movement2

		if (!EnableUserEvent(inputEnableLayer, userFlags, false, 0x3)) {
			InputEnableLayerDecRef(inputEnableLayer);
			inputEnableLayer = nullptr;
			return;
		}

		static UInt32 otherFlags =
			1 << 0 |	//journal
			1 << 1 |	//activate
			1 << 3 |	//camswitch
			1 << 4 |	//vats
			1 << 5 |	//favorites
			1 << 8;		//running

		if (!EnableOtherEvent(inputEnableLayer, otherFlags, false, 0x3)) {
			InputEnableLayerDecRef(inputEnableLayer);
			inputEnableLayer = nullptr;
			return;
		}
	}
	else {
		InputEnableLayerDecRef(inputEnableLayer);
		inputEnableLayer = nullptr;
	}
}