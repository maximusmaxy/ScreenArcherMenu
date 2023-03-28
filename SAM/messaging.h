#pragma once

#include "sam.h"

struct SamMessaging {
	PluginHandle pluginHandle;

	F4SEScaleformInterface* scaleform;
	F4SEMessagingInterface* messaging;
	F4SEPapyrusInterface* papyrus;
	F4SESerializationInterface* serialization;
	F4SEInterface* f4se;

	SamMessaging() :
		pluginHandle(kPluginHandle_Invalid),
		scaleform(nullptr),
		messaging(nullptr),
		papyrus(nullptr),
		serialization(nullptr),
		f4se(nullptr)
	{};
};

extern SamMessaging samMessaging;

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg);