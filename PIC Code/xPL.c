///@brief xPL Framework
///@file xPL.c

//** I N C L U D E S **********************************************************
#include "TCPIP Stack/TCPIP.h" 	/// Microchip TCIP stack
#include "xPL.h"				/// xPL general stuff
#include "SecuritySystem.h"		/// Security, were most of it is at

///******************************************************************************
/** D E C L A R A T I O N S **************************************************/
///******************************************************************************

/// Used as a temp storage for processing messages, holds the key of a key value pair
char xPLMsgKey[xPLMSGKEYLENGTH];
/// Used as a temp storage for processing messages, holds the value of a key value pair
char xPLMsgValue[xPLMSGVALUELENGTH];

/// xPL device name, vendor ID
static rom char xPL_My_Vendor_ID[] = {"oden"};
/// xPL device name, device ID
static rom char xPL_My_Device_ID[] = {"pir"};
/// xPL device name, instance ID
rom char Default_xPL_My_Instance_ID[] = {"thefirst"};

/// xPL Message Types
static ROM char *xPLmessageTypeText[] =
{
	"xpl-cmnd",     // command
	"xpl-stat", 	// trigger
	"xpl-trig"      // status
};

/// xPL Header contents, this data is mandatory for the message to be valid
static ROM char *xPLHeaderKeyText[] =
{
	"hop",		// number of hops the message has made
	"source",	// xpl device that generated the message
	"target",	// xpl device the message is intended for
	"}"			// used to determine that the header is over
};

/// xPL Schema's (class) that are supported by this application. hbeat and config are mandatory for any xPL device, add functionality after these entries.
/// If adding value dont forget to add XPL_SCHEMACLASS_SIZE
static ROM char *xPLschemaClassText[] =
{
	"hbeat",		// heart beat, part of base protocol
	"config",		// config, part of base
	"security"		// security
};

/// xPL Schema's (type) that are supported by this application. The first 6 are mandatory for any xPL device, add functionality after these entries.
static ROM char *xPLschemaTypeText[] =
{
	"basic",		// part of base protocol
	"list",			// part of base protocol
	"end",			// part of base protocol
	"response",		// part of base protocol
	"request",		// part of base protocol
	"current",		// part of base protocol
	"area",		// security starts here
	"zone",
	"gateway",
	"areastat",
	"zonestat",
	"gatestat",
	"areainfo",
	"zoneinfo",
	"gateinfo",
	"arealist",
	"zonelist"
};

/// True if the device has received all needed configuration details from the hub.
BOOL xPLConfiguerd;

/// Time since last Heartbeat was sent
TICK xPLHeartbeatIntervalCount;
/// Time spent in not configured state
TICK xPLNotConfiguredTimer;
/// Declares the Microchip IP socket that will be used for processing xPL messages
UDP_SOCKET XPLSocket = INVALID_UDP_SOCKET;
// Defines a received xPL message
xPLReceivedMessages xPLReceivedMessage;

/*    D A T A B A N K   4     */
#pragma udata xPLvars4

//******************************************************************************
//** P R I V A T E  P R O T O T Y P E S ***************************************/
//******************************************************************************

static void XPLHeartBeatTiming(void);		// Determines when a heart beat message should be sent
static void XPLProcessUDPMsg(void);			// Only called when we have received data on the UDP port
static BOOL XPLProcessUDPMsgHeader(void);	// determines if header is valid and extracts Fields
static void XPLSendHeartBeat(void);
static BOOL xPLProcessMsgCompair (rom char *ValidResult, int MaxSize, char TermChar);
static BOOL xPLProcessSaveValue(char TermChar, char *Result, int MaxSize);
static BOOL xPLProcessMsgChar(char Expected);
static BOOL xPLProcess_MsgField(rom char **ValidOptions, int ArraySize, unsigned short int *Result, char TermChar);


#pragma code

