///@file MainDemo.c
///@brief main.c, says it all.

/// \todo rename to main.c


/*
 * This macro uniquely defines this file as the main entry point.
 * There should only be one such definition in the entire project,
 * and this file must define the AppConfig variable as described below.
 */
#define THIS_IS_STACK_APPLICATION

// Include all headers for any enabled TCPIP Stack functions
#include "TCPIP Stack/TCPIP.h"

// Include functions specific to this stack application
#include "MainDemo.h"

// Include application related header files
#if defined(STACK_USE_xPL)
	#include "SecuritySystem.h"
	#include "xPL.h"
#else
	#define EEDATASTART_CONFIG 0x0000 // EE memory start position for config data
#endif


// Declare AppConfig structure and some other supporting stack variables
APP_CONFIG AppConfig;
BYTE AN0String[8];
BYTE myDHCPBindCount = 0xFF;

// Private helper functions.
// These may or may not be present in all applications.
static void InitAppConfig(void);
static void InitializeBoard(void);


//
// PIC18 Interrupt Service Routines
//
// NOTE: Several PICs, including the PIC18F4620 revision A3 have a RETFIE FAST/MOVFF bug
// The interruptlow keyword is used to work around the bug when using C18
#if defined(__18CXX)
	#pragma interruptlow LowISR
	void LowISR(void) {
	    TickUpdate();
	}

	#pragma interruptlow HighISR
	void HighISR(void){
	    #if defined(STACK_USE_UART2TCP_BRIDGE)
		UART2TCPBridgeISR();
		#endif
	}

	#pragma code lowVector=0x18
	void LowVector(void){_asm goto LowISR _endasm}

	#pragma code highVector=0x8
	void HighVector(void){_asm goto HighISR _endasm}

	#pragma code // Return to default code section

#endif

