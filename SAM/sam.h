#pragma once

#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/GameMenus.h"
#include "f4se/NiTypes.h"

#include "SAF/adjustments.h"
#include "SAF/types.h"
#include "SAF/messaging.h"

#include "gfx.h"

#include <json/json.h>
#include <unordered_map>
#include <mutex>
#include <list>

extern SAF::SafMessagingInterface* saf;

class ScreenArcherMenu : public GameMenuBase
{
public:
	ScreenArcherMenu();
	//virtual ~ScreenArcherMenu() override final;

	virtual void AdvanceMovie(float unk0, void* unk1) override final;
	virtual void RegisterFunctions() override final;
	virtual void Invoke(Args* args) override final;

	//DEFINE_STATIC_HEAP(ScaleformHeap_Allocate, ScaleformHeap_Free);

	class BoneDisplay {
	public:
		bool enabled;
		std::mutex mutex;

		struct {
			double x{};
			double y{};
			double width{};
			double height{};
		} dimensions{};
		
		struct NodeMarker {
			GFxValue marker;
			BSFixedString parent;
			NiAVObject* node;
			bool visible;
			bool enabled;

			NodeMarker() : node(nullptr), visible(true), enabled(true) {}
			NodeMarker(NiAVObject* node, BSFixedString name) : node(node), parent(name), visible(true), enabled(true) {}
		};

		std::vector<NodeMarker> nodes;
		NodeMarker* selectedNode;
		NodeMarker* rootMarker;
		SAF::ActorAdjustmentsPtr actor;
		
		struct BoneMarker {
			GFxValue marker;
			NodeMarker* start;
			NodeMarker* end;

			BoneMarker() : start(nullptr), end(nullptr) {}
			BoneMarker(NodeMarker* start, NodeMarker* end) : start(start), end(end) {}
		};

		std::vector<BoneMarker> bones;

		std::unique_ptr<GFxValue> rotateMarker;

		void Reserve(UInt32 size) {
			nodes.reserve(size);
			bones.reserve(size);
		}

		void Clear() {
			nodes.clear();
			bones.clear();
			actor = nullptr;
		}

		BoneDisplay() :
			enabled(false),
			selectedNode(nullptr),
			rootMarker(nullptr),
			actor(nullptr)
		{}

		void Update();
	};
	
	BoneDisplay boneDisplay;

	void PushNodeMarker(NiAVObject* node, BSFixedString name);
	void PushBoneMarker(BoneDisplay::NodeMarker* start, BoneDisplay::NodeMarker* end);
	void VisitNodes(SAF::BSFixedStringSet& set, SAF::BSFixedStringSet& found, NiAVObject* parent, BSFixedString name);
	void AddBones();

	void EnableBoneDisplay(std::shared_ptr<SAF::ActorAdjustments> adjustments);
	void DisableBoneDisplay();

	void EnableRotateDisplay();
	void DisableRotateDisplay();

	void GetNodeSet(SAF::BSFixedStringSet* set);
	void UpdateBoneFilter();

	bool SelectNode(const char* nodeName);
	void UnselectNode();
};

IMenu* CreateScreenArcherMenu();
void SetBoneDisplay(GFxResult& result, bool enabled);
void SelectNodeMarker(GFxResult& result, const char* name, bool update);
void UnselectNodeMarker(GFxResult& result);
void UpdateBoneFilter();
NiPoint3 GetCameraPivot();

class SelectedRefr {
public:
	TESObjectREFR* refr{};
	bool isFemale{};
	UInt32 race{};
	UInt64 key{};

	void Update(TESObjectREFR* refr);
	void Clear();
};

class IMenuWrapper
{
public:
	IMenu* menu;
	GFxMovieRoot* root;

	IMenuWrapper() : menu(nullptr), root(nullptr) {}
	IMenuWrapper(IMenu* menu);
	~IMenuWrapper();

	operator bool() const {
		return root;
	}
	GFxMovieRoot* operator->() {
		return root;
	}

	template <class T>
	T* Get() {
		return reinterpret_cast<T*>(menu);
	}
	GFxMovieRoot* GetRoot();
	bool IsOpen();
	bool IsRegistered();
};

extern SelectedRefr selected;

class SamManager {
private:
	std::mutex mutex;

public:
	TESObjectREFR* refr{};
	Json::Value data;
	std::string storedName;
	IMenu* storedMenu{};
	TESObjectCELL* storedCoc{};
	std::string storedEdid;
	const char* closeMessage = "$SAM_CloseMenu";

	IMenuWrapper StoreMenu();
	bool ReleaseMenu();
	IMenuWrapper GetWrapped();

	void OpenOrCloseMenu(const char* menuName = nullptr);
	void ToggleMenu();
	void CloseMenu();
	void ShowCloseMessage();
	void CancelClose();
	bool Invoke(const char* name, GFxValue* result, GFxValue* args, UInt32 numArgs);
	void SetVariable(const char* pVarPath, const GFxValue* value, UInt32 setType = 0);

	void SaveData(GFxValue* data);
	bool LoadData(GFxMovieRoot* root, GFxValue* res);
	void ClearData();
	void ForceQuit();

	bool OnMenuOpen();
	bool OnConsoleUpdate();
	bool OnMenuClose();

	void SetVisible(bool visible);

	void OpenExtensionMenu(const char* name);
	void PushMenu(const char* name);
	void PopMenu();
	void PopMenuTo(const char* name);
	void RefreshMenu();
	void RefreshCamera();
	void UpdateMenu();
	void ShowNotification(const char* name, bool store);
	void SetTitle(const char* name);
	void SetNotification(const char* name);
	void SetMenuNames(VMArray<BSFixedString>& names);
	void SetMenuValues(VMArray<VMVariable>& values);
	void SetMenuItems(VMArray<BSFixedString>& names, VMArray<VMVariable>& values);
	void SetSuccess();
	void SetError(const char* error);
	void SetLocal(const char* key, GFxValue* value);
};

extern SamManager samManager;

TESObjectREFR* GetRefr();

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);
//void MenuAlwaysOn(BSFixedString menuStr, bool enabled);

bool GetCursor(SInt32* pos);
bool SetCursor(SInt32 x, SInt32 y);
void GetCursorPosition(GFxResult& result);

void SetWidget(GFxResult& result, const char* type, bool enabled);

void SamSerializeSave(const F4SESerializationInterface* ifc);
void SamSerializeLoad(const F4SESerializationInterface* ifc);
void SamSerializeRevert(const F4SESerializationInterface* ifc);