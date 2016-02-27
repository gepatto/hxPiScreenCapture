# hxPiScreenCapture
Haxe / OpenFL extension for grabbing the screen on a Raspberry Pi

# Requirements
make sure you have libpng12-dev installed:
> sudo apt-get install libpng12-dev

# Usage

Include hxPiScreenCapture in your project.xml 
> <haxelib name="hxPiScreenCapture" />

In your project set a Path (default is /home/pi)
> PiScreenCapture.setPath("/home/pi/Desktop/");

Call capture when you want to make a screenshot
> PiScreenCapture.capture()


----  
uses the code from [ raspi2png ](https://github.com/AndrewFromMelbourne/raspi2png)
