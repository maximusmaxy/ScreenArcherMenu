package 
{
	import Shared.*;
	import Shared.AS3.*;
	import __AS3__.vec.*;
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.ui.*;
	import scaleform.gfx.*;
	import utils.Debug;
	import flash.utils.*;
	import scaleform.clik.events.SliderEvent;
	import scaleform.clik.constants.InputValue;
	import scaleform.clik.events.InputEvent;
	import scaleform.clik.ui.InputDetails;
	import utils.Translator;
	import Mobile.ScrollList.EventWithParams;
	import flash.media.ID3Info;

	public class ScreenArcherMenu extends Shared.IMenu
	{
		public var state:int = 0;
		public var currentState:MenuState;
		public var stateStack:Array;
		public var sam:Object;
		
		public var sliderList:SliderList;
		public var ButtonHintBar_mc:BSButtonHintBar;
		public var filenameInput:MovieClip;
		public var border:MovieClip;
		public var notification:MovieClip;
		
		public var titleName:String = "";
		public var rootMenu:String;
		
		internal var buttonHintData:Vector.<BSButtonHintData > ;
		internal var buttonHintExit:BSButtonHintData;
		internal var buttonHintSave:BSButtonHintData;
		internal var buttonHintLoad:BSButtonHintData;
		internal var buttonHintReset:BSButtonHintData;
		internal var buttonHintBack:BSButtonHintData;
		internal var buttonHintConfirm:BSButtonHintData;
		internal var buttonHintHide:BSButtonHintData;
		internal var buttonHintExtra:BSButtonHintData;
		//internal var buttonHintTarget:BSButtonHintData;
		
		internal var closeTimer:Timer;
		private var delayClose:Boolean = false;
		
		public var BGSCodeObj:Object;
		public var f4seObj:Object;

		public var swapped:Boolean = false;
		public var widescreen:Boolean = false;
		public var hidden:Boolean = false;
		public var saved:Boolean = false;
		
		public var hold:Boolean = false;
		public var holdType:int = 0;
		
		public var papyrusWaiting:Boolean = false;
		
		public static const NAME_MAIN:String = "Main";
		
		public static const STATE_MAIN = 1;
		public static const STATE_FOLDER = 2;
		public static const STATE_ENTRY = 3;
		
		public static const PAPYRUS_GET = 1;
		public static const PAPYURS_SET = 2;
		public static const PAPYRUS_REFRESH = 3;
		
		public function ScreenArcherMenu()
		{
			super();
			
			Util.debug = true;
			widescreen = false;
			
			this.BGSCodeObj = new Object();
			Extensions.enabled = true;
			Translator.Create(root);
			
			initButtonHints();
			initSliderFuncs();
			
			filenameInput.visible = false;
			sliderList.bUseShadedBackground = false;
			ButtonHintBar_mc.bUseShadedBackground = true;
			ButtonHintBar_mc.BackgroundAlpha = 0.05;
			ButtonHintBar_mc.ShadedBackgroundMethod = "Flash";
			notification.visible = false;
			
			state = STATE_MAIN;
			stateStack = [];
			
			currentState = new MenuState(NAME_MAIN, 0, 0, 0);
			
//			if (Util.debug) {
//				menuName = "Main";
//				PushMenu(NAME_MAIN);
//			}
			
			updateAlignment();
		}
		
		internal function initSliderFuncs():void
		{
			//Define callback functions for the slider list entries to avoid generating events
			var functions:EntryFunctions = new EntryFunctions();
			
			functions.list = this.selectList;
			functions.valueInt = this.selectInt;
			functions.valueFloat = this.selectFloat;
			functions.checkbox = this.selectCheckbox;
			functions.checkbox2 = this.selectCheckbox2;

			sliderList.initEntryFunctions(functions);
		}
		
		internal function initButtonHints():void
		{
			buttonHintData = new Vector.<BSButtonHintData>();
			buttonHintBack = new BSButtonHintData("$SAM_Back","Tab","PSN_B","Xenon_B",1,backButton);
			buttonHintSave = new BSButtonHintData("$SAM_Save","Q","PSN_L1","Xenon_L1",1,saveButton);
			buttonHintLoad = new BSButtonHintData("$SAM_Load","E","PSN_R1","Xenon_R1",1,loadButton);
			buttonHintReset = new BSButtonHintData("$SAM_Reset","R","PSN_Y","Xenon_Y",1,resetButton);
			buttonHintConfirm = new BSButtonHintData("$SAM_Confirm","Enter","PSN_A","Xenon_A",1,confirmButton);
			buttonHintHide = new BSButtonHintData("$SAM_Hide","F","PSN_Select","Xenon_Select",1,hideButton);
			buttonHintExtra = new BSButtonHintData("","X","PSN_X","Xenon_X",1,extraButton);
			buttonHintData.push(buttonHintExit);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintHide);
			buttonHintData.push(buttonHintSave);
			buttonHintData.push(buttonHintLoad);
			buttonHintData.push(buttonHintExtra);
			buttonHintData.push(buttonHintReset);
			buttonHintData.push(buttonHintConfirm);
			ButtonHintBar_mc.SetButtonHintData(buttonHintData);
		}
		
		public function menuOpened(data:Object)
		{
			this.sam = root.f4se.plugins.ScreenArcherMenu;
			Data.load(data, this.sam, this.f4seObj, stage);
			
			if (data.title) {
				updateTitle(data.title);
			}

			var alignment:Boolean = false;
			if (data.swap) {
				swapped = data.swap;
				alignment = true;
			}
			if (data.widescreen) {
				widescreen = data.widescreen;
				alignment = true;
			}
			if (alignment) {
				updateAlignment();
			}

			this.rootMenu = data.menuName;
			Data.menuName = data.menuName;
			
			if (data.saved) {
				this.state = data.saved.state;
				currentState = data.saved.current;
				stateStack = data.saved.stack;
				sliderList.focused = data.saved.focused;
				sliderList.updateState(currentState.pos);
				ReloadMenu();
				sliderList.updateSelected(currentState.x, currentState.y);
			} else {
				//push new menu
				PushMenu(Data.menuName);
			}
			
			Util.playOk();
		}
		
		public function consoleRefUpdated(data:Object)
		{
			if (data.updated) {
				
				var isUpdated:Boolean = false;
				
				if (Data.menuData.consoleupdate) {
					var result:GFxResult = GetResult(Data.menuData);
					if (result) {
						LoadMenu(Data.menuName, Data.menuData, result);
						isUpdated = true;
					}
				}
				
				if (!isUpdated) {
					resetState();
				}

				Util.playOk();
			}

			updateTitle(data.title);
		}
		
		public function onF4SEObjCreated(obj:Object)
		{
			this.f4seObj = obj;
		}
		
//		public function onPlatformChange(event:PlatformChangeEvent)
//		{
//			switch (event.uiPlatform) {
//				case PlatformChangeEvent.PLATFORM_PC_KB_MOUSE:
//					break;
//				case PlatformChangeEvent.PLATFORM_PC_GAMEPAD:
//					break;
//			}
//		}
		
//		public function onKeyDown(event:KeyboardEvent) 
//		{
//			processKeyDown(event.keyCode);
//		}
//		
//		public function onKeyUp(event:KeyboardEvent)
//		{
//			processKeyUp(event.keyCode);
//		}

		public function processKeyDown(keyCode:uint)
		{
			//https://www.creationkit.com/fallout4/index.php?title=DirectX_Scan_Codes
			if (this.isWaiting() || hold)
				return

			switch (keyCode)
			{
				case 9://Tab
				case 277://Pad B
					if (buttonHintBack.ButtonVisible) {
						backButton();
					}
					break;
				case 13://Enter
				case 276://Pad A
					if (buttonHintConfirm.ButtonVisible) {
						confirmButton();
					} else {
						sliderList.processInput(SliderList.A);
					}
					Util.unselectText();
					break;
				case 69://E
				case 273://Pad R1
				case 275://Pad R2
					if (buttonHintLoad.ButtonVisible) {
						loadButton();
					} 
//					else if (buttonHintTarget.ButtonVisible) {
//						targetButton();
//					}
					break;
				case 81://Q
				case 272://Pad L1
				case 274://Pad L2
					if (buttonHintSave.ButtonVisible) {
						saveButton();
					}
					break;
				case 82://R
				case 279://Pad Y
					if (buttonHintReset.ButtonVisible) {
						resetButton();
					}
					break;
				case 88://X
				case 278://Pad X
					if (buttonHintExtra.ButtonVisible) {
						extraButton();
					}
					break;
				case 70://F
				case 271://Pad Select
					if (buttonHintHide.ButtonVisible) {
						hideButton();
					}
					break;
//				case 257://Mouse2
//					hide();
//					break;
//				case 282://ScrollUp
//					if (Data.isLocked(Data.KEYBOARD)) {
//						sliderList.scrollList(-1);
//					}
//					break;
//				case 283://ScrollDown
//					if (Data.isLocked(Data.KEYBOARD)) {
//						sliderList.scrollList(1);
//					}
//					break;
			}
		};
		
		public function processKeyHold(keyCode:uint)
		{
			switch (keyCode)
			{
				case 37://Left
				case 65://A
				case 268://Pad Left
					onHoldStep(false);
					break;
				case 39://Right
				case 68://D
				case 269://Pad Right
					onHoldStep(true);
					break;
			}
		}
		
		public function processKeyRepeat(keyCode:uint)
		{
			//block while waiting for latent functions
			if (isWaiting())
				return;
			
			if (hold) {
				processKeyHold(keyCode);
				return;
			}
			
			//block inputs during text selection
			if (Data.selectedText != null)
				return;
				
			switch (keyCode) {
				case 37://Left
				case 65://A
				case 268://Pad Left
					if (sliderList.visible) {
						sliderList.processInput(SliderList.LEFT);
					}
					break;
				case 38://Up
				case 87://W
				case 266://Pad Up
					if (sliderList.visible) {
						sliderList.processInput(SliderList.UP);
					}
					break;
				case 39://Right
				case 68://D
				case 269://Pad Right
					if (sliderList.visible) {
						sliderList.processInput(SliderList.RIGHT);
					}
					break;
				case 40://Down
				case 83://S
				case 267://Pad Down
					if (sliderList.visible)
					{
						sliderList.processInput(SliderList.DOWN);
					}
					break;
			}
		}
		
		public function processKeyUp(keyCode:uint)
		{
			switch (keyCode)
			{
				case 69://E
				case 273://Pad R1
				case 275://Pad R2
					disableHold(Data.BUTTON_LOAD);
					break;
					
				case 81://Q
				case 272://Pad L1
				case 274://Pad L2
					disableHold(Data.BUTTON_SAVE);
					break;
					
				case 82://R
				case 279://Pad Y
					disableHold(Data.BUTTON_RESET);
					break;
					
				case 88://X
				case 278://Pad X
					disableHold(Data.BUTTON_EXTRA);
					break;
				
//				case 257://Mouse2
//					show();
//					break;
			}
		}
		
		public function showNotification(msg:String)
		{
			if (msg && msg.length > 0) {
				notification.visible = true;
				notification.message.text = msg;
			} else {
				notification.visible = false;
			}
		}
		
		public function hideNotification()
		{
			notification.visible = false;
		}
		
		public function setFolder(data:Object)
		{
			var folderResult:GFxResult = Data.getFolder(data.path, data.ext);
			if (!CheckError(folderResult))
				return;
				
			loadFolder(data, folderResult);
		}
		
		public function loadFolder(data:Object, result:GFxResult):void
		{
			Data.setFolder(data, folderResult.result);
			
			state = STATE_FOLDER;
			stateStack.push(GetState());
			sliderList.updateState(0);
			sliderList.update();
			sliderList.updateSelected(0, 0);
			
			updateMenus();
		}
		
		public function loadMenuFolder(data:Object, result:GFxResult, name:String):void
		{
			Data.menuData = data;
			Data.menuName = name;
			loadFolder(data.folder, result);
		}
		
		public function selectFolder(i:int)
		{
			var type:int = Data.getType(i);
			if (type == Data.ITEM_LIST) {
				
				var path:String = Data.getFolderPath(i);
				var selectResult:GFxResult = callDataFunction(Data.folderData.func, [path]);
				
				if (!CheckError(selectResult))
					return;
				
				if (Data.folderData.pop)
					clearFolder();
			} else if (type == Data.ITEM_FOLDER) {
				
				var folderResult:GFxResult = Data.getFolder(Data.menuFolder[i].path, Data.folderData.ext);
				
				if (!CheckError(folderResult))
					return;
					
				Data.pushFolder(i, folderResult.result);
				
				stateStack.push(GetState());
				sliderList.updateState(0);
				sliderList.update();
				sliderList.updateSelected(0, 0);
				
				updateMenus();
			}
		}
		
		public function popFolder() 
		{
			var folderPath:String = Data.folderStack.pop();
			var folderResult:GFxResult = Data.getFolder(folderPath, Data.folderData.ext, true);
			Data.popFolder(folderResult.result);
			
			currentState = stateStack.pop();
			sliderList.updateState(currentState.pos);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);
			
			updateMenus();
		}
		
