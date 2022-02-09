package {
	import flash.display.DisplayObject;

    public class Data {
		public static var sam:Object;
		public static var f4seObj:Object;
		public static var stage:DisplayObject;
		
		public static var menuOptions:Object;
		public static var menuValues:Array;
		
        public static var morphValues:Array;
		public static var morphFiles:Array;

		public static var selectedAdjustment:int;
		public static var selectedCategory:int = 0;
		public static var selectedBone:int;
		public static var boneTransform:Array;
		public static var boneName:String;
		
		public static var eyeX:Number;
		public static var eyeY:Number;
		public static var delayClose:Boolean;
		public static var hideMenu:Boolean;
		
		public static var scriptType:String;
		public static var scriptHandleHigh:uint;
		public static var scriptHandleLow:uint;
		
		public static var hackValues:Array;
		
		public static var selectedSlider:SliderListEntry = null;
		public static var selectedText:SliderListEntry = null;
		
		public static const MORPH_DIRECTORY:String = "Data/F4SE/Plugins/SAM/FaceMorphs";
		public static const ADJUSTMENT_DIRECTORY:String = "Data/F4SE/Plugins/SAF/Adjustments";

        public static const MAIN_MENU:Vector.<String> = new <String>[
            "$SAM_PoseAdjustMenu",
            "$SAM_FaceMorphsMenu",
            "$SAM_EyesMenu",
            "$SAM_HacksMenu",
        ];
		
		public static const TRANSFORM_NAMES:Vector.<String> = new <String>[
			"$SAM_RotX",
			"$SAM_RotY",
			"$SAM_RotZ",
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_Scale"
		];
		
		public static const EYE_NAMES:Vector.<String> = new <String>[
			"$SAM_EyeX",
			"$SAM_EyeY",
			"$SAM_EyeTrackingHack",
		];
		
		public static const HACK_NAMES:Vector.<String> = new <String>[
			"$SAM_BlinkHack",
			"$SAM_MorphHack",
			"$SAM_EyeTrackingHack",
		];
		
		public static function load(data:Object, root:Object, f4se:Object, stageObj:DisplayObject)
		{
			sam = root.f4se.plugins.ScreenArcherMenu;
			f4seObj = f4se;
			
			delayClose = data.delayClose;
			
			stage = stageObj;
			
			hideMenu = true;
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
					menuOptions = ["New Adjustment"];
					menuOptions.values = [0];
				}
			}
		}
		
		public static function loadAdjustmentFiles()
		{
			morphFiles = new Array();
			try 
			{
				var files:Array = f4seObj.GetDirectoryListing(ADJUSTMENT_DIRECTORY,"*.json");
				for (var i:int = 0; i < files.length; i++) {
					var filename = files[i]["nativePath"]; //name is bugged so use nativePath instead
					var startIndex:int = filename.lastIndexOf("\\") + 1;
					morphFiles[i] = filename.substring(startIndex, filename.length - 5);
				}
			}
			catch (e:Error)
			{
				trace("Failed to load adjustment files");
				if (Util.debug) {
					for (var y:int = 0; y < 1000; y++) {
						morphFiles[y] = y.toString();
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
		
		public static function loadCategories()
		{
			try {
				menuOptions = sam.GetCategoryList();
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
				}
			}
		}
		
		public static function loadBones()
		{
			try {
				menuOptions = sam.GetNodeList(selectedCategory);
			}
			catch (e:Error)
			{
				trace("Failed to load bones");
				if (Util.debug) {
					menuOptions = [
						"COM",
						"Pelvis",
						"Spine1",
						"Spine2"
					];
				}
			}
		}

		public static function loadTransforms() 
		{
			try {
				boneTransform = sam.GetNodeTransform(selectedCategory, selectedBone, selectedAdjustment);
				if (boneTransform.length != 0) {
					boneName = boneTransform.pop();
				} else {
					boneTransform = [
						0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
					];
					boneName = "";
				}
			}
			catch (e:Error)
			{
				trace("Failed to load bone transform");
				boneTransform = [
					0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
				];
				boneName = "";
			}
		}
		
		public static function setTransform(id:int, value:Number)
		{
			boneTransform[id] = value;
			if (boneName == ""){
				return;
			}
			try 
			{
				if (id < 3) //rot
				{ 
					sam.SetNodeRotation(boneName, selectedAdjustment, boneTransform[0], boneTransform[1], boneTransform[2]);
				} 
				else if (id < 6) //pos
				{
					sam.SetNodePosition(boneName, selectedAdjustment, boneTransform[3], boneTransform[4], boneTransform[5]);
				}
				else //scale
				{
					sam.SetNodeScale(boneName, selectedAdjustment, boneTransform[6]);
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
				boneTransform[i] = 0.0;
			}
			boneTransform[6] = 1.0;
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
				menuOptions = sam.GetMorphCategories();
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
						"Tongue"
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
		
		public static function loadMfgFiles()
		{
			morphFiles = new Array();
			try
			{
				var files:Array = f4seObj.GetDirectoryListing(MORPH_DIRECTORY,"*.txt");
				for (var i:int = 0; i < files.length; i++) {
					var filename = files[i]["nativePath"]; //name is bugged so use nativePath instead
					var startIndex:int = filename.lastIndexOf("\\") + 1;
					morphFiles[i] = filename.substring(startIndex, filename.length - 4);
				}
			}
			catch (e:Error)
			{
				trace("Get directory listing failed");
				if (Util.debug){
					for (var y:int = 0; y < 1000; y++) {
						morphFiles[y] = y.toString();
					}
				}
			}
		}
		
		public static function saveMfg(filename:String)
		{
			try
			{
				sam.SaveMorphPreset(filename);
			}
			catch (e:Error)
			{
				trace("Failed to save preset");
			}
		}
		
		public static function loadMfg(id:int, update:Boolean) 
		{
			var filename:String = morphFiles[id];
			try
			{
				var morphs:Object = sam.LoadMorphPreset(filename, selectedCategory);
				if (update) {
					menuOptions = morphs.names;
					menuValues = morphs.values;
				}
			}
			catch (e:Error)
			{
				trace("Failed to load file " + filename);
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
				if (id < 2) //eye coord
				{
					sam.SetEyeCoords(menuValues[0], menuValues[1]);
				}
				else //eye tracking hack
				{
					sam.SetEyeTrackingHack(menuValues[2]);
				}
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
				hackValues = sam.GetHacks();
			}
			catch (e:Error)
			{
				trace("Failed to load hacks");
				hackValues = new Array();
				for (var i:int = 0; i < HACK_NAMES.length; i++) {
					hackValues[i] = false;
				}
			}
		}
		
		public static function setHack(id:int, enabled:Boolean)
		{
			hackValues[id] = enabled;
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
    }
}