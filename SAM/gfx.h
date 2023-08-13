#pragma once

#include "f4se/ScaleformAPI.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include <json/json.h>

class GFxResult {
public:
	//Needs to be identical to the const values in the Scaleform
	enum Result{
		Success = 1,
		Error,
		Waiting,
		Menu,
		Values,
		Items,
		Names,
		String,
		Folder,
		FolderCheckbox,
		Notification,
		Title
	};

	GFxMovieRoot* root;
	GFxValue* result;
	UInt32 type;
	GFxValue params[2]; //0 = Type, 1 = Result object
	GFxValue itemParams[2]; //0 = names, 1 = values

	GFxResult(GFxValue* value) : root(nullptr), result(value), type(Success) {}
	GFxResult(GFxMovieRoot* root, GFxValue* value) : root(root), result(value), type(Success) {}
	GFxResult(GFxFunctionHandler::Args* args) : root(args->movie->movieRoot), result(args->result), type(Success) {}
	~GFxResult();

	GFxValue* GetResult(GFxMovieRoot* root);
	void Finalize(GFxMovieRoot* root);
	void Finalized();
	void Invoke(GFxMovieRoot* root, const char* functionPath);
	void InvokeCallback();

	void SetError(const char* message);
	void SetError(const std::string& str);
	void SetWaiting();
	void SetString(const char* name);
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
		GFxValue varValue(var);
		if (type == Items) {
			itemParams[1].PushBack(&varValue);
		}
		else {
			params[1].PushBack(&varValue);
		}
	}

	template <class T>
	void PushItem(const char* name, T var) {
		GFxValue nameValue(name);
		GFxValue varValue(var);
		itemParams[0].PushBack(&nameValue);
		itemParams[1].PushBack(&varValue);
	}

	void PushName(const char* name);
	void PushFolder(const char* name, const char* path);
	void PushFile(const char* name, const char* path);
	void PushFileCheckbox(const char* name, const char* path, bool checked);
};