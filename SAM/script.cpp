//#include "script.h"
//
//#include "f4se/NiTypes.h"
//#include "f4se/GameReferences.h"
//#include "f4se/GameForms.h"
//
//#include "SAF/util.h"
//#include "SAF/conversions.h"
//#include "SAF/adjustments.h"
//#include "constants.h"
//#include "console.h"
//#include "sam.h"
//
//#include <filesystem>
//#include <sol/sol.hpp>
//
//bool ExecuteLuaScript(const char* src) {
//	using namespace sol;
//	state lua;
//
//	std::vector<NiPoint3> vectors{ {} };
//	std::vector<NiMatrix43> matrices{ {} };
//	std::vector<NiTransform> transforms{ {} };
//	std::vector<TESObjectREFR*> refrs{ {} };
//	std::vector<SAF::ActorAdjustmentsPtr> actors{ {} };
//	std::vector<SAF::AdjustmentPtr> adjutments{ {} };
//
//	const auto& TableToMatrix = [](const table& table) -> NiMatrix43 {
//		return {
//			table[1].get<float>(), table[2].get<float>(), table[3].get<float>(),
//			table[4].get<float>(), table[5].get<float>(), table[6].get<float>(),
//			table[7].get<float>(), table[8].get<float>(), table[9].get<float>(),
//		};
//	};
//	const auto& TableToVector = [](const table& table) -> NiPoint3 {
//		return { table[1].get<float>(), table[2].get<float>(), table[3].get<float>() };
//	};
//
//	lua.open_libraries(lib::base, lib::package);
//	auto sam = lua["sam"].get_or_create<table>();
//
//	//Debug
//	sam["print"] = [](const std::string& str) {
//		ConsolePrint(str);
//	};
//
//	//Types
//	sam["vector"] = [&](const table& table) -> uint32_t {
//		vectors.emplace_back(TableToVector(table));
//		return (uint32_t)vectors.size() - 1;
//	};
//	sam["matrix"] = [&](const table& table) -> uint32_t {
//		matrices.emplace_back(TableToMatrix(table));
//		return (uint32_t)matrices.size() - 1;
//	};
//	sam["transform"] = [&](const table& translation, const table& rotation, const float scale) -> uint32_t {
//		transforms.emplace_back(NiTransform{
//			TableToMatrix(rotation),
//			TableToVector(translation),
//			scale
//		});
//		return (uint32_t)transforms.size() - 1;
//	};
//
//	//Math
//	sam["multiplyTransform"] = [&](uint32_t t1, uint32_t t2) -> uint32_t {
//		transforms.emplace_back(SAF::MultiplyNiTransform(transforms.at(t1), transforms.at(t2)));
//		return (uint32_t)transforms.size() - 1;
//	};
//	sam["multiplyMatrix"] = [&](uint32_t m1, uint32_t m2) -> uint32_t {
//		matrices.emplace_back(SAF::MultiplyNiMatrix(matrices.at(m1), matrices.at(m2)));
//		return (uint32_t)matrices.size() - 1;
//	};
//
//	//Refrs
//	sam["getConsoleRefr"] = [&]() -> uint32_t {
//		const auto refr = GetRefr();
//		if (!refr)
//			return 0;
//		refrs.emplace_back(refr);
//		return (uint32_t)refrs.size() - 1;
//	};
//	sam["getFormRefr"] = [&](const uint32_t id) -> uint32_t {
//		const auto form = LookupFormByID(id);
//		if (!form)
//			return 0;
//		refrs.emplace_back(form);
//		return (uint32_t)refrs.size() - 1;
//	};
//
//	//GetSet
//	//sam["getBone"] = [&](const uint32_t refr, const std::string& name) {
//	//	auto adjustments = saf->manager->GetActorAdjustments(refrs.at(refr));
//	//	adjustments->ForEachAdjustment([](SAF::AdjustmentPtr adjustment) {
//
//	//	});
//	//};
//
//	try {
//		lua.script_file(GetPathWithExtension(LUA_PATH, src, ".lua"));
//	}
//	catch (std::exception& e) {
//		ConsolePrint("Exception in lua script {}, {}", src, e.what());
//	}
//	return false;
//}