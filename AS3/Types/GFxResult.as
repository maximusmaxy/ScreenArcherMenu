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
            if (type == Data.RESULT_ITEMS) {
                Util.traceObj(this.result.names);
                Util.traceObj(this.result.values);
            } else {
                Util.traceObj(this.result);
            }
		}
    }
}