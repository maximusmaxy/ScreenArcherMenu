package {

    import flash.utils.*;
	import flash.events.*;

    public class LatentCallback {
    
		public var waiting:Boolean;
		public var success:Boolean
		public var result:GFxResult;
		public var timer:Timer;
		public var timeout:int;
		public var timeoutFunction:Function;
	
		public function LatentCallback(failure:Function)
		{
			timeoutFunction = failure;
		}
	
		public function Init(_timeout:int)
		{
			this.success = false;
			this.waiting = true;
			this.result = null;
			this.timeout = _timeout;
		}
	
		public function Start()
		{
			if (waiting) {
				this.timer = new Timer(this.timeout, 1);
				this.timer.addEventListener(TimerEvent.TIMER_COMPLETE, TimeOut);
				this.timer.start();
			}
		}
	
		public function Recieve(_result:GFxResult)
		{
			if (this.waiting) {
				this.result = _result;
				this.waiting = false;
				this.success = true;
			}
		}
	
		public function Store(_result:GFxResult)
		{
			this.waiting = false;
			this.success = true;
			this.result = _result;
		}
	
		public function TimeOut(e:TimerEvent)
		{
			if (this.waiting) {
				this.timeoutFunction();
			}
			Stop();
		}
	
		public function Clear()
		{
			this.success = false;
			this.waiting = false;
			this.result = null;
			if (this.timer) {
				this.timer.stop();
				this.timer.removeEventListener(TimerEvent.TIMER_COMPLETE, TimeOut);
				this.timer = null;
			}
		}
	}
}