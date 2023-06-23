#pragma once

template <class T, class U>
void _Log(T str1, U str2)
{
	std::stringstream ss;
	ss << str1;
	ss << str2;

	_DMESSAGE(ss.str().c_str());
}

UInt32 GetFormId(const char* modName, UInt32 formId);
UInt32 GetFormId(const char* modName, const char* idString);
UInt32 GetModId(UInt32 formId);
UInt32 GetBaseId(UInt32 formId);
const char* GetModName(UInt32 formId);

float Modulo(float a, float b);

UInt32 HexStringToUInt32(const char* str);
UInt32 StringToUInt32(const char* str);
SInt32 StringToSInt32(const char* str);
std::string UInt32ToHexString(UInt32 hex);
UInt32 ComparePostfix(const char* comparer, UInt32 cLength, const char* postfix, UInt32 pLength);
std::string GetPathWithExtension(const char* folder, const char* path, const char* ext);

constexpr size_t constStrLen(const char* str) {
	return std::char_traits<char>::length(str);
}

std::string GetRelativePath(int rootLen, int extLen, const char* path);

void GetLoweredCString(char* buffer, const char* str);

//case insensitive strstr, substr has been premptively tolowered
template <class T>
inline bool HasInsensitiveSubstring(T str, const char* substr) {
	const char* c = substr;
	while (*str != 0) {
		if (tolower(*str) == *c) {
			c++;
			if (*c == 0)
				return true;
		}
		else
			c = substr;
		str++;
	}
	return false;
}

inline void StringToWide(const std::string& str, std::wstring& result) {
	result.resize(str.size());
	for (int i = 0; i < str.size(); ++i) {
		result[i] = (wchar_t)str[i];
	}
}

inline void CStringToWide(const char* str, std::wstring& result) {
	auto len = strlen(str);
	result.resize(len);
	for (int i = 0; i < len; ++i) {
		result[i] = (wchar_t)str[i];
	}
}

inline void WideToString(const std::wstring& str, std::string& result) {
	result.resize(str.size());
	for (int i = 0; i < str.size(); ++i) {
		result[i] = (char)str[i];
	}
}

inline void WStringToString(const wchar_t* str, std::string& result) {
	auto len = wcslen(str);
	result.resize(len);
	for (int i = 0; i < len; ++i) {
		result[i] = (char)str[i];
	}
}