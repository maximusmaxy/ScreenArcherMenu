package {
    public class GFxResult {
        
        public var type:int;
        public var result:Object;

        public function GFxResult(_type:int, _result:Object) 
        {
            this.type = _type;
            this.result = _result;
        }
		
		public function traceResult():void
		{
			trace("Type: " + this.type);
			Util.traceObj(this.result);
		}
    }
}