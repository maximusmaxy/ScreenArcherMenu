package {
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.ui.*;
	
    public class Menus {
		
		public static const LIST:int = 0;
		public static const SLIDER:int = 0;
		public static const CHECKBOX:int = 0;
		
		public static const MAIN:Array = [
			{
				
			}
		];
		
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
		
		public static const TRANSFORM:Array = [
			{
				name: "$SAM_RotX",
				type: SLIDER,
				func: 
			}
		];	
		
		public static const OPTION_NAMES:Vector.<String> = new <String>[
			"$SAM_Hotswap",
			"$SAM_Alignment",
			"$SAM_Widescreen"
		];
		
		
		public static const OPTIONS:Array = [
			{
				name: "$SAM_Hotswap",
				type: CHECKBOX,
				func: option
			}
	    ];
    }
}