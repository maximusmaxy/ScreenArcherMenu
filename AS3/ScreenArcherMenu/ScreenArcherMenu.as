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
	import flash.net.LocalConnection;
	import flash.display3D.IndexBuffer3D;

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
		
		public var notificationMessage = "";
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
			
			Util.debug = false;
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
			//trace("Menu Opened");
			
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
			//trace("Save state");
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
			//trace("Console ref updated");
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
			if (this.IsWaiting())
				return;
				
			ProcessKeyRepeat(keyCode);
			
			if (hold)
				return;

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
						RefreshNotification();
					}
					break;
				case 38://Up
				case 87://W
				case 266://Pad Up
					if (sliderList.visible) {
						sliderList.processInput(SliderList.UP);
						RefreshNotification();
					}
					break;
				case 39://Right
				case 68://D
				case 269://Pad Right
					if (sliderList.visible) {
						sliderList.processInput(SliderList.RIGHT);
						RefreshNotification();
					}
					break;
				case 40://Down
				case 83://S
				case 267://Pad Down
					if (sliderList.visible)
					{
						sliderList.processInput(SliderList.DOWN);
						RefreshNotification();
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
		
		public function ShowNotification(msg:String, store:Boolean = false)
		{
			if (msg && msg.length > 0) {
				if (store)
					notificationMessage = msg;
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
			//trace("Set folder");
			var folderResult:GFxResult = Data.getFolder(data.path, data.ext);
			if (!CheckError(folderResult))
				return;
				
			PushState();	
				
			LoadFolder(data, folderResult);
		}
		
		public function LoadMenuFolder(data:Object, result:GFxResult, name:String):void
		{
			//trace("Load menu folder");
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
			//trace("Load folder");
			Data.setFolder(data, result);
			
			state = STATE_FOLDER;
			//trace("pushing state, length", stateStack.length);
			sliderList.updateState(currentState.pos);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);
			
			UpdateMenus();
		}
	
		public function SelectFolder(i:int)
		{
			var type:int = Data.getType(i);
			if (type == Data.ITEM_LIST) {
				//trace("Select folder list");
				var path:String = Data.getFolderPath(i);
				var selectResult:GFxResult = CallDataFunction(Data.folderData.func, [path]);
				
				if (!CheckError(selectResult))
					return;
				
				if (Data.folderData.pop)
					ClearFolder();
			} else if (type == Data.ITEM_FOLDER) {
				//trace("Select folder, folder");
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
					for (var j:int = 0; j < Data.menuFolder.length; j++) {
						Data.menuFolder[j].checked = false;
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
			//trace("data push folder");
			Data.pushFolder(path, result.result);
			
			//trace("pushing folder", stateStack.length)
			stateStack.push(GetState());
			sliderList.updateState(0);
			sliderList.update();
			sliderList.updateSelected(0, 0);
		}
		
		public function PopFolder() 
		{
			var folderPath:String = Data.popFolder();
			//trace("Pop folder", folderPath);

			var folderResult:GFxResult;
			if (Data.menuType == Data.MENU_FOLDERCHECKBOX) {
				folderResult = Data.getFolderCheckbox(folderPath, Data.folderData.ext, Data.menuData.race);
			} else {
				folderResult = Data.getFolder(folderPath, Data.folderData.ext);
			}

			//need to force a result on pop fail to prevent locks
			if (!folderResult || !(folderResult.type == Data.RESULT_FOLDER || folderResult.type == Data.RESULT_FOLDERCHECKBOX))
				folderResult = Data.popFailFolder;

			Data.updateFolder(folderResult.result);

			//trace("popping state, length", stateStack.length);
			currentState = stateStack.pop();
			
			sliderList.updateState(currentState.pos);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);
		}
		
		public function ClearFolder()
		{
			//go to first folder then pop to get current state
			//trace("clearing folder, length:", stateStack.length);
			stateStack.length = stateStack.length - Data.folderStack.length;
			//trace("post length:", stateStack.length);
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			PopMenu();
		}
		
		public function PushState()
		{
			//trace("Pushing state, length:", stateStack.length);
			stateStack.push(GetState());
			currentState.x = 0;
			currentState.y = 0;
			currentState.pos = 0;
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
		
		public function GetState():Object
		{
			//trace("get state");
			return {
				"menu": Data.menuName,
				"x": sliderList.selectedX,
				"y": sliderList.selectedY,
				"pos": sliderList.listPosition
			};
		}
		
		public function GetMenuData(name:String):GFxResult
		{
			//trace("Get menu data");
			var menuResult:GFxResult = Data.getMenu(name);
			
			if (!CheckError(menuResult))
				return null;
				
			return menuResult
		}
		
		public function GetMenuGetLatent(data:Object, name:String, type:int):int
		{
			//trace("Get Menu Get Latent");
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
			//trace("Latent timeout");
			//pop cannot fail
			if (Data.latentAction == Data.LATENT_POP) {
				LoadMenu(currentState.menu, Data.popFailMenu.result, Data.popFailGet);
			} else {
				ShowNotification("$SAM_PapyrusTimeout");
				Util.playCancel();
			}

			Data.clearLatents();
		}
		
		public function CheckPushFolder(data:Object, result:GFxResult, name:String):Boolean
		{
			if (result.type == Data.RESULT_FOLDER ||
				result.type == Data.RESULT_FOLDERCHECKBOX) {
					
				PushState();
				
				LoadMenuFolder(data, result, name);
				return false;
			}
			
			return true;
		}
		
		public function PushMenu(name:String)
		{
			//trace("Push menu", name);
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
			//trace("Update Push", name);
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
			//trace("Latent callback");
			//result.traceResult();
			
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
			//trace("Update Latent Get", Data.latentAction);
			
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
//			trace("Pop Menu");
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
			
			//trace("popping menu state", stateStack.length);
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
			//trace("Update pop");
			UpdateEnterLeave(Data.menuData, data.enter);
			
			LoadMenu(name, data, result);
			
			UpdateInit(data.init);
		}
		
		public function PopMenuTo(name:String):void
		{
			//trace("Pop menu to");
			//do not pop from main
			if (stateStack.length < 2)
				return;
				
			//find stack index
			var index:int = stateStack.length;

			while (index > 0) {
				index--;
				var menuName:Object = stateStack[index].menu.toLowerCase();
				if (menuName == name.toLowerCase())
					break;
			}

			//menu not found
			if (index <= 0)
				return;
			
			//go to menu above index and pop
			Data.folderStack.length = 0;
			Data.menuFolder = null;
			
			stateStack.length = index + 1;
			if (filenameInput.visible) {
				SetTextInput(false);
			}

			PopMenu();
		}
		
		public function ResetState():void
		{
			state = STATE_MAIN;
			currentState.menu = rootMenu;
			currentState.pos = 0;
			currentState.x = 0;
			currentState.y = 0;

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
			//trace("Load Menu", data, get);
			Data.updateMenu(name, data, get);
			state = STATE_MAIN;
			Util.unselectText();

			sliderList.updateState(currentState.pos);
			sliderList.update();
			sliderList.updateSelected(currentState.x, currentState.y);

			UpdateMenus();
			Data.latentGet.Clear();
		}
		
		public function UpdateMenus()
		{
			//trace("Update Menus");
			UpdateButtonHints();
			UpdateNotification();
			UpdateTitle();
		}
		
		public function GetResult(menu:Object):GFxResult
		{
			//trace("Get Result");
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
			//trace("Call Set");
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
			//trace("Update Data Function");
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
			//trace("Refresh Values");
			var getResult:int = GetMenuGetLatent(Data.menuData, Data.menuName, Data.LATENT_REFRESH);
			if (getResult != Data.RESULT_SUCCESS)
				return;
			
			UpdateValues();
		}
		
		public function UpdateValues()
		{
			//trace("UpdateValues");
			Data.updateValues(Data.latentGet.result);
			sliderList.updateValues();
			Data.latentGet.Clear();
		}
		
		public function ReloadMenu() {
			if (this.state == STATE_FOLDER) {
				ReloadFolder();
				return;
			}
			
			//trace("Reload Menu");
			var getResult:int = GetMenuGetLatent(Data.menuData, Data.menuName, Data.LATENT_RELOAD);
			if (getResult != Data.RESULT_SUCCESS)
				return;
			
			UpdateReload();
		}
		
		public function UpdateReload()
		{
			//trace("Update reload");
			Data.updateMenu(Data.menuName, Data.menuData, Data.latentGet.result);
			Data.latentGet.Clear();
			sliderList.storeSelected();
			sliderList.update();
			sliderList.restoreSelected();
			UpdateMenus();
		}
		
		public function ReloadFolder()
		{
			//trace("reload folder");
			var folder:String = Data.getCurrentFolder();
			
			var result:GFxResult;
			if (Data.menuType == Data.MENU_FOLDERCHECKBOX) {
				result = Data.getFolderCheckbox(folder, Data.folderData.ext, Data.menuData.race);
			} else {
				result = Data.getFolder(folder, Data.folderData.ext);
			}
			
			if (!CheckError(result))
				return;
			
			LoadMenuFolder(Data.menuData, result, Data.menuName);
		}
		
		public function GetSetData(args:Array):Object
		{
			//trace("Get Set Data");
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
			//trace("Set local variable", name, value);
			Data.locals[name] = value;
		}
		
		public function SelectList(i:int) 
		{
			//trace("Select List", i);
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
			//trace("Select Int", i, value);
			Data.menuValues[i] = value;
			CallSet(i, value);
		}
		
		public function SelectFloat(i:int, value:Number)
		{
			//trace("Select Float", i, value);
			var type:int = Data.getType(i);
			if (type != Data.ITEM_TOUCH) {
				Data.menuValues[i] = value;
			}
			CallSet(i, value);
		}
		
		public function SelectCheckbox(i:int, checked:Boolean = false)
		{
			//trace("Select Checkbox", i, checked);
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
					if (Data.locals.adjustmentOrder) {
						CallDataFunction(Data.menuData.adjustment.down, [i]);
					} else {
						CallDataFunction(Data.menuData.adjustment.edit, [i, Data.menuValues[i]]);
					}
					break;
			}
		}
		
		public function SelectCheckbox2(i:int, checked:Boolean = false)
		{
			//trace("Select Checkbox2", i, checked);
			if (Data.locals.adjustmentOrder) {
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

		public function CallDataFunction(data:Object, args:Array = null):GFxResult
		{
			//trace("Call Data Function");
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
					
					if (!data.name) { 
						Data.error = "Papyrus function missing function name";
						return null;
					}
					
					if (!args) {
						args = [];
					}
					
					args.unshift(data.name); //2nd param
					args.unshift(data.script); //1st param
					break;
				case Data.FUNC_FORM:
					if (!data.name) {
						Data.error = "Papyrus function missing function name";
						return null;
					}

					if (!data.id) {
						Data.error = "Papyrus function missing id";
						return null;
					}
					break;
				case Data.FUNC_MENU:
					//trace("calling menu func");
					PushMenu(data.name);
					return Data.resultSuccess;
				case Data.FUNC_ENTRY: 
					//trace("calling entry func");
					SetEntry(data.entry);
					return Data.resultSuccess;
				case Data.FUNC_FOLDER:
					//trace("calling folder func");
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
			
			//trace("calling data function");
			//Util.traceObj(data);
			//trace("args");
			//Util.traceObj(args);
			
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
			if (!args)
				args = [];
				
			var result:GFxResult;
			try {
				result = this.sam.CallPapyrusForm(data.id, data.name, args);
				if (result)
					return result;
			}
			catch (e:Error) { trace(e.message) }
			
			Data.error = "$SAM_PapyrusTimeout";
			return null;
		}
		
		public function CallPapyrusGlobal(args:Array):GFxResult
		{
			//trace("call papyrus global");
			//Util.traceObj(args);
			
			var result:Boolean = Util.callFuncArgs(this.f4seObj.CallGlobalFunctionNoWait, args); 			
			if (result)
				return Data.resultWaiting;
				
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

		public function GetButton(type:int):BSButtonHintData
		{
			switch(type) {
				case Data.BUTTON_RESET: return this.buttonHintReset;
				case Data.BUTTON_EXTRA: return this.buttonHintExtra;
				case Data.BUTTON_SAVE: return this.buttonHintSave;
				case Data.BUTTON_LOAD: return this.buttonHintLoad;
			}
			
			return null;
		}
		
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
			if (IsWaiting())
				return;
			
			//trace("Call hotkey function", this.state, type);
			var data:Object = (this.state == STATE_FOLDER ? Data.getFolderHotkey(type) : Data.getHotkey(type));
			if (!data)
				return;
				
			var result:GFxResult;

			switch (data.type) {
				case Data.HOTKEY_TOGGLE:
					var local:String = data["var"];
				
					//If no toggle property, initialize one
					if (!Data.locals.hasOwnProperty(local))
						Data.locals[local] = false;
					
					var toggled:Boolean = !Data.locals[local];
					Data.locals[local] = toggled;
					
					var button:BSButtonHintData = GetButton(type);
					button.ButtonText = (toggled ? data.on : data.off);
					
					if (data.func) {
						result = CallDataFunction(data.func, [toggled]);
						if (CheckError(result)) {
							UpdateDataFunction(data.func);
							Util.playOk();
						} else  {
							Util.playCancel();
						}
					} else {
						Util.playOk();
					}

					break;
				case Data.HOTKEY_FUNC:
					result = CallDataFunction(data.func);
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

		internal function BackButton():void
		{
			PopMenu();
			Util.playCancel();
		}
		
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
			//trace("Update button hints");
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
			
			UpdateButton(button, data);
		}
		
		internal function UpdateFolderHotkey(button:BSButtonHintData, type:int)
		{
			var data:Object = Data.getFolderHotkey(type);
			if (!data) {
				button.ButtonVisible = false;
				return;
			}
			
			UpdateButton(button, data);
		}
		
		internal function UpdateButton(button:BSButtonHintData, data:Object)
		{
			button.ButtonVisible = true;
			switch (data.type) {
				case Data.HOTKEY_FUNC:
					button.ButtonText = data.name;
					button.ButtonClickDisabled = false;
					break;
				case Data.HOTKEY_HOLD:
					button.ButtonText = data.name;
					button.ButtonClickDisabled = true;
					break;
				case Data.HOTKEY_TOGGLE:
					var property:String = data["var"];
					if (!Data.locals.hasOwnProperty(property))
						Data.locals[property] = false;
					button.ButtonText = (Data.locals[property] ? data.on : data.off);
					button.ButtonClickDisabled = false;
					break;
			}
		}
		
		internal function RefreshNotification()
		{
			if (notification.message.text != notificationMessage)
				ShowNotification(notificationMessage);
		}
		
		internal function UpdateNotification()
		{
			if (Data.latentNotification.success) {
				notificationMessage = Data.latentNotification.result.result
				ShowNotification(notificationMessage);
				Data.latentNotification.Clear();
			} else {
				notificationMessage = null;
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
		
		public function ToggleOrder(checked:Boolean):GFxResult
		{
			sliderList.update();
			
			return Data.resultSuccess;
		}
		
		public function InitOffset():GFxResult
		{
			Data.locals.adjustmentOrder = sam.GetNodeIsOffsetOnly(Data.locals.boneName);
			buttonHintLoad.ButtonText = (Data.locals.adjustmentOrder ? "$SAM_Offset" : "$SAM_Pose");
			sliderList.title.text = Data.locals.boneName;
			
			return Data.resultSuccess;
		}
		
		public function ToggleOffset(checked:Boolean):GFxResult
		{
			var previousName:String = Data.locals.boneName;
			Data.locals.boneName = sam.ToggleNodeName(Data.locals.boneName);
			Data.locals.adjustmentOrder = sam.GetNodeIsOffset(Data.locals.boneName);
			buttonHintLoad.ButtonText = (Data.locals.adjustmentOrder ? "$SAM_Offset" : "$SAM_Pose");
			sliderList.title.text = Data.locals.boneName;
			if (previousName != Data.locals.boneName) {
				RefreshValues();
			}

			return Data.resultSuccess;
		}
		
		public function ToggleFolderCheckbox(checked:Boolean):GFxResult
		{
			sliderList.update();
						
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
		
		public function ToggleLightVisible(checked:Boolean, selectedLight:int):GFxResult
		{
			var isVisible:Boolean = Data.toggleLightVisible(selectedLight);
			Data.locals.lightVisible = isVisible;
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");

			return Data.resultSuccess;
		}
		
		public function ToggleAllLightsVisible(checked:Boolean):GFxResult
		{
			var isVisible:Boolean = Data.toggleAllLightsVisible();
			Data.locals.allLightsVisible = isVisible;
			buttonHintExtra.ButtonText = (isVisible ? "$SAM_Visible" : "$SAM_Invisible");
			
			return Data.resultSuccess;
		}
		
		public function GetAllLightsVisible():GFxResult
		{
			Data.locals.allLightsVisible = Data.getAllLightsVisible();
			
			return Data.resultSuccess;
		}
		
		public function GetLightVisible(selectedLight:int):GFxResult
		{
			Data.locals.lightVisible = Data.getLightVisible(selectedLight);
			
			return Data.resultSuccess;
		}
		
		public function InitAddItem():GFxResult
		{
			if (!Data.locals.hasOwnProperty("addItemEquip"))
				Data.locals.addItemEquip = false;
				
			return Data.resultSuccess;
		}
		
		public function OpenEntryIfEmpty():GFxResult
		{
			//trace("open entry if empty", Data.menuOptions.length);
			if (Data.menuOptions.length == 0) {
				var hotkey:Object = FindEntryHotkey();
				if (hotkey)
					SetEntry(hotkey.func.entry);
			}
			
			return Data.resultSuccess;
		}
		
		public function FindEntryHotkey():Object
		{
			//TODO this will break if the number of hotkeys increases
			for (var i:int = 1; i < 5; ++i) {
				var hotkey:Object = Data.getHotkey(i);
				if (hotkey && hotkey.func && hotkey.func.type == Data.FUNC_ENTRY) {
					return hotkey;
				}
			}
			
			return null;
		}
		
		public function LoadSkeletonAdjustment():GFxResult
		{
			return Data.resultSuccess;
		}
		
		public function RemoveEquipment(index:int, formId:uint):GFxResult
		{
			var result:GFxResult = sam.RemoveEquipment(index, formId);
			
			if (result.type != Data.RESULT_ERROR) {
				Data.removeMenuIndex(index);
			} else {
				showNotification(result.result);
			}
			
			sliderList.update();
			
			return Data.resultSuccess;
		}
		
		public function RemoveAllEquipment():GFxResult
		{
			var result:GFxResult = sam.RemoveAllEquipment();
			
			if (result.type != Data.RESULT_ERROR) {
				Data.setMenuSize(0);
			} else {
				showNotification(result.result);
			}
			
			sliderList.update();
			
			return Data.resultSuccess;
		}
		
		public function ResetSkeletonAdjustment(race:Boolean):GFxResult
		{
			var result:GFxResult = sam.ResetSkeletonAdjustment(race);
			
			if (result && result.type != Data.RESULT_ERROR) {
				var folderResult:GFxResult = Data.getFolderCheckbox(Data.folderData.path, Data.folderData.ext, race);
				if (folderResult && folderResult.type != Data.RESULT_ERROR) {
					Data.menuFolder = folderResult.result;
					sliderList.updateValues();
				}
			}

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
				Util.playCancel();
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