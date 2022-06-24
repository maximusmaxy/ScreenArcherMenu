#pragma once

#include "f4se/GameInput.h"

class SamInputHandler : public BSInputEventUser
{
public:
	SamInputHandler() : BSInputEventUser() { }

	virtual void OnButtonEvent(ButtonEvent* inputEvent);
};

extern SamInputHandler inputHandler;