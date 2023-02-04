#include "gfx.h"

#include "data.h"

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
	type = kGFxResultNames;
	root->CreateArray(&params[1]);
}

void GFxResult::CreateValues() {
	type = kGFxResultValues;
	root->CreateArray(&params[1]);
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

void GFxResult::SetNotification(GFxMovieRoot* _root, const char* name) {
	type = kGFxResultNotification;
	_root->CreateString(&params[1], name);
}

void GFxResult::SetTitle(GFxMovieRoot* _root, const char* name) {
	type = kGFxResultTitle;
	_root->CreateString(&params[1], name);
}

void GFxResult::SetManagedString(GFxMovieRoot* _root, const char* name) {
	type = kGFxResultString;
	_root->CreateString(&params[1], name);
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