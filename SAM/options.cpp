#include "options.h"

#include "sam.h"

#include "SAF/io.h"

#include "common/IFileStream.h"

Json::Value menuOptions;
bool menuOptionsUpdated = false;
__time64_t optionsTimestamp = 0;

const char* menuOptionNames[] = {
	"hotswap",
	"alignment",
	"widescreen"
};

bool menuOptionDefaults[] = {
	true,
	false,
	false
};

void GetMenuOptionsGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	for (int i = 0; i < kOptionMax; ++i) {
		result->PushBack(&GFxValue(GetMenuOption(i)));
	}
}

bool GetMenuOption(int index) {
	if (index >= 0 && index < kOptionMax) {
		const char* key = menuOptionNames[index];
		if (menuOptions[key].isNull()) {
			menuOptions[key] = Json::Value(menuOptionDefaults[index]);
		}
		return (menuOptions[key].asBool());
	}
	return false;
}

void SetMenuOption(int index, bool value) {
	if (index >= 0 && index < kOptionMax) {
		const char* key = menuOptionNames[index];
		if (menuOptions[key] != value) {
			menuOptions[key] = Json::Value(value);
			menuOptionsUpdated = true;
		}
	}
}

void LoadOptionsFile() {
	menuOptionsUpdated = false;

	//check if file was updated
	struct _stat fileInfo;
	_stat(OPTIONS_PATH, &fileInfo);
	if (optionsTimestamp == fileInfo.st_mtime)
		return;

	IFileStream file;

	if (file.Open(OPTIONS_PATH)) {
		std::string jsonString;
		SAF::ReadAll(&file, &jsonString);
		file.Close();

		Json::Reader reader;

		if (!reader.parse(jsonString, menuOptions)) {
			menuOptions.clear();
		}
	}
	else {
		menuOptions.clear();
	}

	//validate and set defaults
	for (int i = 0; i < kOptionMax; ++i) {
		const char* key = menuOptionNames[i];
		if (menuOptions[key].isNull())
			menuOptions[key] = Json::Value(menuOptionDefaults[i]);
	}
}

void SaveOptionsFile() {
	if (!menuOptionsUpdated)
		return;

	Json::StyledWriter writer;

	IFileStream file;

	IFileStream::MakeAllDirs(OPTIONS_PATH);
	if (!file.Create(OPTIONS_PATH)) {
		_DMESSAGE("Failed to write options json");
		return;
	}

	auto jsonString = writer.write(menuOptions);
	file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
	file.Close();

	//update timestamp
	struct _stat fileInfo;
	_stat(OPTIONS_PATH, &fileInfo);
	optionsTimestamp = fileInfo.st_mtime;
}