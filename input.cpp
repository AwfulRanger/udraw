#include "input.h"
#include "wiimote.h"



#ifdef _WIN32



#include <Windows.h>



struct mousedata {
	
	INPUT input;
	bool pressed;
	
};

bool Mouse::Send( const UDrawData & data ) {
	
	mousedata * mdata = ( mousedata * ) this->data;
	
	bool sendevent = false;
	
	mdata->input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) {
		
		mdata->input.mi.dx = ( LONG ) ( data.x * ( 65535.0f / 1920.0f ) );
		mdata->input.mi.dy = ( LONG ) ( data.y * ( 65535.0f / 1080.0f ) );
		mdata->input.mi.dwFlags |= MOUSEEVENTF_MOVE;
		
		sendevent = true;
		
	}
	
	bool click = data.pressure >= UDRAW_CLICKPRESSURE;
	if ( click != mdata->pressed ) {
		
		mdata->pressed = click;
		
		if ( click == true ) { mdata->input.mi.dwFlags |= MOUSEEVENTF_LEFTDOWN; }
		else { mdata->input.mi.dwFlags |= MOUSEEVENTF_LEFTUP; }
		
		sendevent = true;
		
	}
	
	if ( sendevent == true ) {
		
		SendInput( 1, & mdata->input, sizeof( mdata->input ) );
		
	}
	
	return true;
	
}

Mouse::Mouse() {
	
	mousedata * mdata = new mousedata;
	
	mdata->input.type = INPUT_MOUSE;
	mdata->input.mi.dx = 0;
	mdata->input.mi.dy = 0;
	mdata->input.mi.mouseData = 0;
	mdata->input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE;
	mdata->input.mi.time = 0;
	mdata->input.mi.dwExtraInfo = 0;
	
	mdata->pressed = false;
	
	this->data = mdata;
	
}
Mouse::~Mouse() {
	
	mousedata * mdata = ( mousedata * ) this->data;
	delete mdata;
	
}



struct pendata {
	
	HSYNTHETICPOINTERDEVICE device;
	POINTER_TYPE_INFO pointer;
	
};

bool Pen::Send( const UDrawData & data ) {
	
	pendata * pdata = ( pendata * ) this->data;
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) { pdata->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_INRANGE; }
	else { pdata->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_INRANGE; }
	
	if ( data.pressure >= UDRAW_CLICKPRESSURE ) { pdata->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_INCONTACT; }
	else { pdata->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_INCONTACT; }
	
	if ( data.click == true ) { pdata->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_FIRSTBUTTON; }
	else { pdata->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_FIRSTBUTTON; }
	
	if ( data.sideclick1 == true || data.sideclick2 == true ) { pdata->pointer.penInfo.pointerInfo.pointerFlags |= POINTER_FLAG_SECONDBUTTON; }
	else { pdata->pointer.penInfo.pointerInfo.pointerFlags &= ~POINTER_FLAG_SECONDBUTTON; }
	
	pdata->pointer.penInfo.pointerInfo.ptPixelLocation = { data.x, data.y };
	pdata->pointer.penInfo.pointerInfo.ptHimetricLocation = { data.x, data.y };
	pdata->pointer.penInfo.pointerInfo.ptPixelLocationRaw = { data.x, data.y };
	pdata->pointer.penInfo.pointerInfo.ptHimetricLocationRaw = { data.x, data.y };
	pdata->pointer.penInfo.pressure = data.pressure * 2;
	
	return InjectSyntheticPointerInput( pdata->device, & pdata->pointer, 1 );
	
}

Pen::Pen() {
	
	pendata * pdata = new pendata;
	
	pdata->device = CreateSyntheticPointerDevice( PT_PEN, 1, POINTER_FEEDBACK_DEFAULT );
	
	pdata->pointer.type = PT_PEN;
	memset( & pdata->pointer.penInfo, 0, sizeof( pdata->pointer.penInfo ) );
	pdata->pointer.penInfo.pointerInfo.pointerType = PT_PEN;
	pdata->pointer.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_NONE;
	pdata->pointer.penInfo.penMask = PEN_MASK_PRESSURE;
	
	this->data = pdata;
	
}
Pen::~Pen() {
	
	pendata * pdata = ( pendata * ) this->data;
	DestroySyntheticPointerDevice( pdata->device );
	delete pdata;
	
}



#else



#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <linux/uinput.h>



size_t emit( int fd, int type, int code, int value ) {
	
	input_event ev;
	ev.type = type;
	ev.code = code;
	ev.value = value;
	ev.time.tv_sec = 0;
	ev.time.tv_usec = 0;
	
	return write( fd, & ev, sizeof( input_event ) );
	
}



struct mousedata {
	
	bool pressed;
	int fd;
	
};

