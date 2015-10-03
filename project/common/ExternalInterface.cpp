#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
#define NEKO_COMPATIBLE
#endif


#include <hx/CFFI.h>
#include "Utils.h"


using namespace piscreencapture;


static void piscreencapture_setPath ( value pathName) {
	setPath( val_string(pathName) );
}
DEFINE_PRIM (piscreencapture_setPath, 1);


static value piscreencapture_capture () {
	
	int returnValue = capture();
	return alloc_int(returnValue);
	
}
DEFINE_PRIM (piscreencapture_capture, 0);



extern "C" void piscreencapture_main () {
	
	val_int(0); // Fix Neko init
	
}
DEFINE_ENTRY_POINT (piscreencapture_main);



extern "C" int piscreencapture_register_prims () { return 0; }