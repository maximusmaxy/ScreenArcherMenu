#pragma once

#include "sam.h"
#include "gfx.h"

#include <vector>

typedef std::function<void(GFxFunctionHandler::Args*)> GFxFunctionFunctor;
typedef std::function<void(GFxResult&, GFxFunctionHandler::Args*)> GFxRequestFunctor;

class GFxCallable {
public:
	virtual void Call(GFxFunctionHandler::Args* args) {}
};

class GFxFunction : public GFxCallable {
public:
	GFxFunction(GFxFunctionFunctor functor) : functor(functor) {}
	GFxFunctionFunctor& functor;
	void Call(GFxFunctionHandler::Args* args) override {
		functor(args);
	}
};

class GFxRequest : public GFxCallable {
public:
	GFxRequest(GFxRequestFunctor fuctor) : functor(functor) {}
	GFxRequestFunctor& functor;
	void Call(GFxFunctionHandler::Args* args) override {
		GFxResult result(args);
		functor(result, args);
	}
};

struct GFxFunctions {
	std::vector<std::string> names;
	std::vector<GFxCallable> funcs;
};

extern GFxFunctions samFunctions;

//Binds a standard function to the menu
class GFxFunc {
public:
	GFxFunc(const std::string& name, const GFxFunctionFunctor& func) {
		samFunctions.names.push_back(name);
		samFunctions.funcs.emplace_back(GFxFunction(func));
	}
};

//Binds a callback function to the menu
class GFxReq {
public:
	GFxReq(const std::string& name, const GFxRequestFunctor func) {
		samFunctions.names.push_back(name);
		samFunctions.funcs.emplace_back(GFxRequest(func));
	}
};