//
// Main application entry point.
//

 void main(void) {
    static TICK t = 0;
	BOOL InitalWaitingPast = FALSE;

    // Initialize application specific hardware
    InitializeBoard();

	#if defined(USE_LCD)
		// Initialize and display the stack version on the LCD
		LCDInit();
		DelayMs(100);
		strcpypgm2ram((char*)LCDText, "TCPStack " VERSION "  "
			"                ");
		LCDUpdate();
	#endif

	// Initialize stack-related hardware components that may be
	// required by the UART configuration routines
    TickInit();
	#if defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2)
		MPFSInit();
	#endif

	// Initialize Stack and application related NV variables into AppConfig.
	InitAppConfig();

    // Initiates board setup process if button is depressed
	// on startup

 /*
    if(BUTTON0_IO == 0u)
    {
		#if (defined(MPFS_USE_EEPROM) || defined(MPFS_USE_SPI_FLASH)) && (defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2))
		// Invalidate the EEPROM contents if BUTTON0 is held down for more than 4 seconds
		TICK StartTime = TickGet();
		LED_PUT(0x03);

		while(BUTTON0_IO == 0u)
		{
			if(TickGet() - StartTime > 4*TICK_SECOND)
			{
				#if defined(MPFS_USE_EEPROM)
			    XEEBeginWrite(EEDATASTART_CONFIG);
			    XEEWrite(0x00);
			    XEEEndWrite();
			    #endif

				#if defined(STACK_USE_UART)
					putrsUART("\r\n\r\nBUTTON0 held for more than 4 seconds.  Default settings restored.\r\n\r\n");
				#endif

				#if defined(STACK_USE_xPL) // JN
					xPLFactoryRestart();
				#endif

				LED_PUT(0x0F);
				Reset();
				break;
			}
		}
		#endif

    }
*/
	// Initialize core stack layers (MAC, ARP, TCP, UDP) and
	// application modules (HTTP, SNMP, etc.)
    StackInit();

	// Now that all items are initialized, begin the co-operative
	// multitasking loop.  This infinite loop will continuously
	// execute all stack-related tasks, as well as your own
	// application's functions.  Custom functions should be added
	// at the end of this loop.
    // Note that this is a "co-operative mult-tasking" mechanism
    // where every task performs its tasks (whether all in one shot
    // or part of it) and returns so that other tasks can do their
    // job.
    // If a task needs very long time to do its job, it must be broken
    // down into smaller pieces so that other tasks can have CPU time.


	#if defined(DEBUG_UART)
		putrsUART((ROM char*)"\r\n\n\n------------==============################$$$#################=============----------\r\n");
		putrsUART((ROM char*)"### DEVICE STARTUP ### \r\nStack is init and ready to go, main loop starting\r\n");
	#endif

	t = TickGet() + (TICK_SECOND/2ul);

	#if defined(STACK_USE_xPL)

		// Grab a UDP socket for XPL communication
		XPLSocket = UDPOpen(XPL_UDP_PORT, NULL, XPL_UDP_PORT);
		if (XPLSocket == INVALID_UDP_SOCKET) {
			#if defined(DEBUG_UART)
				putrsUART((ROM char*)"#ERROR# xPLInit failed to open UDP socket\r\n");
				putrsUART((ROM char*)"#ERROR# Restarting device\r\n");
			#endif
			// Crash and die, if we can not get this socket then the show is over
			Reset();
		}
		#if defined(DEBUG_UART)
			putrsUART((ROM char*)"xPL UDP socket opened\r\n");
		#endif

		t = TickGet() + ( TICK_SECOND * 3);
	    LED0_IO = 1;
	    InitalWaitingPast = FALSE;

	#endif




    while(1)
    {
        // Blink LED0 (right most one) every second.
         if(TickGet() > t)
        {
            t = TickGet() + (TICK_SECOND/2ul);
            LED0_IO ^= 1;

            #if defined(STACK_USE_xPL)

            	// We wait for a period of time to ensure that the network is stable before sending any messages.
            	// If we dont do this the inital xPL packet may not be received by the host
        		if (InitalWaitingPast == FALSE) {
					#if defined(DEBUG_UART)
						putrsUART((ROM char*)"Inital waiting period completed\r\n");
					#endif
	        		xPLInit(); // Returns true!
        			InitalWaitingPast = TRUE;
        		}
			#endif
        }

        // This task performs normal stack task including checking
        // for incoming packet, type of packet and calling
        // appropriate stack entity to process it.
        StackTask();

        // This tasks invokes each of the core stack application tasks
        StackApplications();

		// Process application specific tasks here.
		// For this demo app, this will include the Generic TCP
		// client and servers, and the SNMP, Ping, and SNMP Trap
		// demos.  Following that, we will process any IO from
		// the inputs on the board itself.
		// Any custom modules or processing you need to do should
		// go here.


        // If the DHCP lease has changed recently, write the new
        // IP address to the LCD display, UART, and Announce service


		#if defined(STACK_USE_xPL)
			if (InitalWaitingPast)
				XPLTasks();
		#endif

	}
}

// Writes an IP address to the LCD display and the UART as available
void DisplayIPValue(IP_ADDR IPVal)
{
//	printf("%u.%u.%u.%u", IPVal.v[0], IPVal.v[1], IPVal.v[2], IPVal.v[3]);
    BYTE IPDigit[4];
	BYTE i;
#ifdef USE_LCD
	BYTE j;
	BYTE LCDPos=16;
#endif

	for(i = 0; i < sizeof(IP_ADDR); i++)
	{
	    uitoa((WORD)IPVal.v[i], IPDigit);

		#if defined(STACK_USE_UART)
			putsUART(IPDigit);
		#endif

		#ifdef USE_LCD
			for(j = 0; j < strlen((char*)IPDigit); j++)
			{
				LCDText[LCDPos++] = IPDigit[j];
			}
			if(i == sizeof(IP_ADDR)-1)
				break;
			LCDText[LCDPos++] = '.';
		#else
			if(i == sizeof(IP_ADDR)-1)
				break;
		#endif

		#if defined(STACK_USE_UART)
			while(BusyUART());
			WriteUART('.');
		#endif
	}

	#ifdef USE_LCD
		if(LCDPos < 32u)
			LCDText[LCDPos] = 0;
		LCDUpdate();
	#endif
}



