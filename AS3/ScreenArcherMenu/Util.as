﻿package {
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.ui.*;
	import flash.utils.*;
	
    public class Util {
		public static var debug:Boolean = false;

		public static const UI_OK:String = "UIMenuOK";
		public static const UI_FOCUS:String = "UIMenuFocus";
		public static const UI_CANCEL:String = "UIMenuCancel";

		public static function playSound(sound:String) {
			try {
				Data.sam.PlaySound(sound);
			} catch (e:Error) {}
		}
		
		public static function playOk() {
			playSound(UI_OK);
		}

		public static function playFocus() {
			playSound(UI_FOCUS);
		}

		public static function playCancel() {
			playSound(UI_CANCEL);
		}
		
		public static var unselectText:Function = function() {}
//		public static function unselectText() {
//			if (Data.selectedText != null)
//			{
//				Data.selectedText.value.type = TextFieldType.DYNAMIC;
//				Data.selectedText.value.setSelection(0,0);
//				Data.selectedText.value.selectable = false;
//				Data.selectedText = null;
//
//				if (!Data.extraHotkeys)
//					Data.f4se.AllowTextInput(false);
//			}
//		}
		
		public static function shallowCopyArray(arr:Array):Array {
			var result:Array = new Array(arr.length);
			for (var i:int = 0; i < arr.length; i++) {
				result[i] = arr[i];
			}
			return result;
		}
		
		public static function packObjectArray(obj:Array, str:String, arr:Array)
		{
			//fill missing objects
			for (var i:int = obj.length; i < arr.length; i++) {
				obj[i] = {};
			}
			for (var y:int = 0; y < arr.length; y++) {
				obj[i][str] = arr[i];
			}
		}
		
		public static function unpackObjectArray(obj:Array, str:String):Array
		{
			var arr:Array = [];
			for (var i:int = 0; i < obj.length; i++) {
				arr[i] = obj[i][str];
			}
			return arr;
		}
		
		public static function setRect(clip:MovieClip, x:Number, y:Number, width:Number, height:Number)
		{
			clip.x = x;
			clip.y = y;
			clip.width = width;
			clip.height = height;
		}
		
		public static function callFuncArgs(func:Function, args:Array = null):Object
		{			
			try {
				if (!args)
					return func();
			
				//I don't think AS3 has any other way to do this
				switch(args.length) {		
					case 0: return func();
					case 1: return func(args[0]);
					case 2: return func(args[0], args[1]);
					case 3: return func(args[0], args[1], args[2]);
					case 4: return func(args[0], args[1], args[2], args[3]);
					case 5: return func(args[0], args[1], args[2], args[3], args[4]);
					case 6: return func(args[0], args[1], args[2], args[3], args[4], args[5]);
					case 7: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
					case 8: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
					case 9: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
					case 10: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]);
					case 11: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10]);
					case 12: return func(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11]);
				}
				
				Data.error = "Function exceeded parameter limit";
				return null;
			} 
			catch (e:Error) 
			{
				if (e.errorID == 1006) 
				{
					Data.error = "Function name could not be found";
					return null;
				} else {
					trace(e.message);
				}
			}
	
			Data.error = "Failed to call function";
			return null;
		}
		
		private static const floatEpsilon = 0.0001;
		
		public static function floatEqual(a:Number, b:Number):Boolean
		{
			var magnitude:Number = a - b;
			if (magnitude < 0)
				magnitude = -magnitude;
			return magnitude < floatEpsilon;
		}
		
		public static function caseInsensitiveCompare(a:String, b:String):Boolean
		{
			if (a && b) {
				return a.toLowerCase() == b.toLowerCase();
			}
			return false;
		}
		
		public static const X_AXIS:int = 1;
		public static const Y_AXIS:int = 2;
		public static const Z_AXIS:int = 3;
		
		public static function getAxisRotation(type:int, value:Number):Matrix3D
		{
			var v:Vector.<Number>;
			var sin:Number = Math.sin(value);
			var cos:Number = Math.cos(value);
			
			switch (type) {
				case X_AXIS: v = new <Number>[1, 0, 0, 0, 0, cos, sin, 0, 0, -sin, cos, 0, 0, 0, 0, 1]; break;
				case Y_AXIS: v = new <Number>[cos, 0, -sin, 0, 0, 1, 0, 0, sin, 0, cos, 0, 0, 0, 0, 1]; break;
				case Z_AXIS: v = new <Number>[cos, sin, 0, 0, -sin, cos, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]; break;
			}
			
			return new Matrix3D(v);
		}
		
		public static function traceObj(obj, key:Object = null):void
		{
			if (obj == null) {
				if (key == null) {
					trace("Object is null");
				} else {
					trace(key, ":", "null");
				}
				return;
			}
			
			var type:String = getQualifiedClassName(obj);
			switch (type) {
				case "Object":
					if (key == null) {
						trace("Object --->");
					} else {
						trace(key, "Object --->");
					}
					
					for (var id:String in obj)
					{
						traceObj(obj[id], id);
					}
					trace("<---");
					break;
				case "Array":
					var arr:Array = obj;
					if (key == null) {
						trace("Array length: ", arr.length);
					} else {
						trace(key, "Array length: ", arr.length);
					}
					
					for (var i:int = 0; i < arr.length; i++) {
						traceObj(arr[i], i);
					}
					break;
				default:
					if (key == null) {
						trace(obj);
					} else {
						trace(key, ":", obj);
					}
					break;
			}
		}

		public static function traceDisplayObject(dOC:Object, recursionLevel:int = 0)
		{
			var numCh = dOC.numChildren;
			for (var i = 0; i < numCh; i++)
			{
				var child = dOC.getChildAt(i);
				var indentation:String = "";
				for (var j:int = 0; j < recursionLevel; j++)
				{
					indentation +=  "----";
				}
				trace(indentation + "[" + i + "] " + child.name + " Alpha: " + child.alpha + " Visible: " + child.visible + " " + child);

				if (getQualifiedClassName(child) == "Object")
				{
					traceObj(child);
				}

				if (child is DisplayObjectContainer && child.numChildren > 0)
				{
					traceDisplayObject(child, recursionLevel + 1);
				}
			}
		}
    }
}