//		public function pushFolderCheckbox(id:int, func:Function)
//		{
//			stateStack.push(getState());
//			sliderList.updateState(0);
//			if (multi) {
//				sliderList.updateFolderCheckbox(func);
//			} else {
//				sliderList.updateFolder(func);
//			}
//			sliderList.updateSelected(0, 0);
//		}
//		
//		public function popFolderCheckbox(func:Function)
//		{
//			currentState = stateStack.pop();
//			sliderList.updateState(currentState.pos);
//			if (multi) {
//				sliderList.updateFolderCheckbox(func);
//			} else {
//				sliderList.updateFolder(func);
//			}
//			sliderList.updateSelected(currentState.x, currentState.y);
//		}
		
		public function clearFolder()
		{
			//go to first folder then pop to get current state
			stateStack.length = stateStack.length - Data.folderStack.length;
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			PopMenu();
		}
		
		public function CheckError(result:GFxResult):Boolean
		{
			//no result
			if (!result) {
				showNotification(Data.error)
				return false;
			}
			
			//show error message
			if (result.type == Data.RESULT_ERROR) {
				showNotification(result.result);
				return false;
			}
			
			//good result
			return true;
		}
		
		public function CheckWait(menuResult:GFxResult, getResult:GFxResult, type:int):Boolean
		{
			if (getResult.type != Data.RESULT_WAITING)
				return true;
				
			Data.setPapyrusWaiting(menuResult.result, type);
			
			Data.papyrusTimer.Timer = new Timer(menuResult.result.get.timeout, 1);
			Data.papyrusTimer.addEventListener(TimerEvent.TIMER_COMPLETE, function(e:TimerEvent) 
			{
				showNotification("$SAM_PapyrusTimeout");
				Util.playCancel();
				Data.clearPapyrusWaiting();
			});
			
			Data.papyrusTimer.start();

			return false;
		}
		
		public function GetState():MenuState
		{
			return new MenuState(Data.menuName, sliderList.listPosition, sliderList.selectedX, sliderList.selectedY);
		}
		
		public function PushMenu(name:String)
		{
			var menuResult:GFxResult = Data.getMenu(name);
			
			if (!CheckError(menuResult))
				return;
				
			var getResult:GFxResult = GetResult(menuResult.result);
			
			if (!CheckError(getResult))
				return;
				
			if (!CheckWait(menuResult, getResult, PAPYRUS_GET))
				return;
				
//			trace("menuResult");
//			Util.traceObj(menuResult.result);
//			trace("getResult");
//			Util.traceObj(getResult.result);
				
			//Check if result is a folder and load that instead
			if (getResult.type == Data.MENU_FOLDER) {
				loadMenuFolder(menuResult.result, getResult, name);
				return;
			}
			
			stateStack.push(GetState());
			currentState.x = 0;
			currentState.y = 0;
			sliderList.updateState(0);
			
			LoadMenu(name, menuResult.result, getResult);
		}
		
		public function PopMenu():void
		{
			//entry
			if (this.filenameInput.visible)
			{
				clearEntry();
				return;
			}
			//folder
			else if (Data.folderStack.length > 0) {
				popFolder();
				return;
			}
			//exit
			else if (this.stateStack.length == 1)
			{
				exit();
				return;
			}

			currentState = stateStack.pop();
			
			var menuResult:GFxResult = Data.getMenu(currentState.menu, true);
			
			if (!CheckError(menuResult))
				return;
			
			var getResult:GFxResult = GetResult(menuResult.result);
			
			if (!CheckError(getResult))
				return;
			
			Data.menuFolder = null;
			sliderList.updateState(currentState.pos);
			LoadMenu(currentState.menu, menuResult.result, getResult);
		}
		
		public function PopMenuTo(name:String):void
		{
			//do not pop from main
			if (stateStack.length < 2)
				return;
				
			//find stack index
			var index:int = stateStack.length - 2;
			while (index >= 0) {
				var popState:MenuState = stateStack[index];
				if (popState.menu == name)
					break;
				index--;
			}
			
			//menu not found
			if (index < 0)
				return;

			stateStack.length = stateStack.length - Data.folderStack.length;
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			
			//go to menu above index and pop
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			stateStack.length = index + 1;
			if (filenameInput.visible) {
				setTextInput(false);
			}
			
			PopMenu();
		}
		
		public function ResetState():void
		{
			state = STATE_MAIN;
			currentState = new MenuState(NAME_MAIN, 0, 0, 0);
			stateStack.length = 0;
			Data.folderStack.length = 0;

			if (filenameInput.visible) {
				setTextInput(false);
			}
			
			PushMenu(NAME_MAIN);
		}
		
		public function LoadMenu(name:String, data:Object, get:GFxResult):void
		{
			//leave previous menu
			if (Data.menuData && Data.menuData.leave) {
				callDataFunction(Data.menuData.leave);
			}
			
			//enter new menu
			if (data.enter) {
				callDataFunction(data.enter);
			}
			
			Data.updateMenu(name, data, get);
	
			state = STATE_MAIN;
			Util.unselectText();
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);
			updateMenus();
		}
		
		public function updateMenus()
		{
			updateButtonHints();
			updateNotification();
			updateTitle();
		}
		
		public function GetResult(menu:Object):GFxResult
		{
			switch (menu.type) {
				case Data.MENU_MAIN:
					return Data.resultSuccess;
				case Data.MENU_MIXED:
				case Data.MENU_CHECKBOX:
				case Data.MENU_SLIDERS:
					return callDataFunction(menu.get);
				case Data.MENU_LIST:
					//optional
					if (menu.get) {
						return callDataFunction(menu.get);
					} else {
						return Data.resultSuccess;
					}
				case Data.MENU_FOLDER:
					return Data.getFolder(menu.folder.path, menu.folder.ext);
			}
			
			return null;
		}
		
		public function CallSet(... args)
		{

			var data:Object = GetSetData(args);

			if (!data)
				return;

			var result:GFxResult = callDataFunction(data, args);

			if (!CheckError(result))
				return;
				
			if (data.pop) {
				PopMenu();
			} else if (data.popto) {
				PopMenuTo(data.popto);
			} else if (Data.menuData.refresh) {
				RefreshValues();
			}
		}
		
		public function RefreshValues() {
			var result:GFxResult = GetResult(Data.menuData);
				
			if (!CheckError(result))
				return;
				
			if (!CheckWait(Data.menuData, result, PAPYRUS_REFRESH))
				return;
				
			Data.updateValues(result);
			sliderList.updateValues();
		}
		
		public function ReloadMenu() {
			var result:GFxResult = GetResult(Data.menuData);
			
			if (!CheckError(result))
				return;
				
			if (!CheckWait(Data.menuData, result, PAPYRUS_GET))
				return;
			
			sliderList.storeSelected();
			Data.updateMenu(Data.menuName, Data.menuData, result);
			sliderList.update();
			sliderList.restoreSelected();
		}
		
		public function GetSetData(args:Array):Object
		{
			switch (Data.menuType) {
				case Data.MENU_MIXED:
					if (Data.menuData.items[args[0]].func)
						return Data.menuData.items[args[0]].func;
					break;
				case Data.MENU_LIST:
					if (Data.menuData.list)
						if (Data.menuData.list[args[0]].func)
							return Data.menuData.list[args[0]].func;
					break;
			}
			
			if (Data.menuData.set)
				return Data.menuData.set;
				
			return null;
		}
		
		public function selectList(i:int) 
		{
			switch (Data.menuType) {
				case Data.MENU_LIST:
				case Data.MENU_MIXED:
				case Data.MENU_ADJUSTMENT:
					CallSet(i, Data.menuValues[i]);
					break;
				case Data.MENU_MAIN:
					PushMenu(Data.menuData.values[i])
					break;
				case Data.MENU_FOLDER:
					selectFolder(i);
					break;
				case Data.MENU_FOLDERCHECKBOX:
					selectFolderCheckbox(i);
					break;
			}
		}
		
		public function selectInt(i:int, value:int)
		{
			Data.menuValues[i] = value;
			CallSet(i, value);
		}
		
		public function selectFloat(i:int, value:Number)
		{
			Data.menuValues[i] = value;
			CallSet(i, value);
		}
		
		public function selectCheckbox(i:int, checked:Boolean = false)
		{
			switch (Data.menuType) 
			{
				case Data.MENU_CHECKBOX:
				case Data.MENU_FOLDERCHECKBOX:
					Data.menuValues[i] = checked;
					CallSet(i, checked);
					break;
				case Data.MENU_ADJUSTMENT:
					CallSet(i);
					break;
				case Data.MENU_MIXED: 
					switch(Data.getType(i)) 
					{
						case Data.ITEM_CHECKBOX:
							CallSet(i, checked);
							break;
					}
					break;
				case Data.MENU_ADJUSTMENT:
					if (Data.locals.order) {
						callDatafunction(menuData.adjustment.down, [i]);
					} else {
						callDataFunction(menuData.adjustment.edit, [i, Data.menuValues[i]]);
					}
					break;
			}
		}
		
		public function selectCheckbox2(i:int, checked:Boolean = false)
		{
			if (Data.locals.order) {
				callDataFunction(menuData.adjustment.up, [i]);
			} else {
				callDataFunction(menuData.adjustment.remove, [i, Data.menuValues[i]]);
				ReloadMenu();
			}
		}
		
		//Prevent user input when waiting for return values
		public function isWaiting():Boolean {
			return Data.papyrusWaiting;
		}

		public function PapyrusResult(result:GFxResult):void
		{
			if (Data.papyrusWaiting)
			{
				if (!CheckError(result)) {
					Util.playCancel();
					Data.clearPapyrusWaiting();
					return;
				}
				
				switch (Data.papyrusType) {
					case PAPYRUS_GET:
						stateStack.push(GetState());
						currentState.x = 0;
						currentState.y = 0;
						sliderList.updateState(0);
				
						LoadMenu(name, Data.papyrusMenuData, result);
						break;
						
					case PAPYRUS_REFRESH:
						sliderList.updateValues();
						break;
						
					//case PAPYRUS_SET: //ignore
				}
				
				Data.clearPapyrusWaiting();
			}
		}
		
		public function callDataFunction(data:Object, args:Array = null):GFxResult
		{
			if (data["var"]) {
				if (data.all) {
					Data.locals[data["var"]] = Util.shallowCopyArray(Data.menuValues);
				} else {
					if (!args) {
						Data.error = "No index found to store var " + data["var"];
						return null;
					}
					Data.locals[data["var"]] = Data.menuValues[args[0]];
				}
			}

			switch (data.type) {
				case Data.FUNC_GLOBAL:
					if (!data.script) {
						Data.error = "Papyrus function missing script name";
						return null;
					}
					
					if (!data.func) { 
						Data.error = "Papyrus function missing function name";
						return null;
					}
					
					if (!args) {
						args = [];
					}
					
					args.push(data.script);
					args.push(data.func);
					break;
				case Data.FUNC_MENU:
					PushMenu(data.name);
					return Data.resultSuccess;
			}

			if (data.args) {
				if (!args) {
					args = [];
				}
				
				for (var i:int = 0; i < args.length; i++) {
					switch (data.args[i].type) {
						case Data.ARGS_VAR:
							var property:String = data.args[i].name;
							//need to use hasOwnProperty because of falsy values
							if (Data.locals.hasOwnProperty(property)) {
								args.push(Data.locals[property]);
							}
							break;
						case Data.ARGS_INDEX:
							args.push(menuValues[data.args[i]]);
							break;
						case Data.ARGS_VALUE:
							args.push(data.args[i].value)
							break;
					}
				}
			}

			switch (data.type)
			{
				case Data.FUNC_SAM: return callSamFunction(data.name, args);
				case Data.FUNC_LOCAL: return callLocalFunction(data.name, args);
				case Data.FUNC_FORM: return callPapyrusForm(data, args);
				case Data.FUNC_GLOBAL: return callPapyrusGlobal(args, data.wait);
				case Data.FUNC_ENTRY: 
				{
					//Store data and args and open text entry
					Data.entryData = data.entry.func;
					Data.entryArgs = args;
					
					setTextInput(true);
					updateMenus();

					return Data.resultSuccess;
				}
				case Data.FUNC_FOLDER:
				{
					setFolder(data.folder);

					return Data.resultSuccess;
				}
			}
			
			Data.error = "Could not resolve function type";
			return null;
		}
		
		public function callSamFunction(name:String, args:Array):GFxResult 
		{
			if (this.sam[name])
				return Util.callFuncArgs(this.sam[name], args);
			else if (Util.debug)
				return Data.getSamDebugFunction(name);
			
			Data.error = "$SAM_SamFunctionMissing";
			return null;
		}
		
		public function callLocalFunction(name:String, args:Array) {
			if (this[name])
				return Util.callFuncArgs(this[name], args);
			else if (Util.debug)
				return getLocalDebugFunction(name);

			Data.error = "$SAM_LocalFunctionMissing";
			return null;
		}
		
		//Calls a papyrus global function
		public function callPapyrusForm(data:Object, args:Array):GFxResult
		{
			try {
				return this.sam.CallPapyrusForm(data.id, data.func, args);
			}
			catch (e:Error) {}
			
			Data.error = "$SAM_PapyrusTimeout";
			return null;
		}
		
		public function callPapyrusGlobal(args:Array, timeout:int):GFxResult
		{
			try {
				return Util.callFuncArgs(this.f4seObj.CallGlobalFunctionNoWait, args); 			
			} 
			catch (e:Error) {}
			
			Data.error = "$SAM_PapyrusTimeout";
			return null;
		}
		
		public function getLocalDebugFunction(name:String):Object
		{
			return null;
		}
		