/*********************************************************************
 * Function:        void InitializeBoard(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Initialize board specific hardware.
 *
 * Note:            None
 ********************************************************************/
static void InitializeBoard(void)
{
	// LEDs
	LED0_TRIS = 0;
	LED1_TRIS = 0;
	LED2_TRIS = 0;
	LED3_TRIS = 0;
	LED4_TRIS = 0;
	LED5_TRIS = 0;
	LED6_TRIS = 0;
	LED_PUT(0x00);

#if defined(__18CXX)
	// Enable 4x/5x PLL on PIC18F87J10, PIC18F97J60, etc.
    OSCTUNE = 0x40;

	// Enable internal PORTB pull-ups
	// is this really needed?
    INTCON2bits.RBPU = 0;

	// Configure USART
    TXSTA = 0x20;
    RCSTA = 0x90;

	// See if we can use the high baud rate setting
	#if ((GetPeripheralClock()+2*BAUD_RATE)/BAUD_RATE/4 - 1) <= 255
		SPBRG = (GetPeripheralClock()+2*BAUD_RATE)/BAUD_RATE/4 - 1;
		TXSTAbits.BRGH = 1;
	#else	// Use the low baud rate setting
		SPBRG = (GetPeripheralClock()+8*BAUD_RATE)/BAUD_RATE/16 - 1;
	#endif


	// Enable Interrupts
	RCONbits.IPEN = 1;		// Enable interrupt priorities
    INTCONbits.GIEH = 1;
    INTCONbits.GIEL = 1;



#if defined(SPIRAM_CS_TRIS)
	SPIRAMInit();
#endif
}

/*********************************************************************
 * Function:        void InitAppConfig(void)
 *
 * PreCondition:    MPFSInit() is already called.
 *
 * Input:           None
 *
 * Output:          Write/Read non-volatile config variables.
 *
 * Side Effects:    None
 *
 * Overview:        None
 *
 * Note:            None
 ********************************************************************/
// Uncomment these two pragmas for production MAC address
// serialization if using C18. The MACROM=0x1FFF0 statement causes
// the MAC address to be located at aboslute program memory address
// 0x1FFF0 for easy auto-increment without recompiling the stack for
// each device made.  Note, other compilers/linkers use a different
// means of allocating variables at an absolute address.  Check your
// compiler documentation for the right method.
//#pragma romdata MACROM=0x1FFF0
static ROM BYTE SerializedMACAddress[6] = {MY_DEFAULT_MAC_BYTE1, MY_DEFAULT_MAC_BYTE2, MY_DEFAULT_MAC_BYTE3, MY_DEFAULT_MAC_BYTE4, MY_DEFAULT_MAC_BYTE5, MY_DEFAULT_MAC_BYTE6};
//#pragma romdata

