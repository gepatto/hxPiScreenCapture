package;

#if cpp
import cpp.Lib;
#elseif neko
import neko.Lib;
#end

class PiScreenCapture {
	
	
	public static function setPath(pathName:String):Void {
		return piscreencapture_setPath(pathName);
	}

	public static function capture():Int {
		
		return piscreencapture_capture();
		
	}
	
	private static var piscreencapture_setPath = Lib.load ("piscreencapture", "piscreencapture_setPath", 1);	
	private static var piscreencapture_capture = Lib.load ("piscreencapture", "piscreencapture_capture", 0);	
}