//******************************************************************************
/**
Initiates the xPL stack. This must be called before XPLTasks() or things will go wrong
@result Returns true if a socket was selected.
*/
BOOL xPLInit(void) {

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"xPLInit called\r\n");
#endif

	// Load the schema init
	xPLConfiguerd = SchemaInit();

   	// set default values when not configured
   	if(!xPLConfiguerd) {
		xPLConfig.xPL_Heartbeat_Interval = XPL_NOTCONFIGUREDHBINTERVAL;
		xPLNotConfiguredTimer = TickGet() + (XPL_NOTCONFIGUREDTIMEOUT * TICK_SECOND);
		memset(xPLConfig.xPL_My_Instance_ID,'\0', 17);
		strcpypgm2ram(xPLConfig.xPL_My_Instance_ID, Default_xPL_My_Instance_ID);

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"xPLInit detected device not configured\r\n");
#endif
	}

	// Log output if device is configured
#if defined(DEBUG_UART)
	if (xPLConfiguerd) putrsUART((ROM char*)"xPLInit detected the device is configured\r\n");
#endif

	// Kick the heart beat timer off
	xPLHeartbeatIntervalCount = TickGet() + (xPLConfig.xPL_Heartbeat_Interval*(TICK_SECOND*60));

	// Return with sucess
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"All init processes completed, ready to go \r\n");
#endif

	return 1;

}//end UserInit


//*********************************************************************
/* Loads data from EE Prom into the selected config pointer.

@result Returns False is first character is not correct, used to ensure data is valid
@param  *p, pointer to config dat
@param  Start_Address,  EEProm position to start loading from
@param  Data_Size, the size in bytes of the configuration data
 */

BOOL xPLLoadEEConfig(void *p, int Start_Address, int Data_Size) {

    BYTE c;
    BYTE *dest=(BYTE *)p;

    XEEBeginRead(Start_Address);
    c = XEERead();
    XEEEndRead();
    if(c == XPLEECONFIGVERSION)     // When a record is saved, first byte is written to indicate a valid record was saved.
    {
     	// We have valid config data to load
        XEEBeginRead(Start_Address + 1);
        for ( c = 0; c < (unsigned int) Data_Size; c++ )
            *dest++ = XEERead();
        XEEEndRead();
        return TRUE;
    }
    else
    {
		// There is no config data to load
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"The config version did not return correctly. Load failed\r\n");
#endif

		return FALSE;
	}

}

//*********************************************************************
/* Accepts configuration data (in the form of a pointer) and saves it to EE Prom

@param  *p, pointer to config dat
@param  Start_Address,  EEProm position to start loading from
@param  Data_Size, the size in bytes of the configuration data
 */

void  xPLSaveEEConfig(void *p, int Start_Address, int Data_Size) {

    BYTE c;
    BYTE *src=(BYTE *)p;

    XEEBeginWrite(Start_Address);
    XEEWrite(XPLEECONFIGVERSION); // When a record is saved, first byte is written to indicate a valid record was saved.
    XEEEndWrite();
	while(XEEIsBusy());

	putrsUART((ROM char*)"Saving starting");

    for(iCount = 1; iCount < Data_Size; iCount++) {
		XEEBeginWrite(Start_Address + iCount);
		XEEWrite(*src++);
		while(XEEIsBusy());
		XEEEndWrite();
	   	while(XEEIsBusy());
	}// iCount

	putrsUART((ROM char*)"end\r\n");
}



//*********************************************************************
/*
This task manages everything to do with the heart beat message as well as the config heart beat message

*/
void XPLHeartBeatTiming(void) {
	if (TickGet() > xPLHeartbeatIntervalCount) {
 		if(!xPLConfiguerd) {
			if(xPLNotConfiguredTimer > TickGet()) {
				// Not configured
				xPLConfig.xPL_Heartbeat_Interval = XPL_NOTCONFIGUREDHBINTERVAL; // added to change interval if a hub appears after timer expires
			}
			else if (xPLNotConfiguredTimer < TickGet()) {
				// Not configured and no hub time out has been reached
				xPLConfig.xPL_Heartbeat_Interval = XPL_NOTCONFIGUREDNOHUBHBINTERVAL;
			}
		} ///xPLConfiguerd

		xPLHeartbeatIntervalCount = TickGet() + (xPLConfig.xPL_Heartbeat_Interval*(TICK_SECOND*60));
		// Send HB message
		XPLSendHeartBeat();

	} //<= TickGet() > xPLHeartbeatIntervalCount
}// void XPLHeartBeatTiming

