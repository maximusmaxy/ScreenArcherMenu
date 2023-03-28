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

#include "json.h"
#include "SAF/adjustments.h"
#include "SAF/types.h"
#include "SAF/messaging.h"

#include "gfx.h"

#include <unordered_map>
#include <mutex>
#include <list>

extern SAF::SafMessagingInterface* saf;

class ScreenArcherMenu : public GameMenuBase
{
public:
	ScreenArcherMenu();
	virtual ~ScreenArcherMenu();

	virtual void AdvanceMovie(float unk0, void* unk1) override final;
	virtual void RegisterFunctions() override final;
	virtual void Invoke(Args* args) override final;

	DEFINE_STATIC_HEAP(ScaleformHeap_Allocate, ScaleformHeap_Free);

	class BoneDisplay {
	public:
		bool enabled;
		//bool rotateEnabled;
		std::mutex mutex;
		//std::list<const char*> hovers;
		
		struct NodeMarker {
			GFxValue marker;
			NiAVObject* node;
			BSGFxShaderFXTarget* target;
			bool visible;
			bool enabled;

			NodeMarker() : node(nullptr), target(nullptr), visible(true), enabled(true) {}
			NodeMarker(NiAVObject* node) : node(node), target(nullptr), visible(true), enabled(true) {}
			~NodeMarker() { if (target) delete target; };
		};

		std::vector<NodeMarker> nodes;
		NodeMarker* selectedNode;
		NodeMarker* rootMarker;
		SAF::ActorAdjustmentsPtr actor;
		
		struct BoneMarker {
			GFxValue marker;
			NodeMarker* start;
			NodeMarker* end;
			BSGFxShaderFXTarget* target;

			BoneMarker() : start(nullptr), end(nullptr), target(nullptr) {}
			BoneMarker(NodeMarker* start, NodeMarker* end) : start(start), end(end), target(nullptr) {}
			~BoneMarker() { if (target) delete target; };
		};

		std::vector<BoneMarker> bones;
		//BoneMarker* selectedBone;

		//struct AxisMarker {
		//	GFxValue marker;
		//	NiTransform transform;
		//	BSGFxShaderFXTarget* target;

		//	AxisMarker() : target(nullptr) {}
		//	AxisMarker(NiTransform& transform) : transform(transform), target(nullptr) {}
		//	~AxisMarker() { if (target) delete target; };
		//};

		//std::vector<AxisMarker> axes;

		//struct RotateTool {
		//	GFxValue tool;
		//	GFxValue axis[3];
		//	BSGFxShaderFXTarget* targets[3];

		//	RotateTool() :targets() {}
		//	~RotateTool() { for (auto& target : targets) { if (target) delete target; } }
		//};

		//std::unique_ptr<RotateTool> rotateTool;

		//struct RotateMarker {
		//	GFxValue marker;
		//	NiTransform transform;
		//	BSGFxShaderFXTarget* target;

		//	RotateMarker() : target(nullptr) {}
		//	RotateMarker(NiTransform& transform) : transform(transform), target(nullptr) {}
		//	~RotateMarker() { if (target) delete target; };
		//};

		//std::vector<RotateMarker> rotates;

		struct RotateMarker {
			GFxValue marker;
			BSGFxShaderFXTarget* target;

			RotateMarker() : target(nullptr) {}
			~RotateMarker() { if (target) delete target; }
		};

		std::unique_ptr<RotateMarker> rotateMarker;

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
			//rotateEnabled(false),
			selectedNode(nullptr),
			//selectedBone(nullptr),
			rootMarker(nullptr),
			actor(nullptr) 
		{}

		void Update();
	};
	
	BoneDisplay boneDisplay;

	BoneDisplay::NodeMarker* PushNodeMarker(NiAVObject* node);
	void PushBoneMarker(BoneDisplay::NodeMarker* start, BoneDisplay::NodeMarker* end);
	//void PushAxisMarker(NiColor& color, NiTransform& transform);
	//void PushRotateMarker(SInt32 axis, NiColor& color, NiTransform& transform);
	void VisitNodes(SAF::BSFixedStringSet& set, NiAVObject* parent, BoneDisplay::NodeMarker* start);

	void EnableBoneDisplay(std::shared_ptr<SAF::ActorAdjustments> adjustments);
	void DisableBoneDisplay();

	//void EnableAxisDisplay();
	//void DisableAxisDisplay();

	void EnableRotateDisplay();
	void DisableRotateDisplay();

	void GetNodeSet(SAF::BSFixedStringSet* set);
	void UpdateBoneFilter();

	bool SelectNode(const char* nodeName);
	void UnselectNode();

	//void UpdateDebug();
	//void DrawDebug(SInt32 i, const char* text);
};

IMenu* CreateScreenArcherMenu();
void SetBoneDisplay(GFxResult& result, bool enabled);
void SelectNodeMarker(GFxResult& result, const char* name, bool update);
void UnselectNodeMarker(GFxResult& result);
//void OverNodeMarker(GFxResult& result, const char* name);
//void OutNodeMarker(GFxResult& result, const char* name);
void UpdateBoneFilter();
NiPoint3 GetCameraPivot();

class SelectedRefr {
public:
	TESObjectREFR* refr;
	bool isFemale;
	UInt32 race;
	UInt64 key;

	//virtual TESObjectREFR* Refr();
	void Update(TESObjectREFR* refr);
	void Clear();
};

class IMenuWrapper
{
public:
	IMenu* menu;

	IMenuWrapper() : menu(nullptr) {}
	IMenuWrapper(IMenu* menu);
	~IMenuWrapper();

	GFxMovieRoot* GetRoot();
	bool IsOpen();
	bool IsRegistered();
};

extern SelectedRefr selected;

class SamManager {
private:
	std::mutex mutex;

public:
	TESObjectREFR* refr;
	Json::Value data;
	std::string storedName;
	IMenu* storedMenu;

	SamManager() : storedMenu(nullptr), refr(nullptr) {};

	IMenuWrapper StoreMenu();
	bool ReleaseMenu();
	IMenuWrapper GetWrapped();

	void OpenOrCloseMenu(const char* menuName);
	void ToggleMenu();
	void CloseMenu();
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

//void RegisterSam();
//void StartSamQuest();

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);
void MenuAlwaysOn(BSFixedString menuStr, bool enabled);

bool GetCursor(SInt32* pos);
bool SetCursor(SInt32 x, SInt32 y);
void GetCursorPosition(GFxResult& result);

void SetWidget(GFxResult& result, const char* type, bool enabled);

void SamSerializeSave(const F4SESerializationInterface* ifc);
void SamSerializeLoad(const F4SESerializationInterface* ifc);
void SamSerializeRevert(const F4SESerializationInterface* ifc);