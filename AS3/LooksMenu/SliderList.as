package {
    import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	import Shared.AS3.*;
	import flash.text.*;
	import flash.sampler.Sample;
	import flash.utils.*;
	import Shared.EntryEvent;
	import utils.Translator;
	
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
		
		public static const LIST = 0;
		public static const TRANSFORM = 1;
		public static const MORPH = 2;
		public static const CHECKBOX = 3;
		public static const EYES = 4;
		public static const ADJUSTMENT = 5;
		public static const ADJUSTMENTEDIT = 6;
		public static const ADJUSTMENTORDER = 7;
		public static const POSITIONING = 8;
		public static const FOLDER = 9;
		public static const CAMERA = 10;
		
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
		
		public function SliderList() {
			super();
			addSliders(); 
			
			listScroll.minimum = 0;
			listScroll.maximum = 100;
			listScroll.StepSize = 1;
			listScroll.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			updateScroll(Data.MAIN_MENU.length, LIST_MAX);
			
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

		public function onValueChange(event:flash.events.Event)
		{
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
				for (var i:int = 0; i < listSize; i++) {
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
					select();
					break;
				case RIGHT:
					processDirection(true);
					break;
				case DOWN:
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
					break;
				case A:
					confirm();
					break;
			}
		}
		
		public function processDirection(inc:Boolean)
		{
			var entry:SliderListEntry = getEntry(selectedY);
			if (!entry) return;
			
			switch (entry.type) {
				case SliderListEntry.SLIDER:
					if (inc)
						entry.slider.IncrementPad();
					else
						entry.slider.DecrementPad();
					break;
				case SliderListEntry.ADJUSTMENT:
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
				case SliderListEntry.DRAG:
					entry.checkbox.forceDrag(inc);
					break;
			}
		}
		
		public function updateState(pos:int)
		{
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
			if (focused) {
				selectedX = x;
				selectedY = y;
				select();
			}
		}
		
		public function storeSelected()
		{
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
			this.length = Math.min(entrySize, listSize);
			
			listPosition = Math.max(0,Math.min(entrySize - listSize, listPosition));
			
			if (this.entrySize > this.listSize)
			{
				stepSize = 100.0 / (this.entrySize - this.listSize);
				var thumbHeight:Number = listScroll.Track_mc.height *  this.listSize / this.entrySize;
				listScroll.Thumb_mc.height = Math.max(thumbHeight, 40);
				listScroll.visible = true;
				listScroll.position = stepSize * listPosition;
				listScroll.updateHeight();
			}
			else
			{
				listScroll.visible = false;
			}
		}
		
		public function update(entrySize:int, listSize:int, func:Function, func2 = null, func3 = null)
		{
			updateScroll(entrySize, listSize);

			for(var i:int = 0; i < LIST_MAX; i++) 
			{
				if (i < length) {
					entries[i].update(i + listPosition, func, func2, func3);
					updateType(entries[i]);
				} else {
					entries[i].id = i + listPosition;
					entries[i].disable();
				}
			}
			
			updateLayout();
		}
		
		public function updateLayout():void
		{
			var xOffset:int;
			var yOffset:int;
			
			switch (entries[i].type) //init
			{
				case SliderListEntry.DIVIDER:
				case SliderListEntry.SLIDER:
					xOffset = 18;
					yOffset = 10;
					break;
				default:
					xOffset = 12;
					yOffset = 10;
			}
			
			for (var i:int = 0; i < LIST_MAX; i++)
			{
				if (entries[i].visible) {

					switch (entries[i].type) { //pre set pos
						case SliderListEntry.DIVIDER:
						case SliderListEntry.SLIDER:
							xOffset = 18;
							break;
						default:
							xOffset = 12;
					}
					
					entries[i].setPos(xOffset, yOffset);
					
					switch (entries[i].type) { //post set pos
						case SliderListEntry.DIVIDER:
						case SliderListEntry.SLIDER:
							yOffset += 55;
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
				case ADJUSTMENTORDER: updateAdjustmentOrderEntry(entry); break;
				case ADJUSTMENTEDIT: updateAdjustmentEditEntry(entry); break;
				case POSITIONING: updatePositioningEntry(entry); break;
				case FOLDER: updateFolderEntry(entry); break;
				case CAMERA: updateCameraEntry(entry); break;
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
		
		public function updateAdjustmentOrder(func:Function, func2:Function, func3:Function)
		{
			this.type = ADJUSTMENTORDER;
			update(Data.menuOptions.length, LIST_MAX, func, func2, func3);
		}
		
		public function updateAdjustmentOrderEntry(entry:SliderListEntry)
		{
			entry.updateAdjustmentOrder(Data.menuOptions[entry.id]);
		}
		
		public function updateAdjustmentEdit(func:Function)
		{
			this.type = ADJUSTMENTEDIT;
			update(Data.menuValues.length, LIST_MAX, func);
		}
		
		public function updateAdjustmentEditEntry(entry:SliderListEntry)
		{
			switch (entry.id)
			{
				case 0: //Scale
					entry.updateSliderData(0, 100, 1, 1, 0)
					entry.updateSlider("$SAM_Scale", SliderListEntry.INT);
					break;
				case 1: //Save
					entry.updateList("$SAM_SaveAdjustment");
					break;
				case 2: //Rename
					entry.updateList("$SAM_RenameAdjustment");
					break;
				case 3: //Reset
					entry.updateList("$SAM_ResetAdjustment");
					break;
				default:
					entry.updateList("Negate " + Translator.translate(Data.menuValues[entry.id]));
			}
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
				entry.updateSliderData(0.0, 360.0, 0.1, 1.0, 180.0, 2);
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else if (entry.id < 6) //pos
			{
				entry.updateSliderData(0.0, 20.0, 0.01, 0.1, 10.0, 4);
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else if (entry.id < 7)//scale
			{
				entry.updateSliderData(0.0, 2.0, 0.01, 0.02, 0.0, 4);
				entry.updateSlider(Data.TRANSFORM_NAMES[entry.id], SliderListEntry.FLOAT);
			}
			else if (entry.id < 10)//rot2
			{
				entry.updateDrag(Data.TRANSFORM_NAMES[entry.id])
				entry.checkbox.increment = 2.0;
			}
		}
		
		public function updateMorphs(func:Function):void
		{
			this.type = MORPH;
			update(Data.menuOptions.length, SLIDER_MAX, func);
		}
		
		public function updateMorphsEntry(entry:SliderListEntry):void
		{
			entry.updateSliderData(0, 100, 1, 1, 0);
			entry.updateSlider(Data.menuOptions[entry.id], SliderListEntry.INT);
		}
		
		public function updateEyes(func:Function):void
		{
			this.type = EYES;
			update(Data.EYE_NAMES.length, SLIDER_MAX, func);
		}
		
		public function updateEyesEntry(entry:SliderListEntry):void
		{
			entry.updateSliderData(0.0, 2.0, 0.01, 0.05, 1.0, 4);
			entry.updateSlider(Data.EYE_NAMES[entry.id], SliderListEntry.FLOAT);
		}
		
		public function updatePositioning(func:Function):void
		{
			this.type = POSITIONING;
			update(Data.POSITIONING_NAMES.length, LIST_MAX, func);
		}
		
		public function updatePositioningEntry(entry:SliderListEntry):void
		{
			if (entry.id < 1) {
				entry.updateSliderData(0, 500, 1, 1, 0, 0);
				entry.updateSlider(Data.POSITIONING_NAMES[entry.id], SliderListEntry.INT);
			}
			else if (entry.id < 8) {
				entry.updateDragValue(Data.POSITIONING_NAMES[entry.id]);
				if (entry.id < 4) { //pos
					entry.checkbox.increment = 10.0
				} else if (entry.id < 7){ //rot
					entry.checkbox.increment = 2.0;
				} else { //scale
					entry.checkbox.increment = 1.0;
				}
			}
			else {
				entry.updateList(Data.POSITIONING_NAMES[entry.id]);
			}
		}
		
		public function updateFolder(func:Function):void
		{
			this.type = FOLDER;
			update(Data.menuOptions.length, LIST_MAX, func);
		}
		
		public function updateFolderEntry(entry:SliderListEntry):void
		{
			if (Data.menuFolder[entry.id].folder) {
				entry.updateFolder(Data.menuOptions[entry.id]);
			} else {
				entry.updateList(Data.menuOptions[entry.id]);
			}
		}
		
		public function updateCamera(func:Function):void
		{
			this.type = CAMERA;
			update(Data.menuValues.length, LIST_MAX, func);
		}
		
		public function updateCameraEntry(entry:SliderListEntry):void
		{
			switch (entry.id) {
				case 0: //pos
				case 1:
				case 2: 
					entry.updateDragValue(Data.CAMERA_NAMES[entry.id]);
					entry.checkbox.increment = 10.0
					break;
				case 3: //yaw
					entry.updateSliderData(0.0, 360.0, 0.1, 1.0, 0.0, 2);
					entry.updateSlider(Data.CAMERA_NAMES[entry.id], SliderListEntry.FLOAT);
					break;
				case 4: //pitch
					entry.updateSliderData(0.0, 360.0, 0.1, 1.0, 180, 2);
					entry.updateSlider(Data.CAMERA_NAMES[entry.id], SliderListEntry.FLOAT);
					break;
				case 5: //roll
					entry.updateSliderData(0.0, 360.0, 0.1, 1.0, 180, 2);
					entry.updateSlider(Data.CAMERA_NAMES[entry.id], SliderListEntry.FLOAT);
					break;
				case 6: //fov
					entry.updateSliderData(1.0, 160.0, 0.1, 1.0, 0.0, 2);
					entry.updateSlider(Data.CAMERA_NAMES[entry.id], SliderListEntry.FLOAT);
					break;
				default: //save/load state
					//save/load are half the remainder each
					var save:Boolean = entry.id < (7 + ((Data.menuValues.length - 7) / 2));
					entry.updateList(Translator.translate(save ? "$SAM_SaveState" : "$SAM_LoadState") + " " + (Data.menuValues[entry.id] + 1));
					break;
			}
		}
	}
}