#include "mfg.h"

#include "f4se/GameReferences.h"

#include "f4se_common/Relocation.h"

#include "common/IFileStream.h"

#include <regex>

#include "sam.h"

RelocAddr<UInt64> faceGenAnimDataVfTable(0x2CE9C58);

float* GetMorphPointer() {
	if (!selected.refr) return nullptr;
	Actor::MiddleProcess* middleProcess = ((Actor*)selected.refr)->middleProcess;
	if (!middleProcess) return nullptr;
	Actor::MiddleProcess::Data08* middleProcessData = middleProcess->unk08;
	if (!middleProcessData) return nullptr;
	BSFaceGenAnimationData* faceGenAnimData = (BSFaceGenAnimationData*)middleProcessData->unk3B0[3];
	if (!faceGenAnimData || faceGenAnimData->vfTable != faceGenAnimDataVfTable) return nullptr;
	return faceGenAnimData->mfgMorphs;
}

void SetFaceMorph(UInt32 categoryIndex, UInt32 morphIndex, UInt32 scale)
{
	MenuCategoryList* menu = GetMenu(&morphsMenuCache);
	if (!menu) return;

	float* ptr = GetMorphPointer();
	if (!ptr) return;

	UInt32 key = std::stoul((*menu)[categoryIndex].second[morphIndex].second);
	ptr[key] = scale * 0.0099999998f;
}

void SaveMfg(std::string filename) {
	float* ptr = GetMorphPointer();
	if (!ptr) return;

	IFileStream file;
	std::string path = "Data\\F4SE\\Plugins\\SAM\\FaceMorphs\\";
	path += filename;
	path += ".txt";
	IFileStream::MakeAllDirs(path.c_str());
		
	if (!file.Create(path.c_str())) {
		_DMESSAGE("Failed to create file");
		return;
	}

	std::string morphs;
	for (int i = 0; i < 54; ++i) {
		UInt32 scale = std::round(ptr[i] * 100);
		scale = max(0, min(100, scale));
		if (scale != 0) {
			morphs += "mfg morphs ";
			morphs += std::to_string(i);
			morphs += " ";
			morphs += std::to_string(scale);
			morphs += "\n";
		}
	}
	file.WriteString(morphs.c_str());
	file.Close();
}

std::regex mfgRegex("\\s*mfg\\s+morphs\\s+(\\d+)\\s+(\\d+).*");

bool LoadMfg(std::string filename) {
	float* ptr = GetMorphPointer();
	if (!ptr) return false;

	std::string path = "Data\\F4SE\\Plugins\\SAM\\FaceMorphs\\";
	path += filename;
	path += ".txt";

	IFileStream file;

	if (!file.Open(path.c_str())) {
		_DMESSAGE("File not found");
		return false;
	}

	char buf[512];
	std::cmatch match;

	float morphs[54];
	std::memset(morphs, 0, sizeof(morphs));

	while (!file.HitEOF()) {
		file.ReadString(buf, 512, '\n', '\r');
		if (std::regex_match(buf, match, mfgRegex)) {
			int id = max(0, min(53, std::stoi(match[1].str())));
			int scale = max(0, min(100, std::stoi(match[2].str())));
			morphs[id] = scale * 0.0099999998f; 
		}
	}

	for (int i = 0; i < 54; ++i) {
		ptr[i] = morphs[i];
	}

	file.Close();

	return true;
}

void ResetMfg() {
	float* ptr = GetMorphPointer();
	if (!ptr) return;

	memset(ptr, 0, sizeof(float) * 54);
}

void GetMorphCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);
	
	MenuCategoryList* menu = GetMenu(&morphsMenuCache);
	if (!menu) return;

	for (auto& kvp : *menu) {
		GFxValue category(kvp.first.c_str());
		result->PushBack(&category);
	}
}

void GetMorphsGFx(GFxMovieRoot* root, GFxValue* result, UInt32 categoryIndex)
{
	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);
	
	GFxValue values;
	root->CreateArray(&values);

	MenuCategoryList* menu = GetMenu(&morphsMenuCache);
	if (!menu || categoryIndex >= menu->size()) return;

	float* ptr = GetMorphPointer();
	if (!ptr) return;

	for (auto& kvp : (*menu)[categoryIndex].second) {
		GFxValue name(kvp.first.c_str());
		names.PushBack(&name);

		UInt32 key = std::stoul(kvp.second);
		key = max(0, min(53, key));

		GFxValue value((SInt32)std::round(ptr[key] * 100));
		values.PushBack(&value);
	}

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}