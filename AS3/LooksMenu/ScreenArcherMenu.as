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
		public var menuStack:Vector.<int>; 
		public var sliderPosStack:Vector.<int>;
		
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
		
		internal var closeTimer:Timer;
		private var delayClose:Boolean = false;
		
		public var BGSCodeObj:Object;
		public var f4seObj:Object;

		public var swapped:Boolean = false;
		public var saved:Boolean = false;
		public var multi:Boolean = false;
		
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
			}
//			{
//				state: OPTIONS_STATE,
//				check: NO_CHECK
//			}
		];
		
		public function ScreenArcherMenu()
		{
			super();
			
			Util.debug = false;
			
			this.BGSCodeObj = new Object();
			Extensions.enabled = true;
			initButtonHints();
			
			filenameInput.visible = false;
			sliderList.bUseShadedBackground = false;
			ButtonHintBar_mc.bUseShadedBackground = true;
			ButtonHintBar_mc.BackgroundAlpha = 0.05;
			ButtonHintBar_mc.ShadedBackgroundMethod = "Flash";
			notification.visible = false;
			
			state = MAIN_STATE;
			menuStack = new <int>[];
			sliderPosStack = new <int>[];
			updateState();
			
			if (Util.debug) {
				addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
			}
		}
		
		internal function initButtonHints():void
		{
			buttonHintData = new Vector.<BSButtonHintData>();
			buttonHintBack = new BSButtonHintData("$SAM_Back","Tab","PSN_B","Xenon_B",1,back);
			buttonHintSave = new BSButtonHintData("$SAM_Save","X","PSN_L1","Xenon_L1",1,save);
			buttonHintLoad = new BSButtonHintData("$SAM_Load","E","PSN_R1","Xenon_R1",1,load);
			buttonHintReset = new BSButtonHintData("$SAM_Reset","R","PSN_Y","Xenon_Y",1,reset);
			buttonHintConfirm = new BSButtonHintData("$SAM_Confirm","Enter","PSN_A","Xenon_A",1,confirm);
			buttonHintSwap = new BSButtonHintData("$SAM_Swap","Z","PSN_R2","Xenon_R2",1,swap);
			buttonHintExtra = new BSButtonHintData("","X","PSN_L1","Xenon_L1",1,extra);
			buttonHintData.push(buttonHintExit);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintSwap);
			buttonHintData.push(buttonHintSave);
			buttonHintData.push(buttonHintLoad);
			buttonHintData.push(buttonHintExtra);
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
			
			if (data.saved) {
				state = data.saved.state;
				
				menuStack.length = 0;
				for (var i:int = 0; i < data.saved.stack.length; i++) {
					menuStack.push(data.saved.stack[i]);
				}
				
				sliderList.listPosition = data.saved.slider;
				
				sliderPosStack.length = 0;
				for (var j:int = 0; j < data.saved.sliders.length; j++) {
					sliderPosStack.push(data.saved.sliders[j]);
				}
				
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
		
		public function onKeyDown(event:KeyboardEvent) 
		{
			switch(event.keyCode)
			{
				case 13://Enter
					Util.unselectText();
					break;
				case 37://Left
					if (Data.selectedSlider != null)
					{
						Data.selectedSlider.slider.Decrement();
					}
					break;
	
				case 39://Right
					if (Data.selectedSlider != null)
					{
						Data.selectedSlider.slider.Increment();
					}
					break;
			}
		}
		
		public function processKeyDown(keyCode:uint)
		{
			//https://www.creationkit.com/fallout4/index.php?title=DirectX_Scan_Codes
			switch (keyCode)
			{
				case 9://Tab
					if (buttonHintBack.ButtonVisible) {
						back();
					}
					break;
				case 13://Enter
					if (buttonHintConfirm.ButtonVisible) {
						confirm();
					}
					Util.unselectText();
					break;
				case 37://Left
					if (Data.selectedSlider != null)
					{
						Data.selectedSlider.slider.Decrement();
					}
					break;
//				case 38://Up
//					break;
				case 39://Right
					if (Data.selectedSlider != null)
					{
						Data.selectedSlider.slider.Increment();
					}
					break;
//				case 40://Down
//					break;
				case 69://E
					if (buttonHintLoad.ButtonVisible) {
						load();
					}
					break;
				case 82://R
					if (buttonHintReset.ButtonVisible) {
						reset();
					}
					break;
				case 88://X
					if (buttonHintSave.ButtonVisible) {
						save();
					} else if (buttonHintExtra.ButtonVisible) {
						extra();
					}
					break;
				case 90://Z
					if (buttonHintSwap.ButtonVisible) {
						swap();
					}
					break;
				case 257://Mouse2
					hide();
					break;
			}
		};
		
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
			
			menuStack.push(this.state);
			this.state = id;
			
			sliderPosStack.push(sliderList.listPosition);
			sliderList.listPosition = 0;
			
			updateState();
		}
		
		public function popState()
		{
			this.state = menuStack.pop();
			sliderList.listPosition = sliderPosStack.pop();
			updateState();
		}
		
		public function resetState()
		{
			this.state = MAIN_STATE;
			sliderList.listPosition = 0;
			menuStack.length = 0;
			sliderPosStack.length = 0;
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
					Data.selectSamPose(0);
					sliderList.updateList(selectPosePlay);
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
			}
			Data.selectedSlider = null;
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
					Data.resetAdjustment();
					break;
				case 2:
					pushState(SAVEADJUSTMENT_STATE);
					break;
				case 3:
					Data.menuValues[id] = value;	
					Data.setAdjustmentPersistent(value);
					break;
				default:
					Data.negateAdjustmentGroup(id);
			}
		}
		
		public function newAdjustment():void
		{
			Data.newAdjustment();
			sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
		}
		
		public function removeAdjustment(id:int):void
		{
			Data.removeAdjustment(Data.menuValues[id]);
			Data.loadAdjustmentList();
			sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
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

		internal function confirm():void
		{
			switch (this.state) {
				case SAVEMFG_STATE: Data.saveMfg(filenameInput.Input_tf.text); break;
				case SAVEADJUSTMENT_STATE: Data.saveAdjustment(filenameInput.Input_tf.text); break;
				case SAVEPOSE_STATE: Data.savePose(filenameInput.Input_tf.text); break;
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

		internal function save():void
		{
			switch (this.state) {
				case MORPH_STATE:
				case MORPHCATEGORY_STATE:
					pushState(SAVEMFG_STATE); break;
				case ADJUSTMENT_STATE: pushState(SAVEADJUSTMENT_STATE); break;
				case POSEEXPORT_STATE: pushState(SAVEPOSE_STATE); break;
			}
		}

		internal function load():void
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
			Data.loadSkeletonAdjustment(id, !multi, enabled);
		}
		
		internal function selectPosePlay(id:int)
		{
			if (Data.selectSamPose(id)) {
				sliderList.updateList(selectPosePlay);
			}
		}

		internal function reset():void
		{
			switch (this.state) {
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

		internal function back():void
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
				if (Data.folderStack.length > 0) {
					switch(this.state) {
						case POSEPLAY_STATE: sliderList.updateList(selectPosePlay); break;
					}
				}
				else
				{
					popState();
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
		
		public function extra():void
		{
			switch(this.state)
			{
				case SKELETONADJUSTMENT_STATE: //multi
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
					break;
				case ADJUSTMENT_STATE: //new
					Data.newAdjustment();
					sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
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
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
					break;
				case ADJUSTMENT_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_New";
					break;
				case TRANSFORM_STATE :
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
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
					buttonHintExtra.ButtonVisible = false;
					break;
				case SAVEMFG_STATE:
				case SAVEADJUSTMENT_STATE:
				case SAVEPOSE_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = true;
					buttonHintSwap.ButtonVisible = false;
					buttonHintExtra.ButtonVisible = false;
					break;
				case SKELETONADJUSTMENT_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = multi ? "$SAM_Multi" : "$SAM_Single";
					break;
				case POSEPLAY_STATE:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = true;
					buttonHintExtra.ButtonText = "$SAM_Apose";
					break;
				default:
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintExtra.ButtonVisible = false;
			}
		};

		internal function swap():void
		{
			swapped = ! swapped;
			sliderList.x = swapped ? -623:278;
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
				var stack:Array = [];
				for (var i:int = 0; i < menuStack.length; i++) {
					stack.push(menuStack[i]);
				}
				var sliders:Array = [];
				for (var j:int = 0; j < sliderPosStack.length; j++) {
					sliders.push(sliderPosStack[j]);
				}
				Data.saveState(state, stack, sliderList.listPosition, sliders);
				saved = true;
				exit();
			}
		}
	}
}