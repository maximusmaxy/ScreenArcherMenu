#include "gfx.h"

#include "data.h"
#include "saf/util.h"

//if root is managed, automatically finalize result value on destruction
GFxResult::~GFxResult() {
	if (root)
		Finalize(root);
}

//Initialize the GFxResult return value
void GFxResult::Finalize(GFxMovieRoot* _root)
{
	params[0].SetUInt(type);
	if (type == Items) {
		_root->CreateObject(&params[1], "GFxItems", itemParams, 2);
	}
	_root->CreateObject(result, "GFxResult", params, 2);
}

//Prevent the finalize by setting root to null
void GFxResult::Finalized()
{
	root = nullptr;
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
	_root->Invoke(functionPath, nullptr, result, 1);
}

//Finalize and call the latent callback function
void GFxResult::InvokeCallback()
{
	Finalize(root);
	root->Invoke("root1.Menu_mc.LatentCallback", nullptr, result, 1);
	root = nullptr; //prevents finalize
}

void GFxResult::CreateNames() {
	type = Names;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateValues() {
	type = Values;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateMenuItems() {
	type = Items;
	root->CreateArray(&itemParams[0]);
	root->CreateArray(&itemParams[1]);
}

void GFxResult::CreateFolder() {
	type = Folder;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateFolderCheckbox() {
	type = FolderCheckbox;
	root->CreateArray(&params[1]);
}

void GFxResult::SetError(const char* message) {
	type = Error;
	params[1].SetString(message);
}

void GFxResult::SetError(const std::string& str) {
	type = Error;
	root->CreateString(&params[1], str.c_str());
}

void GFxResult::SetWaiting() {
	type = Waiting;
	params[1].SetBool(true);
}

void GFxResult::SetMenu(Json::Value* json) {
	type = Menu;
	JsonToGFx(root, &params[1], *json);
}

void GFxResult::SetNotification(GFxMovieRoot* _root, const char* name) {
	type = Notification;
	_root->CreateString(&params[1], name);
}

void GFxResult::SetTitle(GFxMovieRoot* _root, const char* name) {
	type = Title;
	_root->CreateString(&params[1], name);
}

void GFxResult::SetString(const char* str) {
	type = String;
	params[1].SetString(str);
}

void GFxResult::SetManagedString(GFxMovieRoot* _root, const char* name) {
	type = String;
	_root->CreateString(&params[1], name);
}

void GFxResult::PushName(const char* name) {
	GFxValue nameValue(name);
	if (type == Items) {
		
		itemParams[0].PushBack(&nameValue);
	}
	else {
		params[1].PushBack(&nameValue);
	}
}

void GFxResult::PushFolder(const char* name, const char* path)
{
	GFxValue folder;
	root->CreateObject(&folder);

	GFxValue nameValue(name);
	GFxValue pathValue(path);
	GFxValue folderValue(true);
	folder.SetMember("name", &nameValue);
	folder.SetMember("path", &pathValue);
	folder.SetMember("folder", &folderValue);

	params[1].PushBack(&folder);
}

void GFxResult::PushFile(const char* name, const char* path)
{
	GFxValue file;
	root->CreateObject(&file);

	GFxValue nameValue(name);
	GFxValue pathValue(path);
	GFxValue folderValue(false);
	file.SetMember("name", &nameValue);
	file.SetMember("path", &pathValue);
	file.SetMember("folder", &folderValue);
	
	params[1].PushBack(&file);
}

void GFxResult::PushFileCheckbox(const char* name, const char* path, bool checked) 
{
	GFxValue file;
	root->CreateObject(&file);

	GFxValue nameValue(name);
	GFxValue pathValue(path);
	GFxValue folderValue(false);
	GFxValue checkedValue(checked);
	file.SetMember("name", &nameValue);
	file.SetMember("path", &pathValue);
	file.SetMember("folder", &folderValue);
	file.SetMember("checked", &checkedValue);

	params[1].PushBack(&file);
}