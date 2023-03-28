package  {
	
	import flash.display.MovieClip;
	import flash.geom.*;
	
	public class BoneMarker extends MovieClip {
		
		public function BoneMarker() {
			super();
			
//			var color:ColorTransform = this.transform.colorTransform;
//			color.alphaMultiplier = 0.5;
//			this.transform.colorTransform = color;
			
			this.visible = false;
		}
		
		public function setPosAndAngle(m1:MovieClip, m2:MovieClip) {
			var xdif:Number = m2.x - m1.x;
			var ydif:Number = m2.y - m1.y;
			var distance:Number = Math.sqrt((xdif * xdif) + (ydif * ydif));
			
			var angle:Number;
			if (ydif < 0)
				angle = Math.asin(-xdif / distance) + Math.PI;
			else
				angle = Math.asin(xdif / distance);
			
			this.x = m1.x;
			this.y = m1.y;
			this.rotation = -angle * 180 / Math.PI;
			this.scaleY = distance / 100;
		}
	}
}