package {
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.ui.*;
	
    public class Util {
		public static var debug:Boolean = false;

		public static const UI_OK:String = "UIMenuOK";
		public static const UI_FOCUS:String = "UIMenuFocus";
		public static const UI_CANCEL:String = "UIMenuCancel";

		public static function playOk() {
			if (!debug) {
				Data.sam.PlaySound(UI_OK);
			}
		}

		public static function playFocus() {
			if (!debug) {
				Data.sam.PlaySound(UI_FOCUS);
			}
		}

		public static function playCancel() {
			if (!debug) {
				Data.sam.PlaySound(UI_CANCEL);
			}
		}
		
		public static function unselectText() {
			if (Data.selectedText != null)
			{
				Data.selectedText.value.type = TextFieldType.DYNAMIC;
				Data.selectedText.value.setSelection(0,0);
				Data.selectedText.value.selectable = false;
				try
				{
					Data.f4seObj.AllowTextInput(false);
				}
				catch (e:Error)
				{
					trace("Disable text input failed");
				}
				Data.selectedText = null;
			}
		}
		
		public static function traceObj(obj:Object):void
		{
			for (var id:String in obj)
			{
				var value:Object = obj[id];

				if (getQualifiedClassName(value) == "Object")
				{
					trace("-->");
					traceObj(value);
				}
				else
				{
					trace(id + " = " + value);
				}
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