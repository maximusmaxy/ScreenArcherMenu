#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

void GetGFxValue(GFxMovieRoot* root, GFxValue* result, const Json::Value& value);
Json::Value GetJsonValue(GFxValue* value);

class SavedDataObjVisitor : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	Json::Value& json;

	SavedDataObjVisitor(Json::Value& json) : json(json) {};

	void Visit(const char* member, GFxValue* value);
};

class SavedDataArrVisitor : public GFxValue::ObjectInterface::ArrayVisitor
{
public:
	Json::Value& json;

	SavedDataArrVisitor(Json::Value& json) : json(json) {};

	void Visit(UInt32 idx, GFxValue* val);
};