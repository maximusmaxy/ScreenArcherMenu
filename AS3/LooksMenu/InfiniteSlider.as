package  {
	
	import flash.display.MovieClip;
	import flash.events.*;
	
	public class InfiniteSlider extends MovieClip {
		
		public var button:Checkbox;
		
		public function InfiniteSlider() {
			super()
			stage.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
		}
		
		public function onDown(event:MouseEvent) {
			stage.removeEventListener(MouseEvent.MOUSE_DOWN, onDown);
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
		}
		
		public function onMove(event:MouseEvent) {
			trace(event.localX, event.localY);
		}
		
		public function onUp(event:MouseEvent) {
			stage.addEventListener(MouseEvent.CLICK, onDown);
			stage.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
			stage.removeEventListener(MouseEvent.MOUSE_UP, onUp);
		}
	}
}
