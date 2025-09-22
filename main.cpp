#include "wiimote.h"
#include "input.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif



#define MINX 85
#define MINY 10
#define MAXX 1960
#define MAXY 1370
#define CURSORX 0
#define CURSORY 0
#define CURSORW 1920
#define CURSORH 1080



#ifdef _WIN32
#define CLOCKS CLOCKS_PER_SEC
#else
// this probably isn't a good idea but CLOCKS_PER_SEC isn't accurate anyway so whatever
#define CLOCKS 5000
#endif



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

#define WRITEKV( color, key ) color ANSI_NORMAL key ":" ANSI_BOLD "%i "
#define WRITEKVSTR( color, key ) color ANSI_NORMAL key ":" ANSI_BOLD "%s "
#define WRITEKVPOS( x, y, color, key ) color ANSI_SETPOS( x, y ) ANSI_NORMAL key ":" ANSI_BOLD "%i"
#define WRITEKVPOSSTR( x, y, color, key ) color ANSI_SETPOS( x, y ) ANSI_NORMAL key ":" ANSI_BOLD "%s"



int hasarg( const char * arg, char ** args, int argc ) {
	
	for ( int i = 1; i < argc; i++ ) {
		
		if ( strcmp( args[ i ], arg ) == 0 ) { return i; }
		
	}
	
	return 0;
	
}

int getargindex( int arg, int def, char ** args, int argc ) {
	
	if ( arg == 0 || arg >= argc - 1 ) { return def; }
	
	return atoi( args[ arg + 1 ] );
	
}

int getarg( const char * arg, int def, char ** args, int argc ) {
	
	return getargindex( hasarg( arg, args, argc ), def, args, argc );
	
}

void printhelp( const char * name ) {
	
	printf( "%s: Link a Wiimote with a uDraw tablet extension to use as a mouse or pen\n", name );
	printf( "\nargs:\n\n" );
	printf( "\t-help || --help || /? \t: Print this message\n" );
	printf( "\t-mouse \t: Enable mouse input instead of pen input by default\n" );
	printf( "\t-rumble \t: Enable rumble by default\n" );
	printf( "\t-noinfo \t: Hide tablet info\n" );
	printf( "\t-minx (int) \t: Set the position of the tablet's left side (default %i)\n", MINX );
	printf( "\t-miny (int) \t: Set the position of the tablet's top side (default %i)\n", MINY );
	printf( "\t-maxx (int) \t: Set the position of the tablet's right side (default %i)\n", MAXX );
	printf( "\t-maxy (int) \t: Set the position of the tablet's bottom side (default %i)\n", MAXY );
	printf( "\t-cursorx (int) \t: Set the x offset of the cursor's bounds (default %i)\n", CURSORX );
	printf( "\t-cursory (int) \t: Set the y offset of the cursor's bounds (default %i)\n", CURSORY );
	printf( "\t-cursorw (int) \t: Set the width of the cursor's bounds (default %i)\n", CURSORW );
	printf( "\t-cursorh (int) \t: Set the height of the cursor's bounds (default %i)\n", CURSORH );
	
}

void wait( unsigned int sec ) {
	
	#ifdef _WIN32
	Sleep( 1000 * sec );
	#else
	sleep( sec );
	#endif
	
}

