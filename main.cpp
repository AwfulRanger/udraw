#include "wiimote.h"
#include "input.h"

#include <cstdio>
#include <time.h>



#define ANSI_ESC "\033"

#define ANSI_SAVECURSOR ANSI_ESC "7"
#define ANSI_RESTORECURSOR ANSI_ESC "8"
#define ANSI_ERASELINE ANSI_ESC "[K"
#define ANSI_SETPOS( x, y ) ANSI_ESC "[" #y ";" #x "H"

#define ANSI_RESET ANSI_ESC "[0m"
#define ANSI_BGCOLOR "40"
//#define ANSI_BG ANSI_ESC "[48;5;236m"
//#define ANSI_BG ANSI_ESC "[;" ANSI_BGCOLOR "m"
#define ANSI_BG ""
#define ANSI_BLACK ANSI_ESC "[30m"
#define ANSI_RED ANSI_ESC "[31m"
#define ANSI_GREEN ANSI_ESC "[32m"
#define ANSI_YELLOW ANSI_ESC "[33m"
#define ANSI_BLUE ANSI_ESC "[34m"
#define ANSI_MAGENTA ANSI_ESC "[35m"
#define ANSI_CYAN ANSI_ESC "[36m"
#define ANSI_WHITE ANSI_ESC "[37m"
#define ANSI_BORDER ANSI_BLUE

#define ANSI_NORMAL ANSI_ESC "[22m"
#define ANSI_BOLD ANSI_ESC "[1m"
#define ANSI_FAINT ANSI_ESC "[2m"

#define WRITEKV( x, y, color, key ) color ANSI_SETPOS( x, y ) ANSI_NORMAL key ":" ANSI_BOLD "%i"



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
	printf( "\t-rumble \t: Enable rumble by default\n" );
	
}

int main( int argc, char ** args ) {
	
	if ( hasarg( "-help", args, argc ) == true || hasarg( "--help", args, argc ) == true || hasarg( "/?", args, argc ) == true ) {
		
		printhelp( argc > 0 ? args[ 0 ] : "udraw" );
		
		return 0;
		
	}
	
	HANDLE handle = Wiimote::GetWiimoteHandle();
	if ( handle == INVALID_HANDLE_VALUE ) { printf( "Couldn't find wiimote handle\n" ); return 0; }
	
	Wiimote * wiimote = new Wiimote( handle );
	
	if ( wiimote->InitUDraw() == true ) {
		
		unsigned int lastbuttons = 0;
		bool usepen = !hasarg( "-mouse", args, argc );
		bool userumble = hasarg( "-rumble", args, argc );
		
		Mouse * mouse = nullptr;
		Pen * pen = nullptr;
		
		if ( usepen == true ) { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_THREE ); }
		else { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_FOUR ); }
		
		printf( ANSI_SETPOS( 1, 6 ) "uDraw initialized\n" );
		printf( "Press up on the dpad to switch between mouse mode and pen mode\n" );
		printf( "Press down on the dpad to toggle rumble\n" );
		printf( "Press the home button to exit\n" );
		
		int rate = 0;
		clock_t lastclock = clock();
		int lastrate = 0;
		
		int lastx = -1;
		int lasty = -1;
		
		UDrawData data;

		while ( true ) {
			
			wiimote->PollUDraw( & data );
			
			if ( usepen != true ) {
				
				if ( mouse == nullptr ) { mouse = new Mouse; }
				mouse->Send( data );
				
			} else {
				
				if ( pen == nullptr ) { pen = new Pen; }
				pen->Send( data );
				
			}
			
			printf(
				
				ANSI_SAVECURSOR
				ANSI_BG
				ANSI_SETPOS( 1, 1 ) ANSI_ERASELINE "\n" ANSI_ERASELINE "\n" ANSI_ERASELINE "\n" ANSI_ERASELINE
				WRITEKV( 1, 1, ANSI_RED, "x" )
				WRITEKV( 8, 1, ANSI_GREEN, "y" )
				WRITEKV( 15, 1, ANSI_CYAN, "press" )
				WRITEKV( 25, 1, ANSI_YELLOW, "rate" )
				WRITEKV( 1, 2, ANSI_YELLOW, "click" )
				WRITEKV( 9, 2, ANSI_YELLOW, "side1" )
				WRITEKV( 17, 2, ANSI_YELLOW, "side2" )
				WRITEKV( 25, 2, ANSI_YELLOW, "input" )
				WRITEKV( 1, 3, ANSI_CYAN, "pen" )
				WRITEKV( 7, 3, ANSI_CYAN, "rumble" )
				ANSI_SETPOS( 1, 4 ) ANSI_NORMAL ANSI_BORDER "----------------------------------"
				ANSI_RESET ANSI_RESTORECURSOR 
				
			, data.x, data.y, data.pressure, lastrate, data.click, data.sideclick1, data.sideclick2, data.buttons, usepen, userumble );
			
			if ( data.buttons & WIIMOTE_BUT_UP && !( lastbuttons & WIIMOTE_BUT_UP ) ) {
				
				usepen = !usepen;
				if ( usepen == true ) { wiimote->SetLED( wiimote->GetLED() | WIIMOTE_LED_THREE ); }
				else { wiimote->SetLED( wiimote->GetLED() & ~WIIMOTE_LED_THREE ); }
				
			}
			if ( data.buttons & WIIMOTE_BUT_DOWN && !( lastbuttons & WIIMOTE_BUT_DOWN ) ) {
				
				userumble = !userumble;
				if ( userumble == true ) { wiimote->SetLED( wiimote->GetLED() | WIIMOTE_LED_FOUR ); }
				else { wiimote->SetLED( wiimote->GetLED() & ~WIIMOTE_LED_FOUR ); }
				
			}
			if ( data.buttons & WIIMOTE_BUT_HOME ) { printf( "\nHome pressed, exiting\n" ); break; }
			
			wiimote->SetRumble( userumble == true && data.pressure > 16 && ( data.pressure / 18 ) + abs( data.x - lastx ) + abs( data.y - lasty ) > 32 );
			
			lastbuttons = data.buttons;
			lastx = data.x;
			lasty = data.y;
			
			rate++;
			
			clock_t curclock = clock();
			if ( curclock > lastclock + CLOCKS_PER_SEC ) {
				
				lastclock = curclock;
				lastrate = rate;
				rate = 0;
				
			}
			
		}
		
		if ( mouse != nullptr ) { delete mouse; }
		if ( pen != nullptr ) { delete pen; }
		
	} else { printf( "Couldn't initialize uDraw\n" ); }
	
	delete wiimote;
	
	CloseHandle( handle );
	
	return 0;
	
}
