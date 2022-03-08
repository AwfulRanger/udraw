#include "wiimote.h"
#include "input.h"

#include <cstdio>



bool hasarg( const char * arg, char ** args, int argc ) {
	
	for ( int i = 1; i < argc; i++ ) {
		
		if ( strcmp( args[ i ], arg ) == 0 ) { return true; }
		
	}
	
	return false;
	
}

void printhelp( const char * name ) {
	
	printf( "%s: Link a Wiimote with a uDraw tablet extension to use as a mouse or pen\n", name );
	printf( "\nargs:\n\n" );
	printf( "\t-help || --help || /? \t: Print this message\n" );
	printf( "\t-mouse \t: Enable mouse input instead of pen input by default\n" );
	
}

int main( int argc, char ** args ) {
	
	if ( hasarg( "-help", args, argc ) == true || hasarg( "--help", args, argc ) == true || hasarg( "/?", args, argc ) == true ) {
		
		printhelp( argc > 0 ? args[ 0 ] : "udraw" );
		
		return 0;
		
	}
	
	HANDLE handle = Wiimote::GetWiimoteHandle();
	if ( handle == INVALID_HANDLE_VALUE ) { printf( "Couldn't find wiimote handle\n" ); return 0; }
	
	printf( "Found wiimote handle %p\n", handle );
	
	Wiimote * wiimote = new Wiimote( handle );
	
	if ( wiimote->InitUDraw() == true ) {
		
		unsigned int lastbuttons = 0;
		bool usepen = !hasarg( "-mouse", args, argc );
		
		Mouse * mouse = nullptr;
		Pen * pen = nullptr;
		
		if ( usepen == true ) { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_THREE ); }
		else { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_FOUR ); }
		
		printf( "uDraw initialized\n" );
		printf( "Press up on the dpad to switch between mouse mode and pen mode\n" );
		printf( "Press the home button to exit\n" );
		
		while ( true ) {
			
			UDrawData data = wiimote->PollUDraw();
			
			if ( usepen != true ) {
				
				if ( mouse == nullptr ) { mouse = new Mouse; }
				mouse->Send( data );
				
			} else {
				
				if ( pen == nullptr ) { pen = new Pen; }
				pen->Send( data );
				
			}
			
			printf( "%i %i %i %i %i %i %u %i    \r", data.x, data.y, data.pressure, data.click, data.sideclick1, data.sideclick2, data.buttons, usepen );
			
			if ( data.buttons & WIIMOTE_BUT_UP && !( lastbuttons & WIIMOTE_BUT_UP ) ) {
				
				usepen = !usepen;
				if ( usepen == true ) { wiimote->SetLED( ( wiimote->GetLED() & ~WIIMOTE_LED_FOUR ) | WIIMOTE_LED_THREE ); }
				else { wiimote->SetLED( ( wiimote->GetLED() & ~WIIMOTE_LED_THREE ) | WIIMOTE_LED_FOUR ); }
				
			}
			if ( data.buttons & WIIMOTE_BUT_HOME ) { printf( "\nHome pressed, exiting\n" ); break; }
			
			lastbuttons = data.buttons;
			
		}
		
		if ( mouse != nullptr ) { delete mouse; }
		if ( pen != nullptr ) { delete pen; }
		
	} else { printf( "Couldn't initialize uDraw\n" ); }
	
	delete wiimote;
	
	CloseHandle( handle );
	
	return 0;
	
}
