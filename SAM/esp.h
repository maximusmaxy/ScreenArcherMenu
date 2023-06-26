#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

#include "f4se/GameData.h"

#include "zlib.h"

namespace ESP {
	struct RecordHeader {
		UInt32 sig;
		UInt32 size;
		UInt32 flags;
		UInt32 formId;
		UInt32 info1;
		UInt16 version;
		UInt16 info2;
	};

	struct Group {
		UInt32 sig;
		UInt32 size;
		UInt32 value;
		UInt32 depth;
		UInt32 stamp;
		UInt32 unk;
	};

	union Header {
		RecordHeader record;
		Group group;
	};

	class Reader {
		using Masters = std::vector<std::pair<UInt32, UInt32>>;
	private:
		std::ifstream filestream;
		std::stringstream stringstream;
		std::istream* in;
		const ModInfo* modInfo;
		Masters masters;

	public:
		Reader(const ModInfo* info) : modInfo(info) {
			filestream.open(std::filesystem::path("Data").append(info->name), std::ios::in | std::ios::binary);
			in = &filestream;
		}
		bool Fail() {
			return filestream.fail();
		}
		bool Eof() {
			return filestream.eof();
		}

		inline void Skip(UInt32 offset) {
			in->seekg(offset, std::ios::cur);
		}

		template <class T = UInt32>
		Reader& operator>>(T& rhs) {
			in->read(reinterpret_cast<char*>(&rhs), sizeof(T));
			return *this;
		}

		Reader& operator>>(std::string& str) {
			str.clear();
			char c = Get<char>();
			while (c != 0) {
				str.push_back(c);
				c = Get<char>();
			}
			return *this;
		}

		template <class T = UInt32>
		T Get() {
			T result;
			in->read(reinterpret_cast<char*>(&result), sizeof(T));
			return result;
		}

		void ForEachSig(UInt32 size, const std::function<void(UInt32 sig, UInt32 len)>& functor) {
			auto count = 0;
			UInt32 sig;
			UInt16 len;
			while (count < size) {
				*this >> sig >> len;
				count += (6 + len);
				functor(sig, len);
			}
		}

		void ReadHeader() {
			auto PushMaster = [](Masters& masters, const ModInfo* info) {
				if (info != nullptr) {
					if (info->IsLight()) {
						masters.push_back(std::make_pair(0xFE | info->lightIndex << 12, 0xFFF));
					}
					else {
						masters.push_back(std::make_pair(info->modIndex << 24, 0xFFFFFF));
					}
				}
				else {
					masters.push_back(std::make_pair(0xFF000000, 0xFFFFFF));
				}
			};

			RecordHeader header;
			*this >> header;

			std::string masterName;
			ForEachSig(header.size, [&](auto sig, auto len) {
				switch (sig) {
				case 'TSAM':
				{
					auto count = 0;
					while (count < len) {
						*this >> masterName;
						count += (masterName.size() + 1);
						auto info = (*g_dataHandler)->LookupModByName(masterName.c_str());
						PushMaster(masters, info);
					}
					break;
				}
				default:
					Skip(len);
				}
			});

			PushMaster(masters, modInfo);
		}

		inline UInt32 GetFormId(UInt32 formId) {
			const auto [modIndex, mask] = masters.at(formId >> 24);
			if (modIndex == 0xFF000000)
				return 0;
			return modIndex | (formId & mask);
		}

		inline UInt32 ReadFormId() {
			return GetFormId(Get());
		}

		inline void SkipGroup(UInt32 len) {
			Skip(len - 0x18);
		}

		bool SeekToGroup(UInt32 sig) {
			Group group;
			while (!Eof()) {
				*this >> group;
				if (group.value != sig) {
					SkipGroup(group.size);
				}
				else {
					return true;
				}
			}
			return false;
		}

		void Inflate(UInt32 srcLen) {
			auto dstLen = Get();
			std::string srcbuffer;
			srcbuffer.resize(srcLen);
			filestream.read(srcbuffer.data(), srcLen - 4);

			std::string dstbuffer;
			dstbuffer.resize(dstLen);
			uncompress((UInt8*)dstbuffer.data(), &dstLen, (UInt8*)srcbuffer.data(), srcLen);

			stringstream = std::stringstream(dstbuffer);
			in = &stringstream;
		}

		void EndInflate() {
			in = &filestream;
		}
	};
}