bool Mouse::Send( const UDrawData & data ) {
	
	mousedata * mdata = ( mousedata * ) this->data;
	if ( mdata == nullptr ) { return false; }
	
	bool sendevent = false;
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) {
		
		emit( mdata->fd, EV_ABS, ABS_X, data.x );
		emit( mdata->fd, EV_ABS, ABS_Y, data.y );
		sendevent = true;
		
	}
	
	bool click = data.pressure >= UDRAW_CLICKPRESSURE;
	if ( click != mdata->pressed ) {
		
		mdata->pressed = click;
		emit( mdata->fd, EV_KEY, BTN_LEFT, click == true ? 1 : 0 );
		sendevent = true;
		
	}
	
	if ( sendevent == true ) { emit( mdata->fd, EV_SYN, SYN_REPORT, 0 ); }
	
	return true;
	
}

Mouse::Mouse() {
	
	mousedata * mdata = new mousedata;
	mdata->pressed = false;
	
	mdata->fd = open( "/dev/uinput", O_WRONLY | O_NONBLOCK );
	if ( mdata->fd == -1 ) { printf( "Couldn't open mouse uinput\n" ); delete mdata; this->data = nullptr; return; }
	
	if ( ioctl( mdata->fd, UI_SET_EVBIT, EV_KEY ) == -1 ) { printf( "mouse ioctl UI_SET_EVBIT EV_KEY error\n" ); }
	if ( ioctl( mdata->fd, UI_SET_KEYBIT, BTN_LEFT ) == -1 ) { printf( "mouse ioctl UI_SET_KEYBIT BTN_LEFT error\n" ); }
	if ( ioctl( mdata->fd, UI_SET_PROPBIT, INPUT_PROP_POINTER ) == -1 ) { printf( "mouse ioctl UI_SET_PROPBIT INPUT_PROP_POINTER error\n" ); }
	if ( ioctl( mdata->fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT ) == -1 ) { printf( "mouse ioctl UI_SET_PROPBIT INPUT_PROP_DIRECT error\n" ); }
	if ( ioctl( mdata->fd, UI_SET_EVBIT, EV_ABS ) == -1 ) { printf( "mouse ioctl UI_SET_EVBIT EV_ABS error\n" ); }
	
	uinput_abs_setup abs;
	memset( & abs, 0, sizeof( uinput_abs_setup ) );
	//abs.absinfo.fuzz = 1;
	
	abs.code = ABS_X;
	abs.absinfo.maximum = 1920 * 2;
	if ( ioctl( mdata->fd, UI_ABS_SETUP, & abs ) == -1 ) { printf( "mouse ioctl UI_ABS_SETUP ABS_X error\n" ); }
	
	abs.code = ABS_Y;
	abs.absinfo.maximum = 1080;
	if ( ioctl( mdata->fd, UI_ABS_SETUP, & abs ) == -1 ) { printf( "mouse ioctl UI_ABS_SETUP ABS_Y error\n" ); }
	
	uinput_setup setup;
	memset( & setup, 0, sizeof( uinput_setup ) );
	setup.id.bustype = BUS_BLUETOOTH;
	setup.id.vendor = WIIMOTE_VENDORID;
	setup.id.product = WIIMOTE_PRODUCTID;
	strcpy( setup.name, WIIMOTE_NAME " uDraw mouse" );
	
	if ( ioctl( mdata->fd, UI_DEV_SETUP, & setup ) == -1 ) { printf( "mouse ioctl UI_DEV_SETUP error\n" ); }
	if ( ioctl( mdata->fd, UI_DEV_CREATE ) == -1 ) { printf( "mouse ioctl UI_DEV_CREATE error\n" ); }
	
	this->data = mdata;
	
}
Mouse::~Mouse() {
	
	mousedata * mdata = ( mousedata * ) this->data;
	if ( mdata == nullptr ) { return; }
	
	ioctl( mdata->fd, UI_DEV_DESTROY );
	close( mdata->fd );
	
	delete mdata;
	
}



struct pendata {
	
	int fd;
	bool active;
	bool pressed;
	bool side1;
	bool side2;
	int pressure;
	
};

