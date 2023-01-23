package {
    public class MenuState {
        
        public var menu:String;
        public var pos:int;
        public var x:int;
        public var y:int;

        public function MenuState(_menu:String, _pos:int, _x:int, _y:int) 
        {
            this.menu = _menu;
            this.pos =_pos;
            this.x = _x;
            this.y = _y;
        }
    }
}