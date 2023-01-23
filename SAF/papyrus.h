#pragma once

#include "f4se/PapyrusVM.h"

namespace SAF {
	bool RegisterPapyrus(VirtualMachine* vm);
	void RevertPapyrus();
}