//		internal function updateState()
//		{
//			
//			switch(this.state)
//			{
//				case MAIN_STATE:
//					Data.menuOptions = Data.MAIN_MENU;
//					sliderList.updateList(selectMenu);
//					break;
//				case ADJUSTMENT_STATE:
//					Data.loadAdjustmentList();
//					sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
//					break;
//				case EDITADJUSTMENT_STATE:
//					Data.editAdjustment();
//					sliderList.updateAdjustmentEdit(selectEditAdjustment);
//					break;
//				case POSECATEGORY_STATE:
//					Data.loadCategories();
//					sliderList.updateList(selectCategory);
//					break;
//				case POSENODE_STATE:
//					Data.loadBones();
//					sliderList.updateList(selectBone);
//					break;
//				case TRANSFORM_STATE:
//					Data.loadTransforms();
//					sliderList.updateTransform(selectTransform);
//					break;
//				case MORPHCATEGORY_STATE:
//					Data.loadMorphCategories();
//					sliderList.updateList(selectMorphCategory);
//					break;
//				case MORPH_STATE:
//					Data.loadMorphs();
//					sliderList.updateMorphs(selectMorph);
//					break;
//				case LOADADJUSTMENT_STATE:
//					Data.loadSubFolder(Data.ADJUSTMENT_DIRECTORY, Data.JSON_EXT);
//					sliderList.updateFolder(selectAdjustmentFile);
//					break;
//				case LOADMFG_STATE:
//					Data.loadSubFolder(Data.MORPH_DIRECTORY, Data.TXT_EXT);
//					sliderList.updateFolder(selectMfgFile);
//					break;
//				case SAVEMFG_STATE:
//				case SAVEADJUSTMENT_STATE:
//				case SAVEPOSE_STATE:
//				case RENAMEADJUSTMENT_STATE:
//				case SAVELIGHT_STATE:
//				case RENAMELIGHT_STATE:
//					setTextInput(true);
//					break;
//				case EYE_STATE:
//					Data.loadEyes();
//					sliderList.updateEyes(selectEye);
//					break;
//				case HACK_STATE:
//					Data.loadHacks();
//					Data.menuOptions = Data.HACK_NAMES;
//					sliderList.updateCheckboxes(selectHack);
//					break;
//				case IDLECATEGORY_STATE:
//					Data.loadIdleCategories();
//					sliderList.updateList(selectIdleCategory);
//					break;
//				case IDLE_STATE:
//					Data.loadIdles();
//					sliderList.updateList(selectIdle);
//					break;
//				case POSEEXPORT_STATE:
//					Data.getPoseList();
//					sliderList.updateCheckboxes(selectPose);
//					break;
//				case POSEEXPORTTYPE_STATE:
//					Data.loadPoseExport();
//					sliderList.updateList(selectExportType);
//					break;
//				case POSEPLAY_STATE:
//				case LOADPOSE_STATE:
//					Data.loadSubFolder(Data.POSE_DIRECTORY, Data.JSON_EXT);
//					sliderList.updateFolder(selectPoseFile);
//					break;
//				case SKELETONADJUSTMENT_STATE:
//					Data.getAdjustmentFolder(false);
//					updateMulti(false);
//					break;
//				case RACEADJUSTMENT_STATE:
//					Data.getAdjustmentFolder(true);
//					updateMulti(true);
//					break;
//				case POSITIONING_STATE:
//					Data.loadPositioning();
//					sliderList.updatePositioning(selectPositioning);
//					break;
//				case OPTIONS_STATE:
//					Data.loadOptions();
//					sliderList.updateCheckboxes(selectOptions);
//					break;
//				case CAMERA_STATE:
//					Data.loadCamera();
//					sliderList.updateCamera(selectCamera);
//					break;
//				case LIGHTSELECT_STATE:
//					Data.loadLightSelect();
//					sliderList.updateList(selectLightSelect);
//					break;
//				case LIGHTEDIT_STATE:
//					Data.loadLightEdit();
//					sliderList.updateLight(selectLightEdit);
//					break;
//				case LIGHTCATEGORY_STATE:
//				case LIGHTSWAPCATEGORY_STATE:
//					Data.loadLightCategory();
//					sliderList.updateList(selectLightCategory);
//					break;
//				case LIGHTOBJECT_STATE:
//				case LIGHTSWAPOBJECT_STATE:
//					Data.loadLightObject();
//					sliderList.updateList(selectLightObject);
//					break;
//				case LOADLIGHT_STATE:
//					Data.loadSubFolder(Data.LIGHT_DIRECTORY, Data.JSON_EXT);
//					sliderList.updateFolder(selectLightFile);
//					break;
//				case LIGHTSETTINGS_STATE:
//					Data.loadLightSettings();
//					sliderList.updateLightSettings(selectLightSettings);
//					break;
//				case MORPHTONGUE_STATE:
//					Data.getMorphsTongueNodes();
//					sliderList.updateList(selectMorphTongue);
//					break;
//			}
//
//		}
		
