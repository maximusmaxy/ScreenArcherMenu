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
		
		//Error checks
		public static const NO_CHECK = 0;
		public static const TARGET_CHECK = 1;
		public static const SKELETON_CHECK = 2;
		public static const MORPHS_CHECK = 3;
		public static const EYE_CHECK = 4;
		
		public var sliderList:SliderList;
		public var ButtonHintBar_mc:BSButtonHintBar;
		public var filenameInput:MovieClip;
		public var border:MovieClip;
		public var notification:MovieClip;
		
		internal var buttonHintData:Vector.<BSButtonHintData > ;
		internal var buttonHintExit:BSButtonHintData;
		internal var buttonHintSave:BSButtonHintData;
		internal var buttonHintLoad:BSButtonHintData;
		internal var buttonHintReset:BSButtonHintData;
		internal var buttonHintBack:BSButtonHintData;
		internal var buttonHintConfirm:BSButtonHintData;
		internal var buttonHintSwap:BSButtonHintData;
		internal var buttonHintExtra:BSButtonHintData;
		//internal var buttonHintTarget:BSButtonHintData;
		
		internal var closeTimer:Timer;
		private var delayClose:Boolean = false;
		
		public var BGSCodeObj:Object;
		public var f4seObj:Object;

		public var swapped:Boolean = false;
		public var hidden:Boolean = false;
		public var saved:Boolean = false;
		public var multi:Boolean = false;
		public var widescreen:Boolean = false;
		public var targetRace:Boolean = false;
		public var order:Boolean = false;
		
		public var targetIgnore:Array = [
			HACK_STATE,
			POSITIONING_STATE,
			OPTIONS_STATE
		];
		
		public var resetIgnore:Array = [
			EYE_STATE,
			HACK_STATE,
			IDLECATEGORY_STATE,
			IDLE_STATE,
			POSITIONING_STATE,
			OPTIONS_STATE,
			POSEPLAY_STATE
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
				state: POSEPLAY_STATE,
				check: SKELETON_CHECK
			},
			{
				state: POSEEXPORT_STATE,
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
				state: HACK_STATE,
				check: NO_CHECK,
				ignore: true
			},
			{
				state: OPTIONS_STATE,
				check: NO_CHECK
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
		
		public function ScreenArcherMenu()
		{
			super();
			
			Util.debug = true;
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
			}
			updateState();
			updateAlignment();
			
			//addEventListener(PlatformChangeEvent.PLATFORM_CHANGE, onPlatformChange);
			if (Util.debug) {
				addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
				addEventListener(KeyboardEvent.KEY_UP, onKeyUp);
			}
		}
		
		internal function initButtonHints():void
		{
			buttonHintData = new Vector.<BSButtonHintData>();
			buttonHintBack = new BSButtonHintData("$SAM_Back","Tab","PSN_B","Xenon_B",1,backButton);
			buttonHintSave = new BSButtonHintData("$SAM_Save","Q","PSN_L1","Xenon_L1",1,saveButton);
			buttonHintLoad = new BSButtonHintData("$SAM_Load","E","PSN_R1","Xenon_R1",1,loadButton);
			buttonHintReset = new BSButtonHintData("$SAM_Reset","R","PSN_Y","Xenon_Y",1,resetButton);
			buttonHintConfirm = new BSButtonHintData("$SAM_Confirm","Enter","PSN_A","Xenon_A",1,confirmButton);
			buttonHintSwap = new BSButtonHintData("$SAM_Swap","Z","PSN_Select","Xenon_Select",1,swapButton);
			buttonHintExtra = new BSButtonHintData("","X","PSN_X","Xenon_X",1,extraButton);
			//buttonHintTarget = new BSButtonHintData("","E","PSN_R1","Xenon_R1",1,targetButton);
			buttonHintData.push(buttonHintExit);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintSwap);
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
				sliderList.title.text = data.title;
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
				state = data.saved.state;
				sliderList.listPosition = data.saved.slider;
				
				stateStack.length = 0;
				Util.packObjectArray(stateStack, "menu", data.saved.stateStack);
				Util.packObjectArray(stateStack, "pos", data.saved.posStack);
				Util.packObjectArray(stateStack, "x", data.saved.xStack);
				Util.packObjectArray(stateStack, "y", data.saved.yStack);
				
				updateState();
			}
			
			Util.playOk();
		}
		
		public function consoleRefUpdated(data:Object)
		{
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

			sliderList.title.text = data.title;
			
			notification.visible = false;
			Util.playOk();
		}
		
		public function onF4SEObjCreated(obj:Object)
		{
			this.f4seObj = obj;
		}
		
		public function onPlatformChange(event:PlatformChangeEvent)
		{
			switch (event.uiPlatform) {
				case PlatformChangeEvent.PLATFORM_PC_KB_MOUSE:
					break;
				case PlatformChangeEvent.PLATFORM_PC_GAMEPAD:
					break;
			}
		}
		
		public function onKeyDown(event:KeyboardEvent) 
		{
			processKeyDown(event.keyCode);
		}
		
		public function onKeyUp(event:KeyboardEvent) 
		{

		}
		
		public function processKeyDown(keyCode:uint)
		{
			//https://www.creationkit.com/fallout4/index.php?title=DirectX_Scan_Codes
			trace(keyCode);
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
				case 69://E
					if (buttonHintLoad.ButtonVisible) {
						loadButton();
					} 
//					else if (buttonHintTarget.ButtonVisible) {
//						targetButton();
//					}
					break;
				case 81://Q
					if (buttonHintSave.ButtonVisible) {
						saveButton();
					}
					break;
				case 82://R
					if (buttonHintReset.ButtonVisible) {
						resetButton();
					}
					break;
				case 88://X
					if (buttonHintExtra.ButtonVisible) {
						extraButton();
					}
					break;
				case 90://Z
					if (buttonHintSwap.ButtonVisible) {
						swapButton();
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
				
			}
		}
		
		public function processKeyUp(keyCode:uint)
		{
			switch (keyCode)
			{
				case 257://Mouse2
					show();
					break;
			}
		}
		
		public function setScriptHandle(data:Object)
		{
			Data.scriptType = data.__type__;
			Data.scriptHandleHigh = data.__handleHigh__;
			Data.scriptHandleLow = data.__handleLow__;
		}
		
		public function checkError(id:int):Boolean
		{
			if (Data.checkError(id)) {
				notification.visible = false;
				return true;
			} else {
				notification.visible = true;
				notification.message.text = Data.ERROR_NAMES[id];
				return false;
			}
		}
		
		public function checkIgnore(id:int, arr:Array):Boolean
		{
			if (arr.indexOf(id) >= 0) {
				notification.visible = false;
				return true;
			}
			return false;
		}
		
		public function checkTargetIgnore(id:int):Boolean
		{
			return checkIgnore(id, targetIgnore);
		}
		
		public function pushState(id:int)
		{
			if (!checkTargetIgnore(id) && !checkError(TARGET_CHECK)) return;
			
			stateStack.push({
				"menu": this.state,
				"pos": sliderList.listPosition,
				"x": sliderList.selectedX,
				"y": sliderList.selectedY
			});
			
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
					Data.getSkeletonAdjustments();
					if (multi) {
						sliderList.updateCheckboxes(selectSkeletonAdjustment);
					} else {
						sliderList.updateList(selectSkeletonAdjustment);
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
			}
			sliderList.updateSelected(currentState.x, currentState.y);
			order = false;
			updateButtonHints();
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
				case 4:
					Data.menuValues[id] = value;	
					Data.setAdjustmentPersistent(value);
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
			if (id < sliderList.entrySize - 1) {
				trace("down");
				Data.moveAdjustment(id, true);
				sliderList.storeSelected();
				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
				sliderList.storeY++;
				sliderList.restoreSelected();
			}
		}
		
		public function upAdjustment(id:int):void
		{
			if (id > 0) {
				trace("up");
				Data.moveAdjustment(id, false);
				sliderList.storeSelected();
				sliderList.updateAdjustmentOrder(selectAdjustment, downAdjustment, upAdjustment);
				sliderList.storeY--;
				sliderList.restoreSelected();
			}
		}
		
		public function selectCategory(id:int):void
		{
			Data.selectedCategory = id;
			pushState(POSENODE_STATE);
		}
		
		public function selectBone(id:int):void
		{
			Data.selectedBone = id;
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
			pushState(MORPH_STATE);
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
			}
			setTextInput(false);
			popState();
			Util.playOk();
		}
		
		internal function setTextInput(enabled:Boolean)
		{
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
		}
		
		internal function selectPose(id:int, enabled:Boolean)
		{
			Data.menuValues[id] = enabled;
		}
		
		internal function selectPoseFile(id:int)
		{
			Data.loadPose(id);
			sliderList.updateList(selectPoseFile);
		}
		
		internal function selectSkeletonAdjustment(id:int, enabled:Boolean = true)
		{
			Data.loadSkeletonAdjustment(id, false, !multi, enabled);
		}
		
		internal function selectPosePlay(id:int)
		{
			if (Data.selectSamPose(id)) {
				sliderList.updateList(selectPosePlay);
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
					break;
				case POSEEXPORT_STATE:
				case POSEPLAY_STATE:
					Data.resetIdle();
					Data.resetPose(1);
					break;
				case SKELETONADJUSTMENT_STATE:
					Data.resetSkeletonAdjustment();
					break;
				case POSITIONING_STATE:
					Data.resetPositioning();
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
					case POSEPLAY_STATE: sliderList.updateList(selectPosePlay); break;
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
					root.f4se.plugins.ScreenArcherMenu.CloseMenu("ScreenArcherMenu");
				});
				closeTimer.start();
			}
			else
			{
				root.f4se.plugins.ScreenArcherMenu.CloseMenu("ScreenArcherMenu");
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
				case ADJUSTMENT_STATE: //new
					Data.newAdjustment();
					updateAdjustment();
					break;
				case TRANSFORM_STATE: //negate
					Data.negateAdjustment();
					Data.loadTransforms();
					sliderList.updateValues();
					break;
				case POSEPLAY_STATE: //a-pose
					Data.resetPose(2);
					break;
			}
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
			buttonHintReset.ButtonText = state == "$SAM_Reset";
			switch (state)
			{
				case MAIN_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
				case ADJUSTMENT_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_New";
					buttonHintReset.ButtonVisible = true;
					buttonHintReset.ButtonText = order ? "$SAM_Edit" : "$SAM_Order";
					break;
				case TRANSFORM_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Negate";
					break;
				case IDLECATEGORY_STATE:
				case IDLE_STATE:
				case POSITIONING_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
				case MORPH_STATE:
				case MORPHCATEGORY_STATE:
				case POSEEXPORT_STATE:
					buttonHintSave.ButtonVisible = true;
					buttonHintLoad.ButtonVisible = true;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
				case SAVEMFG_STATE:
				case SAVEADJUSTMENT_STATE:
				case SAVEPOSE_STATE:
				case RENAMEADJUSTMENT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = true;
					buttonHintSwap.ButtonVisible = false;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
				case SKELETONADJUSTMENT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = true;
					//buttonHintTarget.ButtonText = targetRace ? "$SAM_NPC" : "$SAM_Race";
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = multi ? "$SAM_Multi" : "$SAM_Single";
					break;
				case POSEPLAY_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Apose";
					break;
				default:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					//buttonHintTarget.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
			}
		};

		internal function swapButton():void
		{
			if (!filenameInput.visible) {
				hidden = !hidden;
				if (hidden) {
					
				} else {
					
				}
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
				sliderList.x = swapped ? -836 : 496;
			}
			else {
				sliderList.x = swapped ? -623 : 278;
			}
		}
		
		internal function hide():void
		{
//			root.f4se.plugins.ScreenArcherMenu.HideMenu(true, Data.scriptType, Data.scriptHandleHigh, Data.scriptHandleLow);
//			if (Data.hideMenu) {
//				filenameInput.visible = false;
//				sliderList.visible = false;
//				ButtonHintBar_mc.visible = false;
//			}
		}
		
		internal function show():void
		{
//			root.f4se.plugins.ScreenArcherMenu.HideMenu(false, Data.scriptType, Data.scriptHandleHigh, Data.scriptHandleLow);
//			if (this.state == SAVEMFG_STATE || this.state == SAVEADJUSTMENT_STATE) {
//				filenameInput.visible = true;
//			} else {
//				sliderList.visible = true;
//			}
//			ButtonHintBar_mc.visible = true;
		}

		public function tryClose():void
		{
			if (!filenameInput.visible)
			{
				var data:Object = {
					state: menuState,
					slider: sliderPos,
					swap: swapped,
					stateStack: Util.unpackObjectArray(stateStack, "menu"),
					posStack: Util.unpackObjectArray(stateStack, "pos"),
					xStack: Util.unpackObjectArray(stateStack, "x"),
					yStack: Util.unpackObjectArray(stateStack, "y")
				}
				Data.saveState(data);
				saved = true;
				exit();
			}
		}
	}
}