int main( int argc, char ** args ) {
	
	if ( hasarg( "-help", args, argc ) != 0 || hasarg( "--help", args, argc ) != 0 || hasarg( "/?", args, argc ) != 0 ) {
		
		printhelp( argc > 0 ? args[ 0 ] : "udraw" );
		
		return 0;
		
	}
	
	bool found = false;
	void * handle;
	for ( int i = 0; i < 5; i++ ) {
		
		handle = Wiimote::GetWiimoteHandle( & found );
		if ( found == true ) { break; }
		
		wait( 1 );
		
	}
	if ( found != true ) { printf( "Couldn't find wiimote handle\n" ); return 0; }
	
	Wiimote * wiimote = new Wiimote( handle );
	
	bool initudraw = false;
	for ( int i = 0; i < 5; i++ ) {
		
		initudraw = wiimote->InitUDraw();
		if ( initudraw == true ) { break; }
		
		wait( 1 );
		
	}
	
	if ( initudraw == true ) {
		
		unsigned int lastbuttons = 0;
		bool usepen = hasarg( "-mouse", args, argc ) == 0;
		bool userumble = hasarg( "-rumble", args, argc ) != 0;
		bool calibrate = false;
		
		bool showinfo = hasarg( "-noinfo", args, argc ) == 0;
		
		int minx = getarg( "-minx", MINX, args, argc );
		int miny = getarg( "-miny", MINY, args, argc );
		int maxx = getarg( "-maxx", MAXX, args, argc );
		int maxy = getarg( "-maxy", MAXY, args, argc );
		int cursorx = getarg( "-cursorx", CURSORX, args, argc );
		int cursory = getarg( "-cursory", CURSORY, args, argc );
		int cursorw = getarg( "-cursorw", CURSORW, args, argc );
		if ( cursorw <= 0 ) { cursorw = CURSORW; }
		int cursorh = getarg( "-cursorh", CURSORH, args, argc );
		if ( cursorh <= 0 ) { cursorh = CURSORH; }
		
		Mouse * mouse = nullptr;
		Pen * pen = nullptr;
		
		if ( usepen == true ) { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_THREE ); }
		else { wiimote->SetLED( WIIMOTE_LED_ONE | WIIMOTE_LED_FOUR ); }
		
		#ifdef _WIN32
		printf( ANSI_SETPOS( 1, 6 ) "uDraw initialized\n" );
		#else
		printf( "uDraw initialized\n" );
		#endif
		printf( "Press up on the dpad to switch between mouse mode and pen mode\n" );
		printf( "Press down on the dpad to toggle rumble\n" );
		printf( "Press left on the dpad to toggle pen bounds calibration mode (move the pen along the edges to set bounds)\n" );
		printf( "Press the home button to exit\n" );
		
		int rate = 0;
		clock_t lastclock = clock();
		int lastrate = 0;
		
		int lastx = -1;
		int lasty = -1;
		
		UDrawData data;
		memset( & data, 0, sizeof( UDrawData ) );

		while ( true ) {
			
			wiimote->PollUDraw( & data );
			
			int curx = data.x;
			int cury = data.y;
			
			if ( calibrate == true ) {
				
				if ( curx != WIIMOTE_UDRAW_INVALIDX ) {
					
					if ( curx < minx ) { minx = curx; }
					if ( curx > maxx ) { maxx = curx; }
					
				}
				if ( cury != WIIMOTE_UDRAW_INVALIDY ) {
					
					if ( cury < miny ) { miny = cury; }
					if ( cury > maxy ) { maxy = cury; }
					
				}
				
			}
			
			if ( curx != WIIMOTE_UDRAW_INVALIDX ) {
				
				if ( maxx != minx ) {
					
					data.x = ( int ) ( cursorw * ( ( float ) ( data.x - minx ) / ( maxx - minx ) ) );
					
				} else { data.x -= minx; }
				data.x += cursorx;
				
			}
			if ( cury != WIIMOTE_UDRAW_INVALIDY ) {
				
				if ( maxy != miny ) {
					
					data.y = ( int ) ( cursorh * ( ( float ) ( data.y - miny ) / ( maxy - miny ) ) );
					
				} else { data.y -= miny; }
				data.y += cursory;
				
			}
			
			if ( usepen != true ) {
				
				if ( mouse == nullptr ) { mouse = new Mouse; }
				mouse->Send( data );
				
			} else {
				
				if ( pen == nullptr ) { pen = new Pen; }
				pen->Send( data );
				
			}
			
			if ( showinfo == true ) {
				
				#ifdef _WIN32
				printf(
					
					ANSI_SAVECURSOR
					ANSI_BG
					ANSI_SETPOS( 1, 1 ) ANSI_ERASELINE "\n" ANSI_ERASELINE "\n" ANSI_ERASELINE "\n" ANSI_ERASELINE

				);
				
				if ( curx == WIIMOTE_UDRAW_INVALIDX ) { printf( WRITEKVPOSSTR( 1, 1, ANSI_RED, "x" ), "none" ); }
				else { printf( WRITEKVPOS( 1, 1, ANSI_RED, "x" ), data.x ); }
				
				if ( cury == WIIMOTE_UDRAW_INVALIDY ) { printf( WRITEKVPOSSTR( 8, 1, ANSI_GREEN, "y" ), "none" ); }
				else { printf( WRITEKVPOS( 8, 1, ANSI_GREEN, "y" ), data.y ); }
				
				printf(
					
					WRITEKVPOS( 15, 1, ANSI_CYAN, "press" )
					WRITEKVPOS( 25, 1, ANSI_YELLOW, "rate" )
					WRITEKVPOS( 1, 2, ANSI_YELLOW, "click" )
					WRITEKVPOS( 9, 2, ANSI_YELLOW, "side1" )
					WRITEKVPOS( 17, 2, ANSI_YELLOW, "side2" )
					WRITEKVPOS( 25, 2, ANSI_YELLOW, "input" )
					WRITEKVPOS( 1, 3, ANSI_CYAN, "pen" )
					WRITEKVPOS( 7, 3, ANSI_CYAN, "rumble" )
					ANSI_SETPOS( 1, 4 ) ANSI_NORMAL ANSI_BORDER "----------------------------------"
					ANSI_RESET ANSI_RESTORECURSOR 
					
				, data.pressure, lastrate, data.click, data.sideclick1, data.sideclick2, data.buttons, usepen, userumble );
				#else
				printf( ANSI_BG ANSI_ERASELINE "\t" );
				
				if ( curx == WIIMOTE_UDRAW_INVALIDX ) { printf( WRITEKVSTR( ANSI_RED, "x" ), "none" ); }
				else { printf( WRITEKV( ANSI_RED, "x" ), data.x ); }
				
				if ( cury == WIIMOTE_UDRAW_INVALIDY ) { printf( WRITEKVSTR( ANSI_GREEN, "y" ), "none" ); }
				else { printf( WRITEKV( ANSI_GREEN, "y" ), data.y ); }
				
				printf(
					
					WRITEKV( ANSI_CYAN, "press" )
					WRITEKV( ANSI_YELLOW, "rate" )
					WRITEKV( ANSI_YELLOW, "click" )
					WRITEKV( ANSI_YELLOW, "side1" )
					WRITEKV( ANSI_YELLOW, "side2" )
					WRITEKV( ANSI_YELLOW, "input" )
					WRITEKV( ANSI_CYAN, "pen" )
					WRITEKV( ANSI_CYAN, "rumble" )
					ANSI_RESET
					"\r"
					
				, data.pressure, lastrate, data.click, data.sideclick1, data.sideclick2, data.buttons, usepen, userumble );
				#endif
				fflush( stdout );
				
			}
			
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
			if ( data.buttons & WIIMOTE_BUT_LEFT && !( lastbuttons & WIIMOTE_BUT_LEFT ) ) {
				
				calibrate = !calibrate;
				if ( calibrate == true ) {
					
					minx = 9999;
					miny = 9999;
					maxx = -9999;
					maxy = -9999;
					wiimote->SetLED( wiimote->GetLED() | WIIMOTE_LED_TWO );
					
				}
				else {
					
					printf( ANSI_ERASELINE "-minx %i -miny %i -maxx %i -maxy %i\n", minx, miny, maxx, maxy );
					wiimote->SetLED( wiimote->GetLED() & ~WIIMOTE_LED_TWO );
					
				}
				
			}
			if ( data.buttons & WIIMOTE_BUT_HOME ) { printf( "\nHome pressed, exiting\n" ); break; }
			
			wiimote->SetRumble( userumble == true && data.pressure > 16 && ( data.pressure / 18 ) + abs( curx - lastx ) + abs( cury - lasty ) > 32 );
			
			lastbuttons = data.buttons;
			lastx = curx;
			lasty = cury;
			
			rate++;
			
			clock_t curclock = clock();
			if ( curclock > lastclock + CLOCKS ) {
				
				lastclock = curclock;
				lastrate = rate;
				rate = 0;
				
			}
			
		}
		
		if ( mouse != nullptr ) { delete mouse; }
		if ( pen != nullptr ) { delete pen; }
		
	} else { printf( "Couldn't initialize uDraw\n" ); }
	
	delete wiimote;
	
	Wiimote::CloseWiimoteHandle( handle );
	
	return 0;
	
}
