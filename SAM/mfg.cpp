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

void SetFaceMorph(UInt32 id, UInt32 scale) {
	float* ptr = GetMorphPointer();
	if (!ptr) return;

	ptr[id] = (float)scale * 0.0099999998;
}

SInt32 GetFaceMorph(UInt32 id) {
	float* ptr = GetMorphPointer();
	if (!ptr) return -1;

	float scale = ptr[id] * 100;
	return (SInt32)(scale + 0.5);
}

bool GetMorphArray(SInt32* morphs) {
	float* ptr = GetMorphPointer();
	if (!ptr) return false;

	for (int i = 0; i < 50; ++i) {
		UInt32 scale = (UInt32)((ptr[i] * 100) + 0.5);
		scale = max(0, min(100, scale));
		morphs[i] = scale;
	}
	return true;
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
	for (int i = 0; i < 50; ++i) {
		UInt32 scale = (UInt32)((ptr[i] * 100) + 0.5);
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

bool LoadMfg(std::string filename, SInt32* morphs) {
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

	while (!file.HitEOF()) {
		file.ReadString(buf, 512, '\n', '\r');
		if (std::regex_match(buf, match, mfgRegex)) {
			int id = max(0, min(49, std::stoi(match[1].str())));
			int scale = max(0, min(100, std::stoi(match[2].str())));
			morphs[id] = scale;
		}
	}

	file.Close();

	for (int i = 0; i < 50; ++i) {
		ptr[i] = (float)morphs[i] * 0.0099999998;
	}

	return true;
}

void ResetMfg() {
	float* ptr = GetMorphPointer();
	if (!ptr) return;

	memset(ptr, 0, sizeof(float) * 50);
}

bool GetGFxMorphArray(GFxMovieRoot* root, GFxValue* morphArray)
{
	SInt32 morphs[50];
	if (GetMorphArray(morphs)) {
		for (SInt32* p = morphs; p < morphs + 50; ++p) {
			morphArray->PushBack(&GFxValue(*p));
		}
		return true;
	}
	return false;
}