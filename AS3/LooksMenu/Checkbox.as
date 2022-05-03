package  {
	
	import flash.display.*;
	import flash.events.*;
	import flash.text.TextField;
	
	public class Checkbox extends MovieClip {
		
		public var bounds:MovieClip;
		public var check:MovieClip;
		public var settings:MovieClip;
		public var recycle:MovieClip;
		public var position:MovieClip;
		
		public var id:int;
		public var func:Function;
		public var value:Boolean;
		public var dragState:int = 0;

		public static const SETTINGS = 1;
		public static const RECYCLE = 2;
		public static const DRAG = 3;
		
		public static const DISABLED = 0;
		public static const UNSELECTED = 1;
		public static const SELECTED = 2;

		public function Checkbox() {
			super();
			bounds.addEventListener(MouseEvent.CLICK, onClick);
		}
		
		public function init(x:int, id:int, checked:Boolean, type:int, func:Function)
		{
			settings.visible = false;
			recycle.visible = false;
			position.visible = false;
			this.visible = true;
			this.x = x;
			this.id = id;
			setCheck(checked);
			switch (type) {
				case SETTINGS: settings.visible = true; break;
				case RECYCLE: recycle.visible = true; break;
				case DRAG: enableDrag(); break;
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
		
		public function disable() {
			visible = false;
			if (dragState > 0) {
				bounds.addEventListener(MouseEvent.CLICK, onClick);
				bounds.removeEventListener(MouseEvent.MOUSE_DOWN, onDown);
				stage.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
				stage.removeEventListener(MouseEvent.MOUSE_UP, onUp);
				dragState = DISABLED;
				Data.setCursorVisible(true);
			}
		}
		
		public function enableDrag() {			
			bounds.removeEventListener(MouseEvent.CLICK, onClick);
			bounds.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
			position.visible = true;
			dragState = UNSELECTED;
		}
		
		public function onDown(event:MouseEvent) {
			bounds.removeEventListener(MouseEvent.MOUSE_DOWN, onDown);
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
			dragState = SELECTED;
			Data.setCursorVisible(false);
			Data.storeCursorPos();
		}
		
		public function onMove(event:MouseEvent) {
			if (func != null) {
				func.call(null, id, event.stageX);
			}
		}
		
		public function onUp(event:MouseEvent) {
			bounds.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
			stage.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.removeEventListener(MouseEvent.MOUSE_UP, onUp);
			dragState = UNSELECTED;
			Data.setCursorVisible(true);
			Data.endCursorDrag();
		}
	}
}
