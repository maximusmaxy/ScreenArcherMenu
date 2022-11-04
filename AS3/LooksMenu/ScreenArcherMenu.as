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
		public var currentState:Object;
		public var stateStack:Array;
		
		//menu states
		public static const MAIN_STATE:int = 0;
		public static const MORPHCATEGORY_STATE:int = 1;
		public static const MORPH_STATE = 2;
		public static const LOADMFG_STATE:int = 3;
		public static const SAVEMFG_STATE:int = 4;
		public static const POSECATEGORY_STATE:int = 5;
		public static const POSENODE_STATE:int = 6;
		public static const TRANSFORM_STATE:int = 7;
		public static const EYE_STATE:int = 8;
		public static const HACK_STATE:int = 9;
		public static const ADJUSTMENT_STATE:int = 10;
		public static const LOADADJUSTMENT_STATE:int = 11;
		public static const SAVEADJUSTMENT_STATE:int = 12;
		public static const EDITADJUSTMENT_STATE:int = 13;
		public static const IDLECATEGORY_STATE:int = 14;
		public static const IDLE_STATE:int = 15;
		public static const POSEEXPORT_STATE:int = 16;
		public static const SAVEPOSE_STATE:int = 17;
		public static const LOADPOSE_STATE:int = 18;
		public static const SKELETONADJUSTMENT_STATE:int = 19;
		public static const POSITIONING_STATE:int = 20;
		public static const OPTIONS_STATE:int = 21;
		public static const POSEPLAY_STATE:int = 22;
		public static const RENAMEADJUSTMENT_STATE:int = 23;
		public static const CAMERA_STATE:int = 24;
		public static const LIGHTSELECT_STATE:int = 25;
		public static const LIGHTEDIT_STATE:int = 26;
		public static const LIGHTCATEGORY_STATE:int = 27;
		public static const LIGHTOBJECT_STATE:int = 28;
		public static const LIGHTSWAPCATEGORY_STATE:int = 29;
		public static const LIGHTSWAPOBJECT_STATE:int = 30;
		public static const SAVELIGHT_STATE:int = 31;
		public static const LOADLIGHT_STATE:int = 32;
		public static const RENAMELIGHT_STATE:int = 33;
		public static const LIGHTSETTINGS_STATE:int = 34;
		public static const RACEADJUSTMENT_STATE:int = 35;
		public static const POSEEXPORTTYPE_STATE:int = 36;
		public static const MORPHTONGUE_STATE:int = 37;
		
		//Error checks
		public static const NO_CHECK = 0;
		public static const TARGET_CHECK = 1;
		public static const SKELETON_CHECK = 2;
		public static const MORPHS_CHECK = 3;
		public static const EYE_CHECK = 4;
		public static const CAMERA_CHECK = 5;
		
		public var sliderList:SliderList;
		public var ButtonHintBar_mc:BSButtonHintBar;
		public var filenameInput:MovieClip;
		public var border:MovieClip;
		public var notification:MovieClip;
		
		public var titleName:String = "";
		
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
		public var hidden:Boolean = false;
		public var saved:Boolean = false;
		public var textInput:Boolean = false;
		public var multi:Boolean = false;
		public var widescreen:Boolean = false;
		public var targetRace:Boolean = false;
		public var order:Boolean = false;
		public var offset:Boolean = false;
		
		public static const HELD_X:int = 1;
		
		public var held:Boolean = false;
		public var heldType:int = 0;
		public var heldFuncLeft:Function = null;
		public var heldFuncRight:Function = null;
		
		//menu's listed here don't require an actor to be targeted
		public var targetIgnore:Array = [
			HACK_STATE,
			POSITIONING_STATE,
			OPTIONS_STATE,
			CAMERA_STATE,
			LIGHTSELECT_STATE,
			LIGHTCATEGORY_STATE,
			LIGHTOBJECT_STATE,
		];
		
		//Menu's listed here won't reset to main menu on console target hotswap
		public var resetIgnore:Array = [
			EYE_STATE,
			HACK_STATE,
			IDLECATEGORY_STATE,
			IDLE_STATE,
			POSITIONING_STATE,
			OPTIONS_STATE,
			POSEPLAY_STATE,
			CAMERA_STATE,
			LIGHTSELECT_STATE,
			LIGHTCATEGORY_STATE,
			LIGHTOBJECT_STATE,
		];
		
		public var mainMenuOptions:Array = [
			{
				state: ADJUSTMENT_STATE,
				check: SKELETON_CHECK
			},
			{
				state: SKELETONADJUSTMENT_STATE,
				check: SKELETON_CHECK
			},
			{
				state: RACEADJUSTMENT_STATE,
				check: SKELETON_CHECK
			},
			{
				state: POSEPLAY_STATE,
				check: SKELETON_CHECK
			},
			{
				state: POSEEXPORTTYPE_STATE,
				check: SKELETON_CHECK
			},
			{
				state: IDLECATEGORY_STATE,
				check: NO_CHECK
			},
			{
				state: POSITIONING_STATE,
				check: NO_CHECK,
				ignore: true
			},
			{
				state: MORPHCATEGORY_STATE,
				check: MORPHS_CHECK
			},
			{
				state: EYE_STATE,
				check: EYE_CHECK
			},
			{
				state: LIGHTSELECT_STATE,
				check: NO_CHECK,
				ignore: true
			},
			{
				state: CAMERA_STATE,
				check: CAMERA_CHECK,
				ignore: true
			},
			{
				state: HACK_STATE,
				check: NO_CHECK,
				ignore: true
			},
			{
				state: OPTIONS_STATE,
				check: NO_CHECK,
				ignore: true
			}
		];
		
		public var keys = {
			a: 0,
			b: 0,
			x: 0,
			y: 0,
			l1: 0,
			r1: 0,
			select: 0,
			left: 0,
			up: 0,
			right: 0,
			down: 0
		}
		
		//public var testbutton:TextField;
		
		public function ScreenArcherMenu()
		{
			super();
			
			Util.debug = false;
			widescreen = false;
			
			this.BGSCodeObj = new Object();
			Extensions.enabled = true;
			Translator.Create(root);
			initButtonHints();
			
			filenameInput.visible = false;
			sliderList.bUseShadedBackground = false;
			ButtonHintBar_mc.bUseShadedBackground = true;
			ButtonHintBar_mc.BackgroundAlpha = 0.05;
			ButtonHintBar_mc.ShadedBackgroundMethod = "Flash";
			notification.visible = false;
			
			state = MAIN_STATE;
			stateStack = [];
			currentState = {
				"menu": MAIN_STATE,
				"pos": 0,
				"x": 0,
				"y": 0
			};
			updateState();
			updateAlignment();
			
			//addEventListener(PlatformChangeEvent.PLATFORM_CHANGE, onPlatformChange);
//			if (Util.debug) {
//				addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
//				addEventListener(KeyboardEvent.KEY_UP, onKeyUp);
//			}
			//addEventListener("F4SE::Initialized", onF4SEInitialized);
			
			//testbutton.addEventListener(MouseEvent.CLICK, onTestClick);
		}
		
