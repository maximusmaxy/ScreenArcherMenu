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
	kGFxResultBool,
	kGFxResultInt,
	kGFxResultFloat,
	kGFxResultObject,
	kGFxResultFolder,
	kGFxResultFolderCheckbox
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

	void SetError(const char* message);
	void SetWaiting();
	void SetString(const char* name);
	void SetManagedString(GFxMovieRoot* _root, const char* name);
	void SetBool(bool checked);
	void SetInt(SInt32 num);
	void SetFloat(double num);
	void SetMenu(Json::Value* menu);

	void CreateNames();
	void CreateValues();
	void CreateMenuItems();
	void CreateObject();
	void CreateFolder();
	void CreateFolderCheckbox();
	
	void PushName(const char* name);
	void PushValue(GFxValue* var);
	void PushItem(const char* name, GFxValue* var);
	void PushFolder(const char* name, const char* path);
	void PushFile(const char* name, const char* path);
	void PushFileCheckbox(const char* name, const char* path, bool checked);
};

void JsonToGFx(GFxMovieRoot* root, GFxValue* result, const Json::Value& value);
Json::Value GFxToJson(GFxValue* value);

class GFxToJsonObjVisitor : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	Json::Value& json;

	GFxToJsonObjVisitor(Json::Value& json) : json(json) {};

	void Visit(const char* member, GFxValue* value);
};

class GFxToJsonArrVisitor : public GFxValue::ObjectInterface::ArrayVisitor
{
public:
	Json::Value& json;

	GFxToJsonArrVisitor(Json::Value& json) : json(json) {};

	void Visit(UInt32 idx, GFxValue* val);
};