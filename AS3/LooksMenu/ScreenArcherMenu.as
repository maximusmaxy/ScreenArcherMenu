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
		
		public var sliderList:SliderList;
		public var ButtonHintBar_mc:BSButtonHintBar;
		public var filenameInput:MovieClip;
		public var border:MovieClip;
		
		internal var buttonHintData:Vector.<BSButtonHintData > ;
		internal var buttonHintExit:BSButtonHintData;
		internal var buttonHintSave:BSButtonHintData;
		internal var buttonHintLoad:BSButtonHintData;
		internal var buttonHintReset:BSButtonHintData;
		internal var buttonHintBack:BSButtonHintData;
		internal var buttonHintConfirm:BSButtonHintData;
		internal var buttonHintSwap:BSButtonHintData;
		internal var buttonHintNew:BSButtonHintData;
		
		internal var closeTimer:Timer;
		private var delayClose:Boolean = false;
		
		public var BGSCodeObj:Object;
		public var f4seObj:Object;

		public var swapped:Boolean = false;
		
		public var testButton:TextField;
		public var testButton2:TextField;
		
		public function ScreenArcherMenu()
		{
			super();
			
			Util.debug = true;
			
			this.BGSCodeObj = new Object();
			Extensions.enabled = true;
			initButtonHints();
			
			filenameInput.visible = false;
			sliderList.bUseShadedBackground = false;
			ButtonHintBar_mc.bUseShadedBackground = true;
			ButtonHintBar_mc.BackgroundAlpha = 0.05;
			ButtonHintBar_mc.ShadedBackgroundMethod = "Flash";
			
			state = MAIN_STATE;
			menuStack = new <int>[];
			updateState();
			
			if (Util.debug) {
				addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown);
			}
			
			testButton.addEventListener(MouseEvent.CLICK, onClick);
			testButton2.addEventListener(MouseEvent.CLICK, onClick2);
		}
		
		internal function onClick(event:MouseEvent)
		{
			trace("test");
			root.f4se.plugins.ScreenArcherMenu.Test();
		}
		
		internal function onClick2(event:MouseEvent)
		{
			trace("test");
			root.f4se.plugins.ScreenArcherMenu.Test2();
		}
		
		internal function initButtonHints():void
		{
			buttonHintData = new Vector.<BSButtonHintData>();
			buttonHintExit = new BSButtonHintData("$SAM_Exit","Tab","PSN_B","Xenon_B",1,back);
			buttonHintBack = new BSButtonHintData("$SAM_Back","Tab","PSN_B","Xenon_B",1,back);
			buttonHintSave = new BSButtonHintData("$SAM_Save","X","PSN_L1","Xenon_L1",1,save);
			buttonHintLoad = new BSButtonHintData("$SAM_Load","E","PSN_R1","Xenon_R1",1,load);
			buttonHintReset = new BSButtonHintData("$SAM_Reset","R","PSN_Y","Xenon_Y",1,reset);
			buttonHintConfirm = new BSButtonHintData("$SAM_Confirm","Enter","PSN_A","Xenon_A",1,confirm);
			buttonHintSwap = new BSButtonHintData("$SAM_Swap","Z","PSN_R2","Xenon_R2",1,swap);
			buttonHintNew = new BSButtonHintData("$SAM_New","X","PSN_L1","Xenon_L1",1,newButton);
			buttonHintData.push(buttonHintExit);
			buttonHintData.push(buttonHintSave);
			buttonHintData.push(buttonHintLoad);
			buttonHintData.push(buttonHintReset);
			buttonHintData.push(buttonHintBack);
			buttonHintData.push(buttonHintConfirm);
			buttonHintData.push(buttonHintSwap);
			buttonHintData.push(buttonHintNew);
			ButtonHintBar_mc.SetButtonHintData(buttonHintData);
		}
		
		public function menuOpened(data:Object)
		{
			Data.load(data, root, this.f4seObj, stage);
			if (data.title) {
				sliderList.title.text = data.title;
			}
			Util.playOk();
		}
		
		public function consoleRefUpdated(data:Object)
		{
			switch (this.state) {
//				case MORPH_STATE:
//					if (data.morphArray.length != 0)
//					{
//						Data.morphValues = data.morphArray;
//						Data.updateRealMorphValues();
//					}
//					sliderList.updateValues();
//					break;
				case EYE_STATE:
					Data.menuValues[0] = data.eyeX;
					Data.menuValues[1] = data.eyeY;
					sliderList.updateValues();
					break;
				case HACK_STATE:
					Data.menuValues[0] = data.hacks[0];
					Data.menuValues[1] = data.hacks[1];
					Data.menuValues[2] = data.hacks[2];
					sliderList.updateValues();
					break;
			}
			if (data.reset) {
				resetState();
			}
			if (data.title) {
				sliderList.title.text = data.title;
			}
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
					if (buttonHintBack.ButtonVisible || buttonHintExit.ButtonVisible) {
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
					} else if (buttonHintNew.ButtonVisible) {
						newButton();
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
		
		public function pushState(state:int)
		{
			menuStack.push(this.state);
			this.state = state;
			updateState();
		}
		
		public function popState()
		{
			this.state = menuStack.pop();
			updateState();
		}
		
		public function resetState()
		{
			this.state = MAIN_STATE;
			menuStack.length = 0;
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
					Data.menuValues = Data.boneTransform;
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
					Data.menuOptions = Data.morphFiles;
					sliderList.updateList(selectAdjustmentFile);
					break;
				case LOADMFG_STATE:
					Data.loadMfgFiles();
					Data.menuOptions = Data.morphFiles;
					sliderList.updateList(selectMfgFile);
					break;
				case SAVEMFG_STATE:
				case SAVEADJUSTMENT_STATE:
					setTextInput(true);
					break;
				case EYE_STATE:
					Data.loadEyes();
					sliderList.updateEyes(selectEye);
					break;
				case HACK_STATE:
					Data.loadHacks();
					Data.menuOptions = Data.HACK_NAMES;
					Data.menuValues = Data.hackValues;
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
			}
			Data.selectedSlider = null;
			updateButtonHints();
		}
		
		public function selectMenu(id:int):void
		{
			switch (id)
			{
				case 0: pushState(ADJUSTMENT_STATE); break;
				case 1: pushState(IDLECATEGORY_STATE); break;
				case 2:	pushState(MORPHCATEGORY_STATE) ; break;
				case 3: pushState(EYE_STATE); break;
				case 4:	pushState(HACK_STATE); break;
			}
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
			Data.menuValues[id] = value;
			switch (id)
			{
				case 0:
					Data.setAdjustmentScale(value);
					break;
				case 1:
					Data.resetAdjustment();
					break;
				case 2:
					pushState(SAVEADJUSTMENT_STATE);
					break;
				case 3:
					Data.setAdjustmentPersistent(value);
					break;
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
			Data.setHack(id, enabled)
		}

		internal function confirm():void
		{
			switch (this.state) {
				case SAVEMFG_STATE: Data.saveMfg(filenameInput.Input_tf.text); break;
				case SAVEADJUSTMENT_STATE: Data.saveAdjustment(filenameInput.Input_tf.text); break;
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
			}
		}

		internal function load():void
		{
			switch (this.state) {
				case MORPH_STATE: 
				case MORPHCATEGORY_STATE:
					pushState(LOADMFG_STATE); break;
				case ADJUSTMENT_STATE: pushState(LOADADJUSTMENT_STATE); break;
			}
		}
		
		internal function newButton():void
		{
			switch (this.state) {
				case ADJUSTMENT_STATE:
					Data.newAdjustment();
					sliderList.updateAdjustment(selectAdjustment, editAdjustment, removeAdjustment);
					break;
			}
		}
		
		internal function selectMfgFile(id:int)
		{
			var update:Boolean = (this.menuStack[this.menuStack.length - 1] == MORPH_STATE);
			Data.loadMfg(id, update);
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
			else
			{
				popState();
			}
			Util.playCancel();
		}
		
		internal function exit():void
		{
			Util.unselectText();
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
			switch (state)
			{
				case MAIN_STATE :
					buttonHintExit.ButtonVisible = true;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintBack.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintNew.ButtonVisible = false;
					break;
				case ADJUSTMENT_STATE :
					buttonHintExit.ButtonVisible = false;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = true;
					buttonHintReset.ButtonVisible = false;
					buttonHintBack.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintNew.ButtonVisible = true;
					break;
				case TRANSFORM_STATE :
				case IDLECATEGORY_STATE:
				case IDLE_STATE:
					buttonHintExit.ButtonVisible = false;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = true;
					buttonHintBack.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintNew.ButtonVisible = false;
					break;
				case MORPH_STATE :
				case MORPHCATEGORY_STATE:
					buttonHintExit.ButtonVisible = false;
					buttonHintSave.ButtonVisible = true;
					buttonHintLoad.ButtonVisible = true;
					buttonHintReset.ButtonVisible = true;
					buttonHintBack.ButtonVisible = true;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintNew.ButtonVisible = false;
					break;
				case SAVEMFG_STATE :
				case SAVEADJUSTMENT_STATE:
					buttonHintExit.ButtonVisible = false;
					buttonHintBack.ButtonVisible = true;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = true;
					buttonHintSwap.ButtonVisible = false;
					buttonHintNew.ButtonVisible = false;
					break;
				default:
					buttonHintExit.ButtonVisible = false;
					buttonHintBack.ButtonVisible = true;
					buttonHintSave.ButtonVisible = false;
					buttonHintLoad.ButtonVisible = false;
					buttonHintReset.ButtonVisible = false;
					buttonHintConfirm.ButtonVisible = false;
					buttonHintSwap.ButtonVisible = true;
					buttonHintNew.ButtonVisible = false;
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
			if (state != SAVEMFG_STATE && state != SAVEADJUSTMENT_STATE)
			{
				exit();
			}
		}
	}
}