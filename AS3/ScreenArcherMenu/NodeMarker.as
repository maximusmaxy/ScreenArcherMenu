package  
{
	import flash.display.MovieClip;
	import flash.events.*;
	import flash.geom.*;
	
	public class NodeMarker extends MovieClip 
	{
		public var bounds:MovieClip;
		public var boneName:String = null;
		
		public function NodeMarker(boneName:String = null)
		{
			super();
			
			this.scaleX = 0.25;
			this.scaleY = 0.25;
			
//			var color:ColorTransform = this.transform.colorTransform;
//			color.alphaMultiplier = 0.5;
//			this.transform.colorTransform = color;
			
			this.boneName = boneName;
			this.gotoAndStop(1);
			
			if (this.boneName) {
				this.bounds.addEventListener(MouseEvent.CLICK, onClick);
				this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
				this.bounds.addEventListener(MouseEvent.MOUSE_WHEEL, onScroll);
			}
			
			this.visible = false;
		}
		
		public function init(boneName:String) {
			this.boneName = boneName;
			this.bounds.addEventListener(MouseEvent.CLICK, onClick);
			this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.bounds.addEventListener(MouseEvent.MOUSE_WHEEL, onScroll);
		}
		
		public function onClick(event:MouseEvent) 
		{
			if (this.boneName) {
				Data.selectNodeMarker(this.boneName, event.stageX, event.stageY);
				Util.playOk();
			}
		}
		
		public function onOver(event:MouseEvent)
		{
			Data.overNodeMarker(this);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.bounds.addEventListener(MouseEvent.MOUSE_OUT, onOut);
//			if (this.boneName) {
//				this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
//				this.bounds.addEventListener(MouseEvent.MOUSE_OUT, onOut);
//				
//				this.gotoAndStop(2);
//				
//				Data.overNodeMarker(this);
//			}
		}
		
		public function onOut(event:MouseEvent)
		{
			Data.outNodeMarker(this, event.stageX, event.stageY);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
			this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
//			if (this.boneName) {
//				this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
//				this.bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
//				
//				this.gotoAndStop(1);
//				
//				Data.outNodeMarker(this);
//			}
		}
		
		public function onScroll(event:MouseEvent)
		{
			Data.scrollNodeMarker(event.delta >= 0);
		}

		public function destroy()
		{
			this.bounds.removeEventListener(MouseEvent.CLICK, onClick);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
		}
	}
}
