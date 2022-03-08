#ifndef __INPUT__H__
#define __INPUT__H__



#include "wiimote.h"

#include <Windows.h>



#define UDRAW_CLICKPRESSURE 10



class Mouse {
	
	protected:
		
		INPUT input;
		bool pressed;
		
	public:
		
		bool Send( const UDrawData & data );
		
		Mouse();
		~Mouse();
		
};



class Pen {
	
	protected:
		
		HSYNTHETICPOINTERDEVICE device;
		POINTER_TYPE_INFO pointer;
		
	public:
		
		bool Send( const UDrawData & data );
		
		Pen();
		~Pen();
		
};



#endif
