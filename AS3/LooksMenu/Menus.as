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
		
		public static const MAIN:Object = {
			type: LIST,
			options: [
			
			]
		};
		
//		public static const MFG:Object = {
//			type: LIST,
//			options: Data.loadMorphCategories,
//			select: function(i:int) {  },
//			save: Data.saveMfg,
//			files: Data.loadMfgFiles,
//			load: Data.loadMfg
//		}
		
		public static const TRANSFORM:Array = [
			{
				name: "$SAM_RotX",
				type: SLIDER,
				func: null
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
				func: null
			}
	    ];
    }
}