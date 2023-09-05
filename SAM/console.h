#pragma once

#include "f4se/GameAPI.h"

#include <format>

void samObScriptInit();
void samObScriptCommit();

//wraps std::format to print to the console
template<typename... Args>
void ConsolePrint(std::format_string<Args...> fmt, Args&&... args) {
	ConsoleManager* mgr = *g_console;
	if (mgr) {
		CALL_MEMBER_FN(mgr, Print)(std::format(fmt, std::forward<Args>(args)...).c_str());
	}
}