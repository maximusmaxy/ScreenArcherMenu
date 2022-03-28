package {
	import flash.display.DisplayObject;

    public class Data {
		public static var sam:Object;
		public static var f4seObj:Object;
		public static var stage:DisplayObject;
		
		public static var menuOptions:Object = {};
		public static var menuValues:Array = [];
		
		public static var morphFiles:Array = [];

		public static var selectedAdjustment:int = 0;
		public static var selectedCategory:int = 0;
		public static var selectedBone:int = 0;
		public static var boneTransform:Array = [];
		public static var boneName:String = "";
		
		public static var poseHandles:Array = [];

		public static var delayClose:Boolean;
		public static var hideMenu:Boolean;
		
		public static var scriptType:String;
		public static var scriptHandleHigh:uint;
		public static var scriptHandleLow:uint;
		
		public static var selectedSlider:SliderListEntry = null;
		public static var selectedText:SliderListEntry = null;
		
		public static const MORPH_DIRECTORY:String = "Data/F4SE/Plugins/SAM/FaceMorphs";
		public static const ADJUSTMENT_DIRECTORY:String = "Data/F4SE/Plugins/SAF/Adjustments";
		public static const POSE_DIRECTORY:String = "Data/F4SE/Plugins/SAF/Poses";
		
        public static const MAIN_MENU:Vector.<String> = new <String>[
            "$SAM_PoseAdjustMenu",
			"$SAM_SkeletonAdjustMenu",
			"$SAM_PoseExportMenu",
			"$SAM_PlayIdleMenu",
			//"$SAM_Positioning",
            "$SAM_FaceMorphsMenu",
            "$SAM_EyesMenu",
            "$SAM_HacksMenu",
			//"$SAM_OptionsMenu"
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
			
			if (data.saved) {
				menuOptions = data.saved.options;
				menuValues = data.saved.values;
				morphsFiles = data.saved.files;
				selectedAdjustment = data.saved.adjustment;
				selectedCategory = data.saved.category;
				selectedBone = data.saved.bone;
				boneTransform = data.saved.transform;
				boneName = data.saved.boneName;
				poseHandles = data.saved.handles;
			}
		}
		
		public static function saveState(menuState:int, menuStack:Array, sliderPos:int, sliderPosStack:Array)
		{
			var data:Object = {
				state: menuState,
				stack: menuStack,
				options: menuOptions,
				values: menuValues,
				files: morphFiles,
				adjustment: selectedAdjustment,
				category: selectedCategory,
				bone: selectedBone,
				transform: boneTransform,
				boneName: boneName,
				handles: poseHandles,
				slider: sliderPos,
				sliders: sliderPosStack
			}
			
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
		
		public static function checkError(id:int):Boolean
		{
			try {
				return sam.CheckError(id);
			}
			catch (e:Error)
			{
				trace("Failed to check error");
				if (Util.debug)
				{
					return true;
				}
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
					menuOptions = ["New Adjustment"];
					menuValues = [0];
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
		
		public static function editAdjustment()
		{
			try
			{
				menuValues = [0, 0, 0, false];
				var adjustment:Object = sam.GetAdjustment(selectedAdjustment);
				if (adjustment.scale) {
					menuValues[0] = adjustment.scale;
				}
				if (adjustment.persistent) {
					menuValues[3] = adjustment.persistent;				
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
					menuValues = [50, 0, 0, true, "Head", "Left Arm", "Left Leg"]
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
					menuOptions = [];
					for (var i:int = 0; i < 30; i++)
					{
						menuOptions[i] = i.toString();
					}
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
		
		public static function loadMfg(id:int) 
		{
			var filename:String = morphFiles[id];
			try
			{
				sam.LoadMorphPreset(filename, selectedCategory);
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
				menuValues = sam.GetHacks();
			}
			catch (e:Error)
			{
				trace("Failed to load hacks");
				menuValues = [false, false, false];
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
				var handles:Array = []
				for (var i:int = 0; i < menuValues.length; i++) {
					if (menuValues[i]) {
						handles.push(poseHandles[i]);
					}
				}
				sam.SavePose(filename, handles);
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
				sam.LoadPose(morphFiles[id]);
			}
			catch (e:Error)
			{
				trace("failed to load pose");
			}
		}
		
		public static function loadPoseFiles()
		{
			morphFiles = new Array();
			try
			{
				var files:Array = f4seObj.GetDirectoryListing(POSE_DIRECTORY,"*.json");
				for (var i:int = 0; i < files.length; i++) {
					var filename = files[i]["nativePath"]; //name is bugged so use nativePath instead
					var startIndex:int = filename.lastIndexOf("\\") + 1;
					morphFiles[i] = filename.substring(startIndex, filename.length - 5);
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
		
		public static function resetPose()
		{
			try
			{
				sam.ResetPose();
			}
			catch (e:Error)
			{
				trace("Reset pose failed");
			}
		}
		
		public static function loadSkeletonAdjustment(id:int)
		{
			try 
			{
				sam.LoadSkeletonAdjustment(menuOptions[id]);
			}
			catch (e:Error)
			{
				trace("Failed to load skeleton adjustment");
			}
		}
		
		public static function resetSkeletonAdjustment()
		{
			try 
			{
				sam.ResetSkeletonAdjustment();
			}
			catch (e:Error)
			{
				trace("Failed to reset skeleton adjustment");
			}
		}
	}
}