bool Pen::Send( const UDrawData & data ) {
	
	pendata * pdata = ( pendata * ) this->data;
	if ( pdata == nullptr ) { return false; }
	
	bool sendevent = false;
	
	if ( data.x != WIIMOTE_UDRAW_INVALIDX && data.y != WIIMOTE_UDRAW_INVALIDY ) {
		
		emit( pdata->fd, EV_ABS, ABS_X, data.x );
		emit( pdata->fd, EV_ABS, ABS_Y, data.y );
		if ( pdata->active != true ) { emit( pdata->fd, EV_KEY, BTN_TOOL_PEN, 1 ); pdata->active = true; }
		sendevent = true;
		
	} else if ( pdata->active == true ) { emit( pdata->fd, EV_KEY, BTN_TOOL_PEN, 0 ); pdata->active = false; sendevent = true; }
	
	bool pressed = data.pressure >= UDRAW_CLICKPRESSURE;
	if ( pressed != pdata->pressed ) {
		
		pdata->pressed = pressed;
		emit( pdata->fd, EV_KEY, BTN_TOUCH, pressed == true ? 1 : 0 );
		sendevent = true;
		
	}
	
	bool side1 = data.sideclick1;
	if ( side1 != pdata->side1 ) {
		
		pdata->side1 = side1;
		emit( pdata->fd, EV_KEY, BTN_STYLUS, side1 == true ? 1 : 0 );
		sendevent = true;
		
	}
	
	bool side2 = data.sideclick2;
	if ( side2 != pdata->side2 ) {
		
		pdata->side2 = side2;
		emit( pdata->fd, EV_KEY, BTN_STYLUS2, side2 == true ? 1 : 0 );
		sendevent = true;
		
	}
	
	int pressure = data.pressure;
	if ( pressure != pdata->pressure ) {
		
		pdata->pressure = pressure;
		emit( pdata->fd, EV_ABS, ABS_PRESSURE, pressure );
		sendevent = true;
		
	}
	
	if ( sendevent == true ) { emit( pdata->fd, EV_SYN, SYN_REPORT, 0 ); }
	
	return true;
	
}

Pen::Pen() {
	
	pendata * pdata = new pendata;
	pdata->active = false;
	pdata->pressed = false;
	pdata->side1 = false;
	pdata->side2 = false;
	pdata->pressure = 0;
	
	pdata->fd = open( "/dev/uinput", O_WRONLY | O_NONBLOCK );
	if ( pdata->fd == -1 ) { printf( "Couldn't open pen uinput\n" ); delete pdata; this->data = nullptr; return; }
	
	if ( ioctl( pdata->fd, UI_SET_EVBIT, EV_KEY ) == -1 ) { printf( "pen ioctl UI_SET_EVBIT EV_KEY error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_KEYBIT, BTN_TOOL_PEN ) == -1 ) { printf( "pen ioctl UI_SET_KEYBIT BTN_TOOL_PEN error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_KEYBIT, BTN_TOUCH ) == -1 ) { printf( "pen ioctl UI_SET_KEYBIT BTN_TOUCH error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_KEYBIT, BTN_STYLUS ) == -1 ) { printf( "pen ioctl UI_SET_KEYBIT BTN_STYLUS error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_KEYBIT, BTN_STYLUS2 ) == -1 ) { printf( "pen ioctl UI_SET_KEYBIT BTN_STYLUS2 error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_PROPBIT, INPUT_PROP_POINTER ) == -1 ) { printf( "pen ioctl UI_SET_PROPBIT INPUT_PROP_POINTER error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT ) == -1 ) { printf( "pen ioctl UI_SET_PROPBIT INPUT_PROP_DIRECT error\n" ); }
	if ( ioctl( pdata->fd, UI_SET_EVBIT, EV_ABS ) == -1 ) { printf( "pen ioctl UI_SET_EVBIT EV_ABS error\n" ); }
	
	uinput_abs_setup abs;
	memset( & abs, 0, sizeof( uinput_abs_setup ) );
	abs.absinfo.resolution = 1;
	//abs.absinfo.fuzz = 1;
	
	abs.code = ABS_X;
	abs.absinfo.maximum = 1920 * 2;
	if ( ioctl( pdata->fd, UI_ABS_SETUP, & abs ) == -1 ) { printf( "pen ioctl UI_ABS_SETUP ABS_X error\n" ); }
	
	abs.code = ABS_Y;
	abs.absinfo.maximum = 1080;
	if ( ioctl( pdata->fd, UI_ABS_SETUP, & abs ) == -1 ) { printf( "pen ioctl UI_ABS_SETUP ABS_Y error\n" ); }
	
	abs.code = ABS_PRESSURE;
	abs.absinfo.maximum = 512;
	if ( ioctl( pdata->fd, UI_ABS_SETUP, & abs ) == -1 ) { printf( "pen ioctl UI_ABS_SETUP ABS_PRESSURE error\n" ); }
	
	uinput_setup setup;
	memset( & setup, 0, sizeof( uinput_setup ) );
	setup.id.bustype = BUS_BLUETOOTH;
	setup.id.vendor = WIIMOTE_VENDORID;
	setup.id.product = WIIMOTE_PRODUCTID;
	strcpy( setup.name, WIIMOTE_NAME " uDraw pen" );
	
	if ( ioctl( pdata->fd, UI_DEV_SETUP, & setup ) == -1 ) { printf( "pen ioctl UI_DEV_SETUP error\n" ); }
	if ( ioctl( pdata->fd, UI_DEV_CREATE ) == -1 ) { printf( "pen ioctl UI_DEV_CREATE error\n" ); }
	
	this->data = pdata;
	
}
Pen::~Pen() {
	
	pendata * pdata = ( pendata * ) this->data;
	if ( pdata == nullptr ) { return; }
	
	ioctl( pdata->fd, UI_DEV_DESTROY );
	close( pdata->fd );
	
	delete pdata;
	
}



#endif
