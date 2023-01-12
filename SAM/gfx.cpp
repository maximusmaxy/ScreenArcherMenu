#include "gfx.h"

void GetGFxValue(GFxMovieRoot* root, GFxValue* result, const Json::Value& value) {
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
			GetGFxValue(root, &arrValue, member);
			result->PushBack(&arrValue);
		}

		break;
	}
	case Json::ValueType::objectValue:
	{
		root->CreateObject(result);

		for (auto it = value.begin(); it != value.end(); ++it) {
			GFxValue objValue;
			GetGFxValue(root, &objValue, *it);
			result->SetMember(it.key().asCString(), &objValue);
		}

		break;
	}
	}
}

Json::Value GetJsonValue(GFxValue* value)
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
		SavedDataArrVisitor arrVisitor(arr);
		value->VisitElements(&arrVisitor, 0, value->GetArraySize());
		return arr;
	}
	case GFxValue::kType_Object:
	{
		Json::Value obj(Json::ValueType::objectValue);
		SavedDataObjVisitor visitor(obj);
		value->VisitMembers(&visitor);
		return obj;
	}
	default:
		return Json::Value(Json::ValueType::nullValue);
	}
}

void SavedDataObjVisitor::Visit(const char* member, GFxValue* value) {
	json[member] = GetJsonValue(value);
}

void SavedDataArrVisitor::Visit(UInt32 idx, GFxValue* value) {
	json[(int)idx] = GetJsonValue(value);
}