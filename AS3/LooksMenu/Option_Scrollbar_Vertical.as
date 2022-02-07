package 
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	
	public class Option_Scrollbar_Vertical extends flash.display.MovieClip
	{
		public var Track_mc:flash.display.MovieClip;
		public var Thumb_mc:flash.display.MovieClip;
		public var BarCatcher_mc:flash.display.MovieClip;
		internal var fValue:Number;
		protected var fMinThumbY:Number;
		protected var fMaxThumbY:Number;
		internal var fMinValue:Number=0;
		internal var fMaxValue:Number=1;
		internal var fStepSize:Number=0.05;
		internal var iStartDragThumb:int;
		internal var fStartValue:Number;
		internal var dragStart:Point;
        public var offsetTop:Number = 0;
        public var offsetBottom:Number = 0;
		
		public function Option_Scrollbar_Vertical()
		{
			super();
			updateHeight();
			addEventListener(flash.events.MouseEvent.CLICK, onClick);
			Thumb_mc.addEventListener(flash.events.MouseEvent.MOUSE_DOWN, onThumbMouseDown);
		}
		
		public function updateHeight()
		{
			fMinThumbY = Track_mc.y -2;
			fMaxThumbY = Track_mc.y + Track_mc.height - Thumb_mc.height + 2;
		}
		
        public function get maximum():Number { return fMaxValue; }
        public function set maximum(value:Number):void { fMaxValue = value; }

        public function get minimum():Number { return fMinValue; }
        public function set minimum(value:Number):void { fMinValue = value; }
		
		public function get snapInterval():Number { return fStepSize; }
        public function set snapInterval(value:Number):void { fStepSize = value; }

		public function get MinValue():Number
		{
			return fMinValue;
		}

		public function set MinValue(arg1:Number):*
		{
			fMinValue = arg1;
		}

		public function get MaxValue():Number
		{
			return fMaxValue;
		}

		public function set MaxValue(arg1:Number):*
		{
			fMaxValue = arg1;
		}

		public function get StepSize():Number
		{
			return fStepSize;
		}

		public function set StepSize(arg1:Number):*
		{
			fStepSize = arg1;

		}
		
		public function get position():Number
		{
			return fValue;
		}
        public function set position(value:Number):void
		{
			fValue = Math.min(Math.max(value, this.fMinValue), this.fMaxValue);
			var yOffset = (fValue - fMinValue) / (fMaxValue - fMinValue);
			Thumb_mc.y = fMinThumbY + yOffset * (fMaxThumbY - fMinThumbY);
		}

		public function get value():Number
		{
			return fValue;
		}

		public function set value(val:Number):*
		{
			position = val;
			dispatchEvent(new flash.events.Event(Option_Scrollbar.VALUE_CHANGE, true, true));
		}

		public function Decrement():*
		{
			value = value - fStepSize;
		}

		public function Increment():*
		{
			value = value + fStepSize;
		}

		public function HandleKeyboardInput(arg1:flash.events.KeyboardEvent):*
		{
			if (arg1.keyCode == flash.ui.Keyboard.UP && this.value > 0) 
			{
				Decrement();
			}
			else if (arg1.keyCode == flash.ui.Keyboard.DOWN && this.value < 1) 
			{
				Increment();
			}
		}

		public function onClick(event:flash.events.MouseEvent):*
		{
			if (event.target == this.BarCatcher_mc) 
			{
				//var trackHeight:Number = (Track_mc.height - offsetTop - offsetBottom);
				//var newValue:Number = (event.localY * scaleY - offsetTop) / trackHeight * (fMaxValue - fMinValue) + fMinValue;
				var newValue:Number = event.localY;
				this.value = Math.max(fMinValue, Math.min(fMaxValue, newValue));//arg1.currentTarget.mouseX / this.BarCatcher_mc.width * (this.fMaxValue - this.fMinValue);
				dispatchEvent(new flash.events.Event(Option_Scrollbar.VALUE_CHANGE, true, true));
			}
		}

		internal function onThumbMouseDown(arg1:flash.events.MouseEvent):*
		{
			Thumb_mc.startDrag(false, new flash.geom.Rectangle(Thumb_mc.x, fMinThumbY, 0, fMaxThumbY));
			stage.addEventListener(flash.events.MouseEvent.MOUSE_UP, onThumbMouseUp);
			stage.addEventListener(flash.events.MouseEvent.MOUSE_MOVE, onThumbMouseMove);
		}

		internal function onThumbMouseMove(event:flash.events.MouseEvent):*
		{
			var thumbY = Thumb_mc.y - fMinThumbY;
			value = thumbY / (fMaxThumbY - fMinThumbY) * (fMaxValue - fMinValue);
		}

		internal function onThumbMouseUp(arg1:flash.events.MouseEvent):*
		{
			value = value;
			Thumb_mc.stopDrag();
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_UP, onThumbMouseUp);
			stage.removeEventListener(flash.events.MouseEvent.MOUSE_MOVE, onThumbMouseMove);
		}
	}
}