static void InitAppConfig(void)
{
#if (defined(MPFS_USE_EEPROM) || defined(MPFS_USE_SPI_FLASH)) && (defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2))
    BYTE c;
    BYTE *p;
#endif

	AppConfig.Flags.bIsDHCPEnabled = TRUE;
	AppConfig.Flags.bInConfigMode = TRUE;
	memcpypgm2ram((void*)&AppConfig.MyMACAddr, (ROM void*)SerializedMACAddress, sizeof(AppConfig.MyMACAddr));
//	{
//		_prog_addressT MACAddressAddress;
//		MACAddressAddress.next = 0x157F8;
//		_memcpy_p2d24((char*)&AppConfig.MyMACAddr, MACAddressAddress, sizeof(AppConfig.MyMACAddr));
//	}
	AppConfig.MyIPAddr.Val = MY_DEFAULT_IP_ADDR_BYTE1 | MY_DEFAULT_IP_ADDR_BYTE2<<8ul | MY_DEFAULT_IP_ADDR_BYTE3<<16ul | MY_DEFAULT_IP_ADDR_BYTE4<<24ul;
	AppConfig.DefaultIPAddr.Val = AppConfig.MyIPAddr.Val;
	AppConfig.MyMask.Val = MY_DEFAULT_MASK_BYTE1 | MY_DEFAULT_MASK_BYTE2<<8ul | MY_DEFAULT_MASK_BYTE3<<16ul | MY_DEFAULT_MASK_BYTE4<<24ul;
	AppConfig.DefaultMask.Val = AppConfig.MyMask.Val;
	AppConfig.MyGateway.Val = MY_DEFAULT_GATE_BYTE1 | MY_DEFAULT_GATE_BYTE2<<8ul | MY_DEFAULT_GATE_BYTE3<<16ul | MY_DEFAULT_GATE_BYTE4<<24ul;
	AppConfig.PrimaryDNSServer.Val = MY_DEFAULT_PRIMARY_DNS_BYTE1 | MY_DEFAULT_PRIMARY_DNS_BYTE2<<8ul  | MY_DEFAULT_PRIMARY_DNS_BYTE3<<16ul  | MY_DEFAULT_PRIMARY_DNS_BYTE4<<24ul;
	AppConfig.SecondaryDNSServer.Val = MY_DEFAULT_SECONDARY_DNS_BYTE1 | MY_DEFAULT_SECONDARY_DNS_BYTE2<<8ul  | MY_DEFAULT_SECONDARY_DNS_BYTE3<<16ul  | MY_DEFAULT_SECONDARY_DNS_BYTE4<<24ul;

	// Load the default NetBIOS Host Name
	memcpypgm2ram(AppConfig.NetBIOSName, (ROM void*)MY_DEFAULT_HOST_NAME, 16);
	FormatNetBIOSName(AppConfig.NetBIOSName);

#if defined(MPFS_USE_EEPROM) && (defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2))
    p = (BYTE*)&AppConfig;

    XEEBeginRead(0x0000);
    c = XEERead();
    XEEEndRead();

    // When a record is saved, first byte is written as 0x60 to indicate
    // that a valid record was saved.  Note that older stack versions
	// used 0x57.  This change has been made to so old EEPROM contents
	// will get overwritten.  The AppConfig() structure has been changed,
	// resulting in parameter misalignment if still using old EEPROM
	// contents.
    if(c == 0x60u)
    {
        XEEBeginRead(0x0001);
        for ( c = 0; c < sizeof(AppConfig); c++ )
            *p++ = XEERead();
        XEEEndRead();
    }
    else
        SaveAppConfig();

#elif defined(MPFS_USE_SPI_FLASH) && (defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2))
	SPIFlashReadArray(0x00, &c, 1);
	if(c == 0x60u)
		SPIFlashReadArray(0x01, (BYTE*)&AppConfig, sizeof(AppConfig));
	else
		SaveAppConfig();
#endif
}

#if (defined(MPFS_USE_EEPROM) || defined(MPFS_USE_SPI_FLASH)) && (defined(STACK_USE_MPFS) || defined(STACK_USE_MPFS2))
void SaveAppConfig(void)
{
	#if defined(MPFS_USE_EEPROM)

    BYTE c;
    BYTE *p;

    p = (BYTE*)&AppConfig;
    XEEBeginWrite(0x0000);
    XEEWrite(0x60);
    for ( c = 0; c < sizeof(AppConfig); c++ )
    {
        XEEWrite(*p++);
    }

    XEEEndWrite();

    #else

    SPIFlashBeginWrite(0x0000);
    SPIFlashWrite(0x60);
    SPIFlashWriteArray((BYTE*)&AppConfig, sizeof(AppConfig));

    #endif
}
#endif