//*********************************************************************
/*
This is the main task manager where everything xPL related begins.

From main.c you call the StackTask(); and maybe StackApplications(); if you are in the mood for running other services that are a part of
the Microchip stack. Next this function is called to perform all the xPL stuff.

main.c
void main(void) {
	while(1) {
		StackTask();
		StackApplications();
		XPLTasks();
	}
}


SchemaTasks() is called to perform anything functions that the device being implemented needs to perform.
*/
void XPLTasks(void) {

	/* There are 3 tasks here that need to run in this order
	 1 SchemaTasks
	 2 UDP input buffer
	 3 xPL heart beat message
	 Only one can run per task run! Remember the StackTask() needs to be called to flush the transmit buffer

	*/

	if(UDPIsPutReady(XPLSocket)) {
		if (SchemaTasks()) {  // True if no task has ran
			// Process heart beat
			if(UDPIsPutReady(XPLSocket))
				XPLHeartBeatTiming();
			// Process email messages
			if(UDPIsPutReady(XPLSocket))
				SMTPTaskProcess();
		}
	}

	// Is data waiting for us to read?
	if (UDPIsGetReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"\r\n######################\r\nUDP xPL socket contains data\r\n");
#endif
		XPLProcessUDPMsg();
	}

} // XPLTasks


//*********************************************************************
/*
Constructs the heart beat and config hbeat messages. This is called when a message needs to be sent

*/
void XPLSendHeartBeat(void) {

	memset(xPLMsgKey, '\0',xPLMSGKEYLENGTH);

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# XPLSendHeartBeat, no MAC transmit port\r\n");
#endif
	}
	else {

		// Send generic header config.basic
		if(!xPLConfiguerd)
			XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Basic);

		//Send generic header hbeat.basic
		if(xPLConfiguerd)
			XPLSendHeader(xPL_STATUS, TRUE,Class_HBeat,Type_Basic);


		UDPPutROMString((ROM BYTE*) "interval=");

		uitoa((WORD)xPLConfig.xPL_Heartbeat_Interval, xPLMsgKey);
		for (iCount = 0; ! xPLMsgKey[iCount] == '\0'; iCount++) {
			UDPPut(xPLMsgKey[iCount]);
		}

		UDPPutROMString((ROM BYTE*) "\n}\n");
		// Now transmit it.
		UDPFlush();

	} ///UDPIsPutReady(XPLSocket)
}

//*********************************************************************
/**
Used to create any xPL message. Populates the header then adds class type and opens the body.
have a look at XPLSendHeartBeat(); to see it in use. Remember the body of the meesage need to be closed with the following:

UDPPutROMString((ROM BYTE*) "\n}\n");

@param xPL_MessageTypes xPL_MSG_Type
@param BOOL isBroadCast, if the message is not a broadcast then the target address will be taken from xPLReceivedMessage
@param xPL_SchemaClasss SchemaClass
@param xPL_SchemaTypess SchemaType

 */