//		public function updateMulti(race:Boolean):void
//		{
//			var func:Function = (race ? selectRaceAdjustment : selectSkeletonAdjustment);
//			if (multi) {
//				sliderList.updateFolderCheckbox(func);
//			} else {
//				sliderList.updateFolder(func);
//			}
//		}
		
//		public function selectAdjustment(id:int):void
//		{
//			Data.selectedAdjustment = Data.menuValues[id];
//			pushState(POSECATEGORY_STATE);
//		}
//		
//		public function editAdjustment(id:int):void
//		{
//			Data.selectedAdjustment = Data.menuValues[id];
//			pushState(EDITADJUSTMENT_STATE);
//		}
//		
//		public function selectEditAdjustment(id:int, value:Object = null):void
//		{
//			switch (id)
//			{
//				case 0:
//					Data.menuValues[id] = value;
//					Data.setAdjustmentScale(value);
//					break;
//				case 1:
//					pushState(SAVEADJUSTMENT_STATE);
//					break;
//				case 2:
//					pushState(RENAMEADJUSTMENT_STATE);
//					break;
//				case 3:
//					Data.resetAdjustment();
//					break;
//				default:
//					Data.negateAdjustmentGroup(id);
//			}
//		}
		
//		public function removeAdjustment(id:int):void
//		{
//			Data.removeAdjustment(Data.menuValues[id]);
//			Data.loadAdjustmentList();
//			sliderList.storeSelected();
//			sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
//			sliderList.restoreSelected();
//		}
		
		public function MoveAdjustmentDown(id:int):void
		{
			if (Data.moveAdjustment(id, true)) {
				var result:GFxResult = callDataFunction(Data.menuData.get);
				if (result && result.type != Data.RESULT_ERROR) {
					Data.updateMenu(Data.menuName, Data.menuData, result);
					sliderList.storeSelected();
					sliderList.update();
					if (sliderList.focused) {
						sliderList.storeY++;
					}
					sliderList.restoreSelected();
				}
			}
		}
		
		public function MoveAdjustmentUp(id:int):void
		{
			if (Data.moveAdjustment(id, false)) {
				var result:GFxResult = callDataFunction(Data.menuData.get);
				if (result && result.type != Data.RESULT_ERROR) {
					Data.updateMenu(Data.menuName, Data.menuData, result);
					sliderList.storeSelected();
					sliderList.update();
					if (sliderList.focused) {
						sliderList.storeY--;
					}
					sliderList.restoreSelected();
				}
			}
		}
		
