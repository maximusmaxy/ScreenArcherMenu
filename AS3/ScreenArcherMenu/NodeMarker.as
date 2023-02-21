package  
{
	import flash.display.MovieClip;
	import flash.events.*;
	
	public class NodeMarker extends MovieClip 
	{
		public var bounds:MovieClip;
		public var boneName:String = null;
		
		public function NodeMarker(boneName:String = null)
		{
			this.boneName = boneName;
			this.gotoAndStop(1);
			
			if (this.boneName) {
				this.bounds.addEventListener(MouseEvent.CLICK, onClick);
				this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			}
		}
		
		public function onClick(event:MouseEvent) 
		{
			if (this.boneName) {
				Data.selectNodeMarker(this.boneName);
				Util.playOk();
			}
		}
		
		public function onOver(event:MouseEvent)
		{
			if (this.boneName) {
				this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
				this.bounds.addEventListener(MouseEvent.MOUSE_OUT, onOut);
				
				this.gotoAndStop(2);
				
				Data.overNodeMarker(this.boneName);
			}
		}
		
		public function onOut(event:MouseEvent)
		{
			if (this.boneName) {
				this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
				this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
				
				this.gotoAndStop(1);
				
				Data.outNodeMarker(this.boneName);
			}
		}

		public function destroy()
		{
			this.bounds.removeEventListener(MouseEvent.CLICK, onClick);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
		}
	}
}
