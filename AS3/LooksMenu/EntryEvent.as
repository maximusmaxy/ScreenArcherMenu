package
{
    import flash.display.*;
    import flash.events.*;
    
    public class EntryEvent extends flash.events.Event
    {
		public static const SELECTED:String = "Selected";
		
		public var id:int;
		public var _type:int;
		public var horizontal:int;
		
		public static const OVER:int = 1;
		public static const OUT:int = 2;
		
        public function EntryEvent(id:int, type:int, horizontal:int = 0)
        {
            super(SELECTED, true);
            this.id = id;
			this._type = type;
			this.horizontal = horizontal;
        }
    }
}
