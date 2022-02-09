#pragma once

#include "f4se/ScaleformAPI.h"
#include "f4se/ScaleformCallbacks.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

bool RegisterScaleform(GFxMovieView* view, GFxValue* value);

class ModifyFacegenMorph : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetMorphCategories : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetMorphs : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args);
};

class SavePreset : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class LoadPreset : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class ResetMorphs : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SamPlaySound : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SamOpenMenu : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SamCloseMenu : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SamIsMenuOpen : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetHacks : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetBlinkHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetBlinkHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetMorphHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetMorphHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetEyeTrackingHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetEyeTrackingHack : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetEyeCoords : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetEyeCoords : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetCategoryList : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetNodeList : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetNodeTransform : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetNodePosition : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetNodeRotation : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SetNodeScale : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class ResetTransform : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class GetAdjustmentList : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class SaveAdjustment : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class LoadAdjustment : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class NewAdjustment : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args);
};

class RemoveAdjustment : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args);
};

class HideMenu : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args * args);
};

class Test : public GFxFunctionHandler
{
public:
	virtual void	Invoke(Args* args);
};