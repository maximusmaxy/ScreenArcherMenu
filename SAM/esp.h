#pragma once

#include <fstream>
#include <sstream>
#include <filesystem>

#include "f4se/GameData.h"

#include "miniz/miniz.h"

namespace ESP {
	//constexpr UInt32 Sig(const char* val) {
	//	UInt32 result = 0;
	//	result |= (UInt8)val[3];
	//	result |= (UInt8)val[2] << 8;
	//	result |= (UInt8)val[1] << 16;
	//	result |= (UInt8)val[0] << 24;
	//	return result;
	//}

	constexpr UInt32 Sig(const char* val) {
		UInt32 result = 0;
		result |= (UInt8)val[0];
		result |= (UInt8)val[1] << 8;
		result |= (UInt8)val[2] << 16;
		result |= (UInt8)val[3] << 24;
		return result;
	}

	struct Record {
		UInt32 sig;
		UInt32 size;
		UInt32 flags;
		UInt32 formId;
		UInt32 info1;
		UInt16 version;
		UInt16 info2;

		inline bool IsCompressed() {
			return flags & (1 << 18);
		}
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
		Record record;
		Group group;
	};

	struct Element {
		UInt32 sig;
		UInt16 len;
	};

	class Reader {
		using Masters = std::vector<std::pair<UInt32, UInt32>>;
	private:
		std::ifstream filestream;
		std::istringstream stringstream;
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
			return filestream.rdstate() & (std::ios::badbit | std::ios::failbit | std::ios::eofbit);
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

		Reader& operator>>(Element& rhs) {
			*this >> rhs.sig >> rhs.len;
			return *this;
		}

		template <class T = UInt32>
		T Get() {
			T result;
			in->read(reinterpret_cast<char*>(&result), sizeof(T));
			return result;
		}

		void ForEachSig(UInt32 size, const std::function<void(Element& element)>& functor) {
			auto count = 0;
			Element element;
			while (count < size) {
				*this >> element;
				count += (6 + element.len);
				functor(element);
			}
		}

		void ForEachRecord(Group& group, const std::function<void(Record& record)>& functor) {
			Record record;
			auto count = 0;
			const auto size = group.size - 0x18;
			std::string buffer;
			while (count < size) {
				*this >> record;
				count += (0x18 + record.size);

				bool success = true;
				bool compressed = record.IsCompressed();
				if (compressed) {
					auto dstLen = Get();
					success = Inflate(record.size - 4, dstLen, buffer);
					record.size = dstLen;
				}

				if (success) {
					functor(record);

					if (compressed)
						EndInflate();
				}	
			}
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

		bool SeekToGroup(UInt32 sig, Group& group) {
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

		//Returns the remaining length of the record after current element
		UInt32 SeekToElement(UInt32 len, UInt32 sig) {
			UInt32 count = 0;
			Element element;
			while (count < len) {
				*this >> element;
				count += 6;
				if (element.sig == sig)
					return len - count;
				count += element.len;
				Skip(element.len);
			}
			return 0;
		}

		UInt32 SeekToElement(UInt32 len, UInt32 sig, Element& element) {
			UInt32 count = 0;
			while (count < len) {
				*this >> element;
				count += 6;
				if (element.sig == sig)
					return len - count;
				count += element.len;
				Skip(element.len);
			}
			return 0;
		}

		UInt32 NextElement(UInt32 len, Element& element) {
			if (!len) {
				element = { 0, 0 };
				return len;
			}
			*this >> element;
			return (len - 6);
		}

		void ReadHeader() {
			const auto& PushMaster = [](Masters& masters, const ModInfo* info) {
				if (info != nullptr) {
					if (info->IsLight()) {
						masters.push_back(std::make_pair(0xFE000000 | (info->lightIndex << 12), 0xFFF));
					}
					else {
						masters.push_back(std::make_pair(info->modIndex << 24, 0xFFFFFF));
					}
				}
				else {
					masters.push_back(std::make_pair(0xFF000000, 0xFFFFFF));
				}
			};

			Record header;
			*this >> header;

			std::string masterName;
			Element element;
			auto remaining = SeekToElement(header.size, Sig("MAST"), element);
			if (remaining > 0) {
				while (element.sig == Sig("MAST")) {
					remaining -= element.len;
					*this >> masterName;
					auto info = (*g_dataHandler)->LookupModByName(masterName.c_str());
					PushMaster(masters, info);

					//Data
					remaining = NextElement(remaining, element);
					remaining -= element.len;
					Skip(element.len);

					remaining = NextElement(remaining, element);
				}
				Skip(remaining);
			}
			PushMaster(masters, modInfo);
		}

		inline UInt32 GetFormId(UInt32 formId) {
			const auto index = formId >> 24;
			if (index >= masters.size())
				return 0;
			const auto [modIndex, mask] = masters.at(index);
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

		//For single form access only
		bool SeekToFormId(UInt32 formId, UInt32 signature, Record& record) {
			Group group;
			if (!SeekToGroup(signature, group))
				return false;

			auto count = 0;
			const auto size = group.size - 0x18;
			while (count < size) {
				*this >> record;
				count += (0x18 + record.size);
				if (record.formId == formId) {
					if (record.IsCompressed()) {
						std::string buffer;
						auto dstLen = Get();
						bool success = Inflate(record.size - 4, dstLen, buffer);
						record.size = dstLen;
					}
					return true;
				}
			}

			return false;
		}

		//This is not efficient in the slightest. 
		bool Inflate(UInt32 srcLen, UInt32 dstLen, std::string& srcBuffer) {
			srcBuffer.resize(srcLen);
			filestream.read(srcBuffer.data(), srcLen);

			std::string dstBuffer;
			dstBuffer.resize(dstLen);
			if (uncompress((UInt8*)dstBuffer.data(), &dstLen, (UInt8*)srcBuffer.data(), srcLen) != 0)
				return false;

			stringstream = std::istringstream(std::move(dstBuffer));
			in = &stringstream;

			return true;
		}

		void EndInflate() {
			stringstream.clear();
			in = &filestream;
		}
	};
}