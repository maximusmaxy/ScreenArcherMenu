#pragma once

#include <fstream>
#include <filesystem>

struct EspGroup{
	UInt32 sig;
	UInt32 size;
	UInt32 value;
	UInt32 depth;
	UInt32 stamp;
	UInt32 unk;
};

struct EspHeader{
	UInt32 sig;
};

class EspReader{
private:
	std::ifstream stream;

public:
	EspReader(const std::string& path) { 
		stream.open(std::filesystem::path("Data").append(path), std::ios::in | std::ios::binary);
	}
	bool Fail() {
		return stream.fail();
	}
	bool Eof() {
		return stream.eof();
	}

	inline void Skip(UInt32 offset) {
		stream.seekg(offset, std::ios::cur);
	}

	template <class T = UInt32>
	void operator>>(T& rhs) {
		stream.read(reinterpret_cast<char*>(&rhs), sizeof(T));
	}

	void operator>>(std::string& str) {
		auto len = Get<UInt16>();
		str.resize(len);
		stream.read(str.data(), len);
	}

	template <class T = UInt32>
	T Get() {
		T result;
		stream.read(reinterpret_cast<char*>(&result), sizeof(T));
		return result;
	}

	void SkipRecord() {
		Skip(4);
		auto len = Get();
		Skip(0x10 + len);
	}

	void SkipGroup() {
		Skip(4);
		auto len = Get();
		Skip(len - 0x8);
	}

	bool SeekToGroup(UInt32 sig) {
		while (!stream.eof()) {
			Skip(4);
			auto len = Get();
			auto groupSig = Get();
			if (sig != groupSig) {
				Skip(len - 0xC);
			}
			else {
				Skip(0xC);
				return true;
			}
		}
		return false;
	}
};