void XPLSendHeader(xPL_MessageTypes xPL_MSG_Type, BOOL isBroadCast, xPL_SchemaClasss SchemaClass, xPL_SchemaTypess SchemaType) {

	// Clear out the NODE_INFO, when a packet is received the socket is changed to point to the remote node
	UDPSocketInfo[XPLSocket].remoteNode.IPAddr.Val =  0xffffffff;

	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[0]   = 0xff;
	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[1]   = 0xff;
	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[2]   = 0xff;
	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[3]   = 0xff;
	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[4]   = 0xff;
	UDPSocketInfo[XPLSocket].remoteNode.MACAddr.v[5]   = 0xff;

	UDPSocketInfo[XPLSocket].remotePort = XPL_UDP_PORT;

	// Send XPL message type from array
	UDPPutROMArray(xPLmessageTypeText[xPL_MSG_Type],8);

	// open the header
	UDPPutROMString((ROM BYTE*) "\n{\nhop=1\nsource=");
	UDPPutROMString((ROM BYTE*) xPL_My_Vendor_ID);
	UDPPut('-');
	UDPPutROMString((ROM BYTE*) xPL_My_Device_ID);
	UDPPut('.');

	UDPPutArray(xPLConfig.xPL_My_Instance_ID, strlen(xPLConfig.xPL_My_Instance_ID));

	UDPPutROMString((ROM BYTE*) "\ntarget=");
	if (isBroadCast)
		UDPPut('*');
	else
	{
		UDPPutROMString((ROM BYTE*) xPLReceivedMessage.SourceVendor);
		UDPPut('-');
		UDPPutROMString((ROM BYTE*) xPLReceivedMessage.SourceDeviceID);
		UDPPut('.');
		UDPPutROMString((ROM BYTE*) xPLReceivedMessage.SourceInstanceID);
	}
	UDPPutROMString((ROM BYTE*) "\n}\n");

	for (iCount = 0; ! xPLschemaClassText[SchemaClass][iCount] == '\0'; iCount++) {
		UDPPut( xPLschemaClassText[SchemaClass][iCount]);
	}



	//UDPPutROMArray(xPLschemaClassText[SchemaClass],8);
	UDPPut('.');
	for (iCount = 0; ! xPLschemaTypeText[SchemaType][iCount] == '\0'; iCount++) {
		UDPPut( xPLschemaTypeText[SchemaType][iCount]);
	}

	//UDPPutROMArray(xPLschemaTypeText[SchemaType],8);
	UDPPutROMString((ROM BYTE*) "\n{\n");
}


//*********************************************************************
/**
Processes a received xPL message and determines what to do with it.

If the message was not handled here (if config or HBeat related) the message will be passed
to SchemaProcessUDPMsg() if is directed to this device, or is a broadcast.

SchemaProcessUDPMsg() is where the implemented device process received messages.
*/

void XPLProcessUDPMsg(void) {
	BOOL SchemaToProcessMSG = TRUE;
	int intRandomNumber;


	if (XPLProcessUDPMsgHeader() == TRUE) {
		if (xPLReceivedMessage.xPL_MsgDestination == To_Other)
			SchemaToProcessMSG = FALSE;

		// If a xPL message has been received keep the not configured timer from expiring
		if (!xPLConfiguerd)
			xPLNotConfiguredTimer = TickGet() + (XPL_NOTCONFIGUREDTIMEOUT*TICK_SECOND);


#if defined(DEBUG_UART)
		putrsUART((ROM char*)" Message type: ");
		putcUART('0'+ xPLReceivedMessage.xPL_MessageType);
		putrsUART((ROM char*)" Schema Class: ");
		putcUART('0'+ xPLReceivedMessage.xPL_SchemaClass);
		putrsUART((ROM char*)" Schema Type: ");
		putcUART('0'+ xPLReceivedMessage.xPL_SchemaTypes);
		putrsUART((ROM char*)" Destination: ");
		putcUART('0'+ xPLReceivedMessage.xPL_MsgDestination);
		putrsUART((ROM char*)"\r\n");
#endif

		// Heat Beat request
		if ((xPLReceivedMessage.xPL_MessageType == xPL_COMMAND) && (xPLReceivedMessage.xPL_MsgDestination == To_All )) {
			//
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Heart beat request,");
#endif
			if ((xPLReceivedMessage.xPL_SchemaClass == Class_HBeat) && (xPLReceivedMessage.xPL_SchemaTypes == Type_Request)) {
				// Device discovery request
				SchemaToProcessMSG = FALSE;
				if((xPLProcessMsgCompair ((rom char *)"command", 8, '=')) && (xPLProcessMsgCompair ((rom char *)"request", 8, '\n'))) {
					//generate a random number between 2-6 seconds, and produces bheat within that time.
					intRandomNumber = (rand() / 6553) + 2;
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" HBeat sent in ");
					putcUART('0'+ intRandomNumber);
					putrsUART((ROM char*)" seconds \r\n");
#endif
					xPLHeartbeatIntervalCount = intRandomNumber*(TICK_SECOND);
				} // command request
			} // hbeat request

		} // xPL_COMMAND & To_All

		if (SchemaToProcessMSG) {
			//
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Message going to Schema to process \r\n");
#endif
			SchemaProcessUDPMsg();
		}

		else {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"skips schema, processing stopped\r\n");
#endif

		}



	} //XPLProcessUDPMsgHeader() = TRUE

} // XPLProcessUDPMsg


