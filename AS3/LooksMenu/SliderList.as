package {
    import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	import Shared.AS3.*;
	import flash.text.*;
	
	public class SliderList extends BSUIComponent
	{
		public var entries:Vector.<SliderListEntry> = new Vector.<SliderListEntry>(15);
		
		public static const SLIDER_MAX:int = 10;
		public static const LIST_MAX:int = 15;
		
		public var listScroll:Option_Scrollbar_Vertical;
		public var entrySize:int = 57;
		public var listSize:int = 10;
		public var stepSize:Number;
		public var listPosition: int = 0;
		
		public static const LIST = 0;
		public static const TRANSFORM = 1;
		public static const MORPH = 2;
		public static const CHECKBOX = 3;
		public static const EYES = 4;
		public static const ADJUSTMENT = 5;
		
		public var type:int;
		
		public function SliderList() {
			super();
			addSliders(); 
			
			listScroll.minimum = 0;
			listScroll.maximum = 100;
			listScroll.StepSize = 1;
			listScroll.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			updateScroll(Data.MAIN_MENU.length, LIST_MAX);
			
			this.addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel);
		}

		public function addSliders() {
			var entry:SliderListEntry;
			for(var i:int = 0; i < LIST_MAX; i++) 
			{
				entry = new SliderListEntry();
				this.addChild(entry);
				entries[i] = entry;
			}
		}

		public function onValueChange(event:flash.events.Event)
		{
			var newPosition:int = int(event.target.value / stepSize);
			newPosition = Math.max(0,Math.min(entrySize - listSize, newPosition));
			updatePosition(newPosition);
		}
		
		public function onMouseWheel(event:flash.events.Event)
		{
			if (listScroll.visible) {
				var newPosition:int = listPosition - int(event.delta);
				newPosition = Math.max(0,Math.min(entrySize - listSize, newPosition));
				if (updatePosition(newPosition)) {
					listScroll.position = stepSize * newPosition;
				}
			}
		}
		
		public function updatePosition(newPosition:int):Boolean
		{
			if (listPosition != newPosition) {
				var dif:int = newPosition - listPosition;
				listPosition = newPosition;
				for (var i:int = 0; i < listSize; i++) 
				{
					entries[i].id = listPosition + i;
					updateType(entries[i]);
				}
				Util.unselectText();
				//Util.playFocus();
				return true;
			}
			return false;
		}

		public function updateValues():void
		{
			for(var i:int = 0; i < listSize; i++) 
			{
				if (entries[i].visible) {
					entries[i].updateValue(true);
				}
			}
		}
		
		public function updateScroll(entrySize:int, listSize:int):void
		{
			this.entrySize = entrySize;
			this.listSize = listSize;
			if (this.entrySize > this.listSize)
			{
				stepSize = 99.0 / (this.entrySize - this.listSize);
				var thumbHeight:Number = listScroll.Track_mc.height *  this.listSize / this.entrySize;
				listScroll.Thumb_mc.height = Math.max(thumbHeight, 40);
				listScroll.visible = true;
				listScroll.position = 0;
				listScroll.updateHeight();
			}
			else
			{
				listScroll.visible = false;
			}
		}

//		internal function floorMod(a:int, b:int):int
//		{
//			return (a % b + b) % b;
//		}
		
		public function update(entrySize:int, listSize:int, func:Function, func2 = null, func3 = null)
		{
			listPosition = 0;
			updateScroll(entrySize, listSize);
			var length:int = Math.min(entrySize, listSize);
			for(var i:int = 0; i < LIST_MAX; i++) 
			{
				if (i < length) {
					entries[i].update(i, func, func2, func3);
					updateType(entries[i]);
				} else {
					entries[i].disable();
				}
			}
			updateLayout();
		}
		
		public function updateLayout():void
		{
			var xOffset:int;
			var yOffset:int;
			switch (entries[i].type) 
			{
				case SliderListEntry.DIVIDER:
				case SliderListEntry.SLIDER:
					xOffset = 18;
					yOffset = 20;
					break;
				default:
					xOffset = 12;
					yOffset = 10;
			}
			for (var i:int = 0; i < LIST_MAX; i++)
			{
				if (entries[i].visible) {
					entries[i].setPos(xOffset, yOffset);
					switch (entries[i].type) {
						case SliderListEntry.DIVIDER:
						case SliderListEntry.SLIDER:
							yOffset += 54;
							break;
						default:
							yOffset += 36;
					}
				}
			}
		}
		
		public function updateType(entry:SliderListEntry):void
		{
			switch (type) {
				case LIST: updateListEntry(entry); break;
				case TRANSFORM: updateTransformEntry(entry); break;
				case MORPH: updateMorphsEntry(entry); break;
				case CHECKBOX: updateCheckboxEntry(entry); break;
				case EYES: updateEyesEntry(entry); break;
				case ADJUSTMENT: updateAdjustmentEntry(entry); break;
			}
		}
		
		public function updateList(func:Function):void
		{
			this.type = LIST;
			update(Data.menuOptions.length, LIST_MAX, func);
		}
		
		public function updateListEntry(entry:SliderListEntry):void
		{
			entry.updateList(Data.menuOptions[entry.id]);
		}
		
		public function updateCheckboxes(func:Function):void
		{
			this.type = CHECKBOX;
			update(Data.menuOptions.length, LIST_MAX, func);
		}
		
		public function updateCheckboxEntry(entry:SliderListEntry):void
		{
			entry.updateCheckbox(Data.menuOptions[entry.id], Data.menuValues[entry.id]);
		}
		
		public function updateAdjustment(func:Function, func2:Function, func3:Function)
		{
			this.type = ADJUSTMENT;
			update(Data.menuOptions.length, LIST_MAX, func, func2, func3);
		}
		
		public function updateAdjustmentEntry(entry:SliderListEntry)
		{
			entry.updateAdjustment(Data.menuOptions[entry.id]);
		}
		
		public function updateTransform(func:Function):void
		{
			this.type = TRANSFORM;
			update(Data.TRANSFORM_NAMES.length, SLIDER_MAX, func);
		}
	
		public function updateTransformEntry(entry:SliderListEntry)
		{
			if (entry.id < 3) //rot
			{
				entry.slider.minimum = 0.0;
				entry.slider.maximum = 360.0;
				entry.slider.StepSize = 0.1;
				entry.sliderMod = 180.0;
				entry.sliderFixed = 2
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else if (entry.id < 6) //pos
			{
				entry.slider.minimum = 0.0;
				entry.slider.maximum = 20.0;
				entry.slider.StepSize = 0.01;
				entry.sliderMod = 10.0;
				entry.sliderFixed = 4;
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else if (entry.id < 7)//scale
			{
				entry.slider.minimum = 0.0;
				entry.slider.maximum = 2.0;
				entry.slider.StepSize = 0.01;
				entry.sliderMod = 0;
				entry.sliderFixed = 4;
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
		}
		
		public function updateMorphs(func:Function):void
		{
			this.type = MORPH;
			update(Data.menuOptions.length, SLIDER_MAX, func);
		}
		
		public function updateMorphsEntry(entry:SliderListEntry):void
		{
			entry.slider.minimum = 0;
			entry.slider.maximum = 100;
			entry.slider.StepSize = 1;
			entry.sliderMod = 0;
			entry.updateSlider(Data.menuOptions[entry.id], SliderListEntry.INT);
		}
		
		public function updateEyes(func:Function):void
		{
			this.type = EYES;
			update(Data.EYE_NAMES.length, SLIDER_MAX, func);
		}
		
		public function updateEyesEntry(entry:SliderListEntry):void
		{
			if (entry.id < 2) {
				entry.slider.minimum = 0.0;
				entry.slider.maximum = 2.0;
				entry.slider.StepSize = 0.01;
				entry.sliderMod = 1.0;
				entry.sliderFixed = 4;
				entry.updateSlider(Data.EYE_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else {
				entry.updateCheckbox(Data.EYE_NAMES[entry.id], Data.menuValues[entry.id]);
			}
		}
	}
}