package  {
	
	import flash.display.MovieClip;
	import flash.geom.Matrix;
	import flash.display.*;
	import flash.events.MouseEvent;

	public class RotateMarker extends MovieClip {
		
		public static const X_AXIS:int = 1;
		public static const Y_AXIS:int = 2;
		public static const Z_AXIS:int = 3;
		
		public var axis:int = 0;
		public var bounds:Shape;

		public function RotateMarker(axis:int = 0) {
			super();
			
			this.gotoAndStop(1);
			
			this.axis = axis;
			
			this.bounds = this.getChildAt(0);
			
			
			this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
		}
		
		public function setTo(a:Number, b:Number, c:Number, d:Number, tx:Number, ty:Number) {
//			var m:Matrix = this.transform.matrix;
//			m.setTo(a, b, c, d, tx, ty);
//			this.transform.matrix = m;
			this.transform.matrix = new Matrix(a, b, c, d, tx, ty)
		}
		
		public function onOver(e:MouseEvent) {
			this.bounds.gotoAndStop(2);
			
			this.bounds.addEventListener(MouseEvent.MOUSE_OUT, onOut);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
		}
		
		public function onOut(e:MouseEvent) {
			this.gotoAndStop(1);
			
			this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
		}
		
		public function onDown(e:MouseEvent) 
		{
			if (axis != 0)
				Data.selectRotateMarker(axis);
		}
	}
}