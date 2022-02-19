package  {
	
	import flash.display.*;
	import flash.events.*;
	import flash.text.TextField;
	
	public class Checkbox extends MovieClip {
		
		public var bounds:MovieClip;
		public var check:MovieClip;
		public var settings:MovieClip;
		public var recycle:MovieClip;
		
		public var id:int;
		public var func:Function;
		public var value:Boolean;
		
		public static const SETTINGS = 1;
		public static const RECYCLE = 2;
		
		public function Checkbox() {
			super();
			bounds.addEventListener(MouseEvent.CLICK, onClick);
		}
		
		public function init(x:int, id:int, checked:Boolean, type:int, func:Function)
		{
			settings.visible = false;
			recycle.visible = false;
			this.visible = true;
			this.x = x;
			this.id = id;
			setCheck(checked);
			switch (type) {
				case SETTINGS: settings.visible = true; recycle.visible = false; break;
				case RECYCLE: settings.visible = false; recycle.visible = true; break;
				default: settings.visible = false; recycle.visible = false;
			}
			this.func = func;
		}
		
		public function setColor(color:uint)
		{
			bounds.transform.colorTransform.color = color;
			check.transform.colorTransform.color = color;
		}
		
		public function setCheck(checked:Boolean) {
			value = checked;
			check.visible = value;
		}
		
		public function onClick(event:MouseEvent) {
			if (settings.visible || recycle.visible) { //button
				if (func != null) {
					func.call(null, id);
				}
			} else { //checkbox
				value = !value;
				check.visible = value;
				if (func != null) {
					func.call(null, id, value);
				}
			}
		}
	}
}
