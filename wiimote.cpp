#include "wiimote.h"

#include <cstring>



#ifdef _WIN32



#include <Windows.h>
#include <SetupAPI.h>
#include <hidsdi.h>



// Find a Wiimote handle

void * Wiimote::GetWiimoteHandle( bool * found ) {
	
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
				
				if ( attr.VendorID == WIIMOTE_VENDORID && attr.ProductID == WIIMOTE_PRODUCTID ) { handle = hand; break; }
				else { CloseHandle( hand ); }
				
			}
			
		}
		
		delete[] detail_char;
		
		index++;
		
	}
	
	SetupDiDestroyDeviceInfoList( devinfo );
	
	* found = handle != INVALID_HANDLE_VALUE;
	
	return handle;
	
}
void Wiimote::CloseWiimoteHandle( void * handle ) { CloseHandle( handle ); }



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



#else



#include <cstdio>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/l2cap.h>



// Find a Wiimote handle

struct handledata {
	
	bdaddr_t btaddr;
	int controlsock;
	int datasock;
	
};

void * Wiimote::GetWiimoteHandle( bool * found ) {
	
	* found = false;
	
	int btdev = hci_get_route( nullptr );
	if ( btdev < 0 ) { printf( "Couldn't find bluetooth\n" ); return nullptr; }
	
	int btsock = hci_open_dev( btdev );
	if ( btsock < 0 ) { printf( "Couldn't open bluetooth\n" ); return nullptr; }
	
	const int maxinfo = 0xFF;
	inquiry_info info[ maxinfo ];
	inquiry_info * infop = info;
	memset( info, 0, maxinfo * sizeof( inquiry_info ) );
	int numinfo = hci_inquiry( btdev, 1, maxinfo, nullptr, & infop, IREQ_CACHE_FLUSH );
	
	char name[ 250 ];
	
	handledata * handle = nullptr;
	
	// Find the first device with the correct class and name
	// Retrieving the name can take a little bit, so make sure the class matches first to save time
	for ( int i = 0; i < numinfo; i++ ) {
		
		unsigned long devclass = ( unsigned long ) info[ i ].dev_class[ 0 ];
		devclass |= ( unsigned long ) info[ i ].dev_class[ 1 ] << 8;
		devclass |= ( unsigned long ) info[ i ].dev_class[ 2 ] << 16;
		
		if ( devclass != WIIMOTE_CLASS ) { continue; }
		
		memset( name, 0, sizeof( name ) );
		hci_read_remote_name( btsock, & info[ i ].bdaddr, sizeof( name ), name, 0 );
		
		if ( strcmp( name, WIIMOTE_NAME ) != 0 ) { continue; }
		
		handle = new handledata;
		bacpy( & handle->btaddr, & info[ i ].bdaddr );
		handle->controlsock = -1;
		handle->datasock = -1;
		
	}
	
	hci_close_dev( btsock );
	
	if ( handle == nullptr ) { return nullptr; }
	
	sockaddr_l2 addr;
	memset( & addr, 0, sizeof( sockaddr_l2 ) );
	addr.l2_family = AF_BLUETOOTH;
	bacpy( & addr.l2_bdaddr, & handle->btaddr );
	
	// Setup control socket
	addr.l2_psm = htobs( WIIMOTE_PSM_CONTROL );
	handle->controlsock = socket( AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP );
	if ( handle->controlsock == -1 ) { printf( "Couldn't open control socket\n" ); delete handle; return nullptr; }
	if ( connect( handle->controlsock, ( sockaddr * ) & addr, sizeof( addr ) ) < 0 ) {
		
		printf( "Couldn't connect to control socket\n" );
		close( handle->controlsock );
		delete handle;
		return nullptr;
		
	}
	
	// Setup data socket
	addr.l2_psm = htobs( WIIMOTE_PSM_DATA );
	handle->datasock = socket( AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP );
	if ( handle->datasock == -1 ) { printf( "Couldn't open data socket\n" ); close( handle->controlsock ); delete handle; return nullptr; }
	if ( connect( handle->datasock, ( sockaddr * ) & addr, sizeof( addr ) ) < 0 ) {
		
		printf( "Couldn't connect to data socket\n" );
		close( handle->controlsock );
		close( handle->datasock );
		delete handle;
		return nullptr;
		
	}
	
	* found = true;
	return handle;
	
}
void Wiimote::CloseWiimoteHandle( void * handle ) {
	
	handledata * data = ( handledata * ) handle;
	close( data->controlsock );
	close( data->datasock );
	delete data;
	
}