//*********************************************************************
/**
Takes the contents out of the buffer byte by byte to determine if it is a valid xPL message. Will
also populate xPLReceivedMessage with details about the message.

\todo refactoring this section might be a good idea one day

@result BOOL, True if the message is valid
*/

BOOL XPLProcessUDPMsgHeader(void) {

	char UDPchar;
	unsigned short int HeaderType;
	unsigned short int HeaderCount;

	memset(xPLMsgKey,  '\0', xPLMSGKEYLENGTH);

	// ## Message type, terminated by \n
	if (xPLProcess_MsgField( xPLmessageTypeText, 3, &HeaderType, '\n') == FALSE) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" Error in header type. Message rejected \r\n");
#endif
		return 0;
	}
	xPLReceivedMessage.xPL_MessageType = HeaderType;
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Header message type is ");
	uitoa((WORD)xPLReceivedMessage.xPL_MessageType, (BYTE*)UDPchar);
	putsUART ((BYTE*)UDPchar);
	putrsUART((ROM char*) xPLmessageTypeText[HeaderType]);
	putrsUART((ROM char*) "\r\n");
#endif


	// ## Next 2 characters should always be {\n
	if ((xPLProcessMsgChar('{') == FALSE) || (xPLProcessMsgChar('\n') == FALSE)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" Missing new line character. Message rejected \r\n");
#endif
		return 0;
	}

	// ## Cycle though the 3 header Fields, they can be out of order.
	for (HeaderCount = 0; HeaderCount <= (unsigned int)3; HeaderCount++)
	{
		// Collect the header Field and determine what type it is (HeaderType)
		if (xPLProcess_MsgField(xPLHeaderKeyText, 4, &HeaderType, '=') == FALSE) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Unknown header feild. Message rejected \r\n");
#endif
			return 0; // no match found
		}
		//====Begin HOP ====
		if (HeaderType == (unsigned int) 0) {
			if (xPLProcessSaveValue('\n', xPLMsgKey, 2)) //  max Field size is one, set 2 just in case
			{ }
		}//HeaderType == 0 = HOP

		//====Begin Source ====
		else if (HeaderType == (unsigned int)1) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Begin decoding the Source \r\n");
#endif

			memset(xPLReceivedMessage.SourceVendor,  '\0', 8);
			memset(xPLReceivedMessage.SourceDeviceID, '\0', 8);
			memset(xPLReceivedMessage.SourceInstanceID, '\0', 16);

			if(xPLProcessSaveValue('-', xPLReceivedMessage.SourceVendor, 9) == FALSE) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Error in Source, Vendor. Message rejected \r\n");
#endif
				return 0; // empty Field
			}


			if(xPLProcessSaveValue('.', xPLReceivedMessage.SourceDeviceID, 9) == FALSE) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Error in Source, Device. Message rejected \r\n");
#endif
				return 0; // empty Field
			}

			if(xPLProcessSaveValue('\n', xPLReceivedMessage.SourceInstanceID, 17) == FALSE) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Error in Source, Instance. Message rejected \r\n");
#endif
				return 0; // empty Field
			}

#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Message Source - Vendor:");
			putsUART(xPLReceivedMessage.SourceVendor);
			putrsUART((ROM char*)" Device:");
			putsUART(xPLReceivedMessage.SourceDeviceID);
			putrsUART((ROM char*)" Instance:");
			putsUART(xPLReceivedMessage.SourceInstanceID);
			putrsUART((ROM char*)"\r\n");
