package 
{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.ui.*;
	import flash.text.*;
	import Shared.*;
	import Shared.AS3.*;

	public class SliderListEntry extends flash.display.MovieClip
	{
		public var text:TextField;
		public var value:TextField;
		public var slider:Option_Scrollbar;
		public var checkbox:Checkbox;
		public var checkbox2:Checkbox;
		public var background:MovieClip;
		
		internal var id:int = -1;
		public var selected:Boolean = false;
		public var selectable:Boolean = true;
		public var valueSelectable:Boolean = false;
		
		public var valueMod:Number = 0.0;
		public var valueFixed:int = 0;
		
		public var type:int;
		public var valueType:int;
		
		public var functions:EntryFunctions;

		public function SliderListEntry()
		{
			super();
			slider.offsetLeft = 1;
			slider.offsetRight = 1;
			
			slider.visible = false;
			value.visible = false;
			
			slider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			text.addEventListener(MouseEvent.CLICK, onMouseClick);
			text.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			
			value.addEventListener(MouseEvent.CLICK, onValueClick);
			value.addEventListener(Event.CHANGE, onValueInput);
		}
		
		public function initFunctions(funcs:EntryFunctions)
		{
			this.functions = funcs;
			this.checkbox.functions = funcs;
			this.checkbox2.functions = funcs;
		}

		public function onMouseClick(event:MouseEvent)
		{
			if (event.type == MouseEvent.CLICK) //left click
			{
				confirm();
			}
		}
		
		public function confirm()
		{
			if (visible) {
				switch (this.type)
				{
					case Data.ITEM_LIST: 
						functions.list(id);
						Util.playOk();
						break;
					case Data.ITEM_ADJUSTMENT:
						functions.list(id);
						Util.playOk();
						break;
					case Data.ITEM_FOLDER:
						functions.list(id); 
						Util.playOk();
						break;
					case Data.ITEM_CHECKBOX:
					case Data.ITEM_FOLDER:
						checkbox.confirm();
						break;
				}
			}
		}
		
		public function onValueChange(event:Event)
		{
			//Util.playFocus();
			switch(this.valueType) {
				case Data.VALUE_INT:
					functions.valueInt(id, event.target.value - valueMod);
					break;
				case Data.VALUE_FLOAT:
					functions.valueFloat(id, event.target.value - valueMod);
					break;
			}
			updateValue(false);
		}
		
		public function onValueInput(event:Event)
		{
			var menuValue:Number;
			switch (this.valueType)
			{
				case Data.VALUE_INT:
				{
					menuValue = parseInt(value.text);
					if (!isNaN(menuValue)) {
						var parsedInt:int = int(menuValue);
						functions.valueInt(id, parsedInt);
						slider.position = parsedInt + valueMod;
					}
					break;
				}
				case Data.VALUE_FLOAT:
				{
					menuValue = parseFloat(value.text);
					if (!isNaN(menuValue)) {
						functions.valueFloat(id, menuValue);
						slider.position = menuValue + valueMod;
					}
					break;
				}
			}
		}
		
		public function onValueClick(event:MouseEvent)
		{
			if (value.visible && this.valueSelectable) {
				if (Data.selectedText == null) {
					try {
						Data.f4se.AllowTextInput(true);
					} catch (e:Error) {
						trace("Failed to allow text input");
					}
				}
				Data.selectedText = this;
				value.type = TextFieldType.INPUT;
				value.selectable = true;
				value.maxChars = 10;
				stage.focus = value;
				value.setSelection(0, value.text.length);
			}
		}
		
		public function onMouseOver(event:MouseEvent)
		{
			text.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
			text.removeEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			dispatchEvent(new EntryEvent(id, EntryEvent.OVER));
		}
		
		public function onMouseOut(event:MouseEvent)
		{
			text.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			text.removeEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
			dispatchEvent(new EntryEvent(id, EntryEvent.OUT));
		}
		
		public function update(id:int, type:int, xPos:int, yPos:int)
		{	
			this.visible = true;
			this.type = type;
			this.id = id;
			this.x = xPos;
			this.y = yPos;
			
			updateType();
		}
		
		public function updatePosition(id:int, type:int)
		{
			this.visible = true;
			this.type = type;
			this.id = id;
			
		}
		
		public function clear()
		{
			this.visible = false;
			this.func = null;
		}
		
		public function setPos(x:int, y:int)
		{
			this.x = x;
			this.y = y;
		}

		public function updateValue(position:Boolean)
		{
			switch (type)
			{
				case Data.ITEM_LIST:
				case Data.ITEM_ADJUSTMENT:
				case Data.ITEM_FOLDER:
					return;
				case Data.ITEM_TOUCH:
					if (!value.visible) return;
			}
			var menuValue;
			switch (valueType)
			{
				case Data.VALUE_INT:
					menuValue = int(Data.getInt(id));
					value.text = menuValue;
					break;
				case Data.VALUE_FLOAT:
					menuValue = Data.getFloat(id);
					value.text = menuValue.toFixed(valueFixed);
					break;
			}
			switch (type)
			{
				case Data.ITEM_SLIDER:
					if (position) 
					{
						slider.position = menuValue + valueMod;
					}
					break;
				case Data.ITEM_CHECKBOX:
					checkbox.setCheck(Data.getBool(id));
					break;
			}
		}

		public function setText(x:int, y:int, width:int, name:String)
		{
			text.visible = true;
			text.x = x;
			text.y = y;
			text.width = width;
			text.text = name;
			var format:TextFormat = text.getTextFormat();
			if (format.align != TextFormatAlign.LEFT) {
				format.align = TextFormatAlign.LEFT;
				text.setTextFormat(format);
			}
		}
		
		public function setSelectable(enabled:Boolean)
		{
			selectable = enabled;
			if (selected)
			{
				setBackground(selectable);
			}
		}
		
		public function updateType()
		{
			switch(this.type) {
				case Data.ITEM_LIST: updateList(); break;
				case Data.ITEM_SLIDER: updateSlider(); break;
				case Data.ITEM_CHECKBOX: updateCheckbox(); break;
				case Data.ITEM_ADJUSTMENT: updateAdjustment(); break;
				case Data.ITEM_TOUCH: updateTouch(); break;
				case Data.ITEM_FOLDER: updateFolder(); break;
				default: disable();
			}
		}
		
		public function updateList()
		{
			setSelectable(true);
			setText(0, 0, 254, Data.getName(this.id));
			slider.visible = false;
			value.visible = false;
			checkbox.disable();
			checkbox2.disable();
			Util.setRect(background, -2, -3.25, 290, 32);
			this.valueType = Data.NONE;
		}
		
		public function updateSlider()
		{
			setSelectable(false);
			setText(0, 0, 254, Data.getName(this.id));
			slider.visible = true;
			value.visible = true;
			value.x = 219.7;
			value.y = 25.6;
			valueSelectable = true;
			checkbox.disable();
			checkbox2.disable();
			Util.setRect(background, -2, -2, 223, 29);
			
			var data:Object = Data.getSlider(this.id);
			
			//there is a bug with values less than 0 so we modify the values to be 0 and above
			this.valueMod = (data.min < 0 ? -data.min : 0)
			slider.minimum = data.min + this.valueMod;
			slider.maximum = data.max + this.valueMod;
			
			slider.StepSize = data.step;
			slider.StepSizePad = data.stepkey;
			
			this.valueType = data.type;
			if (this.valueType == Data.VALUE_NONE) {
				this.valueType = Data.VALUE_INT; // force to int if none
			} else if (this.valueType == Data.VALUE_FLOAT) {
				if (data.fixed) {
					this.valueFixed = data.fixed;
				} else {
					this.valueFixed = 4;
				}
			}
			
			updateValue(true); 
		}
		
		public function updateCheckbox()
		{
			setSelectable(true);
			setText(31, 0, 254, Data.getName(this.id));
			slider.visible = false;
			value.visible = false;
			checkbox.init(1, this.id, Data.CHECKBOX_CHECK, false);
			checkbox.setCheck(Data.getCheckbox(this.id));
			checkbox2.disable();
			Util.setRect(background, 31, -3.25, 258, 32);
			this.valueType = Data.NONE;
		}
		
		public function updateAdjustment()
		{
			setSelectable(true);
			setText(0, 0, 218, Data.getName(this.id));
			slider.visible = false;
			value.visible = false;
			Util.setRect(background, -2, -3.25, 218, 32);
			this.valueType = Data.NONE;
			
			if (Data.locals.order) {
				checkbox.init(222, this.id, Data.CHECKBOX_DOWN, true);
				checkbox2.init(256, this.id, Data.CHECKBOX_UP, true);
			} else {
				checkbox.init(222, this.id, Data.CHECKBOX_SETTINGS, true);
				checkbox2.init(256, this.id, Data.CHECKBOX_RECYCLE, true);
			}
		}
		
		public function updateTouch()
		{
			setSelectable(false);
			setText(31, 0, 254, Data.getName(this.id));
			slider.visible = false;
			checkbox.init(1, this.id, Data.CHECKBOX_TOUCH, false);
			checkbox2.disable();
			Util.setRect(background, 31, -3.25, 188, 32);
			
			var data:Object = Data.getTouch(this.id);
			this.valueType = data.type;
			this.value.visible = data.visible;
			
			checkbox.increment = data.step;
			checkbox.mod = data.mod;
			
			if (this.value.visible) {
				value.visible = true;
				value.x = 219.7;
				value.y = 1;
				valueSelectable = false;
				valueMod = 0;

				if (data.fixed) {
					this.valueFixed = data.fixed;
				} else {
					this.valueFixed = 2;
				}
					
				updateValue(false)
			}
		}
		
		public function updateFolder()
		{
			setSelectable(true);
			setText(33, 0, 254, Data.getName(this.id));
			slider.visible = false;
			value.visible = false;
			checkbox.init(3, this.id, Data.CHECKBOX_FOLDER, false);
			checkbox.setCheck(selected);
			checkbox2.disable();
			Util.setRect(background, -2, -3.25, 290, 32);
			this.valueType = Data.NONE;
		}
		
		public function disable()
		{
			this.visible = false;
			this.selectable = false;
			//move to force mouse out
			this.y = -100;
		}
		
		public function select()
		{
			selected = true;
			setBackground(true);
		}
		
		public function unselect()
		{
			selected = false;
			setBackground(false);
		}
		
		public function setBackground(select:Boolean)
		{
			if (select) {
				text.textColor = 0x000000;
				background.alpha = 1.0;
				if (checkbox.visible) {
					checkbox.select();
				}
				Util.playFocus();
			} else {
				text.textColor = 0xFFFFFF;
				background.alpha = 0.0;
				if (checkbox.visible) {
					checkbox.unselect();
				}
			}
		}
	}
}