//		public function selectCategory(id:int):void
//		{
//			Data.selectedCategory = Data.menuValues[id];
//			pushState(POSENODE_STATE);
//		}
//		
//		public function selectBone(id:int):void
//		{
//			Data.selectedBone = Data.menuValues[id];
//			Data.getNodeName();
//			offset = false;
//			pushState(TRANSFORM_STATE);
//		}
//		
//		public function selectTransform(id:int, value:Number):void
//		{
//			Data.setTransform(id, value);
//			if (id >= 7) {
//				sliderList.updateValues();
//			}
//		}
//		
//		public function selectMorphCategory(id:int):void
//		{
//			Data.selectedCategory = id;
//
//			//If index less than 0 then it's not a tongue
//			if (Data.menuValues[id] < 0) {
//				pushState(MORPH_STATE);
//			} else {
//				Data.selectedCategory = Data.menuValues[id];
//				pushState(MORPHTONGUE_STATE);
//			}
//		}
//		
//		public function selectMorph(id:int, value:Number):void
//		{
//			Data.setMorph(id, int(value));
//		}
//		
//		public function selectEye(id:int, value:Object):void
//		{
//			Data.setEye(id, value);
//		}
//		
//		public function selectHack(id:int, enabled:Boolean):void
//		{
//			Data.setHack(id, enabled);
//		}
//		
//		public function selectPositioning(id:int, value:Number = 0):void
//		{
//			Data.selectPositioning(id, value);
//			if (id > 0) {
//				sliderList.updateValues();
//			}
//		}
		
		internal function confirmButton():void
		{
			if (this.filenameInput.visible) 
			{
				var result:GFxResult = callDataFunction(Data.entryData, Data.entryArgs);
				CheckError(result);
				
				clearEntry();
				Util.playOk();
			}
		}
		
		internal function clearEntry():void
		{
			setTextInput(false);
			Data.entryData = null;
			Data.entryArgs = null;
			updateMenus();
		}

