package  {
	
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.geom.ColorTransform;
	
	public class RotateTool extends MovieClip {		
		public var xaxis:MovieClip;
		public var xbounds:MovieClip;
		public var xblock:MovieClip;
		
		public var yaxis:MovieClip;
		public var ybounds1:MovieClip;
		public var ybounds2:MovieClip;
		
		public var zaxis:MovieClip;
		public var zbounds1:MovieClip;
		public var zbounds2:MovieClip;
		
		public var xtranslation:MovieClip;
		public var ytranslation:MovieClip;
		public var ztranslation:MovieClip;
		public var xtranslationBounds:MovieClip;
		public var ytranslationBounds:MovieClip;
		public var ztranslationBounds:MovieClip;
		
		public var scale:MovieClip;
		public var scaleBounds:MovieClip;
		
		public var allBounds:Array;
		
		public var overBounds:MovieClip;
		public var selectedBounds:MovieClip;
		
		public function RotateTool() {
			super();
			
			this.visible = false;
			
			this.overBounds = null;
			this.selectedBounds = null;
			
			this.allBounds = [
				this.xbounds,
				this.ybounds1,
				this.ybounds2,
				this.zbounds1,
				this.zbounds2,
				this.scaleBounds,
				this.xtranslationBounds,
				this.ytranslationBounds,
				this.ztranslationBounds,
			];
			
			enableBounds();
			
			this.xaxis.gotoAndStop(1);
			this.yaxis.gotoAndStop(1);
			this.zaxis.gotoAndStop(1);
			this.xtranslation.gotoAndStop(1);
			this.ytranslation.gotoAndStop(1);
			this.ztranslation.gotoAndStop(1);
			this.scale.gotoAndStop(1);
		}
		
		public function enableBounds()
		{
			for (var i:int = 0; i < allBounds.length; i++) {
				var bounds:MovieClip = this.allBounds[i];
				bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			}
			
			this.xblock.addEventListener(MouseEvent.MOUSE_OVER, function(e:MouseEvent) {});
		}
		
		public function disableBounds()
		{
			for (var i:int = 0; i < allBounds.length; i++) {
				var bounds:MovieClip = this.allBounds[i];
				bounds.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
			}
			
			this.xblock.removeEventListener(MouseEvent.MOUSE_OVER, onOver);
		}
		
		public function onOver(e:MouseEvent) {
			this.overBounds = e.target;

			this.overBounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			this.overBounds.addEventListener(MouseEvent.MOUSE_DOWN, onDown);
			this.overBounds.addEventListener(MouseEvent.MOUSE_OUT, onOut);
			
			if (this.selectedBounds == null) {
				var axis:MovieClip = this.getAxis(this.overBounds);
				if (axis) {
					axis.gotoAndStop(2);
				}
			}
		}
		
		public function onDown(e:MouseEvent) {
			var bounds:MovieClip = e.target;
			var axis:MovieClip = getAxis(bounds);
			var index:int = getIndex(axis);
			
			if (index != 0) {
				this.selectedBounds = bounds;
				axis.gotoAndStop(2);
				
				Data.selectRotateMarker(index);
				
				//disableBounds();
				
				stage.addEventListener(MouseEvent.MOUSE_UP, onUp);
				Data.startEditFunction();
			}
		}
		
		public function onUp(e:MouseEvent) {
			stage.removeEventListener(MouseEvent.MOUSE_UP, onUp);
			Data.endEditFunction();
			
			var axis:MovieClip;
			
			if (this.overBounds) {
				axis = getAxis(this.overBounds);
				axis.gotoAndStop(2);
			}

			if (this.overBounds != this.selectedBounds) {
				axis = getAxis(this.selectedBounds);
				axis.gotoAndStop(1);
			}
			
			this.selectedBounds = null;
			//enableBounds();
		}
		
		public function onOut(e:MouseEvent) {
			var bounds:MovieClip = e.target;
			if (this.overBounds == bounds)
				this.overBounds = null;

			bounds.addEventListener(MouseEvent.MOUSE_OVER, onOver);
			bounds.removeEventListener(MouseEvent.MOUSE_DOWN, onDown);
			bounds.removeEventListener(MouseEvent.MOUSE_OUT, onOut);
			
			if (!this.selectedBounds) {
				var axis:MovieClip = this.getAxis(bounds);
				if (axis) {
					axis.gotoAndStop(1);
				}
			}
		}
		
		public function getAxis(bounds:MovieClip):MovieClip {
			switch (bounds) {
				case this.xbounds: 
					return this.xaxis;
				case this.ybounds1:
				case this.ybounds2:
					return this.yaxis;
				case this.zbounds1:
				case this.zbounds2:
					return this.zaxis;
				case this.scaleBounds:
					return this.scale;
				case this.xtranslationBounds:
					return this.xtranslation;
				case this.ytranslationBounds:
					return this.ytranslation;
				case this.ztranslationBounds:
					return this.ztranslation;
			}
			
			return null;
		}
		
		public function getIndex(axis:MovieClip):int {
			switch(axis) {
				case this.xaxis: return Data.TRANSFORM_ROTATEX;
				case this.yaxis: return Data.TRANSFORM_ROTATEY;
				case this.zaxis: return Data.TRANSFORM_ROTATEZ;
				case this.xtranslation: return Data.TRANSFORM_TRANSLATEX;
				case this.ytranslation: return Data.TRANSFORM_TRANSLATEY;
				case this.ztranslation: return Data.TRANSFORM_TRANSLATEZ;
				case this.scale: return Data.TRANSFORM_SCALE;
			}
			
			return 0;
		}
		
		public function setColor(r:Number, g:Number, b:Number) {
			this.xaxis.transform.colorTransform = new ColorTransform(r, 0, 0);
			this.yaxis.transform.colorTransform = new ColorTransform(0, g, 0);
			this.zaxis.transform.colorTransform = new ColorTransform(0, 0, b);
			this.xtranslation.transform.colorTransform = new ColorTransform(r, 0, 0);
			this.ytranslation.transform.colorTransform = new ColorTransform(0, g, 0);
			this.ztranslation.transform.colorTransform = new ColorTransform(0, 0, b);
		}
	}
}
