#include "inventory.h"

#include "f4se/GameForms.h"
#include "f4se_common/Relocation.h"
#include "f4se/GameData.h"

#include "sam.h"
#include "constants.h"
#include "idle.h"

typedef bool (*_OpenActorContainerInternal)(TESObjectREFR* refr, UInt32 unk2, bool unk3);
RelocAddr<_OpenActorContainerInternal> OpenActorContainerInternal(0x1264F40);

void OpenActorContainer(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	OpenActorContainerInternal(selected.refr, 3, false);
}

void GetItemModsFromList(GFxResult& result, tArray<ModInfo*>& list)
{
	//Reverse engineer Form Groups, find out 
	//auto it = list.entries;
	//auto end = list.entries + list.count;
	//while (it != end) {
	//	
	//}
}

//Can't cache entire modlist due to memory concerns so just go through each mod and check if there is a group with addable items
void GetItemMods(GFxResult& result) {
	result.CreateMenuItems();
	GetItemModsFromList(result, (*g_dataHandler)->modList.loadedMods);
	GetItemModsFromList(result, (*g_dataHandler)->modList.lightMods);
}

//Check each equipped item for at least 1 matt swap
void GetMatSwapEquipment(GFxResult& result) {

}

//Get mat swaps for the specified id
void GetMatSwaps(GFxResult& result) {

}