#endif


		}//HeaderType == 1 = SOURCE


		//====Begin Target====
		else if (HeaderType == (unsigned int)2) {
			//
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Begin decoding the Target \r\n");
#endif

			memset(xPLMsgKey,  '\0', xPLMSGKEYLENGTH);
			if(xPLProcessSaveValue('-', xPLMsgKey, 16) == FALSE) {
				if (stricmppgm2ram(xPLMsgKey, (ROM BYTE *)"*") == 0) {
					// The message is a broadcast
					xPLReceivedMessage.xPL_MsgDestination = To_All;
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Broadcasted message, value: ");
					putcUART('0' + xPLReceivedMessage.xPL_MsgDestination );
					putrsUART((ROM char*)"\r\n");
#endif
				} // broadcast message
				else {
					// Message is just an error
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Error in Target. Message rejected \r\n");
#endif
					return 0; // empty Field
				} // error in message
			} // xPLProcessSaveValue


			else if(stricmppgm2ram(xPLMsgKey, xPL_My_Vendor_ID) == 0 ) {
				memset(xPLMsgKey,  '\0', xPLMSGKEYLENGTH);
				if(xPLProcessSaveValue('.', xPLMsgKey, 16) == FALSE) {
					// There is an error in the target message
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Error in Target Vendor. Message rejected \r\n");
#endif

					return 0; // empty Field
				}//xPLProcessSaveValue

				if(stricmppgm2ram(xPLMsgKey, xPL_My_Device_ID) == 0 ) {
					memset(xPLMsgKey,  '\0', xPLMSGKEYLENGTH);
					if(xPLProcessSaveValue('\n', xPLMsgKey, 16) == FALSE) { // will this error if device ID length = 16?
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" Error in Target, Device. Message rejected \r\n");
#endif
						return 0; // empty Field
					}


					if(strcmp(xPLMsgKey, xPLConfig.xPL_My_Instance_ID) == 0 ) {
						xPLReceivedMessage.xPL_MsgDestination = To_Me;
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" Message directed to me ");
#endif
					}
					else {
						xPLReceivedMessage.xPL_MsgDestination = To_Other;
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" Message to Other ");
#endif
					} //xPLConfig.xPL_My_Instance_ID
				}
				else {
					xPLReceivedMessage.xPL_MsgDestination = To_Other;
					if(xPLProcessSaveValue('\n', xPLMsgKey, 16) == FALSE) { // will this error if device ID length = 16?
					}

#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Message to Other ");
#endif
				} // xPL_My_Device_ID

			} // else if(strcmppgm2ram(xPLMsgKey, xPL_My_Vendor_ID) == 0 )
			else {

				if(xPLProcessSaveValue('.', xPLMsgKey, 16) == FALSE) {}
				if(xPLProcessSaveValue('\n', xPLMsgKey, 16) == FALSE) {
					// will this error if device ID length = 16
				}

#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Message to Other \r\n");
#endif
			} // xPL_My_Vendor_ID

		} //HeaderType = 2 = TARGET


		//====Begin End of body ====

		else if (HeaderType == (unsigned int)3) break;

		else {
			// Something went wrong with the HeaderType as this should not happen
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"\r\n #ERROR# Error in HeaderType section of XPLProcessUDPMsgHeader\r\n");
			putrsUART((ROM char*)" Header type: ");
			putcUART('0'+ HeaderType);
			putrsUART((ROM char*)" \r\n");

#endif
			return 0;
		}

	} //for



	// Collect the class
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Begin decoding the Schema Class \r\n");
#endif

	if (xPLProcess_MsgField(xPLschemaClassText, XPL_SCHEMACLASS_SIZE, (unsigned short int *) xPLReceivedMessage.xPL_SchemaClass ,'.') == FALSE) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" error is schema class. Message rejected.\r\n");
#endif
		return 0; // no match found
	}

	// Collect the type
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Begin decoding the Schema Type \r\n");
#endif

	if (xPLProcess_MsgField(xPLschemaTypeText, XPL_SCHEMATYPE_SIZE, (unsigned short int *)xPLReceivedMessage.xPL_SchemaTypes, '\n') == FALSE) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" error is schema type. Message rejected.\r\n");
#endif
		return 0; // no match found
	}


	// Next 2 characters should always be {\n
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Open the body of the message { NewLine \r\n");
#endif
	if ((xPLProcessMsgChar('{') == FALSE) || (xPLProcessMsgChar('\n') == FALSE)) {
		//
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" expected end of header. Message rejected.\r\n");
#endif
		return 0; // no match found
	}



	// We made it :) The packet seems valid
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Message accepted.\r\n");
#endif
	return TRUE;

} //XPLProcessUDPMsgHeader

