#include "bodymorphs.h"

#include "common/IFileStream.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"

#include "SAF/util.h"
#include "SAF/io.h"
#include "sam.h"
#include "papyrus.h"
#include "constants.h"
#include "options.h"
#include "types.h"

#include <filesystem>
#include <unordered_map>
#include <iostream>

typedef float (*BodygenGetMorph)(StaticFunctionTag*, Actor* actor, bool isFemale, BSFixedString morph, BGSKeyword* keyword);
typedef VMArray<BSFixedString>(*BodygenGetMorphs)(StaticFunctionTag*, Actor* actor, bool isFemale);
typedef void (*BodygenSetMorph)(StaticFunctionTag*, Actor* actor, bool isFemale, BSFixedString morph, BGSKeyword* keyword, float value);
typedef void (*BodygenRemoveAllMorphs)(StaticFunctionTag*, Actor* actor, bool isFemale);
typedef void (*BodygenUpdateMorphs)(StaticFunctionTag*, Actor* actor);

std::vector<std::string> bodyMorphNames;

bool GetSliderSet(NaturalSortedSet& sliders, const char* path)
{
	try {
		auto doc = std::make_unique<rapidxml::xml_document<>>();
		rapidxml::file<> file(path);
		doc->parse<0>(file.data());

		auto setInfo = doc->first_node("SliderSetInfo");
		if (!setInfo)
			return false;

		for (auto sliderSet = setInfo->first_node("SliderSet"); sliderSet; sliderSet = sliderSet->next_sibling("SliderSet")) {
			for (auto slider = sliderSet->first_node("Slider"); slider; slider = slider->next_sibling("Slider")) {
				auto sliderName = slider->first_attribute("name", 0, false);
				if (sliderName)
					sliders.emplace(sliderName->value());
			}
		}

		return true;
	}
	catch (std::exception& e) {
		_DMESSAGE(e.what());
	}

	return false;
}

void LoadSliderSet(GFxResult& result, const char* path)
{
	menuOptions.sliderSet = GetRelativePath(constStrLen(BODYSLIDE_SLIDERSETS_PATH), constStrLen(".osp"), path);
	menuOptions.Save();
}

void GetBodyMorphs(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	result.CreateMenuItems();
	bodyMorphNames.clear();

	auto getMorphFunc = (BodygenGetMorph*)GetNativeCallback("Bodygen", "GetMorph");
	if (!getMorphFunc) {
		return;
		//
	}

	std::string path = GetPathWithExtension(BODYSLIDE_SLIDERSETS_PATH, menuOptions.sliderSet.c_str(), ".osp");
	NaturalSortedSet sliders;
	if (!GetSliderSet(sliders, path.c_str())) {
		return;
		//return result.SetError("Failed to read slider set");
	}

	StaticFunctionTag tag;

	for (auto& slider : sliders) {
		float value = (*getMorphFunc)(&tag, (Actor*)selected.refr, selected.isFemale, BSFixedString(slider.c_str()), nullptr);
		SInt32 rounded = std::round(value * 100);
		result.PushItem(slider.c_str(), rounded);
		bodyMorphNames.push_back(std::string(slider));
	}
}

void SetBodyMorph(GFxResult& result, SInt32 index, SInt32 value)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	if (index < 0 || index >= bodyMorphNames.size())
		return result.SetError("Slider index out of range");

	auto setMorphFunc = (BodygenSetMorph*)GetNativeCallback("Bodygen", "SetMorph");
	auto updateMorphsFunc = (BodygenUpdateMorphs*)GetNativeCallback("BodyGen", "UpdateMorphs");
	
	if (!setMorphFunc || !updateMorphsFunc)
		return result.SetError(LOOKSMENU_ERROR);

	StaticFunctionTag tag;
	Actor* actor = (Actor*)selected.refr;
	(*setMorphFunc)(&tag, actor, selected.isFemale, BSFixedString(bodyMorphNames.at(index).c_str()), nullptr, value * 0.01);
	(*updateMorphsFunc)(&tag, actor);
}

void ResetBodyMorphs(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto removeAllFunc = (BodygenRemoveAllMorphs*)GetNativeCallback("BodyGen", "RemoveAllMorphs");
	auto updateMorphsFunc = (BodygenUpdateMorphs*)GetNativeCallback("BodyGen", "UpdateMorphs");

	if (!removeAllFunc || !updateMorphsFunc)
		return result.SetError(LOOKSMENU_ERROR);

	StaticFunctionTag tag;
	(*removeAllFunc)(&tag, (Actor*)selected.refr, selected.isFemale);
	(*updateMorphsFunc)(&tag, (Actor*)selected.refr);
}

