package {
    import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	import Shared.AS3.*;
	import flash.text.*;
	import flash.utils.*;
	import Shared.EntryEvent;
	
	public class SliderList extends BSUIComponent
	{
		public var entries:Vector.<SliderListEntry> = new Vector.<SliderListEntry>(15);
		public var listScroll:Option_Scrollbar_Vertical;
		public var title:TextField;
		
		public var entrySize:int = 57;
		public var listSize:int = 10;
		public var length:int;
		public var stepSize:Number;
		public var listPosition: int = 0;
		
		public static const SLIDER_MAX:int = 10;
		public static const LIST_MAX:int = 15;
		public static const LIST_LENGTH:int = 540;
		
		public static const LEFT = 1;
		public static const UP = 2;
		public static const RIGHT = 3;
		public static const DOWN = 4;
		public static const A = 5;

		public var type:int;
		
		public var focused:Boolean = false;
		public var selectedX:int = 0;
		public var selectedY:int = -1;
		
		public var storeX:int = 0;
		public var storeY:int = -1;
		public var storePos:int = -1;
		
		public var isEnabled:Boolean = true;
		
		public var defaultProperties = {
			x: 12,
			y: 36,
			margin: 10
		};
		
		public var sliderProperties = {
			x: 18,
			y: 55,
			margin: 10
		};
		
		public function SliderList() {
			super();
			addSliders(); 
			
			listScroll.minimum = 0;
			listScroll.maximum = 100;
			listScroll.StepSize = 1;
			listScroll.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			//updateScroll(Data.MAIN_MENU.length, LIST_MAX);
			
			addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel);
			addEventListener(EntryEvent.SELECTED, onEntry);
		}

		public function addSliders() {
			var entry:SliderListEntry;
			for(var i:int = 0; i < LIST_MAX; i++)
			{
				entry = new SliderListEntry();
				entry.checkbox.checkId = 0;
				entry.checkbox2.checkId = 1;
				this.addChild(entry);
				entries[i] = entry;			
			}
		}
		
		public function initEntryFunctions(func:EntryFunctions):void
		{
			for (var i:int = 0; i < LIST_MAX; i++)
			{
				entries[i].initFunctions(func);
			}
		}

		public function onValueChange(event:flash.events.Event)
		{
			//trace("value change", listSize, entrySize, listPosition);
			if (focused) {
				unselect();
				selectedY = -1;
			}
			var newPosition:int = int(event.target.value / stepSize);
			newPosition = Math.max(0,Math.min(entrySize - listSize, newPosition));
			updatePosition(newPosition);
		}
		
		public function scrollList(delta:int)
		{
			//trace("scroll list", listSize, entrySize, listPosition);
			if (listScroll.visible) {
				var newPosition:int = listPosition - delta;
				newPosition = Math.max(0,Math.min(entrySize - listSize, newPosition));
				var dif:int = newPosition - listPosition;
				if (updatePosition(newPosition)) {
					listScroll.position = stepSize * newPosition;
					selectedY += dif;
					return true;
				}
			}
			return;
		}

		public function onMouseWheel(event:MouseEvent)
		{
			//trace("mouse wheel", listSize, entrySize, listPosition);
//			var value:int = int(event.delta);
//			if (scrollList(value) && selectedY != -1) {
//				selectedY -= value;
//			}
			scrollList(int(event.delta));
		}
		
		public function confirm()
		{
			var entry:SliderListEntry = getEntry(selectedY);
			if (entry) {
				switch (selectedX) {
					case 0: entry.confirm(); break;
					case 1: entry.checkbox.confirm(); break;
					case 2: entry.checkbox2.confirm(); break;
				}
			}
		}
		
		public function select()
		{
			var entry:SliderListEntry = getEntry(selectedY);
			if (entry) {
				switch (selectedX) {
					case 0: entry.select(); break;
					case 1: entry.checkbox.selectCheck(); break;
					case 2: entry.checkbox2.selectCheck(); break;
				}
			}
		}
		
		public function unselect()
		{
			var entry:SliderListEntry = getEntry(selectedY);
			if (entry) {
				switch (selectedX) {
					case 0: entry.unselect(); break;
					case 1: entry.checkbox.setCheck(false); break;
					case 2: entry.checkbox2.setCheck(false); break;
				}
			}
		}
		
		public function getEntry(id:int):SliderListEntry
		{
			var entry:SliderListEntry;
			var index:int = id - listPosition;
			if (index >= 0 && index < entries.length) {
				entry = entries[index];
				if (entry.id == id) {
					return entry;
				}
				//if for some reason the entry is incorrect we should just search the entire list
				for (var i:int = 0; i < this.listSize; i++) {
					entry = entries[i];
					if (!entry.visible) {
						return null;
					}
					if (entry.id == id) {
						return entry;
					}
				}
			}
			return null;
		}
		
		public function onEntry(event:EntryEvent)
		{
			switch (event._type) {
				case EntryEvent.OVER:
					if (event.id != selectedY || event.horizontal != selectedX) {
						if (selectedY != -1) {
							unselect();
						}
						var entry:SliderListEntry = getEntry(event.id);
						if (event.horizontal == 0) {
							entry.selected = true;
						}
						selectedX = event.horizontal;
						selectedY = event.id;

						if (entry.selectable && isEnabled) {
							select();
						}
						focused = false;
					}
					break;
				case EntryEvent.OUT:
					if (event.id == selectedY && event.horizontal == selectedX) {
						unselect();
						selectedY = -1;
						focused = false;
					}
					break;
			}
		}
		
		public function processInput(input:int)
		{
			focused = true;
			
			if (selectedY == -1) {
				selectedX = 0;
				selectedY = listPosition;
				select();
				return;
			}
			
			switch (input) {
				case LEFT:
					processDirection(false);
					break;
				case UP:
					processUp(1);
					break;
				case RIGHT:
					processDirection(true);
					break;
				case DOWN:
					processDown(1);
					break;
				case A:
					confirm();
					break;
			}
		}
		
		public function processDirection(inc:Boolean)
		{
			var entry:SliderListEntry = getEntry(selectedY);
			if (!entry) 
				return;
			
			switch (entry.type) {
				case Data.ITEM_LIST:
					if (inc) {
						processDown(listSize);
					}
					else {
						processUp(listSize);
					}
					break;
				case Data.ITEM_SLIDER:
					if (inc)
						entry.slider.IncrementPad();
					else
						entry.slider.DecrementPad();
					break;
				case Data.ITEM_ADJUSTMENT:
					if (selectedY != -1)
						unselect();
					if (inc) {
						selectedX++;
						if (selectedX > 2) 
							selectedX = 0;
					} else {
						selectedX--;
						if (selectedX < 0)
							selectedX = 2;
					}
					select();
					break;
				case Data.ITEM_REMOVEABLE:
					if (selectedY != -1)
						unselect();
					if (inc) {
						selectedX++;
						if (selectedX > 1) 
							selectedX = 0;
					} else {
						selectedX--;
						if (selectedX < 0)
							selectedX = 1;
					}
					select();
					break;
				case Data.ITEM_TOUCH:
					entry.checkbox.forceDrag(inc);
					break;
			}
		}
		
		//TODO make variable
		public function processUp(i:int)
		{
			if (selectedY > listPosition) {
				unselect();
				selectedY--;
				select();
			} else {
				if (listPosition > 0) {
					unselect();
					scrollList(1);
					select();
				} else {
					scrollList(-(entrySize - listSize));
					unselect();
					selectedY = entrySize - 1;
					select();
				}
			}
		}
		
		//TODO make variable
		public function processDown(i:int)
		{
			if (selectedY < (listPosition + length - 1)) {
				unselect();
				selectedY++;
				select();
			} else {
				if (listPosition < (entrySize - listSize)) {
					unselect();
					scrollList(-1);
					select();
				} else {
					scrollList(listPosition);
					unselect();
					selectedY = 0;
					select();
				}
			}
		}
		
		public function updateState(pos:int)
		{
			//trace("updateState", listSize, entrySize, listPosition);
			if (focused) {
				if (selectedY != -1) {
					unselect();
				}
			} else {
				selectedY += (pos - listPosition);
			}
			listPosition = pos;
		}
		
		public function updateSelected(x:int, y:int)
		{
			//trace("update selected", listSize, entrySize, listPosition);
			if (focused) {
				selectedX = x;
				selectedY = y;
				select();
			}
		}
		
		public function storeSelected()
		{
			//trace("store selected", listSize, entrySize, listPosition);
			isEnabled = false;
			storeX = selectedX;
			storeY = selectedY;
			storePos = listPosition;
			if (storeY != -1) {
				unselect();
			}
		}
		
		public function restoreSelected()
		{
			//trace("restore selected", listSize, entrySize, listPosition);
			isEnabled = true;
			if (selectedY != -1) {
				storeY += (listPosition - storePos);
				if (storeY >= entrySize) {
					if (focused) {
						storeY = entrySize - 1;
					} else {
						return;
					}
				}
				selectedX = storeX;
				selectedY = storeY;
				select();
			}
		}
		
		public function getState(data:Object)
		{
			data.x = selectedX;
			data.y = selectedY;
			data.pos = listPosition;
		}
		
		public function updatePosition(newPosition:int):Boolean
		{
			//trace("update position", listSize, entrySize, listPosition);
			if (listPosition != newPosition) {
				var dif:int = newPosition - listPosition;
				listPosition = newPosition;
				for (var i:int = 0; i < listSize; i++) 
				{
					var entry:SliderListEntry = entries[i];
					var id:int = listPosition + i;
					entry.update(id, Data.getType(id), entry.x, entry.y);
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
		
		public function updateScroll():void
		{
			//trace("update scroll", listSize, entrySize, listPosition);
			if (this.entrySize > this.listSize)
			{
				stepSize = 100.0 / (this.entrySize - this.listSize);
				var thumbHeight:Number = listScroll.Track_mc.height * this.listSize / this.entrySize;
				listScroll.Thumb_mc.height = Math.max(thumbHeight, 40);
				listScroll.updateHeight();
				listScroll.visible = true;
				listScroll.position = stepSize * listPosition;
			}
			else
			{
				listScroll.visible = false;
			}
		}
		
		public function getEntryProperties(type:int):Object
		{
			if (type == Data.ITEM_SLIDER) {
				return sliderProperties;
			}
			return defaultProperties;
//			switch(type)
//			{
//				case SliderListEntry.SLIDER: return sliderProperties;
//				default: return defaultProperties;
//			}
		}
		
		public function update():void
		{
			var xOffset:int;
			var yOffset:int = 10;
			
			this.entrySize = Data.getMenuSize();
			this.listSize = 0;
			
			//clamp between 0 and entry size - 1
			this.listPosition = Math.max(0,Math.min(this.listPosition, entrySize - 1));
			
			var i:int;
			var id:int;
			var type:int;
			var properties:Object;
			
			//Need to first fill the screen to get current list position
			while (i < LIST_MAX && yOffset < LIST_LENGTH) {
				if (i + this.listPosition < this.entrySize) {
					//get from below
					id = i + this.listPosition;
					this.listSize++;
				}
				else if (this.listPosition > 0) {
					//get from top
					this.listPosition--;
					id = this.listPosition;
					this.listSize++;
				} else {
					break;
				}
				
				i++;
				
				type = Data.getType(id);
				properties = getEntryProperties(type);
				yOffset += properties.y;
			}
			
			yOffset = 10;
			
			//update actual position
			for(i = 0; i < LIST_MAX; i++)
			{
				id = i + this.listPosition;
				
				//update data until we run out of entries or screen space
				if (i < this.listSize) {
					type = Data.getType(id);
					properties = getEntryProperties(type);
					
					xOffset = properties.x;
					
					entries[i].update(id, type, xOffset, yOffset);
					
					yOffset += properties.y;
				} else {
					entries[i].id = id;
					entries[i].disable();
				}
			}
			
			this.length = Math.min(this.entrySize, this.listSize);
			
			updateScroll();
		}
	}
}