package  {
	
	import flash.display.*;
	import flash.events.*;
	import flash.text.TextField;
	import flash.utils.*;
	
	public class Checkbox extends MovieClip {
		
		public var icon:MovieClip;
		public var bounds:MovieClip;
		public var background:MovieClip;
		
		public var id:int;
		public var checkId:int;
		public var check:Boolean;
		public var type:int;
		public var dragState:int = 0;
		public var selectable:Boolean = false;
		
		public var increment:Number = 0.0;
		public var mod:Number = 1.0;
		
		public var functions:EntryFunctions;
		
		public static const DISABLED = 0;
		public static const UNSELECTED = 1;
		public static const SELECTED = 2;

		public function Checkbox() {
			super();
			bounds.addEventListener(MouseEvent.CLICK, onClick);
			bounds.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			gotoAndStop(1);
		}
		
		public function init(x:int, id:int, type:int, selectable:Boolean)
		{
			this.visible = true;
			this.x = x;
			this.id = id;
			this.type = type;
			this.check = false;
			this.selectable = selectable;
			if (type == Data.CHECKBOX_TOUCH) {
				enableDrag();
			}
			update();
		}
		
		public function update()
		{
			gotoAndStop((type - 1) * 2 + (check ? 1 : 0) + 1);
		}
		
		public function setCheck(checked:Boolean)
		{
			check = checked;
			update();
		}
		
		public function selectCheck()
		{
			setCheck(true);
			Util.playFocus();
		}
		
		public function select()
		{
			if (this.type == Data.CHECKBOX_FOLDER) {
				setCheck(true);
			}
//			switch (type) {
//				case Data.CHECKBOX_FOLDER:
//					setCheck(true);
//					break;
//			}
		}
		
		public function unselect()
		{
			if (this.type == Data.CHECKBOX_FOLDER) {
				setCheck(false);
			}
//			switch (type) {
//				case Data.CHECKBOX_FOLDER:
//					setCheck(false);
//					break;
//			}
		}
		
		public function confirm()
		{
			switch(type) {
				case Data.CHECKBOX_DOWN:
				case Data.CHECKBOX_SETTINGS:
				case Data.CHECKBOX_FOLDER:
					functions.checkbox(id);
					Util.playOk();
					break;
				case Data.CHECKBOX_RECYCLE:
				case Data.CHECKBOX_UP:
					functions.checkbox2(id);
					Util.playOk();
					break;
				case Data.CHECKBOX_CHECK:
					check = !check;
					functions.checkbox(id, check);
					Util.playOk();
					update();
					break;
			}
		}
		
		public function onClick(event:MouseEvent) 
		{
			confirm();
		}
		
		public function onMouseOver(event:MouseEvent) {
			if (selectable) {
				bounds.removeEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
				bounds.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
				dispatchEvent(new EntryEvent(id, EntryEvent.OVER, (checkId == 0 ? 1 : 2)));
			}
		}
		
		public function onMouseOut(event:MouseEvent) {
			bounds.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			bounds.removeEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
			dispatchEvent(new EntryEvent(id, EntryEvent.OUT, (checkId == 0 ? 1 : 2)));
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
			dragState = UNSELECTED;
		}
		
		public function onDown(event:MouseEvent) {
			bounds.removeEventListener(MouseEvent.MOUSE_DOWN, onDown);
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
			dragState = SELECTED;
			Data.setCursorVisible(false);
			Data.storeCursorPos();
			Data.startEditFunction();
		}
		
		public function onMove(event:MouseEvent) {
			var dif:int = Data.updateCursorDrag();
			functions.valueFloat(id, dif * mod);
		}
		
		public function onUp(event:MouseEvent) {
			bounds.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
			stage.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.removeEventListener(MouseEvent.MOUSE_UP, onUp);
			dragState = UNSELECTED;
			Data.setCursorVisible(true);
			Data.endCursorDrag();
			Data.endEditFunction();
		}
		
		public function forceDrag(inc:Boolean)
		{
			Data.startEditFunction();
			functions.valueFloat(id, inc ? increment : -increment);
			Data.endEditFunction();
		}
	}
}