// Write

bool Wiimote::WriteData( void * data, size_t size ) {
	
	if ( this->handle == nullptr ) { return false; }
	if ( size == 0 || size >= WIIMOTE_BUFFERSIZE ) { return false;}
	
	handledata * hdata = ( handledata * ) this->handle;
	
	unsigned char * cdata = ( unsigned char * ) data;
	
	for ( size_t i = size + 1; i > 0; i-- ) { this->buf[ i ] = cdata[ i - 1 ]; }
	this->buf[ 0 ] = 0xA2;
	
	int res = write( hdata->datasock, this->buf, size + 1 );
	
	for ( size_t i = 0; i < size; i++ ) { this->buf[ i ] = this->buf[ i + 1 ]; }
	
	return res != -1;
	
}



// Read

bool Wiimote::ReadData( void * data, size_t size ) {
	
	if ( this->handle == nullptr ) { return false; }
	if ( size == 0 || size >= WIIMOTE_BUFFERSIZE ) { return false; }
	
	handledata * hdata = ( handledata * ) this->handle;
	
	unsigned char * cdata = ( unsigned char * ) data;
	unsigned char report = cdata[ 0 ];
	
	this->buf[ 0 ] = 0xA2;
	this->buf[ 1 ] = report;
	
	int res = write( hdata->datasock, this->buf, 2 );
	if ( res == -1 ) { return false; }
	
	while ( true ) {
		
		res = read( hdata->datasock, this->buf, WIIMOTE_BUFFERSIZE );
		if ( res == -1 ) { return false; }
		
		if ( this->buf[ 1 ] == report ) { break; }
		
	}
	
	for ( size_t i = 0; i < size; i++ ) { cdata[ i ] = this->buf[ i + 1 ]; }
	
	return true;
	
}



#endif




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
	
	#ifdef _WIN32
	Sleep( 1000 );
	#else
	sleep( 1 );
	#endif
	
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
	// Wait a bit just to be sure
	#ifdef _WIN32
	Sleep( 1000 );
	#else
	sleep( 1 );
	#endif
	if ( this->GetExtensionType() != WIIMOTE_EXT_UDRAW ) { return false; }
	
	this->buf[ 0 ] = 0x12;
	//this->buf[ 1 ] = 0x04 | ( int ) this->rumble; // Report even if unchanged
	this->buf[ 1 ] = ( int ) this->rumble;
	this->buf[ 2 ] = 0x32;
	this->WriteData( this->buf, 3 ); // Set report mode
	
	return true;
	
}
bool Wiimote::PollUDraw( UDrawData * data ) {
	
	this->buf[ 0 ] = 0x32;
	if ( this->ReadData( buf, 11 ) != true ) { return false; }
	
	data->x = this->buf[ 3 ] + ( ( this->buf[ 5 ] & 0x0F ) << 8 );
	data->y = 1450 - ( this->buf[ 4 ] + ( ( ( this->buf[ 5 ] & 0xF0 ) << 4 ) ) );
	
	data->click = buf[ 8 ] & WIIMOTE_UDRAW_CLICK;
	data->sideclick1 = ~buf[ 8 ] & WIIMOTE_UDRAW_SIDECLICK1;
	data->sideclick2 = ~buf[ 8 ] & WIIMOTE_UDRAW_SIDECLICK2;
	
	data->pressure = buf[ 6 ];
	if ( data->click == true ) { data->pressure += 0xFF; }
	
	data->buttons = this->buf[ 1 ] << 8;
	data->buttons |= this->buf[ 2 ];
	
	return true;
	
}



// Construction

Wiimote::Wiimote( void * handle ) {
	
	this->handle = handle;
	memset( this->buf, 0, sizeof( this->buf ) );
	this->rumble = false;
	this->led = 0;
	
}
