#include "wiimote.h"

#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>



// Find a Wiimote handle

void * Wiimote::GetWiimoteHandle() {
	
	GUID guid = GUID();
	HidD_GetHidGuid( & guid );
	
	HDEVINFO devinfo = SetupDiGetClassDevs( & guid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
	
	SP_DEVICE_INTERFACE_DATA data;
	data.cbSize = sizeof( data );
	
	HIDD_ATTRIBUTES attr;
	attr.Size = sizeof( attr );
	
	DWORD index = 0;
	DWORD size = 0;
	
	HANDLE handle = INVALID_HANDLE_VALUE;
	
	while ( handle == INVALID_HANDLE_VALUE ) {
		
		bool err1 = SetupDiEnumDeviceInterfaces( devinfo, nullptr, & guid, index, & data );
		if ( err1 != true ) { break; }
		
		// Get buffer size
		bool err2 = SetupDiGetDeviceInterfaceDetail( devinfo, & data, nullptr, 0, & size, nullptr );
		if ( err2 != true && GetLastError() != ERROR_INSUFFICIENT_BUFFER ) { continue; }
		
		// Get device path
		char * detail_char = new char[ size ];
		PSP_DEVICE_INTERFACE_DETAIL_DATA detail = ( PSP_DEVICE_INTERFACE_DETAIL_DATA ) detail_char;
		detail->cbSize = sizeof( SP_DEVICE_INTERFACE_DETAIL_DATA );
		
		bool err3 = SetupDiGetDeviceInterfaceDetail( devinfo, & data, detail, size, nullptr, nullptr );
		if ( err3 == true ) {
			
			HANDLE hand = CreateFile( detail->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr );
			if ( hand != INVALID_HANDLE_VALUE ) {
				
				HidD_GetAttributes( hand, & attr );
				
				if ( attr.VendorID == WIIMOTE_VENDORID && attr.ProductID == WIIMOTE_PRODUCTID ) { handle = hand; }
				else { CloseHandle( hand ); }
				
			}
			
		}
		
		delete[] detail_char;
		
		index++;
		
	}
	
	SetupDiDestroyDeviceInfoList( devinfo );
	
	return handle;
	
}



// Write

bool Wiimote::WriteData( void * data, size_t size ) {
	
	if ( this->handle == nullptr ) { return false; }
	
	return HidD_SetOutputReport( this->handle, data, ( ULONG ) size );
	
}



// Read

bool Wiimote::ReadData( void * data, size_t size ) {
	
	if ( this->handle == nullptr ) { return false; }
	
	return HidD_GetInputReport( this->handle, data, ( ULONG ) size );
	
}



// Rumble

void Wiimote::SetRumble( bool enabled ) {
	
	if ( this->rumble == enabled ) { return; }
	
	this->rumble = enabled;
	
	this->buf[ 0 ] = 0x10;
	this->buf[ 1 ] = this->rumble;
	this->WriteData( this->buf, 2 );
	
}
bool Wiimote::GetRumble() { return this->rumble; }



// LED

void Wiimote::SetLED( unsigned char led ) {
	
	if ( this->led == led ) { return; }
	
	this->led = led;
	
	this->buf[ 0 ] = 0x11;
	this->buf[ 1 ] = ( this->led << 4 ) | ( int ) this->rumble;
	this->WriteData( this->buf, 2 );
	
}
unsigned char Wiimote::GetLED() { return this->led; }



// Extension

void Wiimote::InitExtension() {
	
	this->buf[ 0 ] = 0x16;
	this->buf[ 1 ] = 0x04 | ( int ) this->rumble;
	this->buf[ 2 ] = 0xA4; // addr 1
	this->buf[ 3 ] = 0x00; // addr 2
	this->buf[ 4 ] = 0xF0; // addr 3
	this->buf[ 5 ] = 0x01; // size
	this->buf[ 6 ] = 0x55; // data
	this->WriteData( this->buf, 22 );
	this->buf[ 4 ] = 0xFB; // addr 3
	this->buf[ 6 ] = 0x00; // data
	this->WriteData( this->buf, 22 );
	
}
unsigned long long Wiimote::GetExtensionType() {
	
	this->buf[ 0 ] = 0x17;
	this->buf[ 1 ] = 0x04 | ( int ) this->rumble;
	this->buf[ 2 ] = 0xA4; // addr 1
	this->buf[ 3 ] = 0x00; // addr 2
	this->buf[ 4 ] = 0xFA; // addr 3
	this->buf[ 5 ] = 0x00; // size 1
	this->buf[ 6 ] = 0x06; // size 2
	this->WriteData( this->buf, 7 );
	
	this->buf[ 0 ] = 0x21;
	this->ReadData( buf, 22 );
	
	unsigned long long ext = ( unsigned long long ) this->buf[ 6 ] << 40;
	ext |= ( unsigned long long ) this->buf[ 7 ] << 32;
	ext |= ( unsigned long long ) this->buf[ 8 ] << 24;
	ext |= ( unsigned long long ) this->buf[ 9 ] << 16;
	ext |= ( unsigned long long ) this->buf[ 10 ] << 8;
	ext |= ( unsigned long long ) this->buf[ 11 ];
	
	return ext;
	
}



// uDraw

bool Wiimote::InitUDraw() {
	
	this->InitExtension();
	Sleep( 1000 ); // Wait a bit just to be sure
	if ( this->GetExtensionType() != WIIMOTE_EXT_UDRAW ) { return false; }
	
	this->buf[ 0 ] = 0x12;
	this->buf[ 1 ] = ( int ) this->rumble;
	this->buf[ 2 ] = 0x32;
	this->WriteData( this->buf, 3 ); // Set report mode
	
	return true;
	
}
void Wiimote::PollUDraw( UDrawData * data ) {
	
	this->buf[ 0 ] = 0x32;
	this->ReadData( buf, 11 );
	
	data->x = ( int ) ( ( ( float ) ( this->buf[ 3 ] + ( 0xFF * ( this->buf[ 5 ] & 0x07 ) ) - 80 ) / 1864 ) * 1920 );
	data->y = 1080 - ( int ) ( ( ( float ) ( this->buf[ 4 ] + ( 0xFF * ( ( this->buf[ 5 ] >> 4 ) & 0x07 ) ) - 90 ) / 1350 ) * 1080 );
	
	data->click = buf[ 8 ] & WIIMOTE_UDRAW_CLICK;
	data->sideclick1 = ~buf[ 8 ] & WIIMOTE_UDRAW_SIDECLICK1;
	data->sideclick2 = ~buf[ 8 ] & WIIMOTE_UDRAW_SIDECLICK2;
	
	data->pressure = buf[ 6 ];
	if ( data->click == true ) { data->pressure += 0xFF; }
	
	data->buttons = this->buf[ 1 ] << 8;
	data->buttons |= this->buf[ 2 ];
	
}



// Construction

Wiimote::Wiimote( void * handle ) {
	
	this->handle = handle;
	memset( this->buf, 0, sizeof( this->buf ) );
	this->rumble = false;
	this->led = 0;
	
}
