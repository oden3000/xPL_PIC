#ifndef __XPLH__
#define __XPLH__

#define FALSE (unsigned int)0
#define TRUE  (unsigned int)1
#define BOOL   unsigned short int

// ################   xPL   ################

#define XPL_UDP_PORT	3865 // The correct UDP port for xPL data
#define XPL_NOTCONFIGUREDHBINTERVAL 1 //  heart beat interval time in seconds when not configured
#define XPL_NOTCONFIGUREDNOHUBHBINTERVAL 2 // hbeat interval time in seconds when there is no hub and not configured and time out reached
#define XPL_NOTCONFIGUREDTIMEOUT 120 // timeout in number of seconds when not configured and no hub
#define XPL_SCHEMACLASS_SIZE  3 // Size of the array for Schema Class
#define XPL_SCHEMATYPE_SIZE 6 //  Size of the array for Schema Type

// ################   Security   ################

#define BOUNCEDELAY 5 			// Measured in cycles of ZONECHECKDELAY
#define ZONECHECKDELAY 50		// 	NextZoneCheckDelay = TickGet() + (TICK_SECOND / ZONECHECKDELAY)
#define ZONEPULLUP 1	// 1 means pull up resisters in use, 0 means pull down resisters
#define xPLMSGKEYLENGTH 16		// number of characters in the xPL key feild
#define xPLMSGVALUELENGTH 128	// number of characters in the xPL value field


//******************************************************************************
/** Check the TCP/IP stack is configured ************************************/
//******************************************************************************
#if !defined(STACK_CLIENT_MODE)
    #error xPL says stack client have, you must
#endif 

#if !defined(STACK_USE_UDP)
    #error xPL must have at UDP enabled
#endif 

//******************************************************************************
/** D E C L A R A T I O N S **************************************************/
//******************************************************************************


/// xPL Message types
typedef enum _xPL_MessageTypes {
	xPL_COMMAND = 0,
	xPL_STATUS,
	xPL_TRIGGER
} xPL_MessageTypes;

/// xPL Known message classes
typedef enum _xPL_SchemaClasss {
	Class_HBeat = 0,
	Class_Config,
	Class_Security
} xPL_SchemaClasss;

/// xPL Known message types.
typedef enum _xPL_SchemaTypess {
	Type_Basic = 0,
	Type_List,
	Type_END,
	Type_Response,
	Type_Request,
	Type_Current,
	Type_Area,		///< Part of the security schema
	Type_Zone,		///< Part of the security schema
	Type_Gateway, 	///< Part of the security schema
	Type_Areastat, 	///< Part of the security schema
	Type_Zonestat,	///< Part of the security schema
	Type_Gatestat, 	///< Part of the security schema
	Type_Areainfo,	///< Part of the security schema
	Type_Zoneinfo, 	///< Part of the security schema
	Type_Gateinfo, 	///< Part of the security schema
	Type_Arealist, 	///< Part of the security schema
	Type_Zonelist 	///< Part of the security schema
} xPL_SchemaTypess;


/// Destination, for a received message here are the possible destination types
typedef enum _xPL_MsgDestinations {
	To_Me = 0,
	To_All,
	To_Other
} xPL_MsgDestinations;


/// Defines a received message
typedef struct {
	xPL_MessageTypes xPL_MessageType;
	xPL_MsgDestinations xPL_MsgDestination;
	xPL_SchemaClasss xPL_SchemaClass;
	xPL_SchemaTypess xPL_SchemaTypes;
	char SourceVendor[9];			///< xPL Vendor name of where the message came from (source)
	char SourceDeviceID[9];			///< xPL Device name from the message source device
	char SourceInstanceID[17];		///< xPL Instance name from the message source device
} xPLReceivedMessages;

extern xPLReceivedMessages xPLReceivedMessage;

extern UDP_SOCKET XPLSocket;
extern NODE_INFO xPLBroadcast; // MainDemo.c

extern char xPLMsgKey[xPLMSGKEYLENGTH];
extern char xPLMsgValue[xPLMSGVALUELENGTH];

extern rom char Default_xPL_My_Instance_ID[];
extern TICK xPLNotConfiguredTimer;

extern APP_CONFIG AppConfig;

//******************************************************************************
/**             P R O T O T Y P E S      ***************************************/
//******************************************************************************

// ################   xPL.c   ################

BOOL xPLInit(void);  				//
void XPLTasks(void); 				// Tasks that need to be completed in a regular manner

void XPLSendHeader(xPL_MessageTypes xPL_MSG_Type, BOOL isBroadCast, xPL_SchemaClasss SchemaClass, xPL_SchemaTypess SchemaType );
// generate the standard xPL message header
// Collect data from UDP socket and determine if the data matches contents of an array

BOOL xPLFindArrayInKVP(rom char **ValidOptions, int ArraySize, int *Result, char * TheValue);
BOOL SaveKVP (char * TheKey, char * TheValue) ;
BOOL xPLLoadEEConfig(void *p, int Start_Address, int Data_Size) ;
void xPLSaveEEConfig(void *p, int Start_Address, int Data_Size);


// ################   SecuritySystem.c   ################
BOOL SchemaInit (void);
BOOL SchemaTasks (void);
void SchemaProcessUDPMsg (void);


// Part of MainDemo.c
void DisplayIPValue(IP_ADDR IPVal);

#endif // __XPLH__

