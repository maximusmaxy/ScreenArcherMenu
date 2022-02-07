﻿package {
    public class Data {
		public static var sam:Object;
		public static var f4seObj:Object;
		
		public static var menuOptions:Object;
		public static var menuValues:Array;
		
        public static var morphValues:Array;
		public static var morphFiles:Array;

		public static var selectedAdjustment:int;
		public static var selectedCategory:int;
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
		
		public static const MORPH_DIRECTORY:String = "Data/F4SE/Plugins/SAM/FaceMorphs";
		public static const ADJUSTMENT_DIRECTORY:String = "Data/F4SE/Plugins/SAM/PoseAdjustments";

        public static const MORPH_ORDER:Vector.<int> = new <int>[
			//Lips
			50, 17, 5, 7, 8, 20, 21, 11, 12, 40, 28, 30, 31, 43, 44, 34, 35, 22, 23, 24, 46, 47, 48, 25, 45, 
			//Jaw
			51, 6, 29, 1, 2,
			//Eyes
			52, 18, 19, 9, 10, 41, 42, 32, 33,
			//Eyebrows
			53, 13, 14, 16, 3, 36, 37, 39, 26, 0,
			//Nose
			54, 15, 38,
			//Cheeks
			55, 4, 27,
			//Tongue
			56, 49
		];

        public static const MAIN_MENU:Vector.<String> = new <String>[
            "$SAM_PoseAdjustMenu",
            "$SAM_FaceMorphsMenu",
            "$SAM_EyesMenu",
            "$SAM_HacksMenu",
        ];
		
		public static const TRANSFORM_NAMES:Vector.<String> = new <String>[
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_RotX",
			"$SAM_RotY",
			"$SAM_RotZ",
			"$SAM_Scale"
		];
		
        public static const MORPH_NAMES:Vector.<String> = new <String>[
			"$SAM_BrowSqueeze",
			"$SAM_JawForward",
			"$SAM_JawOpen",
			"$SAM_LeftBrowOuterUp",
			"$SAM_LeftCheekUp",
			"$SAM_LeftFrown",
			"$SAM_LeftJaw",
			"$SAM_LeftLipCornerIn",
			"$SAM_LeftLipCornerOut",
			"$SAM_LeftLowerEyeLidDown",
			"$SAM_LeftLowerEyeLidUp",
			"$SAM_LeftLowerLipDown",
			"$SAM_LeftLowerLipUp",
			"$SAM_LeftMiddleBrowDown",
			"$SAM_LeftMiddleBrowUp",
			"$SAM_LeftNoseUp",
			"$SAM_LeftOuterBrowDown",
			"$SAM_LeftSmile",
			"$SAM_LeftUpperEyeLidDown",
			"$SAM_LeftUpperEyeLidUp",
			"$SAM_LeftUpperLipDown",
			"$SAM_LeftUpperLipUp",
			"$SAM_LowerLipFunnel",
			"$SAM_LowerLipRollIn",
			"$SAM_LowerLipRollOut",
			"$SAM_Pucker",
			"$SAM_RightBrowOuterUp",
			"$SAM_RightCheekUp",
			"$SAM_RightFrown",
			"$SAM_RightJaw",
			"$SAM_RightLipCornerIn",
			"$SAM_RightLipCornerOut",
			"$SAM_RightLowerEyeLidDown",
			"$SAM_RightLowerEyeLidUp",
			"$SAM_RightLowerLipDown",
			"$SAM_RightLowerLipUp",
			"$SAM_RightMiddleBrowDown",
			"$SAM_RightMiddleBrowUp",
			"$SAM_RightNoseUp",
			"$SAM_RightOuterBrowDown",
			"$SAM_RightSmile",
			"$SAM_RightUpperEyeLidDown",
			"$SAM_RightUpperEyeLidUp",
			"$SAM_RightUpperLipDown",
			"$SAM_RightUpperLipUp",
			"$SAM_StickyLips",
			"$SAM_UpperLipFunnel",
			"$SAM_UpperLipRollIn",
			"$SAM_UpperLipRollOut",
			"$SAM_TongueLift"
		];
		
		public static const DIVIDER_NAMES:Vector.<String> = new <String>[
			"$SAM_Lips",
			"$SAM_Jaw",
			"$SAM_Eyes",
			"$SAM_Eyebrows",
			"$SAM_Nose",
			"$SAM_Cheeks",
			"$SAM_Tongue"
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
		
		public static function load(data:Object, root:Object, f4se:Object)
		{
			sam = root.f4se.plugins.ScreenArcherMenu;
			f4seObj = f4se;
			trace(f4seObj);
			
			if (data.morphArray.length != 0)
			{
				Data.morphValues = data.morphArray;
			}
			else
			{
				trace("Failed initial morph load");
				Data.morphValues = new Array();
				var len:int = Data.MORPH_NAMES.length + Data.DIVIDER_NAMES.length;
				for (var i:int = 0; i < len; i++)
				{
					Data.morphValues[i] = 0;
				}
			}
			
			trace("delay close");
			delayClose = data.delayClose;
			
			hideMenu = true;
		}
		
		public static function loadAdjustmentList()
		{
			try {
				menuOptions = sam.GetAdjustmentList();
			}
			catch (e:Error)
			{
				trace("Failed to load adjustment list");
				menuOptions = ["New Adjustment"];
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
		
//		public static function saveAdjustment(filename:String)
//		{
//			try
//			{
//				sam.SaveAdjustment(filename);
//			}
//			catch (e:Error)
//			{
//				trace("Failed to save adjustment");
//			}
//		}
		
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
				menuOptions = sam.NewAdjustment();
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
				if (id < 3) //pos
				{ 
					sam.SetNodePosition(boneName, selectedAdjustment, boneTransform[0], boneTransform[1], boneTransform[2]);
				} 
				else if (id < 6) //rot
				{
					sam.SetNodeRotation(boneName, selectedAdjustment, boneTransform[3], boneTransform[4], boneTransform[5]);
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
		
		public static function loadMorphs()
		{
			menuValues = new Array();
			try
			{
				morphValues = sam.LoadMorphs();
				updateRealMorphValues();
			}
			catch (e:Error)
			{
				trace("Failed to load morphs");
				morphValues = new Array();
				for (var y:int; y < MORPH_ORDER.length; y++)
				{
					var fakeId:int = MORPH_ORDER[y];
					if (fakeId < 50) {
						menuValues[y] = 0;
						morphValues[fakeId] = 0;
					}
				}
			}
		}
		
		public static function updateRealMorphValues()
		{
			for (var i:int; i < MORPH_ORDER.length; i++)
				{
					var realId:int = MORPH_ORDER[i];
					if (realId < 50) {
						menuValues[i] = morphValues[realId];
					}
				}
		}
		
		public static function setMorph(id:int, value:int)
		{
			menuValues[id] = value;
			var realId:int = MORPH_ORDER[id];
			morphValues[realId] = value;
			try
			{
				sam.ModifyFacegenMorph(realId, value);
			}
			catch (e:Error)
			{
				trace("Failed to modify morph");
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
		
		public static function loadMfg(id:int) 
		{
			var filename:String = morphFiles[id]
			try
			{
				var morphs:Array = sam.LoadMorphPreset(filename);
				morphValues = morphs;
			}
			catch (e:Error)
			{
				trace("Failed to load file " + filename);
			}
		}
		
		public static function resetMorphs()
		{
			try 
			{
				sam.ResetMorphs();
				var len:int = Data.MORPH_NAMES.length;
				for (var i:int = 0; i < len; i++)
				{
					Data.morphValues[i] = 0;
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