#include "gfx.h"

//if root is managed, automatically finalize result value on destruction
GFxResult::~GFxResult() {
	if (root)
		Finalize(root);
}

//Initialize the GFxResult return value
void GFxResult::Finalize(GFxMovieRoot* _root)
{
	params[0].SetUInt(type);
	if (type == kGFxResultItems) {
		_root->CreateObject(&params[1], "GFxItems", itemParams, 2);
	}
	_root->CreateObject(result, "GFxResult", params, 2);
}

//Finalize and return ptr to value
GFxValue* GFxResult::GetResult(GFxMovieRoot* _root)
{
	Finalize(_root);
	return result;
}

//Finalize and invoke the specificed function
void GFxResult::Invoke(GFxMovieRoot* _root, const char* functionPath)
{
	Finalize(_root);
	root->Invoke(functionPath, nullptr, result, 1);
}

void GFxResult::CreateNames() {
	type = kGFxResultNames;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateValues() {
	type = kGFxResultValues;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateObject() {
	type = kGFxResultObject;
	root->CreateObject(&params[1]);
}

void GFxResult::CreateMenuItems() {
	type = kGFxResultItems;
	root->CreateArray(&itemParams[0]);
	root->CreateArray(&itemParams[1]);
}

void GFxResult::CreateFolder() {
	type = kGFxResultFolder;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateFolderCheckbox() {
	type = kGFxResultFolderCheckbox;
	root->CreateArray(&params[1]);
}

void GFxResult::SetError(const char* message) {
	type = kGFxResultError;
	params[1].SetString(message);
}

void GFxResult::SetWaiting() {
	type = kGfxResultWaiting;
	params[1].SetBool(true);
}

void GFxResult::SetMenu(Json::Value* json) {
	type = kGFxResultMenu;
	JsonToGFx(root, &params[1], *json);
}

void GFxResult::SetString(const char* name) {
	type = kGFxResultString;
	params[1].SetString(name);
}

void GFxResult::SetManagedString(GFxMovieRoot* _root, const char* name) {
	type = kGFxResultString;
	_root->CreateString(&params[1], name);
}

void GFxResult::SetBool(bool checked) {
	type = kGFxResultBool;
	params[1].SetBool(checked);
}

void GFxResult::SetInt(SInt32 num) {
	type = kGFxResultInt;
	params[1].SetInt(num);
}

void GFxResult::SetFloat(double num) {
	type = kGFxResultFloat;
	params[1].SetNumber(num);
}

void GFxResult::PushName(const char* name) {
	if (type == kGFxResultItems) {
		itemParams[0].PushBack(&GFxValue(name));
	}
	else {
		params[1].PushBack(&GFxValue(name));
	}
}

void GFxResult::PushFolder(const char* name, const char* path)
{
	GFxValue folder;
	root->CreateObject(&folder);

	folder.SetMember("name", &GFxValue(name));
	folder.SetMember("path", &GFxValue(path));
	folder.SetMember("folder", &GFxValue(true));

	params[1].PushBack(&folder);
}

void GFxResult::PushFile(const char* name, const char* path)
{
	GFxValue file;
	root->CreateObject(&file);

	file.SetMember("name", &GFxValue(name));
	file.SetMember("path", &GFxValue(path));
	file.SetMember("folder", &GFxValue(false));
	
	params[1].PushBack(&file);
}

void GFxResult::PushFileCheckbox(const char* name, const char* path, bool checked) 
{
	GFxValue file;
	root->CreateObject(&file);

	file.SetMember("name", &GFxValue(name));
	file.SetMember("path", &GFxValue(path));
	file.SetMember("folder", &GFxValue(false));
	file.SetMember("checked", &GFxValue(checked));

	params[1].PushBack(&file);
}

void JsonToGFx(GFxMovieRoot* root, GFxValue* result, const Json::Value& value) {
	switch (value.type()) {
	case Json::ValueType::booleanValue:
		result->SetBool(value.asBool());
		break;
	case Json::ValueType::intValue:
		result->SetInt((SInt32)value.asInt());
		break;
	case Json::ValueType::uintValue:
		result->SetUInt((UInt32)value.asUInt());
		break;
	case Json::ValueType::realValue:
		result->SetNumber(value.asDouble());
		break;
	case Json::ValueType::stringValue:
		result->SetString(value.asCString());
		break;
	case Json::ValueType::arrayValue:
	{
		root->CreateArray(result);

		for (auto& member : value) {
			GFxValue arrValue;
			JsonToGFx(root, &arrValue, member);
			result->PushBack(&arrValue);
		}

		break;
	}
	case Json::ValueType::objectValue:
	{
		root->CreateObject(result);

		for (auto it = value.begin(); it != value.end(); ++it) {
			GFxValue objValue;
			JsonToGFx(root, &objValue, *it);
			result->SetMember(it.key().asCString(), &objValue);
		}

		break;
	}
	}
}

Json::Value GFxToJson(GFxValue* value)
{
	switch (value->GetType()) {
	case GFxValue::kType_Bool:
		return Json::Value(value->GetBool());
	case GFxValue::kType_Int:
		return Json::Value(value->GetInt());
	case GFxValue::kType_UInt:
	{
		UInt64 uint = value->GetUInt();
		return Json::Value(uint);
	}
	case GFxValue::kType_Number:
		return Json::Value(value->GetNumber());
	case GFxValue::kType_String:
		return Json::Value(value->GetString());
	case GFxValue::kType_Array:
	{
		Json::Value arr(Json::ValueType::arrayValue);
		GFxToJsonArrVisitor arrVisitor(arr);
		value->VisitElements(&arrVisitor, 0, value->GetArraySize());
		return arr;
	}
	case GFxValue::kType_Object:
	{
		Json::Value obj(Json::ValueType::objectValue);
		GFxToJsonObjVisitor visitor(obj);
		value->VisitMembers(&visitor);
		return obj;
	}
	default:
		return Json::Value(Json::ValueType::nullValue);
	}
}

void GFxToJsonObjVisitor::Visit(const char* member, GFxValue* value) {
	json[member] = GFxToJson(value);
}

void GFxToJsonArrVisitor::Visit(UInt32 idx, GFxValue* value) {
	json[(int)idx] = GFxToJson(value);
}