//		internal function confirmButton():void
//		{
//			switch (this.state) {
//				case SAVEMFG_STATE: Data.saveMfg(filenameInput.Input_tf.text); break;
//				case SAVEADJUSTMENT_STATE: Data.saveAdjustment(filenameInput.Input_tf.text); break;
//				case SAVEPOSE_STATE: Data.savePose(filenameInput.Input_tf.text); break;
//				case RENAMEADJUSTMENT_STATE: Data.renameAdjustment(filenameInput.Input_tf.text); break;
//				case SAVELIGHT_STATE: Data.saveLights(filenameInput.Input_tf.text); break;
//				case RENAMELIGHT_STATE: Data.renameLight(filenameInput.Input_tf.text); break;
//			}
//			setTextInput(false);
//			PopMenu();
//			Util.playOk();
//		}
//		
		internal function setTextInput(enabled:Boolean)
		{
			if (enabled)
			{
				state = STATE_ENTRY;
				filenameInput.visible = true;
				sliderList.visible = false;
				filenameInput.Input_tf.type = TextFieldType.INPUT;
				filenameInput.Input_tf.selectable = true;
				filenameInput.Input_tf.maxChars = 100;
				stage.focus = filenameInput.Input_tf;
				filenameInput.Input_tf.setSelection(0, filenameInput.Input_tf.text.length);
				allowTextInput(true);
			}
			else
			{
				state = STATE_MAIN;
				filenameInput.Input_tf.text = "";
				filenameInput.Input_tf.type = TextFieldType.DYNAMIC;
				filenameInput.Input_tf.setSelection(0,0);
				filenameInput.Input_tf.selectable = false;
				filenameInput.Input_tf.maxChars = 0;
				allowTextInput(false);
				filenameInput.visible = false;
				sliderList.visible = true;
				stage.focus = sliderList;
			}
		}

//		internal function saveButton():void
//		{
//			switch (this.state) {
//				case MORPH_STATE:
//				case MORPHCATEGORY_STATE:
//					pushState(SAVEMFG_STATE); break;
//				case ADJUSTMENT_STATE: pushState(SAVEADJUSTMENT_STATE); break;
//				case POSEEXPORT_STATE: pushState(SAVEPOSE_STATE); break;
//				case LIGHTSELECT_STATE: pushState(SAVELIGHT_STATE); break;
//			}
//		}
//
//		internal function loadButton():void
//		{
//			switch (this.state) {
//				case MORPH_STATE: 
//				case MORPHCATEGORY_STATE:
//					pushState(LOADMFG_STATE); break;
//				case ADJUSTMENT_STATE: pushState(LOADADJUSTMENT_STATE); break;
//				case POSEEXPORT_STATE: pushState(LOADPOSE_STATE); break;
//				case LIGHTSELECT_STATE: pushState(LOADLIGHT_STATE); break;
//				case POSEPLAY_STATE: //a-pose
//					Data.resetPose(2);
//					break;
//				case TRANSFORM_STATE: //offset toggle
//					//only toggle if node isn't offset only
//					if (!Data.getNodeIsOffset()) {
//						if (Data.toggleNodeName()) {
//							offset = !offset;
//							buttonHintLoad.ButtonText = offset ? "$SAM_Pose" : "$SAM_Offset";
//							Data.loadTransforms();
//							sliderList.updateValues();
//							updateTitle();
//							Util.playOk();
//						}
//					}
//					break;
//			}
//		}
		
//		internal function selectIdleCategory(id:int)
//		{
//			Data.selectedCategory = id;
//			pushState(IDLE_STATE);
//		}
//		
//		internal function selectIdle(id:int)
//		{
//			Data.playIdle(id);
//			showNotification(Data.menuOptions[id]);
//		}
//		
//		internal function selectPose(id:int, enabled:Boolean)
//		{
//			Data.menuValues[id] = enabled;
//		}
//		
//		internal function selectPoseExport(id:int)
//		{
//			Data.selectedCategory = id;
//			pushState(POSEEXPORT_STATE);
//		}
//		
//		internal function selectSkeletonAdjustment(id:int, enabled:Boolean = true)
//		{
//			if (Data.selectSkeletonFile(id, false, !multi, enabled)) {
//				pushFolderCheckbox(id, selectSkeletonAdjustment);
//			} else {
//				Data.getAdjustmentFolder(false);
//				updateMulti(false);
//			}
//		}
//		
//		internal function selectRaceAdjustment(id:int, enabled:Boolean = true)
//		{
//			if (Data.selectSkeletonFile(id, true, !multi, enabled)) {
//				pushFolderCheckbox(id, selectRaceAdjustment);
//			} else {
//				Data.getAdjustmentFolder(true);
//				updateMulti(true);
//			}
//		}
//		
//		internal function selectPoseFile(id:int)
//		{
//			if (Data.selectSamPose(id)) {
//				pushFolder(id, selectPoseFile);
//			}
//		}
//		
//		internal function selectMfgFile(id:int)
//		{
//			if (Data.selectMfgFile(id)) {
//				pushFolder(id, selectMfgFile);
//			}
//		}
//		
//		internal function selectLightFile(id:int)
//		{
//			if (Data.selectLightFile(id)) {
//				pushFolder(id, selectLightFile);
//			} else {
//				clearFolder();
//			}
//		}
//		
//		internal function selectAdjustmentFile(id:int)
//		{
//			if (Data.selectAdjustmentFile(id)) {
//				pushFolder(id, selectAdjustmentFile);
//			} else {
//				clearFolder();
//			}
//		}
//		
//		internal function selectOptions(id:int, enabled:Boolean)
//		{
//			switch (id) {
//				case 0://hotswap
//					break;
//				case 1://alignment
//					swapped = enabled;
//					updateAlignment();
//					break;
//				case 2://widescreen
//					widescreen = enabled;
//					updateAlignment();
//					break;
//				case 3://autoplay
//					Data.autoPlay = enabled;
//					break;
//			}
//			Data.setOption(id, enabled);
//		}
//		
//		internal function selectCamera(id:int, value:Number = 0)
//		{
//			Data.setCamera(id, value);
//			Data.loadCamera();
//			sliderList.updateValues();
//		}
//		
//		internal function selectLightSelect(id:int)
//		{
//			if (id < Data.menuOptions.length - 3) { //light
//				Data.selectedAdjustment = id;
//				pushState(LIGHTEDIT_STATE);
//			} else if (id < Data.menuOptions.length - 2) { //add new
//				pushState(LIGHTCATEGORY_STATE);
//			} else if (id < Data.menuOptions.length - 1) { //add console
//				Data.addLight();
//				Data.loadLightSelect();
//				sliderList.updateList(selectLightSelect);
//			} else {
//				pushState(LIGHTSETTINGS_STATE);
//			}
//		}
//		
//		internal function selectLightEdit(id:int, value:Number = 0)
//		{
//			if (id < 5) { //properties
//				Data.editLight(id, value);
//				sliderList.updateValues();
//			} else if (id < 6) { //rename
//				pushState(RENAMELIGHT_STATE);
//			} else if (id < 7) { //swap
//				pushState(LIGHTSWAPCATEGORY_STATE);
//			} else { //delete
//				Data.deleteLight();
//				PopMenu();
//			}
//		}
//		
//		internal function selectLightCategory(id:int)
//		{
//			Data.selectedCategory = id;
//			switch (this.state) {
//				case LIGHTCATEGORY_STATE: pushState(LIGHTOBJECT_STATE); break;
//				case LIGHTSWAPCATEGORY_STATE: pushState(LIGHTSWAPOBJECT_STATE); break;
//			}
//		}
//		
//		internal function selectLightObject(id:int)
//		{
//			switch (this.state) {
//				case LIGHTOBJECT_STATE:
//					Data.createLight(id);
//					break;
//				case LIGHTSWAPOBJECT_STATE:
//					Data.swapLight(id);
//					break;
//			}
//			//need to double pop so fake pop first
//			stateStack.pop();
//			PopMenu();
//		}
//		
//		internal function selectLightSettings(id:int, value:Number = 0)
//		{
//			if (id < 4) { // pos/rot
//				Data.editLightSettings(id, value);
//				sliderList.updateValues();
//			} else if (id < 5) { //update all
//				Data.updateAllLights();
//			} else { //delete all
//				Data.deleteAllLights();
//			}
//		}
//		
//		internal function selectExportType(id:int)
//		{
//			Data.selectedCategory = id;
//			pushState(POSEEXPORT_STATE);
//		}
//		
//		internal function selectMorphTongue(id:int)
//		{
//			Data.getMorphsTongue(id);
//			offset = false;
//			pushState(TRANSFORM_STATE);
//		}
		
		public function resetButton()
		{
			callHotkeyFunction(Data.BUTTON_RESET);
		}
		
		public function extraButton()
		{
			callHotkeyFunction(Data.BUTTON_EXTRA);
		}
		
		public function saveButton()
		{
			callHotkeyFunction(Data.BUTTON_SAVE);
		}
		
		public function loadButton()
		{
			callHotkeyFunction(Data.BUTTON_LOAD);
		}
		
		public function callHotkeyFunction(type:int)
		{
			var data:Object = Data.getHotkey(type);
			if (!data)
				return;
			
			switch (data.type) {
				case Data.HOTKEY_FUNC:
					var result:GFxResult = callDataFunction(data);
					CheckError(result);
					break;
				case Data.HOTKEY_HOLD:
					enableHold(data.hold, type);
					break;
			}
		}