//		internal function onTestClick(event:Event):void
//		{
//			trace("test");
//			root.f4se.plugins.ScreenArcherMenu.Test();
//		}
		
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
			//buttonHintTarget = new BSButtonHintData("","E","PSN_R1","Xenon_R1",1,targetButton);
			buttonHintData.push(buttonHintExit);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintHide);
			buttonHintData.push(buttonHintSave);
			buttonHintData.push(buttonHintLoad);
			buttonHintData.push(buttonHintExtra);
			//buttonHintData.push(buttonHintTarget);
			buttonHintData.push(buttonHintReset);
			buttonHintData.push(buttonHintConfirm);
			ButtonHintBar_mc.SetButtonHintData(buttonHintData);
		}
		
		public function menuOpened(data:Object)
		{
			Data.load(data, root, this.f4seObj, stage);

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

			if (data.saved) {
				this.state = data.saved.state;
				currentState = data.saved.current;
				stateStack = data.saved.stack;
				textInput = data.saved.text;
				sliderList.focused = data.saved.focused;
				sliderList.updateState(currentState.pos);
				offset = data.saved.offset;
				updateState();
			}
			
			Util.playOk();
		}
		
		public function consoleRefUpdated(data:Object)
		{
			if (data.updated) {
				switch (this.state) {
					case EYE_STATE:
						Data.loadEyes();
						sliderList.updateValues();
						break;
					case HACK_STATE:
						Data.loadHacks();
						sliderList.updateValues();
						break;
					case POSITIONING_STATE:
						Data.loadPositioning();
						sliderList.updateValues();
						break;
				}
				
				if (resetIgnore.indexOf(this.state) < 0) {
					resetState();
				}
				
				Util.playOk();
			}
			
//			if (data.idle) {
//				showNotification(data.idle);
//			} else {
//				hideNotification();
//			}

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
			if (held) {
				return;
			}
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
		
		public function processKeyHeld(keyCode:uint)
		{
			switch (keyCode)
			{
				case 37://Left
				case 65://A
				case 268://Pad Left
					if (heldFuncLeft != null) {
						heldFuncLeft();
					}
					break;
				case 39://Right
				case 68://D
				case 269://Pad Right
					if (heldFuncRight != null) {
						heldFuncRight();
					}
					break;
			}
		}
		
		public function processKeyRepeat(keyCode:uint)
		{
			if (held) {
				processKeyHeld(keyCode);
				return;
			}
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
				case 88://X
				case 278://Pad X
					disableHold(HELD_X, onZMove);
					break;
//				case 257://Mouse2
//					show();
//					break;
			}
		}
		
		public function setScriptHandle(data:Object)
		{
			Data.scriptType = data.__type__;
			Data.scriptHandleHigh = data.__handleHigh__;
			Data.scriptHandleLow = data.__handleLow__;
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
		
		public function checkError(id:int):Boolean
		{
			if (Data.checkError(id)) {
				hideNotification();
				return true;
			} else {
				showNotification(Data.ERROR_NAMES[id]);
				return false;
			}
		}
		
		public function checkIgnore(id:int, arr:Array):Boolean
		{
			if (arr.indexOf(id) >= 0) {
				hideNotification();
				return true;
			}
			return false;
		}
		
		public function checkTargetIgnore(id:int):Boolean
		{
			return checkIgnore(id, targetIgnore);
		}
		
		public function getState():Object
		{
			return {
				"menu": this.state,
				"pos": sliderList.listPosition,
				"x": sliderList.selectedX,
				"y": sliderList.selectedY
			};
		}
		
		public function pushState(id:int)
		{
			if (!checkTargetIgnore(id) && !checkError(TARGET_CHECK)) return;
			
			stateStack.push(getState());
			
			this.state = id;
			
			currentState.x = 0;
			currentState.y = 0;
			
			sliderList.updateState(0);
			
			updateState();
		}
		
		public function popState()
		{
			currentState = stateStack.pop();
			
			this.state = currentState.menu;
			sliderList.updateState(currentState.pos);
			
			updateState();
		}
		
		public function resetState()
		{
			this.state = MAIN_STATE;
			sliderList.updateState(0);
			currentState.x = 0;
			currentState.y = 0;
			stateStack.length = 0;

			if (filenameInput.visible) {
				setTextInput(false);
			}
			
			updateState();
		}
		
		public function pushFolder(id:int, func:Function)
		{
			stateStack.push(getState());
			sliderList.updateState(0);
			sliderList.updateFolder(func);
			sliderList.updateSelected(0, 0);
		}
		
		public function popFolder(func:Function) 
		{
			currentState = stateStack.pop();
			sliderList.updateState(currentState.pos);
			sliderList.updateFolder(func);
			sliderList.updateSelected(currentState.x, currentState.y);
		}
		
		internal function updateState()
		{
			Util.unselectText();
			switch(this.state)
			{
				case MAIN_STATE:
					Data.menuOptions = Data.MAIN_MENU;
					sliderList.updateList(selectMenu);
					break;
				case ADJUSTMENT_STATE:
					Data.loadAdjustmentList();
					sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
					break;
				case EDITADJUSTMENT_STATE:
					Data.editAdjustment();
					sliderList.updateAdjustmentEdit(selectEditAdjustment);
					break;
				case POSECATEGORY_STATE:
					Data.loadCategories();
					sliderList.updateList(selectCategory);
					break;
				case POSENODE_STATE:
					Data.loadBones();
					sliderList.updateList(selectBone);
					break;
				case TRANSFORM_STATE:
					Data.loadTransforms();
					sliderList.updateTransform(selectTransform);
					break;
				case MORPHCATEGORY_STATE:
					Data.loadMorphCategories();
					sliderList.updateList(selectMorphCategory);
					break;
				case MORPH_STATE:
					Data.loadMorphs();
					sliderList.updateMorphs(selectMorph);
					break;
				case LOADADJUSTMENT_STATE:
					Data.loadAdjustmentFiles();
					Data.menuOptions = Data.menuFiles;
					sliderList.updateList(selectAdjustmentFile);
					break;
				case LOADMFG_STATE:
					Data.loadMfgFiles();
					Data.menuOptions = Data.menuFiles;
					sliderList.updateList(selectMfgFile);
					break;
				case SAVEMFG_STATE:
				case SAVEADJUSTMENT_STATE:
				case SAVEPOSE_STATE:
				case RENAMEADJUSTMENT_STATE:
				case SAVELIGHT_STATE:
				case RENAMELIGHT_STATE:
					setTextInput(true);
					break;
				case EYE_STATE:
					Data.loadEyes();
					sliderList.updateEyes(selectEye);
					break;
				case HACK_STATE:
					Data.loadHacks();
					Data.menuOptions = Data.HACK_NAMES;
					sliderList.updateCheckboxes(selectHack);
					break;
				case IDLECATEGORY_STATE:
					Data.loadIdleCategories();
					sliderList.updateList(selectIdleCategory);
					break;
				case IDLE_STATE:
					Data.loadIdles();
					sliderList.updateList(selectIdle);
					break;
				case POSEEXPORT_STATE:
					Data.getPoseList();
					sliderList.updateCheckboxes(selectPose);
					break;
				case POSEEXPORTTYPE_STATE:
					Data.loadPoseExport();
					sliderList.updateList(selectExportType);
					break;
				case POSEPLAY_STATE:
					Data.loadSamPoses();
					sliderList.updateFolder(selectPosePlay);
					break;
				case LOADPOSE_STATE:
					Data.loadPoseFiles();
					Data.menuOptions = Data.menuFiles;
					sliderList.updateList(selectPoseFile);
					break;
				case SKELETONADJUSTMENT_STATE:
					Data.getSkeletonAdjustments(false);
					if (multi) {
						sliderList.updateCheckboxes(selectSkeletonAdjustment);
					} else {
						sliderList.updateList(selectSkeletonAdjustment);
					}
					break;
				case RACEADJUSTMENT_STATE:
					Data.getSkeletonAdjustments(true);
					if (multi) {
						sliderList.updateCheckboxes(selectRaceAdjustment);
					} else {
						sliderList.updateList(selectRaceAdjustment);
					}
					break;
				case POSITIONING_STATE:
					Data.loadPositioning();
					sliderList.updatePositioning(selectPositioning);
					break;
				case OPTIONS_STATE:
					Data.loadOptions();
					sliderList.updateCheckboxes(selectOptions);
					break;
				case CAMERA_STATE:
					Data.loadCamera();
					sliderList.updateCamera(selectCamera);
					break;
				case LIGHTSELECT_STATE:
					Data.loadLightSelect();
					sliderList.updateList(selectLightSelect);
					break;
				case LIGHTEDIT_STATE:
					Data.loadLightEdit();
					sliderList.updateLight(selectLightEdit);
					break;
				case LIGHTCATEGORY_STATE:
				case LIGHTSWAPCATEGORY_STATE:
					Data.loadLightCategory();
					sliderList.updateList(selectLightCategory);
					break;
				case LIGHTOBJECT_STATE:
				case LIGHTSWAPOBJECT_STATE:
					Data.loadLightObject();
					sliderList.updateList(selectLightObject);
					break;
				case LOADLIGHT_STATE:
					Data.loadLightFiles();
					Data.menuOptions = Data.menuFiles;
					sliderList.updateList(selectLightFile);
					break;
				case LIGHTSETTINGS_STATE:
					Data.loadLightSettings();
					sliderList.updateLightSettings(selectLightSettings);
					break;
				case MORPHTONGUE_STATE:
					Data.menuOptions = Data.TONGUEBONES_NAMES;
					sliderList.updateList(selectMorphTongue);
					break;
			}
			sliderList.updateSelected(currentState.x, currentState.y);
			order = false;
			updateButtonHints();
			updateNotification();
			updateTitle();
		}
		
		public function updateFolder():void
		{
			Data.popFolder();
			switch(this.state) {
				case POSEPLAY_STATE: popFolder(selectPosePlay); break;
			}
		}
		
		public function selectMenu(id:int):void
		{
			var menu:Object = mainMenuOptions[id];
			
			if (!menu.ignore && !checkError(TARGET_CHECK)) return;
			
			if (menu.check > 0 && !checkError(menu.check)) return;
			
			pushState(menu.state);
		}
		
		public function selectAdjustment(id:int):void
		{
			Data.selectedAdjustment = Data.menuValues[id];
			pushState(POSECATEGORY_STATE);
		}
		
		public function editAdjustment(id:int):void
		{
			Data.selectedAdjustment = Data.menuValues[id];
			pushState(EDITADJUSTMENT_STATE);
		}
		
		public function selectEditAdjustment(id:int, value:Object = null):void
		{
			switch (id)
			{
				case 0:
					Data.menuValues[id] = value;
					Data.setAdjustmentScale(value);
					break;
				case 1:
					pushState(SAVEADJUSTMENT_STATE);
					break;
				case 2:
					pushState(RENAMEADJUSTMENT_STATE);
					break;
				case 3:
					Data.resetAdjustment();
					break;
				default:
					Data.negateAdjustmentGroup(id);
			}
		}
		
		public function removeAdjustment(id:int):void
		{
			Data.removeAdjustment(Data.menuValues[id]);
			Data.loadAdjustmentList();
			sliderList.storeSelected();
			sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
			sliderList.restoreSelected();
		}
		
		public function downAdjustment(id:int):void
		{
			if (Data.moveAdjustment(id, true)) {
				Data.loadAdjustmentList();
				sliderList.storeSelected();
				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
				if (sliderList.focused) {
					sliderList.storeY++;
				}
				sliderList.restoreSelected();
			}
		}
		
		public function upAdjustment(id:int):void
		{
			if (Data.moveAdjustment(id, false)) {
				Data.loadAdjustmentList();
				sliderList.storeSelected();
				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
				if (sliderList.focused) {
					sliderList.storeY--;
				}
				sliderList.restoreSelected();
			}
		}
		
		public function selectCategory(id:int):void
		{
			Data.selectedCategory = Data.menuValues[id];
			pushState(POSENODE_STATE);
		}
		
		public function selectBone(id:int):void
		{
			Data.selectedBone = Data.menuValues[id];
			Data.getNodeName();
			offset = false;
			pushState(TRANSFORM_STATE);
		}
		
		public function selectTransform(id:int, value:Number):void
		{
			Data.setTransform(id, value);
			if (id >= 7) {
				sliderList.updateValues();
			}
		}
		
		public function selectMorphCategory(id:int):void
		{
			Data.selectedCategory = id;
			//If selected tongue bones
			if (id == Data.menuOptions.length - 1) {
				pushState(MORPHTONGUE_STATE);
			} else {
				pushState(MORPH_STATE);
			}
		}
		
		public function selectMorph(id:int, value:Number):void
		{
			Data.setMorph(id, int(value));
		}
		
		public function selectEye(id:int, value:Object):void
		{
			Data.setEye(id, value);
		}
		
		public function selectHack(id:int, enabled:Boolean):void
		{
			Data.setHack(id, enabled);
		}
		
		public function selectPositioning(id:int, value:Number = 0):void
		{
			Data.selectPositioning(id, value);
			if (id > 0) {
				sliderList.updateValues();
			}
		}

		internal function confirmButton():void
		{
			switch (this.state) {
				case SAVEMFG_STATE: Data.saveMfg(filenameInput.Input_tf.text); break;
				case SAVEADJUSTMENT_STATE: Data.saveAdjustment(filenameInput.Input_tf.text); break;
				case SAVEPOSE_STATE: Data.savePose(filenameInput.Input_tf.text); break;
				case RENAMEADJUSTMENT_STATE: Data.renameAdjustment(filenameInput.Input_tf.text); break;
				case SAVELIGHT_STATE: Data.saveLights(filenameInput.Input_tf.text); break;
				case RENAMELIGHT_STATE: Data.renameLight(filenameInput.Input_tf.text); break;
			}
			setTextInput(false);
			popState();
			Util.playOk();
		}
		
		internal function setTextInput(enabled:Boolean)
		{
			textInput = enabled;
			if (enabled)
			{
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

		internal function saveButton():void
		{
			switch (this.state) {
				case MORPH_STATE:
				case MORPHCATEGORY_STATE:
					pushState(SAVEMFG_STATE); break;
				case ADJUSTMENT_STATE: pushState(SAVEADJUSTMENT_STATE); break;
				case POSEEXPORT_STATE: pushState(SAVEPOSE_STATE); break;
				case LIGHTSELECT_STATE: pushState(SAVELIGHT_STATE); break;
			}
		}

		internal function loadButton():void
		{
			switch (this.state) {
				case MORPH_STATE: 
				case MORPHCATEGORY_STATE:
					pushState(LOADMFG_STATE); break;
				case ADJUSTMENT_STATE: pushState(LOADADJUSTMENT_STATE); break;
				case POSEEXPORT_STATE: pushState(LOADPOSE_STATE); break;
				case LIGHTSELECT_STATE: pushState(LOADLIGHT_STATE); break;
				case POSEPLAY_STATE: //a-pose
					Data.resetPose(2);
					break;
				case TRANSFORM_STATE: //offset toggle
					//only toggle if node isn't offset only
					if (!Data.getNodeIsOffset()) {
						if (Data.toggleNodeName()) {
							offset = !offset;
							buttonHintLoad.ButtonText = offset ? "$SAM_Pose" : "$SAM_Offset";
							Data.loadTransforms();
							sliderList.updateValues();
							updateTitle();
							Util.playOk();
						}
					}
					break;
			}
		}
		
		internal function selectMfgFile(id:int)
		{
			Data.loadMfg(id);
		}
		
		internal function selectAdjustmentFile(id:int)
		{
			Data.loadAdjustment(id);
			popState();
		}
		
		internal function selectIdleCategory(id:int)
		{
			Data.selectedCategory = id;
			pushState(IDLE_STATE);
		}
		
		internal function selectIdle(id:int)
		{
			Data.playIdle(id);
			showNotification(Data.menuOptions[id]);
		}
		
		internal function selectPose(id:int, enabled:Boolean)
		{
			Data.menuValues[id] = enabled;
		}
		
		internal function selectPoseExport(id:int)
		{
			Data.selectedCategory = id;
			pushState(POSEEXPORT_STATE);
		}
		
		internal function selectPoseFile(id:int)
		{
			Data.loadPose(id);
		}
		
		internal function selectSkeletonAdjustment(id:int, enabled:Boolean = true)
		{
			Data.loadSkeletonAdjustment(id, false, !multi, enabled);
		}
		
		internal function selectRaceAdjustment(id:int, enabled:Boolean = true)
		{
			Data.loadSkeletonAdjustment(id, true, !multi, enabled);
		}
		
		internal function selectPosePlay(id:int)
		{
			if (Data.selectSamPose(id)) {
				pushFolder(id, selectPosePlay);
			}
		}
		
		internal function selectOptions(id:int, enabled:Boolean)
		{
			switch (id) {
				case 0://hotswap
					break;
				case 1://alignment
					swapped = enabled;
					updateAlignment();
					break;
				case 2://widescreen
					widescreen = enabled;
					updateAlignment();
					break;
				case 3://autoplay
					Data.autoPlay = enabled;
					break;
			}
			Data.setOption(id, enabled);
		}
		
		internal function selectCamera(id:int, value:Number = 0)
		{
			Data.setCamera(id, value);
			Data.loadCamera();
			sliderList.updateValues();
		}
		
		internal function selectLightSelect(id:int)
		{
			if (id < Data.menuOptions.length - 3) { //light
				Data.selectedAdjustment = id;
				pushState(LIGHTEDIT_STATE);
			} else if (id < Data.menuOptions.length - 2) { //add new
				pushState(LIGHTCATEGORY_STATE);
			} else if (id < Data.menuOptions.length - 1) { //add console
				Data.addLight();
				Data.loadLightSelect();
				sliderList.updateList(selectLightSelect);
			} else {
				pushState(LIGHTSETTINGS_STATE);
			}
		}
		
		internal function selectLightEdit(id:int, value:Number = 0)
		{
			if (id < 5) { //properties
				Data.editLight(id, value);
				sliderList.updateValues();
			} else if (id < 6) { //rename
				pushState(RENAMELIGHT_STATE);
			} else if (id < 7) { //swap
				pushState(LIGHTSWAPCATEGORY_STATE);
			} else { //delete
				Data.deleteLight();
				popState();
			}
		}
		
		internal function selectLightCategory(id:int)
		{
			Data.selectedCategory = id;
			switch (this.state) {
				case LIGHTCATEGORY_STATE: pushState(LIGHTOBJECT_STATE); break;
				case LIGHTSWAPCATEGORY_STATE: pushState(LIGHTSWAPOBJECT_STATE); break;
			}
		}
		
		internal function selectLightObject(id:int)
		{
			switch (this.state) {
				case LIGHTOBJECT_STATE:
					Data.createLight(id);
					break;
				case LIGHTSWAPOBJECT_STATE:
					Data.swapLight(id);
					break;
			}
			//need to double pop so fake pop first
			stateStack.pop();
			popState();
		}
		
		internal function selectLightFile(id:int)
		{
			Data.loadLights(id);
			popState();
		}
		
		internal function selectLightSettings(id:int, value:Number = 0)
		{
			if (id < 4) { // pos/rot
				Data.editLightSettings(id, value);
				sliderList.updateValues();
			} else if (id < 5) { //update all
				Data.updateAllLights();
			} else { //delete all
				Data.deleteAllLights();
			}
		}
		
		internal function selectExportType(id:int)
		{
			Data.selectedCategory = id;
			pushState(POSEEXPORT_STATE);
		}
		
		internal function selectMorphTongue(id:int)
		{
			Data.getMorphsTongue(id);
			offset = false;
			pushState(TRANSFORM_STATE);
		}

		internal function resetButton():void
		{
			switch (this.state) {
				case ADJUSTMENT_STATE:
					order = !order;
					updateAdjustment();
					break;
				case TRANSFORM_STATE:
					Data.resetTransform(); 
					break;
				case MORPH_STATE:
				case MORPHCATEGORY_STATE:
					var update:Boolean = (this.state == MORPH_STATE);
					Data.resetMorphs(update);
					break;
				case IDLECATEGORY_STATE:
				case IDLE_STATE:
					Data.resetIdle();
					hideNotification();
					break;
				case POSEEXPORT_STATE:
				case POSEPLAY_STATE:
					Data.resetIdle();
					Data.resetPose(1);
					break;
				case SKELETONADJUSTMENT_STATE:
					Data.resetSkeletonAdjustment(false);
					Data.getSkeletonAdjustments(false);
					break;
				case RACEADJUSTMENT_STATE:
					Data.resetSkeletonAdjustment(true);
					Data.getSkeletonAdjustments(true);
					break;
				case POSITIONING_STATE:
					Data.resetPositioning();
					break;
				case LIGHTEDIT_STATE:
					Data.resetLight();
					break;
				case LIGHTSETTINGS_STATE:
					Data.resetLightSettings();
					break;
			}
			sliderList.updateValues();
			Util.playOk();
		}

		internal function backButton():void
		{
			if (this.filenameInput.visible)
			{
				setTextInput(false);
			}
			if (this.state == MAIN_STATE)
			{
				exit();
			}
			else if (Data.folderStack.length > 0) //folder state
			{
				Data.popFolder();
				switch(this.state) {
					case POSEPLAY_STATE: popFolder(selectPosePlay); break;
				}			
			}
			else
			{
				popState();
			}
			Util.playCancel();
		}
		
		internal function exit():void
		{
			Util.unselectText();
			if (!saved) {
				Data.clearState();
			}
			if (Data.delayClose)
			{
				//delay close event so it doesn't close multiple menus at once
				closeTimer = new Timer(100,1);
				closeTimer.addEventListener(TimerEvent.TIMER_COMPLETE, function(e:TimerEvent) {
					close();
				});
				closeTimer.start();
			}
			else
			{
				close();
			}
		}
		
		public function close()
		{
			try {
				root.f4se.plugins.ScreenArcherMenu.SamCloseMenu("ScreenArcherMenu");
			}
			catch (e:Error)
			{
				trace("No escape");
			}
		}
		
		public function extraButton():void
		{
			switch(this.state)
			{
				case SKELETONADJUSTMENT_STATE: //multi
					sliderList.storeSelected();
					multi = !multi;
					if (multi) {
						buttonHintExtra.ButtonText = "$SAM_Multi";
						sliderList.updateCheckboxes(selectSkeletonAdjustment);
					} 
					else
					{
						buttonHintExtra.ButtonText = "$SAM_Single";
						sliderList.updateList(selectSkeletonAdjustment);
					}
					sliderList.restoreSelected();
					break;
				case RACEADJUSTMENT_STATE: //multi
					sliderList.storeSelected();
					multi = !multi;
					if (multi) {
						buttonHintExtra.ButtonText = "$SAM_Multi";
						sliderList.updateCheckboxes(selectRaceAdjustment);
					} 
					else
					{
						buttonHintExtra.ButtonText = "$SAM_Single";
						sliderList.updateList(selectRaceAdjustment);
					}
					sliderList.restoreSelected();
					break;
				case ADJUSTMENT_STATE: //new
					Data.newAdjustment();
					updateAdjustment();
					break;
				case TRANSFORM_STATE: //negate
					Data.negateAdjustment();
					Data.loadTransforms();
					sliderList.updateValues();
					break;
				case IDLECATEGORY_STATE: //z-rotate
				case IDLE_STATE:
				case POSEPLAY_STATE:
					enableHold(HELD_X, onZMove, onZLeft, onZRight);
					break;
				case LIGHTSELECT_STATE: //visible
				case LIGHTSETTINGS_STATE:
					buttonHintExtra.ButtonText = (Data.toggleLightsVisibility() ? "$SAM_Invisible" : "$SAM_Visible");
					break;
				case LIGHTEDIT_STATE: //visible
					buttonHintExtra.ButtonText = (Data.toggleLightVisible() ? "$SAM_Invisible" : "$SAM_Visible");
					break;
			}
		}
		
		public function enableHold(type:int, move:Function, left:Function, right:Function)
		{
			if (!held) {
				held = true;
				heldType = type;
				stage.addEventListener(MouseEvent.MOUSE_MOVE, move);
				heldFuncMove = move;
				heldFuncLeft = left;
				heldFuncRight = right;
				Data.setCursorVisible(false);
				Data.storeCursorPos();
				sliderList.storeSelected();
			}
		}
		
		public function disableHold(type:int, move:Function) {
			if (held && heldType == type) {
				held = false;
				stage.removeEventListener(MouseEvent.MOUSE_MOVE, move);
				heldFuncMove = null;
				heldFuncLeft = null;
				heldFuncRight = null;
				Data.setCursorVisible(true);
				Data.endCursorDrag();
				sliderList.restoreSelected();
			}
		}
		
		public function onZMove(event:MouseEvent) {
			Data.rotateIdle(NaN);
		}
		
		public function onZLeft() {
			Data.rotateIdle(-2.0);
		}
		
		public function onZRight() {
			Data.rotateIdle(2.0);
		}

		public function targetButton()
		{
			targetRace = !targetRace;
			buttonHintTarget.ButtonText = targetRace ? "$SAM_NPC" : "$SAM_Race";
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
			buttonHintBack.ButtonText = state == MAIN_STATE ? "$SAM_Exit" : "$SAM_Back";
			buttonHintBack.ButtonVisible = true;
			switch (state)
			{
				case MAIN_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					break;
				case ADJUSTMENT_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintLoad.ButtonText = "$SAM_Load";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_New";
					buttonHintExtra.ButtonClickDisabled = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = order ? "$SAM_Edit" : "$SAM_Order";
					break;
				case TRANSFORM_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintLoad.ButtonText = offset ? "$SAM_Pose" : "$SAM_Offset";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Negate";
					buttonHintExtra.ButtonClickDisabled = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					break;
				case IDLECATEGORY_STATE:
				case IDLE_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Rotate";
					buttonHintExtra.ButtonClickDisabled = true;
					break;
				case POSITIONING_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					break;
				case MORPH_STATE:
				case MORPHCATEGORY_STATE:
				case POSEEXPORT_STATE:
					buttonHintSave.ButtonVisible = true;
					buttonHintLoad.ButtonVisible = true;
					buttonHintLoad.ButtonText = "$SAM_Load";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					break;
				case SAVEMFG_STATE:
				case SAVEADJUSTMENT_STATE:
				case SAVEPOSE_STATE:
				case RENAMEADJUSTMENT_STATE:
				case SAVELIGHT_STATE:
				case RENAMELIGHT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = true;
					buttonHintHide.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					break;
				case SKELETONADJUSTMENT_STATE:
				case RACEADJUSTMENT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = multi ? "$SAM_Multi" : "$SAM_Single";
					buttonHintExtra.ButtonClickDisabled = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					break;
				case POSEPLAY_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintLoad.ButtonText = "$SAM_Apose";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Rotate";
					buttonHintExtra.ButtonClickDisabled = true;
					break;
				case LIGHTSELECT_STATE:
					buttonHintSave.ButtonVisible = true;
					buttonHintLoad.ButtonVisible = true;
					buttonHintLoad.ButtonText = "$SAM_Load";
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = (Data.getLightsVisibility() ? "$SAM_Invisible" : "$SAM_Visible");
					buttonHintExtra.ButtonClickDisabled = false;
					break;
				case LIGHTSETTINGS_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = (Data.getLightsVisibility() ? "$SAM_Invisible" : "$SAM_Visible");
					buttonHintExtra.ButtonClickDisabled = false;
					break;
				case LIGHTEDIT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = "$SAM_Reset";
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = (Data.getLightVisible() ? "$SAM_Invisible" : "$SAM_Visible");
					buttonHintExtra.ButtonClickDisabled = false;
					break;
				default:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintHide.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
					buttonHintExtra.ButtonClickDisabled = false;
			}
		};
		
		internal function updateNotification()
		{
			switch (state)
			{
				case IDLECATEGORY_STATE:
				case IDLE_STATE:
					showNotification(Data.getIdleName());
					break;
				default:
					hideNotification();
					break;
			}
		}
		
		public function updateTitle(name:String = null)
		{
			if (name) {
				titleName = name;
			}
			switch (this.state) {
				case TRANSFORM_STATE: sliderList.title.text = Data.boneName; break;
				default: sliderList.title.text = titleName; break;
			}
		}

		internal function hideButton():void
		{
			if (!filenameInput.visible) {
				hidden = Data.toggleMenu();
				sliderList.isEnabled = !hidden;
			}
		}
		
		internal function updateAdjustment():void
		{
			sliderList.storeSelected();
			if (order) {
				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
			} else {
				sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
			}
			sliderList.restoreSelected();
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

		public function tryClose():void
		{
			if (!filenameInput.visible)
			{
				sliderList.getState(currentState);
				currentState.menu = this.state;

				var data:Object = {
					state: this.state,
					current: currentState,
					stack: stateStack,
					text: textInput,
					focused: sliderList.focused,
					offset: offset
				}
				
				Data.saveState(data);
				saved = true;
				exit();
			}
		}
	}
}