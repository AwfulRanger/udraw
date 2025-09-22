#ifndef __INPUT__H__
#define __INPUT__H__



#include "wiimote.h"



#define UDRAW_CLICKPRESSURE 10



class Mouse {
	
	protected:
		
		void * data;
		
	public:
		
		bool Send( const UDrawData & data );
		
		Mouse();
		~Mouse();
		
};



class Pen {
	
	protected:
		
		void * data;
		
	public:
		
		bool Send( const UDrawData & data );
		
		Pen();
		~Pen();
		
};



#endif
