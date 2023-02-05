#pragma once

#define SAM_ESP "ScreenArcherMenu.esp"

#define SAM_MENU_NAME "ScreenArcherMenu"
#define CURSOR_MENU_NAME "CursorMenu"
#define CONSOLE_MENU_NAME "Console"
#define PHOTO_MENU_NAME "PhotoMenu"
#define CONTAINER_MENU_NAME "ContainerMenu"

#define MAIN_MENU_NAME "Main"

#define MENUS_PATH "Data\\F4SE\\Plugins\\SAM\\Menus"
#define IDLES_PATH "Data\\F4SE\\Plugins\\SAM\\Idles"
#define OVERRIDE_PATH "Data\\F4SE\\Plugins\\SAM\\Override"
#define MENUDATA_PATH "Data\\F4SE\\Plugins\\SAM\\Data"
#define EXTENSIONS_PATH "Data\\F4SE\\Plugins\\SAM\\Extensions"
#define FACEMORPHS_PATH "Data\\F4SE\\Plugins\\SAM\\FaceMorphs"
#define CAMERA_PATH "Data\\F4SE\\Plugins\\SAM\\Camera"
#define LIGHTS_PATH "Data\\F4SE\\Plugins\\SAM\\Lights"
#define OPTIONS_PATH "Data\\F4SE\\Plugins\\SAM\\options.json"

#define IDLE_FAVORITES "Data\\F4SE\\Plugins\\SAM\\IdleFavorites.txt"
//#define POSE_FAVORITES "Data\\F4SE\\Plugins\\SAM\\PoseFavorites.txt"
#define POSE_FAVORITES "Favorites"

//Errors 
#define CONSOLE_ERROR "$SAM_ConsoleError"						//selected.refr is null
#define SKELETON_ERROR "$SAM_SkeletonError"						//skeleton for targeted actor failed to load
#define EYE_ERROR "$SAM_EyeError"								//cannot find eye texture
#define MORPHS_ERROR "$SAM_MorphsError"							//Cannot get morph pointer
#define CAMERA_ERROR "$SAM_CameraError"							//Camera not in free state
#define EXPORT_ERROR "$SAM_ExportError"							//failed to get export types
#define MENU_MISSING_ERROR "$SAM_MenuMissingError"				//menu cache did not have name, maybe failed to parse
#define MENU_ITEM_LENGTH_ERROR "$SAM_PapyrusArrayLengthError"	//papyrus names,values have different length
#define ADJUSTMENT_MISSING_ERROR "$SAM_AdjustmentMissing"		//No adjustment for character found
#define PAPYRUS_FUNCTION_ERROR "$SAM_PapyrusFunctionError"		//Papyrus function not found
#define LIGHT_INDEX_ERROR "Light index out of range"			//Gave up on translation :)
#define LIGHT_FORM_ERROR "Light form ID not found"
#define BONE_MISSING_ERROR "Bone could not be found"
#define TONGUE_BONES_ERROR "Tongue menu not found"

#define F4SE_NOT_INSTALLED "Menu not registered. Make sure F4SE is installed correctly"

#define TONGUE_BONES_MENU "$SAM_TongueBones"

#define LIGHTS_ADD_NEW "$SAM_AddNew"
#define LIGHTS_ADD_CONSOLE "$SAM_AddConsole"
#define LIGHTS_GLOBAL_SETTINGS "$SAM_LightGlobal"

#define LIGHTS_SERIALIZE_VERSION 2
#define CAM_SERIALIZE_VERSION 1

#define MORPH_MAX 54
#define CAM_SAVE_STATE_SLOTS 3