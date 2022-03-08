#include "input.h"
#include "wiimote.h"



bool Mouse::Send( const UDrawData & data ) {
	
	bool sendevent = false;
	
	this->input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) {
		
		this->input.mi.dx = ( LONG ) ( data.x * ( 65535.0f / 1920.0f ) );
		this->input.mi.dy = ( LONG ) ( data.y * ( 65535.0f / 1080.0f ) );
		this->input.mi.dwFlags |= MOUSEEVENTF_MOVE;
		
		sendevent = true;
		
	}
	
	bool click = data.pressure >= UDRAW_CLICKPRESSURE;
	if ( click != this->pressed ) {
		
		this->pressed = click;
		
		if ( click == true ) { input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN; }
		else { input.mi.dwFlags |= MOUSEEVENTF_LEFTUP; }
		
		sendevent = true;
		
	}
	
	if ( sendevent == true ) {
		
		SendInput( 1, & this->input, sizeof( input ) );
		
	}
	
	return true;
	
}

Mouse::Mouse() {
	
	this->input.type = INPUT_MOUSE;
	this->input.mi.dx = 0;
	this->input.mi.dy = 0;
	this->input.mi.mouseData = 0;
	this->input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	this->input.mi.time = 0;
	this->input.mi.dwExtraInfo = 0;
	
	this->pressed = false;
	
}
Mouse::~Mouse() {}



bool Pen::Send( const UDrawData & data ) {
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) { this->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_INRANGE; }
	else { this->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_INRANGE; }
	
	if ( data.pressure >= UDRAW_CLICKPRESSURE ) { this->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_INCONTACT; }
	else { this->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_INCONTACT; }
	
	if ( data.click == true ) { this->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_FIRSTBUTTON; }
	else { this->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_FIRSTBUTTON; }
	
	if ( data.sideclick1 == true || data.sideclick2 == true ) { this->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_SECONDBUTTON; }
	else { this->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_SECONDBUTTON; }
	
	this->pointer.penInfo.pointerInfo.ptPixelLocation = { data.x, data.y };
	this->pointer.penInfo.pointerInfo.ptHimetricLocation = { data.x, data.y };
	this->pointer.penInfo.pointerInfo.ptPixelLocationRaw = { data.x, data.y };
	this->pointer.penInfo.pointerInfo.ptHimetricLocationRaw = { data.x, data.y };
	this->pointer.penInfo.pressure = data.pressure * 2;
	
	return InjectSyntheticPointerInput( this->device, & this->pointer, 1 );
	
}

Pen::Pen() {
	
	this->device = CreateSyntheticPointerDevice( PT_PEN, 1, POINTER_FEEDBACK_DEFAULT );
	
	this->pointer.type = PT_PEN;
	memset( & this->pointer.penInfo, 0, sizeof( this->pointer.penInfo ) );
	this->pointer.penInfo.pointerInfo.pointerType = PT_PEN;
	this->pointer.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_NONE;
	this->pointer.penInfo.penMask = PEN_MASK_PRESSURE;
	
}
Pen::~Pen() {
	
	DestroySyntheticPointerDevice( this->device );
	
}
