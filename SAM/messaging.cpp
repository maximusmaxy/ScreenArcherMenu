#include "messaging.h"

#include "constants.h"
#include "input.h"
#include "io.h"
#include "compatibility.h"
#include "options.h"

SamMessaging samMessaging;

class SamOpenCloseHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual ~SamOpenCloseHandler() { };
	virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent* evn, void* dispatcher) override
	{
		BSFixedString samMenu(SAM_MENU_NAME);
		BSFixedString photoMenu(PHOTO_MENU_NAME);
		BSFixedString consoleMenu(CONSOLE_MENU_NAME);
		BSFixedString cursorMenu(CURSOR_MENU_NAME);
		BSFixedString containerMenu(CONTAINER_MENU_NAME);
		BSFixedString looksMenu(LOOKS_MENU_NAME);

		if (evn->menuName == samMenu) {
			if (evn->isOpen) {
				inputHandler.enabled = true;
				auto handler = (BSInputEventUser*)&inputHandler;
				int idx = (*g_menuControls)->inputEvents.GetItemIndex(handler);
				if (idx == -1) {
					(*g_menuControls)->inputEvents.Push(handler);
				}
				if (samManager.OnMenuOpen())
					SetMenuVisible(photoMenu, "root1.Menu_mc.visible", false);
			}
			else {
				inputHandler.enabled = false;
				if (samManager.OnMenuClose())
					SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);
			}
		}
		else {
			if ((*g_ui)->IsMenuOpen(samMenu)) {
				if (evn->menuName == consoleMenu) {
					//console opened while sam is open
					if (evn->isOpen) {
						inputHandler.enabled = false;
					}
					//console closed while same is open
					else {
						samManager.OnConsoleUpdate();
						inputHandler.enabled = true;
					}
				}
				//else if (evn->menuName == cursorMenu) {
				//	//cursor opened while sam is open
				//	if (evn->isOpen) {
				//		MenuAlwaysOn(cursorMenu, true);
				//	}
				//}
				else if (evn->menuName == containerMenu ||
						 evn->menuName == looksMenu) {
					//container menu opened while sam is open
					if (evn->isOpen) {
						inputHandler.enabled = false;
						samManager.SetVisible(false);
						if (menuOptions.extrahotkeys)
							(*g_inputMgr)->AllowTextInput(false);
					}
					//container menu closed while sam is open
					else {
						inputHandler.enabled = true;
						samManager.SetVisible(true);
						if (menuOptions.extrahotkeys)
							(*g_inputMgr)->AllowTextInput(true);
					}
				}
			}
			//else {
			//	if (evn->menuName == cursorMenu) {
			//		//cursor opened while sam is closed
			//		if (evn->isOpen) {
			//			MenuAlwaysOn(cursorMenu, false);
			//		}
			//	}
			//}
		}
		return kEvent_Continue;
	};
};

SamOpenCloseHandler openCloseHandler;

void SafMessageHandler(F4SEMessagingInterface::Message* msg)
{
	if (msg->type == SAF::kSafAdjustmentManager) {
		saf = reinterpret_cast<SAF::SafMessagingInterface*>(msg->data);
		LoadMenuFiles();
	}
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SEMessagingInterface::kMessage_PostLoad:
	{
		if (samMessaging.messaging)
			samMessaging.messaging->RegisterListener(samMessaging.pluginHandle, "SAF", SafMessageHandler);

		if (samMessaging.f4se) {
			RegisterCompatibility(samMessaging.f4se);
		}
		break;
	}
	case F4SEMessagingInterface::kMessage_GameDataReady:
	{
		if (msg->data) {
			auto ui = *g_ui;
			if (ui) {
				ui->menuOpenCloseEventSource.AddEventSink(&openCloseHandler);
				BSFixedString menuName(SAM_MENU_NAME);
				if (!ui->IsMenuRegistered(menuName))
					ui->Register(SAM_MENU_NAME, CreateScreenArcherMenu);
			}
		}

		break;
	}
	//case F4SEMessagingInterface::kMessage_GameLoaded:
	//{
	//	GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&samEventReciever);
	//	break;
	//}
	}
}