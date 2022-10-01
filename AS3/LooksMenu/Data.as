package {
	import flash.display.DisplayObject;

    public class Data {
		public static var sam:Object;
		public static var f4seObj:Object;
		public static var stage:DisplayObject;
		
		public static var menuOptions:Object = {};
		public static var menuValues:Array = [];
		
		public static var menuFiles:Array = [];
		
		public static var menuFolder:Array = [];
		public static var folderStack:Array = [];

		public static var selectedAdjustment:int = 0;
		public static var selectedCategory:int = 0;
		public static var selectedBone:int = 0;
		public static var boneName:String = "";
		
		public static var lastDragValue:Number = 0;
		public static var stepValue = 100;
		
		public static var cursorStored:Boolean = false;
		public static var cursorPosX:int = -1;
		public static var cursorPosY:int = -1;
		
		public static var poseHandles:Array = [];

		public static var delayClose:Boolean;
		public static var autoPlay:Boolean;
		
		public static var scriptType:String;
		public static var scriptHandleHigh:uint;
		public static var scriptHandleLow:uint;
		
		public static var selectedText:SliderListEntry = null;
		
		public static const MORPH_DIRECTORY:String = "Data\\F4SE\\Plugins\\SAM\\FaceMorphs";
		public static const ADJUSTMENT_DIRECTORY:String = "Data\\F4SE\\Plugins\\SAF\\Adjustments";
		public static const POSE_DIRECTORY:String = "Data\\F4SE\\Plugins\\SAF\\Poses";
		public static const LIGHT_DIRECTORY:String = "Data\\F4SE\\Plugins\\SAM\\Lights";
		
		public static const KEYBOARD = 1;
		public static const PAD = 2;
		
        public static const MAIN_MENU:Vector.<String> = new <String>[
            "$SAM_PoseAdjustMenu",
			"$SAM_SkeletonAdjustMenu",
			"$SAM_RaceAdjustMenu",
			"$SAM_PosePlayMenu",
			"$SAM_PoseExportMenu",
			"$SAM_PlayIdleMenu",
			"$SAM_PositionMenu",
            "$SAM_FaceMorphsMenu",
            "$SAM_EyesMenu",
			"$SAM_LightMenu",
			"$SAM_CameraMenu",
            "$SAM_HacksMenu",
			"$SAM_OptionsMenu"
        ];
		
		public static const ERROR_NAMES:Vector.<String> = new <String>[
		    "",
			"$SAM_ConsoleError", 
			"$SAM_SkeletonError",
			"$SAM_MorphsError",
			"$SAM_EyeError",
			"$SAM_CameraError"
		];
		
		public static const TRANSFORM_NAMES:Vector.<String> = new <String>[
			"$SAM_RotX",
			"$SAM_RotY",
			"$SAM_RotZ",
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_Scale",
			"$SAM_RotX",
			"$SAM_RotY",
			"$SAM_RotZ"
		];
		
		public static const EYE_NAMES:Vector.<String> = new <String>[
			"$SAM_EyeX",
			"$SAM_EyeY"
		];
		
		public static const HACK_NAMES:Vector.<String> = new <String>[
			"$SAM_BlinkHack",
			"$SAM_MorphHack",
			"$SAM_EyeTrackingHack"
		];
		
		public static const POSITIONING_NAMES:Vector.<String> = new <String>[
			"$SAM_Step",
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_RotX",
			"$SAM_RotY",
			"$SAM_RotZ",
			"$SAM_Scale",
			"$SAM_ResetPos",
			"$SAM_ResetRot",
			"$SAM_ResetScale",
			"$SAM_TGP",
			"$SAM_TCL",
			"$SAM_EnableFootIK",
			"$SAM_DisableFootIK"
		];
		
		public static const OPTION_NAMES:Vector.<String> = new <String>[
			"$SAM_Hotswap",
			"$SAM_Alignment",
			"$SAM_Widescreen"
		];
		
		public static const CAMERA_NAMES:Vector.<String> = new <String>[
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_Yaw",
			"$SAM_Pitch",
			"$SAM_Roll",
			"$SAM_FOV"
		];
		
		public static const LIGHT_NAMES:Vector.<String> = new <String>[
			"$SAM_Distance",
			"$SAM_Rotation",
			"$SAM_Height",
			"$SAM_XOffset",
			"$SAM_YOffset",
			"$SAM_Rename",
			"$SAM_Swap",
			"$SAM_Delete"
		];
		
		public static const LIGHTSETTINGS_NAMES:Vector.<String> = new <String>[
			"$SAM_PosX",
			"$SAM_PosY",
			"$SAM_PosZ",
			"$SAM_Rotation",
			"$SAM_UpdateAll",
			"$SAM_DeleteAll"
		];
		
		public static const TONGUEBONES_NAMES:Vector.<String> = new <String>[
			"$BONE_Tongue0",
			"$BONE_Tongue1",
			"$BONE_Tongue2",
			"$BONE_Tongue3",
			"$BONE_Tongue4"
		];
		
		public static function load(data:Object, root:Object, f4se:Object, stageObj:DisplayObject)
		{
			sam = root.f4se.plugins.ScreenArcherMenu;
			f4seObj = f4se;
			
			delayClose = data.delayClose;
			
			stage = stageObj;
			
			if (data.saved) {
				menuOptions = data.saved.options;
				menuValues = data.saved.values;
				menuFiles = data.saved.files;
				selectedAdjustment = data.saved.adjustment;
				selectedCategory = data.saved.category;
				selectedBone = data.saved.bone;
				boneName = data.saved.boneName;
				poseHandles = data.saved.handles;
				stepValue = data.saved.step;
				folderStack = data.saved.folderStack;
				menuFolder = data.saved.folder;
			}
		}
		
		public static function saveState(data:Object)
		{
			data.options = menuOptions;
			data.values = menuValues;
			data.files = menuFiles;
			data.adjustment = selectedAdjustment;
			data.category = selectedCategory;
			data.bone = selectedBone;
			data.boneName = boneName;
			data.handles = poseHandles;
			data.step = stepValue;
			data.folderStack = folderStack;
			data.folder = menuFolder;
			
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
		
		public static function getFileListing(dir:String, ext:String):Boolean
		{
			menuFiles = [];
			try {
				var listing:Array = f4seObj.GetDirectoryListing(dir, ext);
				for (var i:int = 0; i < listing.length; i++) {
					var filename = listing[i]["nativePath"]; //name is bugged so use nativePath instead
					var startIndex:int = filename.lastIndexOf("\\") + 1;
					menuFiles[i] = filename.substring(startIndex, filename.length - ext.length + 1);
				}
				return true;
			}
			catch (e:Error)
			{
				trace("Failed to get file listing from directory: " + dir);
			}
			return false;
		}
		
		public static function setCursorVisible(visible:Boolean)
		{
			try 
			{
				sam.SetCursorVisible(visible);
			}
			catch(e:Error)
			{
				trace("Failed to set cursor visibility");
			}
		}
		
		public static function storeCursorPos()
		{
			try 
			{
				var result:Object = sam.GetCursorPosition();
				cursorStored = result.success;
				if (cursorStored) {
					cursorPosX = result.x;
					cursorPosY = result.y;
				}
			}
			catch (e:Error)
			{
				trace("Failed to store cursor pos");
				cursorStored = false;
			}
		}
		
		public static function updateCursorDrag(value:Number):int
		{
			if (!isNaN(value)) {
				return int(value);
			}
			if (cursorStored) {
				try 
				{
					var result:Object = sam.GetCursorPosition();
					if (result.success) {
						sam.SetCursorPosition(cursorPosX, cursorPosY);
						return result.x - cursorPosX;
					}
				}
				catch (e:Error)
				{
					trace("Failed to update cursor pos");
				}
			}
			return 0;
		}
		
		public static function endCursorDrag()
		{
			cursorStored = false;
		}
		
		public static function isLocked(type:int):Boolean
		{
			try 
			{
				return sam.GetLock(type);
			}
			catch (e:Error)
			{
				trace("Failed to get scroll lock");
			}
			return false;
		}
		
		public static function toggleMenu():Boolean
		{
			try
			{
				return sam.ToggleMenus();
			}
			catch (e:Error)
			{
				trace("Failed to hide menus");
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
					menuOptions = [
						"New Adjustment 1",
						"New Adjustment 2",
						"New Adjustment 3",
						"New Adjustment 4"
					];
					menuValues = [0, 1, 2, 3];
				}
			}
		}
		
		public static function loadAdjustmentFiles()
		{

			if (!getFileListing(ADJUSTMENT_DIRECTORY, "*.json"))
			{
				if (Util.debug) {
					for (var y:int = 0; y < 1000; y++) {
						menuFiles[y] = y.toString();
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
				menuValues = [100, 0, 0, 0];
				var adjustment:Object = sam.GetAdjustment(selectedAdjustment);
				if (adjustment.scale) {
					menuValues[0] = adjustment.scale;
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
					menuValues = [100, 0, 0, 0, "Head", "Left Arm", "Left Leg"]
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
		
		public static function moveAdjustment(id:int, inc:Boolean):Boolean
		{
			try
			{
				return sam.MoveAdjustment(menuValues[id], inc);
			}
			catch (e:Error)
			{
				trace("Failed to move adjustment");
			}
			return false;
		}
		
		public static function renameAdjustment(name:String)
		{
			try {
				sam.RenameAdjustment(selectedAdjustment, name);
			}
			catch (e:Error)
			{
				trace("Failed to rename adjustment");
			}
		}
		
		public static function loadCategories()
		{
			try {
				var categories:Object = sam.GetCategoryList();
				menuOptions = categories.names;
				menuValues = categories.values;
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
					menuValues = [];
				} else {
					menuOptions = [];
					menuValues = [];
				}
			}
		}
		
		public static function loadBones()
		{
			try {
				var nodes:Object = sam.GetNodeList(selectedCategory);
				menuOptions = nodes.names;
				menuValues = nodes.values;
			}
			catch (e:Error)
			{
				trace("Failed to load bones");
				menuOptions = [];
				menuValues = [];
				if (Util.debug) {
					for (var i:int = 0; i < 30; i++)
					{
						menuOptions[i] = i.toString();
						menuValues[i] = i;
					}
				}
			}
		}

		public static function loadTransforms() 
		{
			try {
				menuValues = sam.GetNodeTransform(selectedCategory, selectedBone, selectedAdjustment);
				if (menuValues.length != 0) {
					boneName = menuValues.pop();
				} else {
					menuValues = [
						0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
					];
					boneName = "";
				}
			}
			catch (e:Error)
			{
				trace("Failed to load bone transform");
				menuValues = [
					0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
				];
				boneName = "";
			}
		}
		
		public static function setTransform(id:int, value:Number)
		{
			if (id < 7) {
				menuValues[id] = value;
			}
			if (boneName == ""){
				return;
			}
			try 
			{
				if (id < 3) //rot
				{ 
					sam.SetNodeRotation(boneName, selectedAdjustment, menuValues[0], menuValues[1], menuValues[2]);
				} 
				else if (id < 6) //pos
				{
					sam.SetNodePosition(boneName, selectedAdjustment, menuValues[3], menuValues[4], menuValues[5]);
				}
				else if (id < 7)//scale
				{
					sam.SetNodeScale(boneName, selectedAdjustment, menuValues[6]);
				}
				else
				{
					var dif:int = updateCursorDrag(value);
					menuValues = sam.AdjustNodeRotation(boneName, selectedAdjustment, id, dif);
					if (menuValues.length == 0) {
						menuValues = [
							0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0
						];
						boneName = "";
					}
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
				menuValues[i] = 0.0;
			}
			menuValues[6] = 1.0;
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
				menuOptions.push("$SAM_TongueBones");
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
			if (!getFileListing(MORPH_DIRECTORY, "*.txt"))
			{
				if (Util.debug){
					for (var y:int = 0; y < 1000; y++) {
						menuFiles[y] = y.toString();
					}
				}
			}
		}
		
		public static function saveMfg(filename:String)
		{
			try
			{
				sam.SaveMorphsPreset(filename);
			}
			catch (e:Error)
			{
				trace("Failed to save preset");
			}
		}
		
		public static function loadMfg(id:int) 
		{
			var filename:String = menuFiles[id];
			try
			{
				sam.LoadMorphsPreset(filename, selectedCategory);
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
				sam.SetEyeCoords(menuValues[0], menuValues[1]);
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
				menuValues = [false, true, false];
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
		
		public static function rotateIdle(value:Number)
		{
			try {
				var dif:int = updateCursorDrag(value);
				sam.AdjustPositioning(6, dif, 100);
			}
			catch (e:Error)
			{
				trace("Failed to rotate idle");
			}
		}
		
		public static function getIdleName():String
		{
			try {
				return sam.GetIdleName();
			}
			catch (e:Error)
			{
				trace("Failed to get idle name");
			}
			return null;
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
				var handles:Array = [];
				for (var i:int = 0; i < menuValues.length; i++) {
					if (menuValues[i]) {
						handles.push(poseHandles[i]);
					}
				}
				sam.SavePose(filename, handles, selectedCategory);
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
				sam.LoadPose(POSE_DIRECTORY + "\\Exports\\" + menuFiles[id] + ".json");
			}
			catch (e:Error)
			{
				trace("failed to load pose");
			}
		}
		
		public static function loadPoseFiles()
		{
			if (!getFileListing(POSE_DIRECTORY + "\\Exports", "*.json"))
			{
				if (Util.debug){
					for (var y:int = 0; y < 1000; y++) {
						menuFiles[y] = y.toString();
					}
				}
			}
		}
		
		public static function resetPose(id:int)
		{
			try
			{
				sam.ResetPose(id);
			}
			catch (e:Error)
			{
				trace("Reset pose failed");
			}
		}
		
		public static function getSkeletonAdjustments(race:Boolean)
		{
			try {
				var adjustments:Object = sam.GetSkeletonAdjustments(race);
				menuOptions = adjustments.names;
				menuValues = adjustments.values;
			}
			catch (e:Error)
			{
				trace("Failed to get skeleton adjustments");
				if (Util.debug) {
					menuOptions = ["1", "2", "3", "4", "5"];
					menuValues = [false, false, true, false, false];
				} else {
					menuOptions = [];
					menuValues = [];
				}
			}
		}
		
		public static function loadSkeletonAdjustment(id:int, race:Boolean, clear:Boolean, enabled:Boolean)
		{
			if (clear) {
				for (var i:int = 0; i < menuValues.length; i++) {
					menuValues[i] = false;
				}
			}
			
			menuValues[id] = enabled;
			
			try 
			{
				sam.LoadSkeletonAdjustment(menuOptions[id], race, clear, enabled);
			}
			catch (e:Error)
			{
				trace("Failed to load skeleton adjustment");
			}
		}
		
		public static function resetSkeletonAdjustment(race:Boolean)
		{
			try 
			{
				sam.ResetSkeletonAdjustment(race);
			}
			catch (e:Error)
			{
				trace("Failed to reset skeleton adjustment");
			}
		}
		
		public static function updatePositioning()
		{
			menuValues[0] = stepValue;
			for (var i:int = 8; i < POSITIONING_NAMES.length; i++) {
				menuValues[i] = 0;
			}
		}
		
		public static function loadPositioning()
		{
			try {
				menuValues = sam.GetPositioning();
				updatePositioning();
			}
			catch (e:Error)
			{
				trace("Failed to load positioning");
				if (Util.debug) {
					menuValues = [0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
					updatePositioning();
				}
			}
		}
		
		public static function selectPositioning(id:int, value:Number = 0)
		{
			try
			{
				if (id < 1) {
					var step:int = int(value);
					menuValues[id] = step;
					stepValue = step;
				} else if (id < 8) {
					var dif:int = updateCursorDrag(value);
					menuValues = sam.AdjustPositioning(id, dif, stepValue);
					updatePositioning();
				} else {
					menuValues = sam.SelectPositioning(id);
					updatePositioning();
				}
			}
			catch (e:Error)
			{
				trace("Failed to update position");
			}
		}
		
		public static function resetPositioning()
		{
			try
			{
				menuValues = sam.ResetPositioning();
				updatePositioning();
			}
			catch (e:Error)
			{
				trace("Failed to reset position");
			}
		}
		
		public static function updateFolderNames():Array
		{
			menuOptions = [];
			for (var i:int = 0; i < menuFolder.length; i++) {
				menuOptions.push(menuFolder[i].name);
			}
		}
		
		public static function popFolder()
		{
			folderStack.pop();
			try {
				var path:String = (folderStack.length == 0) ? POSE_DIRECTORY : folderStack[folderStack.length - 1];
				menuFolder = sam.GetSamPoses(path);
				updateFolderNames();
			}
			catch (e:Error) {
				trace("Failed to get sam poses");
				menuOptions = [];
				menuFolder = [];
			}
		}
		
		public static function loadSamPoses()
		{
			try {
				var path:String = (folderStack.length == 0) ? POSE_DIRECTORY : folderStack[folderStack.length - 1];
				menuFolder = sam.GetSamPoses(path);
				updateFolderNames();
			}
			catch (e:Error)
			{
				trace("Failed to load sam poses");
				if (Util.debug)
				{
					menuFolder = [
						{
							"name": "Folder",
							"folder": true,
							"path": "test"
						},
						{
							"name": "File",
							"path": "test"
						}
					];
					updateFolderNames();
				}
			}
		}
		
		public static function selectSamPose(id:int):Boolean
		{
			try {
				if (menuFolder[id].folder) {
					var path:String = menuFolder[id].path;
					menuFolder = sam.GetSamPoses(path);
					folderStack.push(path);
					updateFolderNames();
					return true;
				} 
				else 
				{
					sam.LoadPose(menuFolder[id].path);
				}
			}
			catch (e:Error)
			{
				trace("Failed to select sam pose");
				menuOptions = [];
				menuFolder = [];
			}
			return false;
		}
		
		public static function loadOptions()
		{
			menuOptions = OPTION_NAMES;
			try {
				menuValues = sam.GetOptions();
			}
			catch (e:Error)
			{
				trace("Failed to load options");
				menuValues = [];
				for (var i:int = 0; i < OPTION_NAMES; i++) {
					menuValues[i] = false;
				}
			}
		}
		
		public static function setOption(id:int, enabled:Boolean)
		{
			try {
				sam.SetOption(id, enabled);
			}
			catch (e:Error)
			{
				trace("Failed to set option");
			}
		}
		
		public static function loadCamera()
		{
			menuOptions = CAMERA_NAMES;
			try 
			{
				menuValues = sam.GetCamera();
			}
			catch (e:Error)
			{
				trace("Failed to load camera");
				menuValues = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 1, 2, 0, 1, 2];
			}
		}
		
		public static function setCamera(id:int, value:Number)
		{
			try
			{
				if (id < 3)
				{
					var dif:int = updateCursorDrag(value);
					menuValues[id] += dif * 0.5;
					sam.SetCameraPosition(menuValues[0], menuValues[1], menuValues[2]);
				} 
				else if (id < 6)
				{
					menuValues[id] = value;
					sam.SetCameraRotation(menuValues[3], menuValues[4], menuValues[5]);
				}
				else if (id < 7)
				{
					menuValues[id] = value;
					sam.SetCameraFOV(menuValues[6]);
				}
				else
				{
					var save:Boolean = id < (7 + ((Data.menuValues.length - 7) / 2));
					if (save) {
						sam.SaveCameraState(menuValues[id]);
					} else {
						sam.LoadCameraState(menuValues[id]);
					}
				}
			}
			catch (e:Error)
			{
				trace("Failed to set camera");
			}
		}

		public static function loadLightSelect()
		{
			try
			{
				menuOptions = sam.GetLightSelect();
				menuOptions.push("$SAM_AddNew");
				menuOptions.push("$SAM_AddConsole");
				menuOptions.push("$SAM_LightGlobal");
			}
			catch (e:Error)
			{
				trace("Failed to load lights");
				if (Util.debug) {
					menuOptions = ["Light", "Light", "Light", "$SAM_AddNew", "$SAM_AddConsole", "$SAM_LightGlobal"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightEdit()
		{
			menuOptions = LIGHT_NAMES;
			try
			{
				menuValues = sam.GetLightEdit(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to load light");
				menuValues = [100.0, 0.0, 100.0, 0.0, 0.0];
			}
		}
		
		public static function loadLightCategory()
		{
			try
			{
				menuOptions = sam.GetLightCategories();
			}
			catch (e:Error)
			{
				trace("Failed to load light categories");
				if (Util.debug) {
					menuOptions = ["Hemisphere", "Spotlight", "Omni"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightObject()
		{
			try
			{
				menuOptions = sam.GetLightObjects(selectedCategory);
			}
			catch (e:Error)
			{
				trace("Failed to load light Objects");
				if (Util.debug) {
					menuOptions = ["White", "Warm", "Cold"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function loadLightSettings()
		{
			try
			{
				menuValues = sam.GetLightSettings();
			}
			catch (e:Error)
			{
				trace("Failed to get light settings");
				menuValues = [0.0, 0.0, 0.0, 0.0];
			}
		}
		
		public static function editLight(id:int, value:Number)
		{
			try
			{
				if (id < 3) {
					var dif:int = updateCursorDrag(value);
					menuValues[id] += dif * 0.5;
					if (id == 0 && menuValues[id] < 0) {
						//clamp distance above 0
						menuValues[id] = 0;
					} else if (id == 1) {
						//rotation 0-360
						if (menuValues[id] < 0) {
							menuValues[id] += 360;
						} else if (menuValues[id] >= 360) {
							menuValues[id] -= 360;
						}
					}
				} else {
					menuValues[id] = value;
				}
				sam.EditLight(selectedAdjustment, id, menuValues[id]);
			}
			catch (e:Error)
			{
				trace("Failed to edit light");
			}
		}
		
		public static function resetLight()
		{
			menuValues = [100.0, 0.0, 100.0, 0.0, 0.0];
			try
			{
				sam.ResetLight(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to reset adjustment");
			}
		}
		
		public static function deleteLight()
		{
			try
			{
				sam.DeleteLight(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to delete light");
			}
		}
		
		public static function swapLight(id:int)
		{
			try
			{
				sam.SwapLight(selectedAdjustment, selectedCategory, id);
			}
			catch (e:Error)
			{
				trace("Failed to swap light");
			}
		}
		
		public static function renameLight(name:String)
		{
			try
			{
				sam.RenameLight(selectedAdjustment, name);
			}
			catch (e:Error)
			{
				trace("Failed to rename light");
			}
		}
		
		public static function createLight(id:int)
		{
			try
			{
				sam.CreateLight(selectedCategory, id);
			}
			catch (e:Error)
			{
				trace("Failed to create light");
			}
		}
		
		public static function addLight()
		{
			try
			{
				sam.AddLight();
			}
			catch (e:Error)
			{
				trace("Failed to add light");
			}
		}
		
		public static function getLightVisible():Boolean
		{
			try
			{
				return sam.GetLightVisible(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to get light visibility");
				return false;
			}
		}
		
		public static function toggleLightVisible():Boolean
		{
			try
			{
				return sam.ToggleLightVisible(selectedAdjustment);
			}
			catch (e:Error)
			{
				trace("Failed to toggle light visiblity");
				return true;
			}
		}
		
		public static function getLightsVisibility():Boolean
		{
			try
			{
				return sam.GetAllLightsVisible();
			}
			catch (e:Error)
			{
				trace("Failed to get light visibility");
				return false;
			}
		}
		
		public static function toggleLightsVisibility():Boolean
		{
			try
			{
				return sam.ToggleAllLightsVisible();
			}
			catch (e:Error)
			{
				trace("Failed to toggle lights visibility");
				return true;
			}
		}
		
		public static function editLightSettings(id:int, value:Number)
		{
			try
			{
				var dif:int = updateCursorDrag(value);
				menuValues[id] += dif * 0.5;
				if (id == 3) { //rotation clamp
					if (menuValues[id] < 0) {
						menuValues[id] += 360;
					} else if (menuValues[id] >= 360) {
						menuValues[id] -= 360;
					}
				}
				sam.EditLightSettings(id, menuValues[id]);
			}
			catch (e:Error)
			{
				trace("Failed to edit light settings");
			}
		}
		
		public static function resetLightSettings()
		{
			menuValues = [0.0, 0.0, 0.0, 0.0];
			try
			{
				sam.ResetLightSettings();
			}
			catch (e:Error)
			{
				trace("Reset light settings");
			}
		}
		
		public static function updateAllLights()
		{
			try
			{
				sam.UpdateAllLights();
			}
			catch (e:Error)
			{
				trace("Failed to update lights");
			}
		}
		
		public static function deleteAllLights()
		{
			try 
			{
				sam.DeleteAllLights();
			}
			catch (e:Error)
			{
				trace("Failed to delete lights");
			}
		}
		
		public static function loadLightFiles()
		{
			if (!getFileListing(LIGHT_DIRECTORY, "*.json"))
			{
				menuFiles = [];
			}
		}
		
		public static function loadLights(id:int)
		{
			try
			{
				sam.LoadLights(menuOptions[id]);
			}
			catch (e:Error)
			{
				trace("Failed to load lights");
			}
		}
		
		public static function saveLights(filename:String)
		{
			try
			{
				sam.SaveLights(filename);
			}
			catch (e:Error)
			{
				trace("Failed to save lights");
			}
		}
		
		public static function loadPoseExport()
		{
			try
			{
				menuOptions = sam.GetPoseExportTypes();
			}
			catch (e:Error)
			{
				trace("Failed to get pose export types");
				if (Util.debug) {
					menuOptions = ["Vanilla", "ZeX", "All", "Outfit Studio"];
				} else {
					menuOptions = [];
				}
			}
		}
		
		public static function getMorphsTongue(id:int)
		{
			try {
				var tongue:Array = sam.GetMorphsTongue(id);
				if (tongue.length == 0) {
					selectedCategory = -1;
					selectedBone = -1;
					selectedAdjustment = -1;
				} else {
					selectedCategory = tongue[0];
					selectedBone = tongue[1];
					selectedAdjustment = tongue[2];
				}
			}
			catch (e:Error)
			{
				trace("Failed to get morphs tongue");
				selectedCategory = -1;
				selectedBone = -1;
				selectedAdjustment = -1;
			}
		}
	}
}