//		public function resetButton():void
//		{
//			switch (this.state) {
//				case ADJUSTMENT_STATE:
//					order = !order;
//					updateAdjustment();
//					break;
//				case TRANSFORM_STATE:
//					Data.resetTransform(); 
//					break;
//				case MORPH_STATE:
//				case MORPHCATEGORY_STATE:
//					var update:Boolean = (this.state == MORPH_STATE);
//					Data.resetMorphs(update);
//					break;
//				case IDLECATEGORY_STATE:
//				case IDLE_STATE:
//					Data.resetIdle();
//					hideNotification();
//					break;
//				case POSEEXPORT_STATE:
//				case POSEPLAY_STATE:
//					Data.resetIdle();
//					Data.resetPose(1);
//					break;
//				case SKELETONADJUSTMENT_STATE:
//					Data.resetSkeletonAdjustment(false);
//					Data.getAdjustmentFolder(false);
//					updateMulti(false);
//					break;
//				case RACEADJUSTMENT_STATE:
//					Data.resetSkeletonAdjustment(true);
//					Data.getAdjustmentFolder(true);
//					updateMulti(true);
//					break;
//				case POSITIONING_STATE:
//					Data.resetPositioning();
//					break;
//				case LIGHTEDIT_STATE:
//					Data.resetLight();
//					break;
//				case LIGHTSETTINGS_STATE:
//					Data.resetLightSettings();
//					break;
//			}
//			sliderList.updateValues();
//			Util.playOk();
//		}

		internal function backButton():void
		{
			PopMenu();
			Util.playCancel();
		}
//		
//		public function updateFolder()
//		{
//			switch(this.state) {
//				case SKELETONADJUSTMENT_STATE:
//					Data.popSkeletonAdjustment(false);
//					popFolderCheckbox(selectSkeletonAdjustment);
//					break;
//				case RACEADJUSTMENT_STATE:
//					Data.popSkeletonAdjustment(true);
//					popFolderCheckbox(selectRaceAdjustment);
//					break;
//				case POSEPLAY_STATE: 
//				case LOADPOSE_STATE:
//					Data.popFolder(Data.POSE_DIRECTORY, Data.JSON_EXT);
//					popFolder(selectPoseFile); 
//					break;
//				case LOADLIGHT_STATE: 
//					Data.popFolder(Data.LIGHT_DIRECTORY, Data.JSON_EXT);
//					popFolder(selectLightFile); 
//					break;
//				case LOADADJUSTMENT_STATE:
//					Data.popFolder(Data.ADJUSTMENT_DIRECTORY, Data.JSON_EXT);
//					popFolder(selectAdjustmentFile); 
//					break;
//				case LOADMFG_STATE:
//					Data.popFolder(Data.MORPH_DIRECTORY, Data.TXT_EXT);
//					popFolder(selectMfgFile); 
//					break;
//			}
//		}
		
		public function exit():void
		{
			Util.unselectText();
			if (!saved) {
				Data.clearState();
			}
//			if (Data.delayClose)
//			{
				//delay close event so it doesn't close multiple menus at once
				closeTimer = new Timer(100,1);
				closeTimer.addEventListener(TimerEvent.TIMER_COMPLETE, function(e:TimerEvent) {
					close();
				});
				closeTimer.start();
//			}
//			else
//			{
//				close();
//			}
		}
		
		public function close()
		{
			try {
				this.sam.SamCloseMenu("ScreenArcherMenu");
			}
			catch (e:Error)
			{
				trace("No escape");
			}
		}

