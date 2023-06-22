#pragma once

#include "gfx.h"

#include <vector>

typedef void(*GFxFunctionFunctor)(GFxFunctionHandler::Args*);
typedef void(*GFxRequestFunctor)(GFxResult&, GFxFunctionHandler::Args*);

class GFxCallable {
public:
	virtual void Call(GFxFunctionHandler::Args* args) {}
};

class GFxFunction : public GFxCallable {
public:
	GFxFunction(GFxFunctionFunctor _functor) : functor(_functor) {}
	GFxFunctionFunctor functor;
	void Call(GFxFunctionHandler::Args* args) override {
		functor(args);
	}
};

class GFxRequest : public GFxCallable {
public:
	GFxRequest(GFxRequestFunctor _functor) : functor(_functor) {}
	GFxRequestFunctor functor;
	void Call(GFxFunctionHandler::Args* args) override {
		GFxResult result(args);
		functor(result, args);
	}
};

struct GFxFunctions {
	std::vector<std::string> names;
	std::vector<GFxCallable*> funcs;
};
extern GFxFunctions samFunctions;

class GFxFunctionBinding {
public:
	constexpr GFxFunctionBinding(const std::string& name, const GFxFunctionFunctor& func) {
		samFunctions.names.push_back(name);
		samFunctions.funcs.push_back(new GFxFunction(func));
	}
};

class GFxRequestBinding {
public:
	GFxRequestBinding(const std::string& name, const GFxRequestFunctor& func) {
		samFunctions.names.push_back(name);
		samFunctions.funcs.push_back(new GFxRequest(func));
	}
};

//Binds a standard function to the menu
typedef const GFxFunctionBinding GFxFunc;
//Binds a callback function to the menu
typedef const GFxRequestBinding GFxReq;