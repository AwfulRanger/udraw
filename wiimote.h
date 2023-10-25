#ifndef __WIIMOTE__H__
#define __WIIMOTE__H__



#define WIIMOTE_VENDORID 0x057E
#define WIIMOTE_PRODUCTID 0x0306

#define WIIMOTE_BUFFERSIZE 0x20

#define WIIMOTE_BUT_TWO 0x0001
#define WIIMOTE_BUT_ONE 0x0002
#define WIIMOTE_BUT_B 0x0004
#define WIIMOTE_BUT_A 0x0008
#define WIIMOTE_BUT_MINUS 0x0010
#define WIIMOTE_BUT_HOME 0x0080
#define WIIMOTE_BUT_LEFT 0x0100
#define WIIMOTE_BUT_RIGHT 0x0200
#define WIIMOTE_BUT_DOWN 0x0400
#define WIIMOTE_BUT_UP 0x0800
#define WIIMOTE_BUT_PLUS 0x1000

#define WIIMOTE_LED_NONE 0x00
#define WIIMOTE_LED_ONE 0x01
#define WIIMOTE_LED_TWO 0x02
#define WIIMOTE_LED_THREE 0x04
#define WIIMOTE_LED_FOUR 0x08

#define WIIMOTE_EXT_UDRAW 0xFF00A4200112

#define WIIMOTE_UDRAW_INVALIDX 2018
#define WIIMOTE_UDRAW_INVALIDY -480

#define WIIMOTE_UDRAW_SIDECLICK1 0x01
#define WIIMOTE_UDRAW_SIDECLICK2 0x02
#define WIIMOTE_UDRAW_CLICK 0x04



struct UDrawData {
	
	int x;
	int y;
	bool click;
	bool sideclick1;
	bool sideclick2;
	int pressure;
	unsigned int buttons;
	
};



class Wiimote {
	
	public:
		
		// Find a Wiimote handle
		static void * GetWiimoteHandle();
		
	protected:
		
		void * handle;
		unsigned char buf[ WIIMOTE_BUFFERSIZE ];
		bool rumble;
		unsigned char led;
		
	public:
		
		// Write
		bool WriteData( void * data, size_t size );
		
		// Read
		bool ReadData( void * data, size_t size );
		
		// Rumble
		void SetRumble( bool enabled );
		bool GetRumble();
		
		// LED
		void SetLED( unsigned char led );
		unsigned char GetLED();
		
		// Extension
		void InitExtension();
		unsigned long long GetExtensionType();
		
		// uDraw
		bool InitUDraw();
		void PollUDraw( UDrawData * data );
		
		// Construction
		Wiimote( void * handle );
		
};



#endif