//		public function extraButton():void
//		{
//			switch(this.state)
//			{
//				case SKELETONADJUSTMENT_STATE: //multi
//					sliderList.storeSelected();
//					multi = !multi;
//					if (multi) {
//						buttonHintExtra.ButtonText = "$SAM_Multi";
//						sliderList.updateFolderCheckbox(selectSkeletonAdjustment);
//					} 
//					else
//					{
//						buttonHintExtra.ButtonText = "$SAM_Single";
//						sliderList.updateFolder(selectSkeletonAdjustment);
//					}
//					sliderList.restoreSelected();
//					break;
//				case RACEADJUSTMENT_STATE: //multi
//					sliderList.storeSelected();
//					multi = !multi;
//					if (multi) {
//						buttonHintExtra.ButtonText = "$SAM_Multi";
//						sliderList.updateFolderCheckbox(selectRaceAdjustment);
//					} 
//					else
//					{
//						buttonHintExtra.ButtonText = "$SAM_Single";
//						sliderList.updateFolder(selectRaceAdjustment);
//					}
//					sliderList.restoreSelected();
//					break;
//				case ADJUSTMENT_STATE: //new
//					Data.newAdjustment();
//					updateAdjustment();
//					break;
//				case TRANSFORM_STATE: //negate
//					Data.negateAdjustment();
//					Data.loadTransforms();
//					sliderList.updateValues();
//					break;
//				case IDLECATEGORY_STATE: //z-rotate
//				case IDLE_STATE:
//				case POSEPLAY_STATE:
//					enableHold(HELD_X, onZMove, onZLeft, onZRight);
//					break;
//				case LIGHTSELECT_STATE: //visible
//				case LIGHTSETTINGS_STATE:
//					buttonHintExtra.ButtonText = (Data.toggleLightsVisibility() ? "$SAM_Invisible" : "$SAM_Visible");
//					break;
//				case LIGHTEDIT_STATE: //visible
//					buttonHintExtra.ButtonText = (Data.toggleLightVisible() ? "$SAM_Invisible" : "$SAM_Visible");
//					break;
//			}
//		}

		public function enableHold(data:Object, type:int)
		{
			if (!hold) {
				hold = true;
				holdType = type;
				stage.addEventListener(MouseEvent.MOUSE_MOVE, move);
				Data.holdData = data;
				Data.setCursorVisible(false);
				Data.storeCursorPos();
				sliderList.storeSelected();
			}
		}
		
		public function disableHold(type:int) {
			if (hold && holdType == type) {
				hold = false;
				stage.removeEventListener(MouseEvent.MOUSE_MOVE, move);
				Data.holdData = null;
				Data.setCursorVisible(true);
				Data.endCursorDrag();
				sliderList.restoreSelected();
			}
		}
		
		public function onHoldMove(event:MouseEvent) {
			var delta:int = Data.updateCursorDrag();
			var result:GFxResult = callDataFunction(Data.holdData, delta * mod);
			CheckError(result);
		}
		
		public function onHoldStep(inc:Boolean) {
			var result:GFxResult = callDataFunction(Data.holdData, (inc ? Data.holdData.step : -Data.holdData.step));
			CheckError(result);
		}
		
		public function allowTextInput(allow:Boolean)
		{
			try
			{
				this.f4seObj.AllowTextInput(allow);
			}
			catch (e:Error)
			{
				trace(allow ? "Allow text input failed" : "Disable text input failed");
			}
		}

		internal function updateButtonHints():void
		{
			switch (this.state) {
				case STATE_MAIN:
					buttonHintBack.ButtonText = this.stateStack.length == 1 ? "$SAM_Exit" : "$SAM_Back";
					buttonHintBack.ButtonVisible = true;
					buttonHintHide.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					
					updateHotkey(buttonHintSave, Data.BUTTON_SAVE);
					updateHotkey(buttonHintLoad, Data.BUTTON_LOAD);
					updateHotkey(buttonHintReset, Data.BUTTON_RESET);
					updateHotkey(buttonHintExtra, Data.BUTTON_EXTRA);
					break;
				case STATE_FOLDER:
					buttonHintBack.ButtonVisible = true;
					buttonHintHide.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					
					updateFolderHotkey(buttonHintSave, Data.BUTTON_SAVE);
					updateFolderHotkey(buttonHintLoad, Data.BUTTON_LOAD);
					updateFolderHotkey(buttonHintReset, Data.BUTTON_RESET);
					updateFolderHotkey(buttonHintExtra, Data.BUTTON_EXTRA);
					break;
				case STATE_ENTRY:
					buttonHintBack.ButtonVisible = true;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = true;
					buttonHintHide.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
			}
		};
		
		internal function updateHotkey(button:BSButtonHintData, type:int)
		{
			var data:Object = Data.getHotkey(type);
			if (!data) {
				button.ButtonVisible = false;
				return;
			}
			
			button.ButtonVisible = true;
			button.ButtonText = data.name;
			button.ButtonClickDisabled = (data.type == Data.HOTKEY_HOLD);
		}
		
		internal function updateFolderHotkey(button:BSButtonHintData, type:int)
		{
			var data:Object = Data.getFolderHotkey(type);
			if (!data) {
				button.ButtonVisible = false;
				return;
			}
			
			button.ButtonVisible = true;
			button.ButtonText = data.name;
			button.ButtonClickDisabled = (data.type == Data.HOTKEY_HOLD);
		}
		
		internal function updateNotification()
		{
			var notifData:Object = Data.menuData.notification;
			if (notifData) {
				var result:GFxResult = callDataFunction(notifData);
				if (result && result.type == Data.RESULT_STRING) //ignore errors
					showNotification(result.result);
			}
			else {
				hideNotification();
			}
		}
		
		public function updateTitle(name:String = null)
		{
			if (name)
				titleName = name;
			
			var titleData:Object = Data.menuData.title;
			if (titleData) {
				var result:GFxResult = callDataFunction(titleData);
				if (result && result.type == Data.RESULT_STRING)
					sliderList.title.text = titleName;
			}
		}

		internal function hideButton():void
		{
			if (!filenameInput.visible) {
				hidden = Data.toggleMenu();
				sliderList.isEnabled = !hidden;
			}
		}
		
		public function InitOrder():void
		{
			Data.locals.order = false;
		}
		
		public function ToggleOrder():void
		{
			Data.locals.order = !Data.locals.order
			sliderList.updateValues();
		}
		
		public function InitOffset():void
		{
			Data.locals.offset = false;
		}
		
		public function ToggleOffset():void
		{
			Data.locals.offset = !Data.locals.offset
//			if (Data.locals.offset) {
//				
//			} else {
//				
//			}
		}
		
//		internal function updateAdjustment():void
//		{
//			sliderList.storeSelected();
//			if (order) {
//				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
//			} else {
//				sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
//			}
//			sliderList.restoreSelected();
//		}
		
		public function SetAlignment(i:int, checked:Boolean)
		{
			swapped = checked;
			updateAlignment();
			try {
				this.sam.SetOption(i, checked);
			} catch (e:Error) {
				trace("Failed to set alignment");
			}
		}
		
		public function SetWidescreen(i:int, checked:Boolean)
		{
			widescreen = checked;
			updateAlignment();
			try {
				this.sam.SetOption(i, checked);
			} catch (e:Error) {
				trace("Failed to get alignment");
			}
		}
		
		internal function updateAlignment():void
		{
			if (widescreen) {
				notification.x = swapped ? 480 : -834;
				sliderList.x = swapped ? -836 : 496;
			}
			else {
				notification.x = swapped ? 278 : -631;
				sliderList.x = swapped ? -623 : 278;
			}
		}
		
		public function canClose():Boolean
		{
			if (filenameInput.visible)
				return false;
			
			if (isWaiting())
				return false;
				
			return true;
		}

		public function tryClose():Boolean
		{
			if (canClose())
			{
				Util.unselectText();
				
				sliderList.getState(currentState);
				currentState.menu = Data.menuName;

				var data:Object = {
					rootMenu: this.rootMenu,
					menuName: Data.menuName,
					state: this.state,
					current: currentState,
					stack: stateStack,
					focused: sliderList.focused
				}
				
				Data.saveState(data);
				saved = true;
				return true;
			}
			return false;
		}
	}
}