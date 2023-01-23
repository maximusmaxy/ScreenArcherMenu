package {
	import flash.display.DisplayObject;
	import flashx.textLayout.formats.Float;
	import flash.utils.Timer;

    public class Data {
		public static var sam:Object;
		public static var f4seObj:Object;
		public static var stage:DisplayObject;
		
		public static var menuName:String = "";
		public static var menuOptions:Array = [];
		public static var menuValues:Array = [];
		public static var menuSize:int = 0;
		
		public static var menuFolder:Array = [];
		public static var folderStack:Array = [];
		
		public static var entryData:Object = null;
		public static var folderData:Object = null;
		public static var holdData:Object = null;
		
		public static var menuData:Object = {};
		public static var menuType:int = 0;
		public static var error:String = "";
		public static var locals:Object = {};

		public static var cursorStored:Boolean = false;
		public static var cursorPosX:int = -1;
		public static var cursorPosY:int = -1;

		public static var delayClose:Boolean;
		
		public static var selectedText:SliderListEntry = null;
		
		public static var papyrusWaiting:Boolean = false;
		public static var papyrusMenuData:Object = null;
		public static var papyrusTimer:Timer = null;
		public static var papyrusType = 0;
	
		public static const EMPTY:String = "";
		public static const ERROR:String = "Error";
		
		//The data constants need to be identical to the sam plugin enums
		public static const NONE:int = 0;
		
		public static const RESULT_SUCCESS:int = 1;
		public static const RESULT_ERROR:int = 2;
		public static const RESULT_WAITING:int = 3;
		public static const RESULT_MENU:int = 4;
		public static const RESULT_VALUES:int = 5;
		public static const RESULT_ITEMS:int = 6;
		public static const RESULT_NAMES:int = 7;
		public static const RESULT_STRING:int = 8;
		public static const RESULT_BOOL:int = 9;
		public static const RESULT_INT:int = 10;
		public static const RESULT_FLOAT:int = 11;
		public static const RESULT_OBJECT:int = 12;
		public static const RESULT_FOLDER:int = 13;
		public static const RESULT_FOLDERCHECKBOX:int = 14;
		
		public static const MENU_MAIN:int = 1;
		public static const MENU_MIXED:int = 2;
		public static const MENU_LIST:int = 3;
		public static const MENU_CHECKBOX:int = 4;
		public static const MENU_SLIDER:int = 5;
		public static const MENU_FOLDER:int = 6;
		public static const MENU_FOLDERCHECKBOX:int = 7;
		public static const MENU_ADJUSTMENT:int = 8;
		
		public static const FUNC_SAM:int = 1;
		public static const FUNC_LOCAL:int = 2;
		public static const FUNC_FORM:int = 3;
		public static const FUNC_GLOBAL:int = 4;
		public static const FUNC_ENTRY:int = 5;
		public static const FUNC_MENU:int = 6;
		public static const FUNC_FOLDER:int = 7;
		
		public static const HOTKEY_FUNC:int = 1;
		public static const HOTKEY_HOLD:int = 2;
		
		public static const BUTTON_SAVE = 1;
		public static const BUTTON_LOAD = 2;
		public static const BUTTON_RESET = 3;
		public static const BUTTON_EXTRA = 4;
		
		public static const ITEM_LIST:int = 1;
		public static const ITEM_SLIDER:int = 2;
		public static const ITEM_CHECKBOX:int = 3;
		public static const ITEM_TOUCH:int = 4;
		public static const ITEM_FOLDER:int = 5;
		public static const ITEM_ADJUSTMENT:int = 6;
		
		public static const VALUE_NONE:int = 1
		public static const VALUE_INT:int = 2;
		public static const VALUE_FLOAT:int = 3;
		
		public static const ARGS_VAR:int = 1;
		public static const ARGS_INDEX:int = 2;
		public static const ARGS_VALUE:int = 3;
		
		public static const CHECKBOX_CHECK = 1;
		public static const CHECKBOX_SETTINGS = 2;
		public static const CHECKBOX_RECYCLE = 3;
		public static const CHECKBOX_TOUCH = 4;
		public static const CHECKBOX_FOLDER = 5;
		public static const CHECKBOX_DOWN = 6;
		public static const CHECKBOX_UP = 7;
		
		public static const PATH_FILE = 1;
		public static const PATH_RELATIVE = 2;
		public static const PATH_FULL = 3;
		
		public static const SORT_ALPHANUMERIC = 1;
		public static const SORT_NATURAL = 2;
		
		public static var defaultSliderData:Object = {
			type: 2,
			min: 0,
			max: 100,
			step: 1,
			stepKey: 1
		};
		
		public static var defaultTouchData:Object = {
			type: 3,
			fixed: 2,
			step: 2.0
		};
		
		public static var resultSuccess:GFxResult = new GFxResult(RESULT_SUCCESS, null);
		public static var resultWaiting:GFxResult = new GFxResult(RESULT_WAITING, null);
		public static var resultError:GFxResult = new GFxResult(RESULT_ERROR, ERROR);
		
		public static var popFail:GFxResult = new GFxResult(RESULT_MENU, {
			type: MENU_LIST,
			names: []
		});

		public static function getMenu(name:String, pop:Boolean = false):GFxResult
		{
			try {
				var result:GFxResult = sam.GetMenu(name);
				
				//If pop fails, return a dummy menu instead to prevent locks
				if (result && result.type == RESULT_MENU)
					return result;
			}
			catch(e:Error) {
				if (Util.debug) {
					var menu:Object = getDebugMenu(name);
					if (menu)
						return new GFxResult(RESULT_MENU, menu);
				}
			}
			
			if (pop)
				return popFail;
				
			error = "$SAM_MenuMissingError";
			return null;
		}
		
		public static function updateMenu(name:String, data:Object, result:GFxResult)
		{
			trace("updating menu");
			menuName = name;
			menuData = data;
			menuType = data.type;
			
			var i:int;

			//load base menu data first
			switch(menuType) {
				case MENU_MAIN:
					menuSize = data.values.length;
					menuOptions = data.names;
					menuValues = data.values;
					break;
				case MENU_MIXED:
					trace("updating mix");
					setMenuSize(data.items.length);
					var items:Object = data.items;
					trace("menu size", menuSize);
					for (i = 0; i < menuSize; i++) {
						if (items[i].name)
							menuOptions[i] = items[i].name;
						else
							menuOptions[i] = EMPTY;
						menuValues[i] = null;
					}
					break;
				case MENU_LIST:
					if (data.list) {
						setMenuSize(data.list.length);
						for (i = 0; i < menuSize; i++) {
							menuOptions[i] = data.list[i].name;
							menuValues[i] = data.list[i].name;
						}
					} else if (data.names) {
						setMenuSize(data.names.length);
						for (i = 0; i < menuSize; i++) {
							menuOptions[i] = data.names[i];
							menuValues[i] = data.names[i];
						}
					} else {
						//length specified by get function
						if (result.type == RESULT_ITEMS) {
							setMenuSize(result.result.names.length);
						} else {
							setMenuSize(result.result.length);
						}
						for (i = 0; i < menuSize; i++) {
							menuOptions[i] = EMPTY;
							menuValues[i] = EMPTY;
						}
					}
					break;
				case MENU_CHECKBOX:
				case MENU_SLIDER:
				case MENU_ADJUSTMENT:
					menuNames = result.names;
					menuValues = result.values;
					menuSize = result.values.length;
					break;
			}
			
			updateValues(result);
		}
		
		public static function updateValues(result:GFxResult) {
			var i:int;
			var len:int;
			trace("result type", result.type);
			switch (result.type)
			{
				case Data.RESULT_NAMES:
					len = result.result.length;
					for (i = 0; i < len; i++) {
						menuOptions[i] = result.result[i];
						menuValues[i] =  result.result[i];
					}
					break;
				case Data.RESULT_VALUES:
					trace("getting values");
					len = result.result.length;
					trace("len", len);
					for (i = 0; i < len; i++) {
						trace(result.result[i]);
						menuValues[i] = result.result[i];
					}
					break;
				case Data.RESULT_ITEMS:
					len = result.result.names.length;
					for (i = 0; i < len; i++) {
						menuOptions[i] = result.result.names[i];
					}
					len = result.result.values.length;
					for (i = 0; i < len; i++) {
						menuValues[i] = result.result.values[i];
					}
					break;
			}
		}
		
		public static function setMenuSize(length:int) {
			menuSize = length;
			menuOptions.length = length;
			menuValues.length = length;
		}

		public static function getType(index:int):int
		{
			switch (menuType) {
				case MENU_MAIN: return ITEM_LIST;
				case MENU_MIXED: return menuData.items[index].type;
				case MENU_CHECKBOX: return ITEM_CHECKBOX;
				case MENU_LIST: return ITEM_LIST;
				case MENU_SLIDER: return ITEM_SLIDER;
				case MENU_FOLDER: return (menuFolder[index].folder ? ITEM_FOLDER : ITEM_LIST);
				case MENU_FOLDERCHECKBOX: return (menuFolder[index].folder ? ITEM_FOLDER : ITEM_CHECKBOX);
				case MENU_ADJUSTMENT: return ITEM_ADJUSTMENT;
			}
			
			return 0;
		}
		
		public static function getName(index:int):String
		{
			return menuOptions[index];
//			switch (menuType) {
//				case MENU_MAIN: return menuData.names[index];
//				case MENU_CHECKBOX: return menuData.names[index];
//				case MENU_MIXED: return menuData.items[index].name;
//			}
		}
		
		public static function getValue(index:int):Object
		{
			return menuValues[index];
		}
		
		public static function getInt(index:int):int
		{
			return menuValues[index];
		}
		
		public static function getFloat(index:int):Number
		{
			return menuValues[index];
		}
		
		public static function getString(index:int):String
		{
			return menuValues[index];
		}
		
		public static function getBool(index:int):Boolean
		{
			return menuValues[index];
		}
		
		public static function getHotkey(type:int):Object
		{
			switch (type) {
				case BUTTON_SAVE: if (menuData.save) return menuData.save;
				case BUTTON_LOAD: if (menuData.load) return menuData.load;
				case BUTTON_RESET: if (menuData.reset) return menuData.reset;
				case BUTTON_EXTRA: if (menuData.extra) return menuData.extra;
			}
			
			return null;
		}
		
		public static function getFolderHotkey(type:int):Object
		{
			switch (type) {
				case BUTTON_SAVE: if (folderData.save) return folderData.save;
				case BUTTON_LOAD: if (folderData.load) return folderData.load;
				case BUTTON_RESET: if (folderData.reset) return folderData.reset;
				case BUTTON_EXTRA: if (folderData.extra) return folderData.extra;
			}
			
			return null;
		}
		
		public static function getTouch(index:int):Object
		{
			var data:Object = menuData.items[index].touch;
			if (data)
				return data;
			
			return defaultTouchData;
		}
		
		public static function getSlider(index:int):Object
		{
			var data:Object = (menuType == MENU_SLIDER ? menuData.slider : menuData.items[index].data);
			if (data)
				return data;
			
			return defaultSliderData;
		}
		
		public static function setPapyrusWaiting(data:Object, type:int)
		{
			papyrusWaiting = true;
			papyrusMenuData = data;
			papyrusType = type;
		}
		
		public static function clearPapyrusWaiting()
		{
			papyrusWaiting = false;
			papyrusMenuData = null;
			if (papyrusTimer) {
				papyrusTimer.stop();
				papyrusTimer = null;
			}
		}
		
		public static function getDebugMenu(name:String):Object
		{
			switch (name) {
				
			case "Main": return {
				type: MENU_MAIN,
				names: [
					"$SAM_PoseAdjustMenu",
					"$SAM_SkeletonAdjustMenu",
					"$SAM_RaceAdjustMenu",
					"$SAM_PosePlayMenu",
					"$SAM_PoseExportMenu",
					"$SAM_PlayIdleMenu",
					"$SAM_PositionMenu",
					"$SAM_FaceMorphsMenu",
					"$SAM_EyesMenu",
					"$SAM_InventoryMenu",
					"$SAM_LightMenu",
					"$SAM_CameraMenu",
					"$SAM_HacksMenu",
					"$SAM_OptionsMenu",
					"$SAM_ExtensionsMenu"
				],
				values: [
					"PoseAdjustment",
					"SkeletonAdjustment",
					"RaceAdjustment",
					"SamPoses",
					"ExportType",
					"PlayIdle",
					"Positioning",
					"FaceMorphs",
					"Eyes",
					"Inventory",
					"Lights",
					"Camera",
					"Hacks",
					"Options",
					"Extensions"
				]
			};
			
			case "Options": return {
				type: MENU_MIXED,
				get: {
					type: FUNC_SAM,
					name: "GetOptions"
				},
				set: {
					type: FUNC_SAM,
					name: "SetOptions"
				},
				update: true,
				items: [
					{
						name: "$SAM_Hotswap",
						type: ITEM_CHECKBOX
					},
					{
						name: "$SAM_Alignment",
						type: ITEM_CHECKBOX,
						func: {
							type: FUNC_LOCAL,
							name: "SetAlignment"
						}
					},
					{
						name: "$SAM_Widescreen",
						type: ITEM_CHECKBOX,
						func: {
							type: FUNC_LOCAL,
							name: "SetWidescreen"
						}
					}
				]
			};
			
			case "Positioning": return {
				"type": MENU_MIXED,
				"get": {
					"type": FUNC_SAM,
					"name": "GetPositioning"
				},
				"set": {
					"type": FUNC_SAM,
					"name": "SetPositioning"
				},
				"update": true,
				"items": [
					{
						"name": "$SAM_Step",
						"type": ITEM_SLIDER,
						"slider": {
							"type": VALUE_INT,
							"min": 0,
							"max": 500,
							"step": 1,
							"stepkey": 1
						}
					},
					{
						"name": "$SAM_PosX",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 10.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_PosY",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 10.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_PosZ",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 10.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_RotX",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 2.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_RotY",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 2.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_RotZ",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 2.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_Scale",
						"type": ITEM_TOUCH,
						"touch": {
							"type": VALUE_FLOAT,
							"visible": true,
							"step": 1.0,
							"fixed": 2
						}
					},
					{
						"name": "$SAM_ResetPos",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_ResetRot",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_ResetScale",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_TGP",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_TCL",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_EnableFootIK",
						"type": ITEM_LIST
					},
					{
						"name": "$SAM_DisableFootIK",
						"type": ITEM_LIST
					}
				]
			};
			
			case "Inventory": return {
				type: MENU_LIST,
				names: [
					"Hello"
				],
				load: {
					"name": "$SAM_Load",
					"type": FUNC_FOLDER,
					"folder": {
						"type": PATH_FULL,
						"path": "Data\\F4SE\\Plugins\\SAF\\Adjustments",
						"ext": ".json",
						"pop": false,
						"func": {
							"type": FUNC_SAM,
							"name": "LoadAdjustment"
						}
					}
				},
				save: {
					"name": "$SAM_SAVE",
					"type": FUNC_ENTRY,
					"entry": {
						"func": {
							"type": FUNC_SAM,
							"name": "SaveAdjustment"
						}
					}
				}
			};
			
			}
			
			error = "$SAM_MenuMissing";
			return null;
		}
		
		public static function getSamDebugFunction(name:String):GFxResult
		{
			switch (name)
			{
				
			case "GetOptions": return new GFxResult(RESULT_VALUES, [
				false, false, false
			]);
			
			case "GetPositioning": return new GFxResult(RESULT_VALUES, [
				 100, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 1.0
			]);
			
			}
			
			error = "$SAM_SamFunctionMissing";
			return null;
		}
		
		public static function load(data:Object, samObj:Object, f4se:Object, stageObj:DisplayObject)
		{
			sam = samObj;
			f4seObj = f4se;
			delayClose = data.delayClose;
			stage = stageObj;
			
			if (data.saved) {
				menuOptions = data.saved.options;
				menuValues = data.saved.values;
				menuSize = data.saved.size;
				folderStack = data.saved.folderStack;
				menuFolder = data.saved.folder;
				locals = data.saved.locals;
				menuData = data.saved.menuData;
				entryData = data.saved.entryData;
				folderData = data.saved.folderData;
				holdData = data.saved.holdData;
				menuType = data.saved.menuType;
			}
		}
		
		public static function saveState(data:Object)
		{
			data.options = menuOptions;
			data.values = menuValues;
			data.size = menuSize
			data.folderStack = folderStack;
			data.folder = menuFolder;
			data.locals = locals;
			data.menuData = menuData;
			data.entryData = entryData;
			data.folderData = folderData;
			data.holdData = holdData;
			data.menuType = menuType;
			
			try 
			{
				sam.SaveState(data);
			}
			catch (e:Error)
			{
				trace("Failed to save state");
			}
		}
		
		public static function clearState()
		{
			try {
				sam.ClearState();
			}
			catch (e:Error)
			{
				trace("Failed to clear state");
			}
		}
		
//		public static function checkError(id:int):Boolean
//		{
//			try {
//				return sam.CheckError(id);
//			}
//			catch (e:Error)
//			{
//				trace("Failed to check error");
//				if (Util.debug)
//				{
//					return true;
//				}
//			}
//			return false;
//		}
//		
//		public static function getFileListing(dir:String, ext:String):Boolean
//		{
//			menuFiles = [];
//			try {
//				var listing:Array = f4seObj.GetDirectoryListing(dir, ext);
//				for (var i:int = 0; i < listing.length; i++) {
//					var filename = listing[i]["nativePath"]; //name is bugged so use nativePath instead
//					var startIndex:int = filename.lastIndexOf("\\") + 1;
//					menuFiles[i] = filename.substring(startIndex, filename.length - ext.length + 1);
//				}
//				return true;
//			}
//			catch (e:Error)
//			{
//				trace("Failed to get file listing from directory: " + dir);
//			}
//			return false;
//		}
		
		public static function setCursorVisible(visible:Boolean)
		{
			try 
			{
				sam.SetCursorVisible(visible);
			}
			catch(e:Error)
			{
				trace("Failed to set cursor visibility");
			}
		}
		
		public static function storeCursorPos()
		{
			try {
				var result:Object = sam.GetCursorPosition();
				cursorStored = result.success;
				if (cursorStored) {
					cursorPosX = result.x;
					cursorPosY = result.y;
				}
			}
			catch (e:Error)
			{
				trace("Failed to store cursor pos");
				cursorStored = false;
			}
		}
		
		public static function updateCursorDrag():int
		{
			if (cursorStored) {
				try 
				{
					var result:Object = sam.GetCursorPosition();
					if (result.success) {
						sam.SetCursorPosition(cursorPosX, cursorPosY);
						return result.x - cursorPosX;
					}
				}
				catch (e:Error)
				{
					trace("Failed to update cursor pos");
				}
			}
			return 0;
		}
		
		public static function endCursorDrag()
		{
			cursorStored = false;
		}
		
//		public static function isLocked(type:int):Boolean
//		{
//			try 
//			{
//				return sam.GetLock(type);
//			}
//			catch (e:Error)
//			{
//				trace("Failed to get scroll lock");
//			}
//			return false;
//		}
		
		public static function toggleMenu():Boolean
		{
			try
			{
				return sam.ToggleMenus();
			}
			catch (e:Error)
			{
				trace("Failed to hide menus");
			}
			return false;
		}
		
		public static function loadAdjustmentList()
		{
			try {
				var adjustments:Object = sam.GetAdjustmentList();
				menuOptions = adjustments.names;
				menuValues = adjustments.values;
			}
			catch (e:Error)
			{
				trace("Failed to load adjustment list");
				if (Util.debug) {
					menuOptions = [
						"New Adjustment 1",
						"New Adjustment 2",
						"New Adjustment 3",
						"New Adjustment 4"
					];
					menuValues = [0, 1, 2, 3];
				}
			}
		}
		
		public static function loadAdjustmentFiles()
		{

			if (!getFileListing(ADJUSTMENT_DIRECTORY, "*.json"))
			{
				if (Util.debug) {
					for (var y:int = 0; y < 1000; y++) {
						menuFiles[y] = y.toString();
					}
				}
			}
		}
		
		public static function saveAdjustment(filename:String)
		{
			try
			{
				sam.SaveAdjustment(selectedAdjustment, filename);
			}
			catch (e:Error)
			{
				trace("Failed to save adjustment");
			}
		}
		
		public static function loadAdjustment(id:int)
		{
			try
			{
				sam.LoadAdjustment(menuOptions[id]);
			}
			catch (e:Error)
			{
				trace("Failed to load adjustment");
			}
		}
		
		public static function editAdjustment()
		{
			try
			{
				menuValues = [100, 0, 0, 0];
				var adjustment:Object = sam.GetAdjustment(selectedAdjustment);
				if (adjustment.scale != null) {
					menuValues[0] = adjustment.scale;
				}
				if (adjustment.groups) {
					for (var i:int = 0; i < adjustment.groups.length; i++) {
						menuValues.push(adjustment.groups[i]);
					}
				}
			}
			catch (e:Error)
			{
				trace("Failed to get adjustment");
				if (Util.debug) {
					menuValues = [100, 0, 0, 0, "Head", "Left Arm", "Left Leg"]
				}
			}
		}
		
		public static function setAdjustmentPersistent(persistent:Boolean)
		{
			try
			{
				sam.SetAdjustmentPersistence(selectedAdjustment, persistent);
			}
			catch (e:Error)
			{
				trace("Failed to set adjustment persistence");
			}
		}
		
		public static function setAdjustmentScale(scale:int)
		{
			try
			{
				sam.SetAdjustmentScale(selectedAdjustment, scale);
			}
			catch (e:Error)
			{
				trace("Failed to set adjustment weight");
			}
		}
		
		public static function resetAdjustment()
		{
			try
			{
				sam.ResetAdjustment(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to reset adjustment");
			}
		}
		
		public static function newAdjustment()
		{
			try
			{
				var adjustments:Object = sam.NewAdjustment();
				menuOptions = adjustments.names;
				menuValues = adjustments.values;
			}
			catch (e:Error)
			{
				trace("Failed to create new adjustment");
				if (Util.debug){
					menuOptions.push((menuOptions.length + 1).toString());
				}
			}
		}
		
		public static function removeAdjustment(id:int)
		{
			try
			{
				sam.RemoveAdjustment(id);
			}
			catch (e:Error)
			{
				trace("Failed to remove adjustment");
			}
		}
		
		public static function negateAdjustment()
		{
			try
			{
				sam.NegateAdjustment(boneName, selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to negate adjustment");
			}
		}
		
		public static function negateAdjustmentGroup(id:int)
		{
			try
			{
				sam.NegateAdjustmentGroup(selectedAdjustment, menuValues[id]);
			}
			catch (e:Error)
			{
				trace("Failed to negate adjustment group");
			}
		}
		
		public static function moveAdjustment(id:int, inc:Boolean):Boolean
		{
			try
			{
				return sam.MoveAdjustment(menuValues[id], inc);
			}
			catch (e:Error)
			{
				trace("Failed to move adjustment");
			}
			return false;
		}
		
		public static function renameAdjustment(name:String)
		{
			try {
				sam.RenameAdjustment(selectedAdjustment, name);
			}
			catch (e:Error)
			{
				trace("Failed to rename adjustment");
			}
		}
		
		public static function loadCategories()
		{
			try {
				var categories:Object = sam.GetCategoryList();
				menuOptions = categories.names;
				menuValues = categories.values;
			}
			catch (e:Error)
			{
				trace("Failed to load categories");
				if (Util.debug){
					menuOptions = [
						"Main",
						"Left Arm",
						"Right Arm",
						"Legs",
						"Tongue",
						"NSFW",
						"Knee Fix",
						"Misc"
					];
					menuValues = [];
				} else {
					menuOptions = [];
					menuValues = [];
				}
			}
		}
		
		public static function loadBones()
		{
			try {
				var nodes:Object = sam.GetNodeList(selectedCategory);
				menuOptions = nodes.names;
				menuValues = nodes.values;
			}
			catch (e:Error)
			{
				trace("Failed to load bones");
				menuOptions = [];
				menuValues = [];
				if (Util.debug) {
					for (var i:int = 0; i < 30; i++)
					{
						menuOptions[i] = i.toString();
						menuValues[i] = i;
					}
				}
			}
		}
		
		public static function getNodeName()
		{
			try {
				boneName = sam.GetNodeNameFromIndexes(selectedCategory, selectedBone);
			}
			catch (e:Error)
			{
				trace("Failed to get node");
				boneName = "";
			}
		}
		
		public static function getNodeIsOffset() : Boolean
		{
			try {
				return sam.GetNodeIsOffset(boneName);
			}
			catch (e:Error) {
				trace("Failed to check if node is offset");
			}
			return true;
		}
		
		public static function toggleNodeName() : Boolean
		{
			try {
				boneName = sam.ToggleNodeName(boneName);
				return true;
			}
			catch (e:Error) {
				trace("Failed to toggle node name");
			}
			return false;
		}

		public static function loadTransforms() 
		{
			try {
				menuValues = sam.GetNodeTransform(boneName, selectedAdjustment);
				if (menuValues.length == 0) {
					menuValues = [ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 ];
				}
			}
			catch (e:Error)
			{
				trace("Failed to load bone transform");
				menuValues = [ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0 ];
			}
		}
		
		public static function setTransform(id:int, value:Number)
		{
			if (id < 7) {
				menuValues[id] = value;
			}
			if (boneName == ""){
				return;
			}
			try 
			{
				if (id < 3) //rot
				{ 
					sam.SetNodeRotation(boneName, selectedAdjustment, menuValues[0], menuValues[1], menuValues[2]);
				} 
				else if (id < 6) //pos
				{
					sam.SetNodePosition(boneName, selectedAdjustment, menuValues[3], menuValues[4], menuValues[5]);
				}
				else if (id < 7)//scale
				{
					sam.SetNodeScale(boneName, selectedAdjustment, menuValues[6]);
				}
				else
				{
					var dif:int = updateCursorDrag(value);
					menuValues = sam.AdjustNodeRotation(boneName, selectedAdjustment, id, dif);
					if (menuValues.length == 0) {
						menuValues = [
							0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
						];
						boneName = "";
					}
				}
			}
			catch (e:Error)
			{
				trace("Set transform failed");
			}
		}
		
		public static function resetTransform()
		{
			for (var i:int = 0; i < 6; i++) {
				menuValues[i] = 0.0;
			}
			menuValues[6] = 1.0;
			if (boneName == "") {
				return;
			}
			try 
			{
				sam.ResetTransform(boneName, selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to reset transform");
			}
		}
		
		public static function loadMorphCategories()
		{
			try
			{
				var categories:Object = sam.GetMorphCategories();
				menuOptions = categories.names;
				menuValues = categories.values;
			}
			catch (e:Error)
			{
				trace("Failed to load morph categories");
				if (Util.debug) {
					menuOptions = [
						"Lips",
						"Jaw",
						"Eyes",
						"Eyebrows",
						"Nose",
						"Cheek",
						"Tongue",
						"Tongue Bones"
					];
				}
			}
		}
		
		public static function loadMorphs()
		{
			try
			{
				var morphs:Object = sam.GetMorphs(selectedCategory);
				menuOptions = morphs.names;
				menuValues = morphs.values;
			}
			catch (e:Error)
			{
				trace("Failed to load morphs");
				menuOptions = new Array();
				menuValues = new Array();
				if (Util.debug) {
					for (var i:int = 0; i < 10; i++) {
						menuOptions[i] = "Test" + i;
						menuValues[i] = i;
					}
				}
			}
		}
		
		public static function setMorph(id:int, value:int)
		{
			menuValues[id] = value;
			try
			{
				sam.ModifyFacegenMorph(selectedCategory, id, value);
			}
			catch (e:Error)
			{
				trace("Failed to set morph");
			}
		}
		
		public static function saveMfg(filename:String)
		{
			try
			{
				sam.SaveMorphsPreset(filename);
			}
			catch (e:Error)
			{
				trace("Failed to save preset");
			}
		}
		
		public static function resetMorphs(update:Boolean)
		{
			try 
			{
				sam.ResetMorphs();
				if (update) {
					var len:int = menuValues.length;
					for (var i:int = 0; i < len; i++)
					{
						menuValues[i] = 0;
					}
				}
			}
			catch (e:Error)
			{
				trace("Morph reset failed");
			}
		}
		
		public static function loadEyes()
		{
			try 
			{
				menuValues = sam.GetEyeCoords();
				var eyeHack:Boolean = sam.GetEyeTrackingHack();
				menuValues.push(eyeHack);
			}
			catch (e:Error)
			{
				trace("Failed to load eye coords");
				menuValues = [0.0, 0.0, false];
			}
		}
		
		public static function setEye(id:int, value:Object)
		{
			menuValues[id] = value;
			try
			{
				sam.SetEyeCoords(menuValues[0], menuValues[1]);
			}
			catch (e:Error)
			{
				trace("Failed to set eye coords");
			}
		}
		
		public static function loadHacks()
		{
			try 
			{
				menuValues = sam.GetHacks();
			}
			catch (e:Error)
			{
				trace("Failed to load hacks");
				menuValues = [false, true, false];
			}
		}
		
		public static function setHack(id:int, enabled:Boolean)
		{
			menuValues[id] = enabled;
			try {
				switch (id)
				{
					case 0: sam.SetBlinkHack(enabled); break;
					case 1: sam.SetMorphHack(enabled); break;
					case 2: sam.SetEyeTrackingHack(enabled); break;
				}
			}
			catch (e:Error)
			{
				trace("Failed to set hack");
			}
		}
		
		public static function loadIdleCategories()
		{
			try {
				menuOptions = sam.GetIdleCategories();
			}
			catch (e:Error)
			{
				trace("Failed to get idle categories")
				if (Util.debug) {
					menuOptions = ["Pose mod"];
				} else {
					menuOption = new Array();
				}
			}
		}
		
		public static function loadIdles()
		{
			try {
				var idles:Object = sam.GetIdles(selectedCategory);
				menuOptions = idles.names;
				menuValues = idles.values;
			} 
			catch (e:Error)
			{
				trace("Failed to load idles");
				if (Util.debug) {
					menuOptions =  ["Idle 1", "Idle 2", "Idle 3"];
					menuValues = [0, 1, 2];
				} else {
					menuOptions = new Array();
					menuValues = new Array();
				}
			}
		}
		
		public static function playIdle(id:int)
		{
			try {
				sam.PlayIdle(menuValues[id]);
			} 
			catch (e:Error)
			{
				trace("Failed to play idle");
			}
		}
		
		public static function resetIdle()
		{
			try {
				sam.ResetIdle();
			} 
			catch (e:Error)
			{
				trace("Failed to reset idle");
			}
		}
		
		public static function rotateIdle(value:Number)
		{
			try {
				var dif:int = updateCursorDrag(value);
				sam.AdjustPositioning(6, dif, 100);
			}
			catch (e:Error)
			{
				trace("Failed to rotate idle");
			}
		}
		
		public static function getIdleName():String
		{
			try {
				return sam.GetIdleName();
			}
			catch (e:Error)
			{
				trace("Failed to get idle name");
			}
			return null;
		}
		
		public static function getPoseList() 
		{
			try {
				var poses:Object = sam.GetPoseList();
				menuOptions = poses.names;
				menuValues = poses.values;
				poseHandles = poses.handles;
			}
			catch (e:Error)
			{
				trace("Failed to get pose list");
				if (Util.debug) {
					menuOptions = ["1", "2", "3"];
					menuValues = [false, true, false];
					poseHandles = [1, 2, 3];
				} else {
					menuOptions = [];
					menuValues = [];
					poseHandles = [];
				}
			}
		}
		
		public static function savePose(filename:String)
		{
			try 
			{
				var handles:Array = [];
				for (var i:int = 0; i < menuValues.length; i++) {
					if (menuValues[i]) {
						handles.push(poseHandles[i]);
					}
				}
				sam.SavePose(filename, handles, selectedCategory);
			}
			catch (e:Error)
			{
				trace("Failed to save pose");
			}
		}
		
		public static function loadPose(id:int)
		{
			try 
			{
				sam.LoadPose(POSE_DIRECTORY + "\\Exports\\" + menuFiles[id] + ".json");
			}
			catch (e:Error)
			{
				trace("failed to load pose");
			}
		}
		
		public static function loadPoseFiles()
		{
			if (!getFileListing(POSE_DIRECTORY + "\\Exports", "*.json"))
			{
				if (Util.debug){
					for (var y:int = 0; y < 1000; y++) {
						menuFiles[y] = y.toString();
					}
				}
			}
		}
		
		public static function resetPose(id:int)
		{
			try
			{
				sam.ResetPose(id);
			}
			catch (e:Error)
			{
				trace("Reset pose failed");
			}
		}
		
		public static function loadSkeletonAdjustment(id:int, race:Boolean, clear:Boolean, enabled:Boolean)
		{
			if (clear) {
				for (var i:int = 0; i < menuValues.length; i++) {
					menuValues[i] = false;
				}
			}
			
			menuValues[id] = enabled;
			
			try 
			{
				sam.LoadSkeletonAdjustment(menuOptions[id], race, clear, enabled);
			}
			catch (e:Error)
			{
				trace("Failed to load skeleton adjustment");
			}
		}
		
		public static function resetSkeletonAdjustment(race:Boolean)
		{
			try 
			{
				sam.ResetSkeletonAdjustment(race);
			}
			catch (e:Error)
			{
				trace("Failed to reset skeleton adjustment");
			}
		}
		
//		public static function updatePositioning()
//		{
//			menuValues[0] = stepValue;
//			for (var i:int = 8; i < POSITIONING_NAMES.length; i++) {
//				menuValues[i] = 0;
//			}
//		}
		
		public static function loadPositioning()
		{
			try {
				menuValues = sam.GetPositioning();
				updatePositioning();
			}
			catch (e:Error)
			{
				trace("Failed to load positioning");
				if (Util.debug) {
					menuValues = [0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
					updatePositioning();
				}
			}
		}
		
//		public static function selectPositioning(id:int, value:Number = 0)
//		{
//			try
//			{
//				if (id < 1) {
//					var step:int = int(value);
//					menuValues[id] = step;
//					stepValue = step;
//				} else if (id < 8) {
//					var dif:int = updateCursorDrag(value);
//					menuValues = sam.AdjustPositioning(id, dif, stepValue);
//					updatePositioning();
//				} else {
//					menuValues = sam.SelectPositioning(id);
//					updatePositioning();
//				}
//			}
//			catch (e:Error)
//			{
//				trace("Failed to update position");
//			}
//		}
		
		public static function resetPositioning()
		{
			try
			{
				menuValues = sam.ResetPositioning();
				updatePositioning();
			}
			catch (e:Error)
			{
				trace("Failed to reset position");
			}
		}
		
		public static function updateFolderNames():Array
		{
			setMenuSize(menuFolder.length);
			for (var i:int = 0; i < menuFolder.length; i++) {
				menuOptions[i] = menuFolder[i].name;
			}
		}
		
		public static function updateFolderCheckbox():Array
		{
			menuValues.length = menuFolder.length;
			for (var i:int = 0; i < menuFolder.length; i++) {
				menuValues[i] = menuFolder[i].checked;
			}
		}
//		
//		public static function popFolder(dir:String, ext:String)
//		{
//			folderStack.pop();
//			try {
//				var path:String = (folderStack.length == 0) ? dir : folderStack[folderStack.length - 1];
//				menuFolder = sam.GetSubFolder(path, ext);
//				updateFolderNames();
//			}
//			catch (e:Error) {
//				trace("Failed to get sub folder");
//				menuOptions = [];
//				menuFolder = [];
//			}
//		}
		
		public static function popSkeletonAdjustment(race:Boolean)
		{
			folderStack.pop();
			try {
				var path:String = (folderStack.length == 0) ? ADJUSTMENT_DIRECTORY : folderStack[folderStack.length - 1];
				menuFolder = sam.GetSkeletonAdjustments(path, race);
				updateFolderNames();
				updateFolderCheckbox();
			}
			catch (e:Error) {
				trace("Failed to get skeleton adjustments");
				menuOptions = [];
				menuFolder = [];
			}
		}
		
		public static function getFolder(path:String, ext:String, pop:Boolean = false):GFxResult
		{
			try {
				//var path:String = (folderStack.length == 0) ? folderData.path : folderStack[folderStack.length - 1];
				var result:GFxResult = sam.GetFolder(path, ext);
				
				if (result && result.type == RESULT_FOLDER)
					return result;
			}
			catch (e:Error) {
				if (Util.debug) {
					var debugFolder:Object = getDebugFolder(path);
					if (debugFolder)
						return new GFxResult(RESULT_FOLDER, debugFolder);
				}
			}
			
			if (pop)
				return popFail;
			
			error = "Failed to get folder";
			return null;
		}
		
		public static function setFolder(data:Object, result:Object)
		{
			menuType = MENU_FOLDER;
			folderData = data;
			folderStack.length = 0;
			menuFolder = result;
			updateFolderNames();
		}
		
		public static function pushFolder(i:int, result:Object)
		{
			folderStack.push(menuFolder[i].path);
			menuFolder = result;
			updateFolderNames();
		}
		
		public static function popFolder(result:Object):String
		{
			menuFolder = result;
			updateFolderNames();
		}
		
		public static function getFolderPath(i:int)
		{
			try {
				switch(folderData.type) {
					case PATH_FILE:
						var fileResult:GFxResult = sam.GetPathStem(menuFolder[i].path);
						if (fileResult && fileResult.type == RESULT_STRING)
							return fileResult.result;
						break;
						
					case PATH_RELATIVE:
						var relativeResult:GFxResult = sam.GetPathRelative(menuFolder[i].path, folderData.path, folderData.ext);
						if (relativeResult && relativeResult.type == RESULT_STRING)
							return relativeResult.result;
						break;
						
					case PATH_FULL:
						return menuFolder[i].path;
				}
			}
			catch (e:Error) {
				trace(e.message);
			}
			
			return EMPTY;
		}
		
		public static function getDebugFolder(path:String)
		{
			return [
				{
					"name": "Folder",
					"folder": true,
					"path": "test"
				},
				{
					"name": "File",
					"folder": false,
					"path": "test"
				}
			];
			
			error = "Failed to get folder";
			return null;
		}
		
//		public static function loadSubFolder(dir:String, ext:String)
//		{
//			try {
//				var path:String = (folderStack.length == 0) ? dir : folderStack[folderStack.length - 1];
//				menuFolder = sam.GetSubFolder(path, ext);
//				updateFolderNames();
//			}
//			catch (e:Error)
//			{
//				trace("Failed to load sam poses");
//				if (Util.debug)
//				{
//					menuFolder = [
//						{
//							"name": "Folder",
//							"folder": true,
//							"path": "test"
//						},
//						{
//							"name": "File",
//							"path": "test"
//						}
//					];
//					updateFolderNames();
//				} else {
//					menuFolder = [];
//					menuOptions = [];
//				}
//			}
//		}
		
		public static function getAdjustmentFolder(race:Boolean)
		{
			try {
				var path:String = (folderStack.length == 0) ? ADJUSTMENT_DIRECTORY : folderStack[folderStack.length - 1];
				menuFolder = sam.GetSkeletonAdjustments(path, race);
				updateFolderNames();
				updateFolderCheckbox();
			}
			catch (e:Error)
			{
				trace("Failed to get skeleton adjustments");
				if (Util.debug) {
					menuFolder = [
						{
							"name": "Folder",
							"folder": true,
							"path": "test"
						},
						{
							"name": "File",
							"path": "test",
							"checked": true
						},
						{
							"name": "File",
							"path": "test",
							"checked": false
						}
					];
					updateFolderNames();
				} else {
					menuOptions = [];
					menuFolder = [];
				}
			}
		}
		
//		public static function selectSubFolder(id:int, ext:String, func:Function):Boolean
//		{
//			try {
//				if (menuFolder[id].folder) {
//					var path:String = menuFolder[id].path;
//					menuFolder = sam.GetSubFolder(path, ext);
//					folderStack.push(path);
//					updateFolderNames();
//					return true;
//				} 
//				else 
//				{
//					func(menuFolder[id].path);
//				}
//			}
//			catch (e:Error)
//			{
//				trace("Failed to select sub folder");
//				menuOptions = [];
//				menuFolder = [];
//			}
//			return false;
//		}
		
		public static function selectSkeletonFile(id:int, race:Boolean, clear:Boolean, enabled:Boolean):Boolean
		{
			try {
				if (menuFolder[id].folder) {
					var path:String = menuFolder[id].path;
					menuFolder = sam.GetSkeletonAdjustments(path, race);
					folderStack.push(path);
					updateFolderNames();
					updateFolderCheckbox();
					return true;
				} 
				else 
				{
//					if (clear) {
//						for (var i:int = 0; i < menuValues.length; i++) {
//							menuFolder[i].checked = false;
//						}
//					}
//					
//					menuFolder[id].checked = enabled;
//					updateFolderCheckbox();
//					
					sam.LoadSkeletonAdjustment(menuFolder[id].path, race, clear, enabled);
				}
			}
			catch (e:Error)
			{
				trace("Failed to select adjustment folder");
				menuOptions = [];
				menuFolder = [];
			}
			return false;
		}

		public static function selectSamPose(id:int):Boolean
		{
			return selectSubFolder(id, JSON_EXT, sam.LoadPose);
		}
		
		public static function selectLightFile(id:int):Boolean
		{
			return selectSubFolder(id, JSON_EXT, sam.LoadLights);
		}
		
		public static function selectMfgFile(id:int):Boolean
		{
			return selectSubFolder(id, TXT_EXT, sam.LoadMorphsPreset);
		}
		
		public static function selectAdjustmentFile(id:int):Boolean
		{
			return selectSubFolder(id, JSON_EXT, sam.LoadAdjustment);
		}
		
		public static function loadOptions()
		{
			menuOptions = OPTION_NAMES;
			try {
				menuValues = sam.GetOptions();
			}
			catch (e:Error)
			{
				trace("Failed to load options");
				menuValues = [];
				for (var i:int = 0; i < OPTION_NAMES; i++) {
					menuValues[i] = false;
				}
			}
		}
		
		public static function setOption(id:int, enabled:Boolean)
		{
			try {
				sam.SetOption(id, enabled);
			}
			catch (e:Error)
			{
				trace("Failed to set option");
			}
		}
		
		public static function loadCamera()
		{
			menuOptions = CAMERA_NAMES;
			try 
			{
				menuValues = sam.GetCamera();
			}
			catch (e:Error)
			{
				trace("Failed to load camera");
				menuValues = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 1, 2, 0, 1, 2];
			}
		}
		
		public static function setCamera(id:int, value:Number)
		{
			try
			{
				if (id < 3)
				{
					var dif:int = updateCursorDrag(value);
					menuValues[id] += dif * 0.5;
					sam.SetCameraPosition(menuValues[0], menuValues[1], menuValues[2]);
				} 
				else if (id < 6)
				{
					menuValues[id] = value;
					sam.SetCameraRotation(menuValues[3], menuValues[4], menuValues[5]);
				}
				else if (id < 7)
				{
					menuValues[id] = value;
					sam.SetCameraFOV(menuValues[6]);
				}
				else
				{
					var save:Boolean = id < (7 + ((Data.menuValues.length - 7) / 2));
					if (save) {
						sam.SaveCameraState(menuValues[id]);
					} else {
						sam.LoadCameraState(menuValues[id]);
					}
				}
			}
			catch (e:Error)
			{
				trace("Failed to set camera");
			}
		}

		public static function loadLightSelect()
		{
			try
			{
				menuOptions = sam.GetLightSelect();
				menuOptions.push("$SAM_AddNew");
				menuOptions.push("$SAM_AddConsole");
				menuOptions.push("$SAM_LightGlobal");
			}
			catch (e:Error)
			{
				trace("Failed to load lights");
				if (Util.debug) {
					menuOptions = ["Light", "Light", "Light", "$SAM_AddNew", "$SAM_AddConsole", "$SAM_LightGlobal"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightEdit()
		{
			menuOptions = LIGHT_NAMES;
			try
			{
				menuValues = sam.GetLightEdit(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to load light");
				menuValues = [100.0, 0.0, 100.0, 0.0, 0.0];
			}
		}
		
		public static function loadLightCategory()
		{
			try
			{
				menuOptions = sam.GetLightCategories();
			}
			catch (e:Error)
			{
				trace("Failed to load light categories");
				if (Util.debug) {
					menuOptions = ["Hemisphere", "Spotlight", "Omni"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightObject()
		{
			try
			{
				menuOptions = sam.GetLightObjects(selectedCategory);
			}
			catch (e:Error)
			{
				trace("Failed to load light Objects");
				if (Util.debug) {
					menuOptions = ["White", "Warm", "Cold"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightSettings()
		{
			try
			{
				menuValues = sam.GetLightSettings();
			}
			catch (e:Error)
			{
				trace("Failed to get light settings");
				menuValues = [0.0, 0.0, 0.0, 0.0];
			}
		}
		
//		public static function editLight(id:int, value:Number)
//		{
//			try
//			{
//				if (id < 3) {
//					var dif:int = updateCursorDrag(value);
//					menuValues[id] += dif * 0.5;
//					if (id == 0 && menuValues[id] < 0) {
//						//clamp distance above 0
//						menuValues[id] = 0;
//					} else if (id == 1) {
//						//rotation 0-360
//						if (menuValues[id] < 0) {
//							menuValues[id] += 360;
//						} else if (menuValues[id] >= 360) {
//							menuValues[id] -= 360;
//						}
//					}
//				} else {
//					menuValues[id] = value;
//				}
//				sam.EditLight(selectedAdjustment, id, menuValues[id]);
//			}
//			catch (e:Error)
//			{
//				trace("Failed to edit light");
//			}
//		}
		
		public static function resetLight()
		{
			menuValues = [100.0, 0.0, 100.0, 0.0, 0.0];
			try
			{
				sam.ResetLight(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to reset adjustment");
			}
		}
		
		public static function deleteLight()
		{
			try
			{
				sam.DeleteLight(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to delete light");
			}
		}
		
		public static function swapLight(id:int)
		{
			try
			{
				sam.SwapLight(selectedAdjustment, selectedCategory, id);
			}
			catch (e:Error)
			{
				trace("Failed to swap light");
			}
		}
		
		public static function renameLight(name:String)
		{
			try
			{
				sam.RenameLight(selectedAdjustment, name);
			}
			catch (e:Error)
			{
				trace("Failed to rename light");
			}
		}
		
		public static function createLight(id:int)
		{
			try
			{
				sam.CreateLight(selectedCategory, id);
			}
			catch (e:Error)
			{
				trace("Failed to create light");
			}
		}
		
		public static function addLight()
		{
			try
			{
				sam.AddLight();
			}
			catch (e:Error)
			{
				trace("Failed to add light");
			}
		}
		
		public static function getLightVisible():Boolean
		{
			try
			{
				return sam.GetLightVisible(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to get light visibility");
				return false;
			}
		}
		
		public static function toggleLightVisible():Boolean
		{
			try
			{
				return sam.ToggleLightVisible(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to toggle light visiblity");
				return true;
			}
		}
		
		public static function getLightsVisibility():Boolean
		{
			try
			{
				return sam.GetAllLightsVisible();
			}
			catch (e:Error)
			{
				trace("Failed to get light visibility");
				return false;
			}
		}
		
		public static function toggleLightsVisibility():Boolean
		{
			try
			{
				return sam.ToggleAllLightsVisible();
			}
			catch (e:Error)
			{
				trace("Failed to toggle lights visibility");
				return true;
			}
		}
		
		public static function editLightSettings(id:int, value:Number)
		{
			try
			{
				var dif:int = updateCursorDrag(value);
				menuValues[id] += dif * 0.5;
				if (id == 3) { //rotation clamp
					if (menuValues[id] < 0) {
						menuValues[id] += 360;
					} else if (menuValues[id] >= 360) {
						menuValues[id] -= 360;
					}
				}
				sam.EditLightSettings(id, menuValues[id]);
			}
			catch (e:Error)
			{
				trace("Failed to edit light settings");
			}
		}
		
		public static function resetLightSettings()
		{
			menuValues = [0.0, 0.0, 0.0, 0.0];
			try
			{
				sam.ResetLightSettings();
			}
			catch (e:Error)
			{
				trace("Reset light settings");
			}
		}
		
		public static function updateAllLights()
		{
			try
			{
				sam.UpdateAllLights();
			}
			catch (e:Error)
			{
				trace("Failed to update lights");
			}
		}
		
		public static function deleteAllLights()
		{
			try 
			{
				sam.DeleteAllLights();
			}
			catch (e:Error)
			{
				trace("Failed to delete lights");
			}
		}
		
		public static function saveLights(filename:String)
		{
			try
			{
				sam.SaveLights(filename);
			}
			catch (e:Error)
			{
				trace("Failed to save lights");
			}
		}
		
		public static function loadPoseExport()
		{
			try
			{
				menuOptions = sam.GetPoseExportTypes();
			}
			catch (e:Error)
			{
				trace("Failed to get pose export types");
				if (Util.debug) {
					menuOptions = ["Vanilla", "ZeX", "All", "Outfit Studio"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function getMorphsTongueNodes()
		{
			try 
			{
				var nodes:Object = sam.GetMorphsTongueNodes(selectedCategory);
				menuOptions = nodes.names;
				menuValues = nodes.values;
			}
			catch (e:Error)
			{
				trace("Failed to get tongue nodes");
				menuOptions = [];
				menuValues = [];
			}
		}
		
		public static function getMorphsTongue(id:int)
		{
			try {
				var tongue:Array = sam.GetMorphsTongue(selectedCategory, menuValues[id]);
				if (tongue.length == 0) {
					boneName = "";
					selectedAdjustment = 0;
				} else {
					boneName = tongue[0];
					selectedAdjustment = tongue[1];
				}
			}
			catch (e:Error)
			{
				trace("Failed to get morphs tongue");
				boneName = "";
				selectedAdjustment = 0;
			}
		}
	}
}