void SaveBodyslidePreset(GFxResult& result, const char* filename)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto getMorphFunc = (BodygenGetMorph*)GetNativeCallback("Bodygen", "GetMorph");
	if (!getMorphFunc)
		return result.SetError(LOOKSMENU_ERROR);

	if (!bodyMorphNames.size())
		return result.SetError("No sliders found for target actor");

	auto doc = std::make_unique<rapidxml::xml_document<>>();
	auto declaration = doc->allocate_node(rapidxml::node_declaration);
	declaration->append_attribute(doc->allocate_attribute("version", "1.0"));
	declaration->append_attribute(doc->allocate_attribute("encoding", "UTF-8"));
	doc->append_node(declaration);
	auto presets = doc->allocate_node(rapidxml::node_element, "SliderPresets");
	doc->append_node(presets);
	auto preset = doc->allocate_node(rapidxml::node_element, "Preset");
	std::string presetName = std::filesystem::path(filename).filename().string();
	preset->append_attribute(doc->allocate_attribute("name", presetName.c_str()));
	preset->append_attribute(doc->allocate_attribute("set", menuOptions.sliderSet.c_str()));
	presets->append_node(preset);

	for (auto& group : menuOptions.sliderGroups) {
		auto groupNode = doc->allocate_node(rapidxml::node_element, "Group");
		groupNode->append_attribute(doc->allocate_attribute("name", group.c_str()));
		preset->append_node(groupNode);
	}

	StaticFunctionTag tag;

	for (auto& name : bodyMorphNames) {
		float value = (*getMorphFunc)(&tag, (Actor*)selected.refr, selected.isFemale, BSFixedString(name.c_str()), nullptr);
		SInt32 rounded = std::round(value * 100);

		if (rounded) {
			auto slider = doc->allocate_node(rapidxml::node_element, "SetSlider");
			slider->append_attribute(doc->allocate_attribute("name", name.c_str()));
			slider->append_attribute(doc->allocate_attribute("size", "big"));
			slider->append_attribute(doc->allocate_attribute("value", doc->allocate_string(std::to_string(rounded).c_str())));
			preset->append_node(slider);
		}
	}

	std::string path = GetPathWithExtension(BODYSLIDE_PRESETS_PATH, filename, ".xml");
	
	SAF::OutStreamWrapper wrapper(path.c_str());
	if (!wrapper.fail)
		wrapper.stream << *doc;
}

typedef std::vector<std::pair<std::string, SInt32>> SliderList;

bool GetBodyslidePreset(SliderList& sliders, const char* path) {
	try {
		auto doc = std::make_unique< rapidxml::xml_document<>>();
		rapidxml::file<> file(path);
		doc->parse<0>(file.data());

		auto presets = doc->first_node("SliderPresets");
		if (!presets)
			return false;

		auto preset = presets->first_node("Preset");
		if (!preset)
			return false;

		menuOptions.sliderGroups.clear();

		for (auto node = preset->first_node(); node; node = node->next_sibling())
		{
			auto nodeName = node->name();
			if (!_stricmp(nodeName, "SetSlider")) {
				auto nameAttribute = node->first_attribute("name", 0, false);
				if (nameAttribute) {
					auto valueAttribute = node->first_attribute("value", 0, false);
					if (valueAttribute) {
						sliders.emplace_back(nameAttribute->value(), StringToSInt32(valueAttribute->value()));
					}
				}
			}
			else if (!_stricmp(nodeName, "Group")) {
				auto nameAttribute = node->first_attribute("name", 0, false);
				if (nameAttribute)
					menuOptions.sliderGroups.emplace(nameAttribute->value());
			}
		}

		menuOptions.Save();

		return true;
	}
	catch (std::exception& e) {
		_DMESSAGE(e.what());
	}

	return false;
}

void LoadBodyslidePreset(GFxResult& result, const char* path) 
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto setMorphFunc = (BodygenSetMorph*)GetNativeCallback("BodyGen", "SetMorph");
	auto removeAllFunc = (BodygenRemoveAllMorphs*)GetNativeCallback("BodyGen", "RemoveAllMorphs");
	auto updateMorphsFunc = (BodygenUpdateMorphs*)GetNativeCallback("BodyGen", "UpdateMorphs");
	if (!setMorphFunc || !removeAllFunc || !updateMorphsFunc)
		return result.SetError(LOOKSMENU_ERROR);

	SliderList sliders;
	if (!GetBodyslidePreset(sliders, path))
		return result.SetError(BODYSLIDE_PRESET_ERROR);

	StaticFunctionTag tag;
	(*removeAllFunc)(&tag, (Actor*)selected.refr, selected.isFemale);
	for (auto& slider : sliders) {
		(*setMorphFunc)(&tag, (Actor*)selected.refr, selected.isFemale, BSFixedString(slider.first.c_str()), nullptr, slider.second * 0.01);
	}
	(*updateMorphsFunc)(&tag, (Actor*)selected.refr);
}