//*********************************************************************
/**
Extracts data out of the buffer and populates the result into the pointer can converts to lower case. If the
terminator character does not appear before the maximum size is reached then the result will be fales (fail)

@param char TermChar, character that will terminate data collection
@param char *Result, pointer where the data will come from
@param  MaxSize, maximum size of the data
@result BOOL, was the value correctly saved
*/

BOOL xPLProcessSaveValue(char TermChar, char *Result, int MaxSize) {

	char NextChar;
	int CountBufferChars;

	for(CountBufferChars = 0; CountBufferChars <= MaxSize; CountBufferChars++) {
		UDPGet(&NextChar);
		if(NextChar >= 'A' && NextChar <= 'Z')
			NextChar -= 'A' - 'a'; //Convert to lower case
		if((NextChar == TermChar) || (NextChar == '\n')) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"xPLProcessSaveValue: ");
			putsUART(Result);
			if((NextChar == TermChar) && (NextChar != '\n')) {
				putrsUART((ROM char*)" Correctly ended by term char:");
				putcUART((BYTE)NextChar);
				putrsUART((ROM char*)" \r\n");
			}
			if(NextChar == '\n') putrsUART((ROM char*)" Ended by New Line\r\n");
#endif

			if(NextChar == TermChar) return TRUE; // term character found but we have data
			return FALSE;
		} // term chars

		Result[CountBufferChars] = NextChar;

	} // for

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"xPLProcessSaveValue: ");
	putsUART(Result);
	putrsUART((ROM char*)" ended by max size\r\n");
#endif
	return TRUE; // reached the max number of characters
} //xPLProcessSaveValue


//*********************************************************************
/**
Check one character from the input source

@param char Expected, the char that is expected to be in the buffer
@result BOOL, True if the value are the same
*/
BOOL xPLProcessMsgChar(char Expected) {

	char NextChar;
	UDPGet(&NextChar);
	if(NextChar >= 'A' && NextChar <= 'Z')
		NextChar -= 'A' - 'a';  //Convert to lower case
	if (NextChar == Expected)
		return TRUE;
	return FALSE;

}


//*********************************************************************
/**

Takes a data from the buffer and determines if it matches a value in the supplied ROM array.
If the terminator character does not appear before the maximum size is reached then the result will be fales (fail)

@param rom char **ValidOptions, looks for a match with on of the values stored in this array
@param  ArraySize, maximum size of the expected value
@param unsigned short int *Result, The position in the array where the match was found
@param char TermChar, terminator character
@result BOOL, true if processed as expected and match was found

*/

BOOL xPLProcess_MsgField(rom char **ValidOptions, int ArraySize, unsigned short int *Result, char TermChar) {

	char NextChar;
	int CountBufferChars =0;

	// Collect characters until terminator is found or max size is reached.
	UDPGet(&NextChar);
	while ((NextChar != TermChar) && (NextChar != '\n' ) && ( CountBufferChars < 127) ) {
		if(NextChar >= 'A' && NextChar <= 'Z')
			NextChar -= 'A' - 'a';
		//tolower(NextChar); // Convert to lower case
		xPLMsgValue[CountBufferChars] = NextChar;
		CountBufferChars++;
		UDPGet(&NextChar);
	}
	xPLMsgValue[CountBufferChars] = '\0';

	// cycle thought the array to see if there is a match
	for(iCount=0; iCount < ArraySize; iCount++) {
		if (strncmppgm2ram(xPLMsgValue, ValidOptions[iCount], CountBufferChars) ==0) { // a match was found
			*Result = iCount;
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" - xPLProcess_MsgField, int match: ");
			putcUART('0'+ iCount);
			putrsUART((ROM char*)" With Result:");
			putrsUART((ROM char*) ValidOptions[iCount]);
			putrsUART((ROM char*)"\r\n");
#endif
			return TRUE;
		}

	}

	// no match was found
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" - xPLProcess_MsgField,No match found with string:");
	putsUART(xPLMsgValue);
	putrsUART((ROM char*)"\r\n");
#endif

	return 0;

}

