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
		public var divider:MovieClip;
		public var checkbox:Checkbox;
		public var checkbox2:Checkbox;
		public var background:MovieClip;
		
		internal var id:int = -1;
		public var selectable:Boolean = false;
		public var selected:Boolean = false;
		public var func:Function;
		public var func2:Function;
		public var func3:Function;
		
		public var sliderMod:Number = 0.0;
		public var sliderFixed:int = 0;
		
		public static const LIST = 0;
		public static const SLIDER = 1;
		public static const DIVIDER = 2;
		public static const CHECKBOX = 3;
		public static const ADJUSTMENT = 4;
		public static const DRAG = 5;
		
		public static const INT = 0;
		public static const FLOAT = 1;
		
		public var type:int;
		public var valueType:int;

		public function SliderListEntry()
		{
			super();
			slider.offsetLeft = 1;
			slider.offsetRight = 1;
			
			divider.visible = false;
			slider.visible = false;
			value.visible = false;
			
			slider.addEventListener(Option_Scrollbar.VALUE_CHANGE, onValueChange);
			text.addEventListener(MouseEvent.CLICK, onMouseClick);
			text.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			
			value.addEventListener(MouseEvent.CLICK, onValueClick);
			value.addEventListener(Event.CHANGE, onValueInput);
		}

		public function onMouseClick(event:flash.events.Event)
		{
			if (selectable) {
				switch (this.type)
				{
					case LIST: 
					case ADJUSTMENT:
						func.call(null, id); 
						break;
					case CHECKBOX:
						checkbox.onClick(event);
						break;
				}
				Util.playOk();
			}
		}
		
		public function onValueChange(event:flash.events.Event)
		{
			//Util.playFocus();
			func.call(null, id, event.target.value - sliderMod);
			Data.selectedSlider = this;
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
						slider.position = parsedInt + sliderMod;
					}
					break;
				}
				case FLOAT:
				{
					menuValue = parseFloat(value.text);
					if (!isNaN(menuValue)) {
						func.call(null, id, menuValue);
						slider.position = menuValue + sliderMod;
					}
					break;
				}
			}
		}
		
		public function onValueClick(event:MouseEvent)
		{
			if (value.visible) {
				if (Data.selectedText == null) {
					try {
						Data.f4seObj.AllowTextInput(true);
					} catch (e:Error) {
						trace("Failed to allow text input");
					}
				}
				Data.selectedSlider = null;
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
			if (selectable) {
				select();
			}
		}
		
		public function onMouseOut(event:MouseEvent)
		{
			unselect();
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
			var menuValue;
			switch (valueType)
			{
				case INT:
					menuValue = int(Data.menuValues[id]);
					value.text = menuValue;
					break;
				case FLOAT:
					menuValue = Data.menuValues[id];
					value.text = menuValue.toFixed(sliderFixed);
					break;
			}
			switch (type)
			{
				case SLIDER:
					if (position) 
					{
						slider.position = menuValue + sliderMod;
					}
					break;
				case CHECKBOX:
					checkbox.setCheck(Data.menuValues[id]);
					break;
			}
		}
		
		public function setText(x:int, y:int, name:String, align:String)
		{
			text.visible = true;
			text.x = x;
			text.y = y;
			text.text = name;
			var format:TextFormat = text.getTextFormat();
			if (format.align != align) {
				format.align = align;
				text.setTextFormat(format);
			}
		}
		
		public function updateList(name:String)
		{
			setText(0, 0, name, TextFormatAlign.LEFT);
			slider.visible = false;
			value.visible = false;
			divider.visible = false;
			checkbox.disable();
			checkbox2.disable();
			background.x = -2;
			background.width = 290;
			selectable = true;
			this.type = LIST;
		}
		
		public function updateSlider(name:String, valueType:int)
		{
			unselect();
			selectable = false;
			setText(0, 0, name, TextFormatAlign.LEFT);
			slider.visible = true;
			value.visible = true;
			divider.visible = false;
			checkbox.disable();
			checkbox2.disable();
			this.type = SLIDER;
			this.valueType = valueType;
			updateValue(true); 
		}
		
		public function updateDivider(name:String)
		{
			unselect();
			selectable = false;
			setText(0, 7, name, TextFormatAlign.CENTER);
			slider.visible = false;
			value.visible = false;
			divider.visible = true;
			checkbox.disable();
			checkbox2.disable();
			this.type = DIVIDER;
		}
		
		public function updateCheckbox(name:String, checked:Boolean)
		{
			selectable = true;
			setText(31, 0, name, TextFormatAlign.LEFT);
			slider.visible = false;
			value.visible = false;
			divider.visible = false;
			checkbox.init(1, this.id, checked, 0, func);
			checkbox2.disable();
			background.x = 31;
			background.width = 258;
			this.type = CHECKBOX;
		}
		
		public function updateAdjustment(name:String) 
		{
			selectable = true;
			setText(0, 0, name, TextFormatAlign.LEFT);
			slider.visible = false;
			value.visible = false;
			divider.visible = false;
			checkbox.init(222, this.id, false, Checkbox.SETTINGS, func2);
			checkbox2.init(256, this.id, false, Checkbox.RECYCLE, func3);
			background.x = -2;
			background.width = 218;
			this.type = ADJUSTMENT;
		}
		
		public function updateDrag(name:String)
		{
			unselect();
			selectable = false;
			setText(31, 0, name, TextFormatAlign.LEFT)
			slider.visible = false;
			value.visible = false;
			divider.visible = false;
			checkbox.init(1, this.id, false, Checkbox.DRAG, func);
			checkbox2.disable();
			this.type = DRAG;
		}
		
		public function updateSliderData(min:Number, max:Number, step:Number, mod:Number, fixed:int = 0)
		{
			slider.minimum = min;
			slider.maximum = max;
			slider.StepSize = step;
			this.sliderMod = mod;
			this.sliderFixed = fixed;
		}
		
		public function disable()
		{
			this.visible = false;
			this.func = null;
			this.selectable = false;
		}
		
		public function select()
		{
			if (!selected) {
				selected = true;
				//text.background = true;
				//text.backgroundColor = 0xFFFFFF;
				text.textColor = 0x000000;
				background.alpha = 1.0;
				if (type == CHECKBOX) {
					checkbox.setColor(0x000000);
				}
				text.addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
				text.removeEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
				Util.playFocus();
			}
		}
		
		public function unselect()
		{
			if (selected) {
				text.textColor = 0xFFFFFF;
				//text.background = false;
				background.alpha = 0.0;
				if (type == CHECKBOX) {
					checkbox.setColor(0xFFFFFF);
				}
				text.addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
				text.removeEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
				selected = false;
			}
		}
	}
}