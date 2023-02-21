package  {
	
	import flash.display.MovieClip;
	
	
	public class BoneMarker extends MovieClip {
		
		
		public function BoneMarker() {
			
		}
		
		public function setPosAndAngle(m1:MovieClip, m2:MovieClip) {
			var xdif:Number = m2.x - m1.x;
			var ydif:Number = m2.y - m1.y;
			var distance:Number = Math.sqrt((xdif * xdif) + (ydif * ydif));
			var angle:Number = getAngle(m1, m2, distance);
			
			this.x = m1.x;
			this.y = m1.y;
			this.rotation = -angle * 180 / Math.PI;
			this.scaleY = distance / 100;
		}
		
		public function getAngle(m1:MovieClip, m2:MovieClip, distance:Number) {
			var asin1:Number = Math.asin((m2.x - m1.x) / distance);
			var asin2:Number = Math.PI - asin1;
			if (asin1 < 0.0)
				asin1 += (Math.PI * 2);
			
			var acos1:Number = Math.acos((m2.y - m1.y) / distance);
			var acos2:Number = (Math.PI * 2) - acos1;
			
			if (Util.floatEqual(asin1, acos1) || Util.floatEqual(asin1, acos2))
				return asin1;
		
			if (Util.floatEqual(asin2, acos1) || Util.floatEqual(asin2, acos2))
				return asin2;
				
			return 0.0;
		}
	}
}