//*********************************************************************
/**

Compairs data in the input buffer to a string to see if they are a match.

@param rom char *ValidResult,  string we are looking for
@param  MaxSize, max chars to process,
@param char TermChar, character that terminates matching
@result BOOL, was correct data received
*/

BOOL xPLProcessMsgCompair (rom char *ValidResult, int MaxSize, char TermChar) {
	char NextChar;

	// Collect characters until terminator is found or max size is reached.
	UDPGet(&NextChar);
	while ((NextChar != TermChar) && (NextChar != '\n' ) && ( iCount < MaxSize) ) {
		if(NextChar >= 'A' && NextChar <= 'Z')
			NextChar -= 'A' - 'a';  //Convert to lower case
		if(!(NextChar==ValidResult[iCount])) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" - xPLProcessMsgCompair, No Match. iCount:");
			putcUART('0'+ iCount);
			putrsUART((ROM char*)" Compaired to:");
			putcUART((char)ValidResult);
			putrsUART((ROM char*)" no match on char:");
			putcUART((char)NextChar);
			putrsUART((ROM char*)"\r\n");
#endif
			return FALSE;
		}

		iCount++;
		UDPGet(&NextChar);
	}

	if 	(iCount == 1) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" - xPLProcessMsgCompair, No Match. iCount:");
		putcUART('0'+ iCount);
		putrsUART((ROM char*)" Compaired to:");
		putrsUART(ValidResult);
		putrsUART((ROM char*)" no match as it was to short");
		putrsUART((ROM char*)"\r\n");
#endif

		return FALSE; // First char was a terminator
	}

	return TRUE;
}

//*********************************************************************
/**

 searches though an array looking for a match to TheValue. When a match is found
 it will return the position in the array

@param rom char **ValidOptions, array of posible matches
@param  ArraySize, size of ValidOptions
@param  *Result,  position in array where match was found
@param char * TheValue,  text to be searched
@result BOOL, true if processed as expected and match was found
*/

BOOL xPLFindArrayInKVP(rom char **ValidOptions, int ArraySize, int *Result, char * TheValue) {

 	int CountBufferChars;

	// cycle thought the array to see if there is a match
	for(iCount=0; iCount < ArraySize; iCount++)
	{

		CountBufferChars = sizeof(ValidOptions[iCount]);
		if (strncmppgm2ram(TheValue, ValidOptions[iCount],CountBufferChars) ==0)
		{ // a match was found
			*Result = iCount;
			return TRUE;
		}
	}

	// no match was found
	return 0;
}


//*********************************************************************
/**

 Loads a Key and a Value from the UDP message. Populates this into variables.

@param char * TheKey, location to save the data for the key
@param char * TheValue, location to save the data for the value
@result BOOL, true if processed as expected
*/

BOOL SaveKVP (char * TheKey, char * TheValue) {

	int short iCount;
	char NextChar;

	// clear out any old data
	memset (TheKey, '\0', xPLMSGKEYLENGTH);
	memset (TheValue, '\0', xPLMSGVALUELENGTH);

	// Load character by character
	for (iCount =0; iCount < xPLMSGKEYLENGTH; iCount++) {
		UDPGet(&NextChar);
		if(NextChar >= 'A' && NextChar <= 'Z')
			NextChar -= 'A' - 'a'; //Convert to lower case for the key but we dont want to do that for the value
		if (NextChar == '=') break;
		if (NextChar == '}') return FALSE;
		TheKey[iCount] = NextChar;
	} // for key


#if defined(DEBUG_UART)
	putrsUART((ROM char*)"TheKey = ");
	putsUART(TheKey);
#endif


	for (iCount =0; iCount < xPLMSGVALUELENGTH; iCount++) {
		UDPGet(&NextChar);
		if (NextChar == '\n') break;
		if (NextChar == '}') return FALSE;
		TheValue[iCount] = NextChar;
	} // for key


#if defined(DEBUG_UART)
	putrsUART((ROM char*)" TheValue = ");
	putsUART(TheValue);
	putrsUART((ROM char*)"\r\n");
#endif


	return TRUE;

}


//******************************************************************************
//*                                The End
//******************************************************************************

