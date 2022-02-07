package  {
	
	import flash.display.*;
	import flash.events.*;
	import flash.text.TextField;
	
	public class Checkbox extends MovieClip {
		
		public var bounds:MovieClip;
		public var check:MovieClip;
		public var text:TextField;
		
		public var id:int;
		public var func:Function;
		public var value:Boolean;
		
		public function Checkbox() {
			super();
			bounds.addEventListener(MouseEvent.CLICK, onClick);
		}
		
		public function init(x:int, id:int, checked:Boolean, name:String, func:Function)
		{
			this.visible = true;
			this.x = x;
			this.id = id;
			setCheck(checked);
			if (name != null) {
				this.text.visible = true;
				this.text.text = name;
			} else {
				this.text.visible = false;
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
			if (this.text.visible) { //button
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
