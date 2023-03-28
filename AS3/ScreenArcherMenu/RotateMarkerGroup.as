package  {
	
	import flash.display.MovieClip;
	import flash.events;
	import flash.events.MouseEvent;

	public class RotateMarkerGroup extends MovieClip {
		
		public var xaxis:RotateMarker;
		public var yaxis:RotateMarker;
		public var zaxis:RotateMarker;
		
		public var selected:RotateMarker;
		
		public function RotateMarkerGroup() {
			super();
			
			this.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			
			this.selected = null;
		}
		
		public function onOver(e:MouseEvent) {
			this.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
			
			var overMarker:RotateMarker = getMarker(e.stageX, e.stageY);
			
			if (overMarker) {
				selectMarker(overMarker);
			} else {
				this.addEventListener(MouseEvent.MOUSE_OUT, onOut);
				this.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
			}
		}
		
		public function onOut(e:MouseEvent) {
			this.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
			this.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
		}
		
		public function onMove(e:MouseEvent) {
			var overMarker:RotateMarker = getMarker(e.stageX, e.stageY);
			
			if (overMarker) {
				this.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
				this.removeEventListener(MouseEvent.MOUSE_MOVE, onMove);
				selectMarker(overMarker);
			}
		}
		
		public function getMarker(a:Number, b:Number):RotateMarker {
			if (this.xaxis.hitTest(a, b)) return this.xaxis;
			if (this.yaxis.hitTest(a, b)) return this.yaxis;
			if (this.zaxis.hitTest(a, b)) return this.zaxis;
			
			return null;
		}
		
		public function selectMarker(marker:RotateMarker) {
			this.selected = marker;
			
			this.selected.gotoAndStop(2);
			
			this.selected.bounds.addEventListener(MouseEvent.MOUSE_OUT, onBoundsOut);
			this.selected.bounds.addEventListener(MouseEvent.MOUSE_DOWN, onBoundsDown);
			this.selected.block.addEventListener(MouseEvent.MOUSE_OVER, onBlockOver);
		}
		
		public function unselectMarker() {
			this.selected.gotoAndStop(1);
			
			this.selected.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onBoundsOut);
			this.selected.bounds.removeEventListener(MouseEvent.MOUSE_DOWN, onBoundsDown);
			this.selected.block.removeEventListener(MouseEvent.MOUSE_OVER, onBlockOver);
			
			this.selected = null;
		}
		
		public function onBoundsOut(e:MouseEvent) {
			unselectMarker();
			
			this.addEventListener(MouseEvent.MOUSE_OVER, onOver);
		}
		
		public function onBoundsDown(e:MouseEvent) {
			trace("hi");
			if (this.selected.axis != 0)
				Data.selectRotateMarker(this.selected.axis);
		}
		
		public function onBlockOver(e:MouseEvent) {
			unselectMarker();

			this.addEventListener(MouseEvent.MOUSE_OUT, onOut);
			this.addEventListener(MouseEvent.MOUSE_MOVE, onMove);
		}
	}
}
