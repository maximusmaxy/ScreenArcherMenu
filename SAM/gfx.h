#pragma once

#include "f4se/ScaleformAPI.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "json/json.h"

//Needs to be identical to the const values in the Scaleform
enum {
	kGFxResultSuccess = 1,
	kGFxResultError,
	kGfxResultWaiting,
	kGFxResultMenu,
	kGFxResultValues,
	kGFxResultItems,
	kGFxResultNames,
	kGFxResultString,
	kGFxResultFolder,
	kGFxResultFolderCheckbox,
	kGFxResultNotification,
	kGFxResultTitle
};

class GFxResult {
public:
	GFxMovieRoot* root;
	GFxValue* result;
	UInt32 type;
	GFxValue params[2]; //0 = Type, 1 = Result object
	GFxValue itemParams[2]; //0 = names, 1 = values

	GFxResult(GFxValue* value) : root(nullptr), result(value), type(kGFxResultSuccess) {}
	GFxResult(GFxFunctionHandler::Args* args) : root(args->movie->movieRoot), result(args->result), type(kGFxResultSuccess) {}
	~GFxResult();

	GFxValue* GetResult(GFxMovieRoot* root);
	void Finalize(GFxMovieRoot* root);
	void Invoke(GFxMovieRoot* root, const char* functionPath);
	void InvokeCallback(GFxMovieRoot* root);

	void SetError(const char* message);
	void SetWaiting();
	void SetManagedString(GFxMovieRoot* _root, const char* name);
	void SetMenu(Json::Value* menu);
	void SetNotification(GFxMovieRoot* _root, const char* message);
	void SetTitle(GFxMovieRoot* _root, const char* message);

	void CreateNames();
	void CreateValues();
	void CreateMenuItems();
	void CreateObject();
	void CreateFolder();
	void CreateFolderCheckbox();
	
	template <class T>
	void PushValue(T var) {
		if (type == kGFxResultItems) {
			itemParams[1].PushBack(&GFxValue(var));
		}
		else {
			params[1].PushBack(&GFxValue(var));
		}
	}

	template <class T>
	void PushItem(const char* name, T var) {
		itemParams[0].PushBack(&GFxValue(name));
		itemParams[1].PushBack(&GFxValue(var));
	}

	void PushName(const char* name);
	void PushFolder(const char* name, const char* path);
	void PushFile(const char* name, const char* path);
	void PushFileCheckbox(const char* name, const char* path, bool checked);
};