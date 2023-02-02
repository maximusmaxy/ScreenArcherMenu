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
		public var currentState:Object = {};
		public var stateStack:Array = [];
		public var sam:Object = {};
		
		public var sliderList:SliderList;
		public var ButtonHintBar_mc:BSButtonHintBar;
		public var filenameInput:MovieClip;
		public var border:MovieClip;
		public var notification:MovieClip;
		
		public var titleName:String = "";
		public var rootMenu:String;
		
		internal var buttonHintData:Vector.<BSButtonHintData > ;
		internal var buttonHintSave:BSButtonHintData;
		internal var buttonHintLoad:BSButtonHintData;
		internal var buttonHintReset:BSButtonHintData;
		internal var buttonHintBack:BSButtonHintData;
		internal var buttonHintConfirm:BSButtonHintData;
		internal var buttonHintHide:BSButtonHintData;
		internal var buttonHintExtra:BSButtonHintData;
		
		internal var closeTimer:Timer;
		
		public var BGSCodeObj:Object;
		public var f4seObj:Object = {};

		public var swapped:Boolean = false;
		public var widescreen:Boolean = false;
		public var hidden:Boolean = false;
		public var saved:Boolean = false;
		
		public var hold:Boolean = false;
		public var holdType:int = 0;
		
		public var isOpen:Boolean = false;
		
		public static const NAME_MAIN:String = "Main";
		
		public static const STATE_MAIN = 1;
		public static const STATE_FOLDER = 2;
		public static const STATE_ENTRY = 3;

		public function ScreenArcherMenu()
		{
			super();
			//trace("SAM Constructed");
			
			Util.debug = true;
			widescreen = false;
			isOpen = true;
			
			this.BGSCodeObj = new Object();
			Extensions.enabled = true;
			Translator.Create(root);
			
			InitButtonHints();
			InitSliderFuncs();
			Data.initLatents(LatentTimeout);
			
			filenameInput.visible = false;
			sliderList.bUseShadedBackground = false;
			ButtonHintBar_mc.bUseShadedBackground = true;
			ButtonHintBar_mc.BackgroundAlpha = 0.05;
			ButtonHintBar_mc.ShadedBackgroundMethod = "Flash";
			notification.visible = false;
			
			InitState();
			
//			if (Util.debug) {
//				Data.menuName = "Main";
//				rootMenu = "Main";
//				PushMenu(NAME_MAIN);
//			}
			
			UpdateAlignment();
		}
		
		internal function InitState():void
		{
			state = STATE_MAIN;
			stateStack.length = 0;
			currentState.menu = NAME_MAIN;
			currentState.x = 0;
			currentState.y = 0;
			currentState.pos = 0;
		}
		
		internal function InitSliderFuncs():void
		{
			//Define callback functions for the slider list entries to avoid generating events
			var functions:EntryFunctions = new EntryFunctions();
			
			functions.list = this.SelectList;
			functions.valueInt = this.SelectInt;
			functions.valueFloat = this.SelectFloat;
			functions.checkbox = this.SelectCheckbox;
			functions.checkbox2 = this.SelectCheckbox2;

			sliderList.initEntryFunctions(functions);
		}
		
		internal function InitButtonHints():void
		{
			buttonHintData = new Vector.<BSButtonHintData>();
			buttonHintBack = new BSButtonHintData("$SAM_Back","Tab","PSN_B","Xenon_B",1,BackButton);
			buttonHintSave = new BSButtonHintData("$SAM_Save","Q","PSN_L1","Xenon_L1",1,SaveButton);
			buttonHintLoad = new BSButtonHintData("$SAM_Load","E","PSN_R1","Xenon_R1",1,LoadButton);
			buttonHintReset = new BSButtonHintData("$SAM_Reset","R","PSN_Y","Xenon_Y",1,ResetButton);
			buttonHintConfirm = new BSButtonHintData("$SAM_Confirm","Enter","PSN_A","Xenon_A",1,ConfirmButton);
			buttonHintHide = new BSButtonHintData("$SAM_Hide","F","PSN_Select","Xenon_Select",1,HideButton);
			buttonHintExtra = new BSButtonHintData("","X","PSN_X","Xenon_X",1,ExtraButton);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintHide);
			buttonHintData.push(buttonHintSave);
			buttonHintData.push(buttonHintLoad);
			buttonHintData.push(buttonHintExtra);
			buttonHintData.push(buttonHintReset);
			buttonHintData.push(buttonHintConfirm);
			ButtonHintBar_mc.SetButtonHintData(buttonHintData);
		}
		
		public function MenuOpened(data:Object)
		{
			trace("Menu Opened");
			
			this.sam = root.f4se.plugins.ScreenArcherMenu;
			
			Data.load(data, this.sam, this.f4seObj, stage);

			if (data.title) {
				titleName = data.title;
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
				UpdateAlignment();
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
			
			isOpen = true;
			
			Util.playOk();
		}
		
		public function SaveState() {
			trace("Save state");
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
		}
		
		public function ConsoleRefUpdated(data:Object)
		{
			trace("Console ref updated");
			if (data.title)
				titleName = data.title;
				
			if (data.updated) {
				
				var isUpdated:Boolean = false;
				
				if (Data.menuData.update) {
					
					var result:int = this.GetMenuGetLatent(Data.menuData, Data.menuName, Data.LATENT_REFRESH);
					if (result != Data.RESULT_ERROR) {
						isUpdated = true;
					} 
					if (result == Data.RESULT_SUCCESS) {
						LoadMenu(Data.menuName, Data.menuData, Data.latentGet.result);
					}
				}
				
				if (!isUpdated) {
					ResetState();
				}

				Util.playOk();
			}
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

		public function ProcessKeyDown(keyCode:uint)
		{
			//trace("Process Key Down", keyCode);
			//https://www.creationkit.com/fallout4/index.php?title=DirectX_Scan_Codes
			if (this.IsWaiting() || hold)
				return true;
				
			ProcessKeyRepeat(keyCode);

			switch (keyCode)
			{
				case 9://Tab
				case 277://Pad B
					if (buttonHintBack.ButtonVisible) {
						BackButton();
					}
					break;
				case 13://Enter
				case 276://Pad A
					if (buttonHintConfirm.ButtonVisible) {
						ConfirmButton();
					} else {
						sliderList.processInput(SliderList.A);
					}
					Util.unselectText();
					break;
				case 69://E
				case 273://Pad R1
				case 275://Pad R2
					if (buttonHintLoad.ButtonVisible) {
						LoadButton();
					} 
//					else if (buttonHintTarget.ButtonVisible) {
//						targetButton();
//					}
					break;
				case 81://Q
				case 272://Pad L1
				case 274://Pad L2
					if (buttonHintSave.ButtonVisible) {
						SaveButton();
					}
					break;
				case 82://R
				case 279://Pad Y
					if (buttonHintReset.ButtonVisible) {
						ResetButton();
					}
					break;
				case 88://X
				case 278://Pad X
					if (buttonHintExtra.ButtonVisible) {
						ExtraButton();
					}
					break;
				case 70://F
				case 271://Pad Select
					if (buttonHintHide.ButtonVisible) {
						HideButton();
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
		
		public function ProcessKeyHold(keyCode:uint)
		{
			//trace("Process Key Hold", keyCode);
			switch (keyCode)
			{
				case 37://Left
				case 65://A
				case 268://Pad Left
					OnHoldStep(false);
					break;
				case 39://Right
				case 68://D
				case 269://Pad Right
					OnHoldStep(true);
					break;
			}
		}
		
		public function ProcessKeyRepeat(keyCode:uint)
		{
			//block while waiting for latent functions
			if (IsWaiting())
				return;
			
			if (hold) {
				ProcessKeyHold(keyCode);
				return;
			}
			
			//trace("Process Key Repeat", keyCode);
			
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
		
		public function ProcessKeyUp(keyCode:uint)
		{
			//trace("Process Key Up", keyCode);
			switch (keyCode)
			{
				case 69://E
				case 273://Pad R1
				case 275://Pad R2
					DisableHold(Data.BUTTON_LOAD);
					break;
					
				case 81://Q
				case 272://Pad L1
				case 274://Pad L2
					DisableHold(Data.BUTTON_SAVE);
					break;
					
				case 82://R
				case 279://Pad Y
					DisableHold(Data.BUTTON_RESET);
					break;
					
				case 88://X
				case 278://Pad X
					DisableHold(Data.BUTTON_EXTRA);
					break;
				
//				case 257://Mouse2
//					show();
//					break;
			}
		}
		
		public function ShowNotification(msg:String)
		{
			if (msg && msg.length > 0) {
				notification.visible = true;
				notification.message.text = msg;
			} else {
				notification.visible = false;
			}
		}
		
		public function HideNotification()
		{
			notification.visible = false;
		}
		
		public function SetFolder(data:Object)
		{
			trace("Set folder");
			var folderResult:GFxResult = Data.getFolder(data.path, data.ext);
			if (!CheckError(folderResult))
				return;
				
			LoadFolder(data, folderResult);
		}
		
		public function LoadMenuFolder(data:Object, result:GFxResult, name:String):void
		{
			trace("Load menu folder");
//			Data.menuName = name;
		
			//update enter and leave functions
			UpdateEnterLeave(Data.menuData, data.enter);
			Data.menuData = data;

			LoadFolder(data.folder, result);
			
			//update init function
			UpdateInit(data.init);
		}
		
		public function LoadFolder(data:Object, result:GFxResult):void
		{
			trace("Load folder");
			Data.setFolder(data, result);
			
			state = STATE_FOLDER;
			trace("pushing state, length", stateStack.length);
			stateStack.push(GetState());
			sliderList.updateState(0);
			sliderList.update();
			sliderList.updateSelected(0, 0);
			
			UpdateMenus();
		}
	
		public function SelectFolder(i:int)
		{
			var type:int = Data.getType(i);
			if (type == Data.ITEM_LIST) {
				trace("Select folder list");
				var path:String = Data.getFolderPath(i);
				var selectResult:GFxResult = CallDataFunction(Data.folderData.func, [path]);
				
				if (!CheckError(selectResult))
					return;
				
				if (Data.folderData.pop)
					ClearFolder();
			} else if (type == Data.ITEM_FOLDER) {
				trace("Select folder, folder");
				var folderResult:GFxResult = Data.getFolder(Data.menuFolder[i].path, Data.folderData.ext);
				
				if (!CheckError(folderResult))
					return;
					
				PushFolder(Data.menuFolder[i].path, folderResult);
			}
		}
		
		public function SelectFolderCheckbox(i:int, checked:Boolean = true)
		{
			//trace("Select folder checkbox", i, checked);
			var type:int = Data.getType(i);
			if (type == Data.ITEM_FOLDER) {
				var folderResult:GFxResult = Data.getFolderCheckbox(Data.menuFolder[i].path, Data.folderData.ext, Data.menuData.race);
				if (!CheckError(folderResult))
					return;
				PushFolder(Data.menuFolder[i].path, folderResult);
			} else {
				var checkbox:Boolean;
				
				if (type == Data.ITEM_CHECKBOX) {
					checkbox = true;
					Data.menuFolder[i].checked = checked;
					
				} else {
					checkbox = false;
					for (var i:int = 0; i < Data.menuFolder.length; i++) {
						Data.menuFolder[i].checked = false;
					}
					Data.menuFolder[i].checked = true;
				}
				
				var path:String = Data.getFolderPath(i);
				var race:Boolean = Data.menuData.race;
				
				var selectResult:GFxResult = sam.LoadSkeletonAdjustment(path, checked, checkbox, race);

				if (!CheckError(selectResult))
					return;

				if (Data.folderData.pop)
					ClearFolder();
			}
		}
		
		public function PushFolder(path:String, result:GFxResult) 
		{
			trace("data push folder");
			Data.pushFolder(path, result.result);
			
			trace("pushing folder", stateStack.length)
			stateStack.push(GetState());
			sliderList.updateState(0);
			sliderList.update();
			sliderList.updateSelected(0, 0);
			
			UpdateMenus();
		}
		
		public function PopFolder() 
		{
			var folderPath:String = Data.popFolder();
			trace("Pop folder", folderPath);

			var folderResult:GFxResult = Data.getFolder(folderPath, Data.folderData.ext);
			
			//need to force a result on pop fail to prevent locks
			if (!folderResult || folderResult.type != Data.RESULT_FOLDER)
				folderResult = Data.popFailFolder;

			Data.updateFolder(folderResult.result);

			trace("popping state, length", stateStack.length);
			currentState = stateStack.pop();
			sliderList.updateState(currentState.pos);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);

			UpdateMenus();
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
		
		public function ClearFolder()
		{
			//go to first folder then pop to get current state
			trace("clearing folder, length:", stateStack.length);
			stateStack.length = stateStack.length - Data.folderStack.length;
			trace("post length:", stateStack.length);
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			PopMenu();
		}
		
		public function PushState()
		{
			trace("Pushing state, length:", stateStack.length);
			stateStack.push(GetState());
			currentState.x = 0;
			currentState.y = 0;
			sliderList.updateState(0);
		}
		
//		public function PopState()
//		{
//			Data.menuFolder = null;
//			currentState = stateStack.pop();
//			sliderList.updateState(currentState.pos);
//		}
		
		public function CheckError(result:GFxResult):Boolean
		{
			//no result
			if (!result) {
				ShowNotification(Data.error)
				return false;
			}
			
			//show error message
			if (result.type == Data.RESULT_ERROR) {
				ShowNotification(result.result);
				return false;
			}
			
			//good result
			return true;
		}
		
//		public function CheckWait(menuResult:GFxResult, getResult:GFxResult, type:int):Boolean
//		{
//			if (getResult.type != Data.RESULT_WAITING)
//				return true;
//				
//			Data.setPapyrusWaiting(menuResult.result, type);
//			
//			Data.papyrusTimer.Timer = new Timer(menuResult.result.get.timeout, 1);
//			Data.papyrusTimer.addEventListener(TimerEvent.TIMER_COMPLETE, function(e:TimerEvent) 
//			{
//				ShowNotification("$SAM_PapyrusTimeout");
//				Util.playCancel();
//				Data.clearPapyrusWaiting();
//			});
//			
//			Data.papyrusTimer.start();
//
//			return false;
//		}
		
		public function GetState():Object
		{
			trace("get state");
			return {
				"menu": Data.menuName,
				"x": sliderList.selectedX,
				"y": sliderList.selectedY,
				"pos": sliderList.listPosition
			};
		}
		
		public function GetMenuData(name:String):GFxResult
		{
			trace("Get menu data");
			var menuResult:GFxResult = Data.getMenu(name);
			
			if (!CheckError(menuResult))
				return null;
				
			return menuResult
		}
		
		public function GetMenuGetLatent(data:Object, name:String, type:int):int
		{
			trace("Get Menu Get Latent");
			//get menu result
			var getResult:GFxResult = GetResult(data);
			
			if (!CheckError(getResult))
				return Data.RESULT_ERROR;
				
			//notification and title are optional
			var notifResult:GFxResult = null;
			var titleResult:GFxResult = null;
			
			if (type != Data.LATENT_REFRESH) {
				if (data.notification) {
					notifResult = CallDataFunction(data.notification);
					if (notifResult && notifResult.type == Data.RESULT_ERROR) //ignore error
						notifResult = null;
				}
	
				if (data.title) {
					titleResult = CallDataFunction(data.title);
					if (titleResult && titleResult.type == Data.RESULT_ERROR)  //ignore error
						titleResult = null;
				}
			}
			
			//update latent callbacks
			Data.latentWaiting = false;
			Data.updateLatent(Data.latentGet, getResult, data.get);
			Data.updateLatent(Data.latentNotification, notifResult, data.notification);
			Data.updateLatent(Data.latentTitle, titleResult, data.title);
			
			//halt if any callbacks are waiting
			if (!Data.startLatents(data, name, type))
				return Data.RESULT_WAITING;
			
			//continue if no callbacks are waiting
			return Data.RESULT_SUCCESS;
		}
		
		public function LatentTimeout() {
			trace("Latent timeout");
			//pop cannot fail
			if (Data.latentAction == Data.LATENT_POP) {
				LoadMenu(currentState.menu, Data.popFailMenu.result, Data.popFailGet);
			} else {
				ShowNotification("$SAM_PapyrusTimeout");
			}

			Data.clearLatents();
		}
		
		public function CheckPushFolder(data:Object, result:GFxResult, name:String):Boolean
		{
			if (result.type == Data.RESULT_FOLDER ||
				result.type == Data.RESULT_FOLDERCHECKBOX) {
				LoadMenuFolder(data, result, name);
				return false;
			}
			
			return true;
		}
		
		public function PushMenu(name:String)
		{
			trace("Push menu", name);
			//get menu data
			var menuData:GFxResult = GetMenuData(name);
			if (!menuData)
				return;
				
			//get result of get function
			var getResult:int = GetMenuGetLatent(menuData.result, name, Data.LATENT_PUSH);
			if (getResult != Data.RESULT_SUCCESS)
				return;
			
			UpdatePush(menuData.result, Data.latentGet.result, name);
		}
		
		public function UpdatePush(data:Object, result:GFxResult, name:String)
		{
			trace("Update Push", name);
			//check if menu is a folder and push that instead
			if (!CheckPushFolder(data, result, name))
				return;
	
			//push new state
			PushState();
			
			//update enter and leave functions
			UpdateEnterLeave(Data.menuData, data.enter);
			
			//load the menu
			LoadMenu(name, data, result);
			
			//update init function
			UpdateInit(data.init);
		}
		
		public function LatentCallback(result:GFxResult):void
		{
			trace("Latent callback");
			switch (result.type) {
				case Data.RESULT_ERROR:
					ShowNotification(result.result);
					Data.clearLatents();
					break;
				case Data.RESULT_SUCCESS:
					Data.latentSet.Clear();
					Data.clearLatents();
					break;
				case Data.RESULT_NAMES:
				case Data.RESULT_VALUES:
				case Data.RESULT_ITEMS:
					Data.latentGet.Recieve(result);
					UpdateLatentGet();
					break;
				case Data.RESULT_NOTIFICATION:
					Data.latentNotification.Recieve(result);
					UpdateLatentGet();
					break;
				case Data.RESULT_TITLE:
					Data.latentTitle.Recieve(result);
					UpdateLatentGet();
					break;
			}
		}
		
		public function UpdateLatentGet():void
		{
			trace("Update Latent Get");
			if (Data.latentGet.success) {
				if (Data.latentNotification.waiting || Data.latentTitle.waiting)
					return;
				
				switch (Data.latentAction)
				{
					case Data.LATENT_PUSH:
						UpdatePush(Data.latentMenuData, Data.latentGet.result, Data.latentMenuName);
						break;
					case Data.LATENT_POP:
						UpdatePop(Data.latentMenuData, Data.latentGet.result, Data.latentMenuName);
						break;
					case Data.LATENT_REFRESH:
						UpdateValues();
						break;
					case Data.LATENT_RELOAD:
						UpdateReload();
						break;
				}
				Data.latentWaiting = false;
			}
		}
		
		public function PopMenu():void
		{
			trace("Pop Menu");
			if (this.filenameInput.visible)
			{
				ClearEntry();
				return;
			}
			//folder
			else if (Data.folderStack.length > 0) {
				PopFolder();
				return;
			}
			//exit
			else if (this.stateStack.length == 1)
			{
				Exit();
				return;
			}
			
			trace("popping menu state", stateStack.length);
			currentState = stateStack.pop();
			var menuData:GFxResult = GetMenuData(currentState.menu);
			
			//if menu data get fails, need to use a dummy menu and dummy get result to prevent locks
			if (!menuData) {
				LoadMenu(currentState.menu, Data.popFailMenu.result, Data.popFailGet);
				return;
			}

			var getResult:int = GetMenuGetLatent(menuData.result, name, Data.LATENT_POP);
			
			//If get fails load dummy menu
			if (getResult == Data.RESULT_ERROR) {
				LoadMenu(currentState.menu, Data.popFailMenu.result, Data.popFailGet);
				return;
			}
				
			//If waiting, it's safe to return without loading a menu
			if (getResult == Data.RESULT_WAITING)
				return;
				
			UpdatePop(menuData.result, Data.latentGet.result, currentState.menu);
		}
		
		public function UpdatePop(data:Object, result:GFxResult, name:String)
		{
			trace("Update pop");
			UpdateEnterLeave(Data.menuData, data.enter);
			
			LoadMenu(name, data, result);
			
			UpdateInit(data.init);
		}
		
		public function PopMenuTo(name:String):void
		{
			trace("Pop menu to");
			//do not pop from main
			if (stateStack.length < 2)
				return;
				
			//find stack index
			var index:int = stateStack.length;
			trace("start index", index);
			while (index > 0) {
				index--;
				var menuName:Object = stateStack[index].menu.toLowerCase();
				if (menuName == name.toLowerCase())
					break;
			}
			trace("found index", index);
			//menu not found
			if (index <= 0)
				return;
			
			//go to menu above index and pop
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			
			trace("state stack length", stateStack.length);
			stateStack.length = index + 1;
			trace("new state stack length", stateStack.length);
			if (filenameInput.visible) {
				SetTextInput(false);
			}
			
			trace("pop menu to complete");
			PopMenu();
		}
		
		public function ResetState():void
		{
			trace("Reset state");
			state = STATE_MAIN;
			currentState.menu = rootMenu;
			currentState.pos = 0;
			currentState.x = 0;
			currentState.y = 0;
			trace("resetting state", stateStack.length);
			stateStack.length = 0;
			Data.folderStack.length = 0;

			if (filenameInput.visible) {
				SetTextInput(false);
			}
			
			PushMenu(NAME_MAIN);
		}
		
		public function UpdateEnterLeave(previousMenuData:Object, enterData:Object):void
		{
			if (previousMenuData && previousMenuData.leave)
				CallDataFunction(previousMenuData.leave);

			if (enterData)
				CallDataFunction(enterData);
		}
		
		public function UpdateInit(initData:Object):void
		{
			if (initData)
				CallDataFunction(initData);
		}
		
		public function LoadMenu(name:String, data:Object, get:GFxResult):void
		{
			trace("Load Menu", data, get);
			Data.updateMenu(name, data, get);
			state = STATE_MAIN;
			Util.unselectText();
			trace("slider list update");
			sliderList.updateState(0);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);
			trace("update menus");
			UpdateMenus();
			Data.latentGet.Clear();
		}
		
		public function UpdateMenus()
		{
			trace("Update Menus");
			UpdateButtonHints();
			UpdateNotification();
			UpdateTitle();
		}
		
		public function GetResult(menu:Object):GFxResult
		{
			trace("Get Result");
			switch (menu.type) {
				case Data.MENU_MIXED:
				case Data.MENU_CHECKBOX:
				case Data.MENU_SLIDER:
				case Data.MENU_ADJUSTMENT:
					return CallDataFunction(menu.get);
				case Data.MENU_LIST:
					//optional
					if (menu.get) {
						return CallDataFunction(menu.get);
					} else {
						return Data.resultSuccess;
					}
				case Data.MENU_FOLDER:
					return Data.getFolder(menu.folder.path, menu.folder.ext);
				case Data.MENU_FOLDERCHECKBOX:
					return Data.getFolderCheckbox(menu.folder.path, menu.folder.ext, menu.race);
				case Data.MENU_MAIN:
					return Data.resultSuccess;
			}
			
			return null;
		}
		
		public function CallSet(... args)
		{
			trace("Call Set");
			var data:Object = GetSetData(args);

			if (!data)
				return;

			var result:GFxResult = CallDataFunction(data, args);
			
			if (result) {
				if (result.type == Data.RESULT_ERROR) {
					ShowNotification(result.result);
					return;
				} else if (result.type == Data.RESULT_WAITING) {
					//Wait for latent function only if specified
					if (data.wait) {
						Data.latentSet.Init(data.timeout)
						Data.latentWaiting = true;
						Data.latentSet.Start();
						Data.latentAction = Data.LATENT_SET;
						return;
					}
				}
			} else {
				ShowNotification(Data.error);
			}
			
			UpdateDataFunction(data);
		}
		
		public function UpdateDataFunction(data:Object)
		{
			trace("Update Data Function");
			if (data.pop) {
				PopMenu();
			} else if (data.popto) {
				PopMenuTo(data.popto);
			} else if (data.update) {
				ReloadMenu();
			} else if (data.refresh) {
				RefreshValues();
			}
		}
		
		public function RefreshValues() {
			trace("Refresh Values");
			var getResult:int = GetMenuGetLatent(Data.menuData, Data.menuName, Data.LATENT_REFRESH);
			if (getResult != Data.RESULT_SUCCESS)
				return;
			
			UpdateValues();
		}
		
		public function UpdateValues()
		{
			trace("UpdateValues");
			Data.updateValues(Data.latentGet.result);
			sliderList.updateValues();
			Data.latentGet.Clear();
		}
		
		public function ReloadMenu() {
			if (this.state == STATE_FOLDER) {
				ReloadFolder();
				return;
			}
			
			trace("Reload Menu");
			var getResult:int = GetMenuGetLatent(Data.menuData, Data.menuName, Data.LATENT_RELOAD);
			if (getResult != Data.RESULT_SUCCESS)
				return;
			
			UpdateReload();
		}
		
		public function UpdateReload()
		{
			trace("Update reload");
			Data.updateMenu(Data.menuName, Data.menuData, Data.latentGet.result);
			Data.latentGet.Clear();
			sliderList.storeSelected();
			sliderList.update();
			sliderList.restoreSelected();
			UpdateMenus();
		}
		
		public function ReloadFolder()
		{
			trace("reload folder");
			var result:GFxResult;
			if (Data.menuType == Data.MENU_FOLDERCHECKBOX) {
				result = Data.getFolderCheckbox(Data.folderData.path, Data.folderData.ext, Data.menuData.race);
			} else {
				result = Data.getFolder(Data.folderData.path, Data.folderData.ext);
			}
			
			if (!CheckResult(result))
				return;
			
			LoadMenuFolder(Data.menuData, result, Data.menuName);
		}
		
		public function GetSetData(args:Array):Object
		{
			trace("Get Set Data");
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
		
		public function SetLocalVariable(name:String, value:Object)
		{
			trace("Set local variable", name, value);
			Data.locals[name] = value;
		}
		
		public function SelectList(i:int) 
		{
			trace("Select List", i);
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
					SelectFolder(i);
					break;
				case Data.MENU_FOLDERCHECKBOX:
					SelectFolderCheckbox(i);
					break;
			}
		}
		
		public function SelectInt(i:int, value:int)
		{
			trace("Select Int", i, value);
			Data.menuValues[i] = value;
			CallSet(i, value);
		}
		
		public function SelectFloat(i:int, value:Number)
		{
			trace("Select Float", i, value);
			var type:int = Data.getType(i);
			if (type != Data.ITEM_TOUCH) {
				Data.menuValues[i] = value;
			}
			CallSet(i, value);
		}
		
		public function SelectCheckbox(i:int, checked:Boolean = false)
		{
			trace("Select Checkbox", i, checked);
			switch (Data.menuType) 
			{
				case Data.MENU_CHECKBOX:
					Data.menuValues[i] = checked;
					CallSet(i, checked);
					break;
				case Data.MENU_FOLDERCHECKBOX:
					SelectFolderCheckbox(i, checked);
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
						CallDataFunction(Data.menuData.adjustment.down, [i]);
					} else {
						CallDataFunction(Data.menuData.adjustment.edit, [i, Data.menuValues[i]]);
					}
					break;
			}
		}
		
		public function SelectCheckbox2(i:int, checked:Boolean = false)
		{
			trace("Select Checkbox2", i, checked);
			if (Data.locals.order) {
				CallDataFunction(Data.menuData.adjustment.up, [i]);
			} else {
				CallDataFunction(Data.menuData.adjustment.remove, [i, Data.menuValues[i]]);
				ReloadMenu();
			}
		}
		
		//Prevent user input when waiting for return values
		public function IsWaiting():Boolean {
			return Data.latentWaiting || !isOpen;
		}

//		public function PapyrusResult(result:GFxResult):void
//		{
//			if (Data.papyrusWaiting)
//			{
//				if (!CheckError(result)) {
//					Util.playCancel();
//					Data.clearPapyrusWaiting();
//					return;
//				}
//				
//				switch (Data.papyrusType) {
//					case PAPYRUS_GET:
//						stateStack.push(GetState());
//						currentState.x = 0;
//						currentState.y = 0;
//						sliderList.updateState(0);
//				
//						LoadMenu(name, Data.papyrusMenuData, result);
//						break;
//						
//					case PAPYRUS_REFRESH:
//						sliderList.updateValues();
//						break;
//						
//					//case PAPYRUS_SET: //ignore
//				}
//				
//				Data.clearPapyrusWaiting();
//			}
//		}
		
		public function CallDataFunction(data:Object, args:Array = null):GFxResult
		{
			trace("Call Data Function");
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
					trace("calling menu func");
					PushMenu(data.name);
					return Data.resultSuccess;
				case Data.FUNC_ENTRY: 
					trace("calling entry func");
					SetEntry(data.entry);
					return Data.resultSuccess;
				case Data.FUNC_FOLDER:
					trace("calling folder func");
					SetFolder(data.folder);
					return Data.resultSuccess;
			}

			if (data.args) {
				if (!args) {
					args = [];
				}
				
				for (var i:int = 0; i < data.args.length; i++) {
					switch (data.args[i].type) {
						case Data.ARGS_VAR:
							var property:String = data.args[i].name;
							//need to use hasOwnProperty because of falsy values
							if (Data.locals.hasOwnProperty(property)) {
								args.push(Data.locals[property]);
							} else {
								Data.error = ("Local property not found: " + property);
								return null;
							}
							break;
						case Data.ARGS_INDEX:
							args.push(Data.menuValues[data.args[i].index]);
							break;
						case Data.ARGS_VALUE:
							args.push(data.args[i].value)
							break;
					}
				}
			}
			
			trace("calling data function");
			Util.traceObj(data);
			trace("args");
			Util.traceObj(args);
			
			switch (data.type)
			{
				case Data.FUNC_SAM: return CallSamFunction(data.name, args);
				case Data.FUNC_LOCAL: return CallLocalFunction(data.name, args);
				case Data.FUNC_FORM: return CallPapyrusForm(data, args);
				case Data.FUNC_GLOBAL: return CallPapyrusGlobal(args);
				case Data.FUNC_DEBUG: return CallDebugFunction(data.name, args);
			}
			
			Data.error = "Could not resolve function type";
			return null;
		}
		
		public function CallSamFunction(name:String, args:Array):GFxResult 
		{
			try {
				var result:GFxResult = Util.callFuncArgs(this.sam[name], args);
				
				if (result)
					return result;
			} catch (e:Error) { 
				Data.error = "$SAM_SamFunctionMissing";
				trace(e.message)
			}
			
			if (Util.debug)
				return Data.getSamDebugFunction(name);
			
			return null;
		}
		
		public function CallLocalFunction(name:String, args:Array):GFxResult
		{
			try {
				var result:GFxResult = Util.callFuncArgs(this[name], args);
				
				if (result)
					return result;
			}
			catch (e:Error) { 
				Data.error = "$SAM_LocalFunctionMissing";
				trace(e.message)
			}

			return null;
		}
		
		//Calls a papyrus global function
		public function CallPapyrusForm(data:Object, args:Array):GFxResult
		{
			var result:GFxResult;
			try {
				result = this.sam.CallPapyrusForm(data.id, data.func, args);
				if (result)
					return result;
			}
			catch (e:Error) { trace(e.message) }
			
			Data.error = "$SAM_PapyrusTimeout";
			return null;
		}
		
		public function CallPapyrusGlobal(args:Array):GFxResult
		{
			
			var result:GFxResult = Util.callFuncArgs(this.f4seObj.CallGlobalFunctionNoWait, args); 			
			if (result)
				return result;
			
			Data.error = "$SAM_PapyrusTimeout";
			return null;
		}
		
		public function CallDebugFunction(name:String, args:Array):GFxResult
		{
			var func:Object = GetLocalDebugFunction(name);
			if (func) {
				Util.callFuncArgs(func, args);
			}
			return Data.resultSuccess;
		}
		
		public function GetLocalDebugFunction(name:String)
		{
			switch (name)
			{
				
			case "SetEyes": return (function(index:int, value:Number, left:Number, right:Number) {
				trace(index, value, left, right);
			});
			
			case "LoadAdjustment": return (function(name:String) {
				trace(name);
			});
			
			case "SaveAdjustment": return (function(name:String) {
				trace(name);
			});
			
			case "SetFaceMorphCategory": return (function(index:int, value:int) {
				 if (index < 4) {
					 PushMenu("FaceMorphSliders");
				 } else {
					 PushMenu("TongueBones");
				 }
			});
				
			}
			
			error = "$SAM_LocalFunctionMissing";
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
//					SetTextInput(true);
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
//
//		public function NewAdjustment():void
//		{
//			try {
//				sam.NewAdjustment();
//			}
//			catch (e:Error) {}
//			ReloadMenu();
//		}
		
		public function MoveAdjustmentDown(id:int):GFxResult
		{
			if (Data.moveAdjustment(id, true)) {
				var result:GFxResult = CallDataFunction(Data.menuData.get);
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
			
			return Data.resultSuccess;
		}
		
		public function MoveAdjustmentUp(id:int):GFxResult
		{
			if (Data.moveAdjustment(id, false)) {
				var result:GFxResult = CallDataFunction(Data.menuData.get);
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
			
			return Data.resultSuccess;
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
		
		internal function ConfirmButton():void
		{
			if (this.filenameInput.visible) 
			{
				var func:Object = Data.entryData.func;
				var result:GFxResult = CallDataFunction(func, [this.filenameInput.Input_tf.text]);
				
				ClearEntry();
				
				if (CheckError(result))	
					Util.playOk();
				else
					Util.playCancel();
				
				//check for these only, ignore pops
				if (func.refresh)
					RefreshValues();
				else if (func.update)
					ReloadMenu();
			}
		}
		
		public function SetEntry(data:Object):void
		{
			Data.entryData = data;
			SetTextInput(true);
			UpdateMenus();
		}
		
		public function ClearEntry():void
		{
			SetTextInput(false);
			Data.entryData = null;
			UpdateMenus();
		}

//		internal function ConfirmButton():void
//		{
//			switch (this.state) {
//				case SAVEMFG_STATE: Data.saveMfg(filenameInput.Input_tf.text); break;
//				case SAVEADJUSTMENT_STATE: Data.saveAdjustment(filenameInput.Input_tf.text); break;
//				case SAVEPOSE_STATE: Data.savePose(filenameInput.Input_tf.text); break;
//				case RENAMEADJUSTMENT_STATE: Data.renameAdjustment(filenameInput.Input_tf.text); break;
//				case SAVELIGHT_STATE: Data.saveLights(filenameInput.Input_tf.text); break;
//				case RENAMELIGHT_STATE: Data.renameLight(filenameInput.Input_tf.text); break;
//			}
//			SetTextInput(false);
//			PopMenu();
//			Util.playOk();
//		}
//		
		internal function SetTextInput(enabled:Boolean)
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
				AllowTextInput(true);
			}
			else
			{
				state = STATE_MAIN;
				filenameInput.Input_tf.text = "";
				filenameInput.Input_tf.type = TextFieldType.DYNAMIC;
				filenameInput.Input_tf.setSelection(0,0);
				filenameInput.Input_tf.selectable = false;
				filenameInput.Input_tf.maxChars = 0;
				AllowTextInput(false);
				filenameInput.visible = false;
				sliderList.visible = true;
				stage.focus = sliderList;
			}
		}

//		internal function SaveButton():void
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
//		internal function LoadButton():void
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
//			ShowNotification(Data.menuOptions[id]);
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
//					UpdateAlignment();
//					break;
//				case 2://widescreen
//					widescreen = enabled;
//					UpdateAlignment();
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
		
		public function ResetButton()
		{
			CallHotkeyFunction(Data.BUTTON_RESET);
		}
		
		public function ExtraButton()
		{
			CallHotkeyFunction(Data.BUTTON_EXTRA);
		}
		
		public function SaveButton()
		{
			CallHotkeyFunction(Data.BUTTON_SAVE);
		}
		
		public function LoadButton()
		{
			CallHotkeyFunction(Data.BUTTON_LOAD);
		}
		
		public function CallHotkeyFunction(type:int)
		{
			trace("Call hotkey function", this.state, type);
			Util.traceObj(Data.folderData);
			var data:Object = (this.state == STATE_FOLDER ? Data.getFolderHotkey(type) : Data.getHotkey(type));
			Util.traceObj(data);
			if (!data)
				return;
			
			switch (data.type) {
				case Data.HOTKEY_FUNC:
					var result:GFxResult = CallDataFunction(data.func);
					if (CheckError(result)) {
						UpdateDataFunction(data.func);
						Util.playOk();
					} else  {
						Util.playCancel();
					}
					break;
				case Data.HOTKEY_HOLD:
					EnableHold(data.hold, type);
					break;
			}
		}

//		public function ResetButton():void
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
//					HideNotification();
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

		internal function BackButton():void
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
		
		public function Exit():void
		{
			isOpen = false;
			CleanUp();
			if (!saved) {
				Data.clearState();
			}
			
			closeTimer = new Timer(100,1);
			closeTimer.addEventListener(TimerEvent.TIMER_COMPLETE, function(e:TimerEvent) {
				try {
					this.BGSCodeObj.CloseMenu("ScreenArcherMenu");
				} 
				catch (e:Error) 
				{
					Close();
				}
			});
			
			closeTimer.start();
		}
		
		public function Close()
		{
			try {
				this.sam.SamCloseMenu("ScreenArcherMenu");
			}
			catch (e:Error)
			{
				trace("No escape");
			}
		}

//		public function ExtraButton():void
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
//					EnableHold(HELD_X, onZMove, onZLeft, onZRight);
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

		public function EnableHold(data:Object, type:int)
		{
			if (!hold) {
				hold = true;
				holdType = type;
				stage.addEventListener(MouseEvent.MOUSE_MOVE, OnHoldMove);
				Data.holdData = data;
				Data.setCursorVisible(false);
				Data.storeCursorPos();
				sliderList.storeSelected();
			}
		}
		
		public function DisableHold(type:int) {
			if (hold && holdType == type) {
				hold = false;
				stage.removeEventListener(MouseEvent.MOUSE_MOVE, OnHoldMove);
				Data.holdData = null;
				Data.setCursorVisible(true);
				Data.endCursorDrag();
				sliderList.restoreSelected();
			}
		}
		
		public function OnHoldMove(event:MouseEvent) {
			var dif:int = Data.updateCursorDrag();
			var result:GFxResult = CallDataFunction(Data.holdData.func, [dif * Data.holdData.mod]);
			//CheckError(result);
		}
		
		public function OnHoldStep(inc:Boolean) {
			var result:GFxResult = CallDataFunction(Data.holdData.func, [(inc ? Data.holdData.step : -Data.holdData.step)]);
			//CheckError(result);
		}
		
		public function AllowTextInput(allow:Boolean)
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
		
		internal function UpdateButtonHints():void
		{
			trace("Update button hints");
			switch (this.state) {
				case STATE_MAIN:
					buttonHintBack.ButtonText = this.stateStack.length == 1 ? "$SAM_Exit" : "$SAM_Back";
					buttonHintBack.ButtonVisible = true;
					buttonHintHide.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					
					UpdateHotkey(buttonHintSave, Data.BUTTON_SAVE);
					UpdateHotkey(buttonHintLoad, Data.BUTTON_LOAD);
					UpdateHotkey(buttonHintReset, Data.BUTTON_RESET);
					UpdateHotkey(buttonHintExtra, Data.BUTTON_EXTRA);
					break;
				case STATE_FOLDER:
					buttonHintBack.ButtonVisible = true;
					buttonHintHide.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					
					UpdateFolderHotkey(buttonHintSave, Data.BUTTON_SAVE);
					UpdateFolderHotkey(buttonHintLoad, Data.BUTTON_LOAD);
					UpdateFolderHotkey(buttonHintReset, Data.BUTTON_RESET);
					UpdateFolderHotkey(buttonHintExtra, Data.BUTTON_EXTRA);
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
		
		internal function UpdateHotkey(button:BSButtonHintData, type:int)
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
		
		internal function UpdateFolderHotkey(button:BSButtonHintData, type:int)
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
		
		internal function UpdateNotification()
		{
			if (Data.latentNotification.success) {
				ShowNotification(Data.latentNotification.result.result);
				Data.latentNotification.Clear();
			} else {
				HideNotification();
			}
		}
		
		public function UpdateTitle()
		{
			if (Data.latentTitle.success) {
				sliderList.title.text = Data.latentTitle.result.result;
				Data.latentTitle.Clear();
			} else {
				sliderList.title.text = titleName;
			}
		}

		internal function HideButton():void
		{
			if (!filenameInput.visible) {
				hidden = Data.toggleMenu();
				sliderList.isEnabled = !hidden;
			}
		}

		public function EnterOrder():GFxResult
		{
			Data.locals.order = false;
			
			return Data.resultSuccess;
		}
		
		public function ToggleOrder():GFxResult
		{
			Data.locals.order = !Data.locals.order
			sliderList.updateValues();
			
			return Data.resultSuccess;
		}
		
		public function InitOffset():GFxResult
		{
			Data.locals.offset = sam.GetNodeIsOffset(Data.locals.boneName);
			buttonHintLoad.ButtonText = (Data.locals.offset ? "$SAM_Offset" : "$SAM_Pose");
			sliderList.title.text = Data.locals.boneName;
			
			return Data.resultSuccess;
		}
		
		public function ToggleOffset():GFxResult
		{
			var newName = sam.ToggleNodeName(Data.locals.boneName);
			if (newName != Data.locals.boneName) {
				Data.locals.offset = !Data.locals.offset;
				Data.locals.boneName = newName;
				sliderList.title.text = newName;
				buttonHintLoad.ButtonText = (Data.locals.offset ? "$SAM_Offset" : "$SAM_Pose");
				RefreshValues();
			} else {
				ShowNotification("This bone is offset only");
			}

			return Data.resultSuccess;
		}
		
		public function EnterFolderCheckbox():GFxResult
		{
			//trace("enter folder checkbox");
			Data.locals.folderCheckbox = false;
			
			return Data.resultSuccess;
		}
		
//		public function InitFolderCheckbox():GFxResult
//		{
//			buttonHintLoad.ButtonText = "$SAM_Multi";
//			
//			return Data.resultSuccess;
//		}
		
		public function ToggleFolderCheckbox():GFxResult
		{
			//trace("toggle folder checkbox");
			Data.locals.folderCheckbox = !Data.locals.folderCheckbox;
			trace(Data.locals.folderCheckbox);
			sliderList.update();
			buttonHintLoad.ButtonText = (Data.locals.folderCheckbox ? "$SAM_Multi" : "$SAM_Single");
						
			return Data.resultSuccess;
		}
		
		public function InitLightVisible(selectedLight:int):GFxResult
		{
			var isVisible:Boolean = Data.getLightVisible(selectedLight);
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");
			
			return Data.resultSuccess;
		}
		
		public function InitAllLightsVisible():GFxResult
		{
			var isVisible:Boolean = Data.getAllLightsVisible();
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");
			
			return Data.resultSuccess;
		}
		
		public function ToggleLightVisible(selectedLight:int):GFxResult
		{
			var isVisible:Boolean = Data.toggleLightVisible(selectedLight);
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");

			return Data.resultSuccess;
		}
		
		public function ToggleAllLightsVisible():GFxResult
		{
			var isVisible:Boolean = Data.toggleAllLightsVisible();
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");
			
			return Data.resultSuccess;
		}
		
		public function LoadSkeletonAdjustment():GFxResult
		{
			return Data.resultSuccess();
		}
		
		public function InitEquipItem():GFxResult
		{
			Data.locals.addItemEquip = false;
			
			return Data.resultSuccess;
		}
		
		public function ToggleEquipItem():GFxResult
		{
			Data.locals.addItemEquip = !Data.locals.addItemEquip;
			buttonHintLoad.ButtonText = (Data.locals.addItemEquip ? "$SAM_Equip" : "$SAM_Add");
			
			return Data.resultSuccess;
		}
		
		public function SetAlignment(i:int, checked:Boolean)
		{
			swapped = checked;
			Data.menuValues[i] = checked;
			UpdateAlignment();
			try {
				this.sam.SetOption(i, checked);
			} catch (e:Error) {
				trace("Failed to set alignment");
			}
		}
		
		public function SetWidescreen(i:int, checked:Boolean)
		{
			widescreen = checked;
			Data.menuValues[i] = checked;
			UpdateAlignment();
			try {
				this.sam.SetOption(i, checked);
			} catch (e:Error) {
				trace("Failed to get alignment");
			}
		}
		
		internal function UpdateAlignment():void
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
		
		public function CanClose():Boolean
		{
			if (filenameInput.visible)
				return false;
				
//			Need to work out some kind of cancellation method on latent functions
//			if (isWaiting())
//				return false;
				
			return true;
		}

		public function TryClose():Boolean
		{
			//trace("Try close");
			if (CanClose())
			{
				SaveState();
				CleanUp();
				saved = true;
				isOpen = false;
				return true;
			}
			return false;
		}
		
		//To work around a crash on close bug, sam is no longer destroyed so we have to clean up manually
		public function CleanUp()
		{
			//trace("Clean up");
			InitState();
			Util.unselectText();
			stage.removeEventListener(MouseEvent.MOUSE_MOVE, OnHoldMove);
			hold = false;
			Data.clearLatents();
		}
	}
}