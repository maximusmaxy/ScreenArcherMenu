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
		public var func:Function;
		public var func2:Function;
		public var func3:Function;
		
		public var valueMod:Number = 0.0;
		public var valueFixed:int = 0;
		
		public static const LIST = 0;
		public static const SLIDER = 1;
		public static const CHECKBOX = 2;
		public static const ADJUSTMENT = 3;
		public static const DRAG = 4;
		public static const FOLDER = 5;
		
		public static const NONE = 0;
		public static const INT = 1;
		public static const FLOAT = 2;
		
		public var type:int;
		public var valueType:int;

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
					case LIST: 
					case ADJUSTMENT:
					case FOLDER:
						func.call(null, id); 
						Util.playOk();
						break;
					case CHECKBOX:
					case FOLDERCHECKBOX:
						checkbox.confirm();
						break;
				}
			}
		}
		
		public function onValueChange(event:Event)
		{
			//Util.playFocus();
			func.call(null, id, event.target.value - valueMod);
			updateValue(false);
		}
		
		public function onValueInput(event:Event)
		{
			var menuValue:Number;
			switch (valueType)
			{
				case INT:
				{
					menuValue =  parseInt(value.text);
					if (!isNaN(menuValue)) {
						var parsedInt:int = int(menuValue);
						func.call(null, id, parsedInt);
						slider.position = parsedInt + valueMod;
					}
					break;
				}
				case FLOAT:
				{
					menuValue = parseFloat(value.text);
					if (!isNaN(menuValue)) {
						func.call(null, id, menuValue);
						slider.position = menuValue + valueMod;
					}
					break;
				}
			}
		}
		
		public function onValueClick(event:MouseEvent)
		{
			if (value.visible && valueSelectable) {
				if (Data.selectedText == null) {
					try {
						Data.f4seObj.AllowTextInput(true);
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
		
		public function update(id:int, func:Function, func2:Function, func3:Function)
		{	
			this.visible = true;
			this.id = id;
			this.func = func;
			this.func2 = func2;
			this.func3 = func3;
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
				case LIST:
				case ADJUSTMENT:
				case FOLDER:
					return;
				case DRAG:
					if (!value.visible) return;
			}
			var menuValue;
			switch (valueType)
			{
				case INT:
					menuValue = int(Data.menuValues[id]);
					value.text = menuValue;
					break;
				case FLOAT:
					menuValue = Data.menuValues[id];
					value.text = menuValue.toFixed(valueFixed);
					break;
			}
			switch (type)
			{
				case SLIDER:
					if (position) 
					{
						slider.position = menuValue + valueMod;
					}
					break;
				case CHECKBOX:
					checkbox.setCheck(Data.menuValues[id]);
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
		
		public function updateList(name:String)
		{
			setSelectable(true);
			setText(0, 0, 254, name);
			slider.visible = false;
			value.visible = false;
			checkbox.disable();
			checkbox2.disable();
			Util.setRect(background, -2, -3.25, 290, 32);
			this.type = LIST;
			this.valueType = NONE;
		}
		
		public function updateSlider(name:String, valueType:int)
		{
			setSelectable(false);
			setText(0, 0, 254, name);
			slider.visible = true;
			value.visible = true;
			value.x = 219.7;
			value.y = 25.6;
			valueSelectable = true;
			checkbox.disable();
			checkbox2.disable();
			Util.setRect(background, -2, -2, 223, 29);
			this.type = SLIDER;
			this.valueType = valueType;
			updateValue(true); 
		}
		
		public function updateCheckbox(name:String, checked:Boolean)
		{
			setSelectable(true);
			setText(31, 0, 254, name);
			slider.visible = false;
			value.visible = false;
			checkbox.init(1, this.id, Checkbox.CHECK, false, func);
			checkbox.setCheck(checked);
			checkbox2.disable();
			Util.setRect(background, 31, -3.25, 258, 32);
			this.type = CHECKBOX;
			this.valueType = NONE;
		}
		
		public function updateAdjustment(name:String) 
		{
			setSelectable(true);
			setText(0, 0, 218, name);
			slider.visible = false;
			value.visible = false;
			checkbox.init(222, this.id, Checkbox.SETTINGS, true, func2);
			checkbox2.init(256, this.id, Checkbox.RECYCLE, true, func3);
			Util.setRect(background, -2, -3.25, 218, 32);
			this.type = ADJUSTMENT;
			this.valueType = NONE;
		}
		
		public function updateAdjustmentOrder(name:String) 
		{
			setSelectable(true);
			setText(0, 0, 218, name);
			slider.visible = false;
			value.visible = false;
			checkbox.init(222, this.id, Checkbox.DOWN, true, func2);
			checkbox2.init(256, this.id, Checkbox.UP, true, func3);
			Util.setRect(background, -2, -3.25, 218, 32);
			this.type = ADJUSTMENT;
			this.valueType = NONE;
		}
		
		public function updateDrag(name:String)
		{
			setSelectable(false);
			setText(31, 0, 254, name)
			slider.visible = false;
			value.visible = false;
			checkbox.init(1, this.id, Checkbox.DRAG, false, func);
			checkbox2.disable();
			Util.setRect(background, 31, -3.25, 188, 32);
			this.type = DRAG;
			this.valueType = NONE;
		}
		
		public function updateDragValue(name:String)
		{
			setSelectable(false);
			setText(31, 0, 254, name)
			slider.visible = false;
			value.visible = true;
			value.x = 219.7;
			value.y = 1;
			valueSelectable = false;
			valueMod = 0;
			valueFixed = 2;
			checkbox.init(1, this.id, Checkbox.DRAG, false, func);
			checkbox2.disable();
			Util.setRect(background, 31, -3.25, 188, 32);
			this.type = DRAG;
			this.valueType = FLOAT;
			updateValue(false);
		}
		
		public function updateFolder(name:String)
		{
			setSelectable(true);
			setText(33, 0, 254, name);
			slider.visible = false;
			value.visible = false;
			checkbox.init(3, this.id, Checkbox.FOLDER, false, func);
			checkbox.setCheck(selected);
			checkbox2.disable();
			Util.setRect(background, -2, -3.25, 290, 32);
			this.type = FOLDER;
			this.valueType = NONE;
		}
		
		public function updateSliderData(min:Number, max:Number, step:Number, stepPad:Number, mod:Number, fixed:int = 0)
		{
			slider.minimum = min;
			slider.maximum = max;
			slider.StepSize = step;
			slider.StepSizePad = stepPad;
			this.valueMod = mod;
			this.valueFixed = fixed;
		}
		
		public function disable()
		{
			this.visible = false;
			this.func = null;
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