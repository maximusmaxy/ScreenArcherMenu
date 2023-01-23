#pragma once

#define SAM_ESP "ScreenArcherMenu.esp"

#define SAM_MENU_NAME "ScreenArcherMenu"
#define CURSOR_MENU_NAME "CursorMenu"
#define CONSOLE_MENU_NAME "ConsoleMenu"
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
#define POSE_FAVORITES "Data\\F4SE\\Plugins\\SAM\\PoseFavorites.txt"

//selected.refr is null
#define CONSOLE_ERROR "$SAM_ConsoleError" 
//skeleton for targeted actor failed to load
#define SKELETON_ERROR "$SAM_SkeletonError"
//cannot find eye texture
#define EYE_ERROR "$SAM_EyeError"
//Cannot get morph pointer
#define MORPHS_ERROR "$SAM_MorphsError"
//Camera not in free state
#define CAMERA_ERROR "$SAM_CameraError"
//failed to get export types
#define EXPORT_ERROR "$SAM_ExportError"
//menu cache did not have name, maybe failed to parse
#define MENU_MISSING_ERROR "$SAM_MenuMissingError"
//papyrus names,values have different length
#define MENU_ITEM_LENGTH_ERROR "$SAM_PapyrusArrayLengthError"
//No adjustment for character found
#define ADJUSTMENT_MISSING_ERROR "$SAM_AdjustmentMissing"
//Papyrus function not found
#define PAPYRUS_FUNCTION_ERROR "$SAM_PapyrusFunctionError"

#define LIGHTS_SERIALIZE_VERSION 2
#define CAM_SERIALIZE_VERSION 1

#define MORPH_MAX 54
#define CAM_SAVE_STATE_SLOTS 3