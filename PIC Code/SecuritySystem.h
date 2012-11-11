#ifndef __SECURITYH__
#define __SECURITYH__


//			****NOTE****
//Change XPLEECONFIGVERSION if updating any of the values below
#define ZONECOUNT 24 			/// The number of zones that are included in the system
#define AREACOUNT 8  			/// The number of areas that are included in the system
#define USERNAMELENGTH 10		/// Descriptive name of the user, visable in xPL messages
#define USERPASSLENGTH 40		/// Maximum lenght of password system wide
#define USERCOUNT 5				/// maximum number of users in the system, first user is always the admin login
#define DEVICEDESCLENGTH 10		/// Maximum number of characters in the Zone and Area description text
#define XPLEECONFIGVERSION 0x60u /// First character of the EEProm config, used to ensure data is valid and we are not reading an old config after system update

// \todo XPLBUFFERSIZE fails at 5, why?
#define XPLBUFFERSIZE 2 		///< the number of xPL messages that can be buffered for transmit

// EOL
#define EOLNORMAL 960 		/// Value of the normal state
#define EOLTRIGGER 1024		/// Value of the triggered state
#define EOLTOLERANCE 50 	/// Tolerance factor, ie not triggered = EOLNORMAL +/- EOLTOLERANCE. Anything outside of normal or triggered is a tamper result

// EE Memory Map
/*
 #define EEDATASIZE_USER (int)USERCOUNT * (int)(USERNAMELENGTH + USERPASSLENGTH)
 #define EEDATASIZE_DEVICEDESC (int)DEVICEDESCLENGTH * (int)(ZONECOUNT + AREACOUNT)
 #define EEDATASTART_CONFIG 0x0000 // EE memory start position for config data (size 51)
 #define EEDATASTART_XPL  sizeof(AppConfig) + 2 //0x0035 // EE memory start position for config data (size 31)
 #define EEDATASTART_ZONE EEDATASTART_XPL + sizeof(xPLConfig) + 2 //0x0056 // EE memory start position for config data (size 80)
 #define EEDATASTART_AREA EEDATASTART_ZONE + sizeof(ZoneConfig) + 2//0x00A8 // EE memory start position for config data(size 40)
 #define EEDATASTART_USER (EEDATASTART_AREA + sizeof(AreaConfig) + 2) // 220   //was 0x00D2  (size 50*5=250)
 #define EEDATASTART_DEVICEDESC ((int)EEDATASTART_USER + (int)EEDATASIZE_USER + 2)  // EE data start position for Device Description text (size 10*(8+20)=280)
 #define EEDATASTART_MPFS ((int)EEDATASTART_DEVICEDESC + (int)EEDATASIZE_DEVICEDESC + 2)
 */

#define EEDATASIZE_USER 250
#define EEDATASIZE_DEVICEDESC 320

#define EEDATASTART_CONFIG 0
#define EEDATASTART_XPL  53
#define EEDATASTART_ZONE 282
#define EEDATASTART_AREA 379
#define EEDATASTART_USER 395
#define EEDATASTART_DEVICEDESC 645
#define EEDATASTART_MPFS 964

// HTTP2 server authentication
#define HTTP_AUTH_ADMIN     (0x00)	/// Access level granted to admin user
#define HTTP_AUTH_OTHER     (0x01)	/// Access to all other pages

// Battery conditions
#define BATTERY_DISCONNECTED 6 		/// Below this voltage the battery is determined as disconnected
#define BATTERY_LOW 12 				/// Below this voltage the battery needs charging


#define GATEWAYCHANGEDDELAY 20		/// Number of seconds delay before sending gateway changed message. Prevents issues with clients flooding network with updates
#define SMTPSMSIZE 4 				/// Number of tasks that can be queued in the SMTP state machine

#define SMTP_TEST_WAITING (0x8002u)	/// this result means we are still waiting for a response
#define SMTP_TEST_DONE (0x8003u)  	/// this result means there is no test under way

#define NTP_TEST_DONE 0				/// Network Time Procotol response
#define NTP_TEST_PROGRESS 1			/// Network Time Procotol response
#define NTP_TEST_COMPLETED 2		/// Network Time Procotol response
#define NTP_TEST_DNSERROR 3			/// Network Time Procotol response
#define NTP_TEST_ARPERROR 4			/// Network Time Procotol response
#define NTP_TEST_TIMEOUT 5			/// Network Time Procotol response

//******************************************************************************
/** Check the TCP/IP stack is configured ************************************/
//******************************************************************************
#if !defined(STACK_USE_SMTP_CLIENT)
    #error Security requires SMTP client
#endif 

#if !defined(STACK_USE_DNS)
    #error Security  needs DNS for resolving SMTP Server names
#endif 

#if !defined(EEPROM_CS_TRIS) && !defined(SPIFLASH_CS_TRIS)
    #error Security requires some where to save its config data to.
#endif 

#if !defined(STACK_USE_HTTP2_SERVER)
    #error Security would love to use the web server
#endif 

#if !defined(STACK_CLIENT_MODE)
    #error Security says stack client have, you must
#endif 


//******************************************************************************
/** D E C L A R A T I O N S **************************************************/
//******************************************************************************


/// Refelcts the possible states of the backup battery
typedef enum _BatteryStates {
	Battery_Disconnected = 0,
	Battery_Charging,
	Battery_Charged,
	Battery_InUse
} BatteryStates;


/// Alarm states, check SendGateStat before changing
/// \todo is Alarming still needed?
typedef enum _Alarm_States {
	Disarmed = 0,
	Alarming, // TODO, is this state still needed?
	Alarming24,
	Alarmed,
	Alarmed24,
	Armed_LatchKey,
	Armed
} Alarm_States;
extern Alarm_States AlarmState;


/// Different zone types supported by xPL
typedef enum _ZoneTypes {
	Perimiter = 0,
	Interior,
	TwentyFourHour
} ZoneTypes;
extern ROM char *ZoneTypesText[]; // determines the text outputed for each of the zone types


/// Different Alarm types that are using in xPL, used in zones and areas
typedef enum _AlarmTypes {
	Buglary =0,
	Fire,
	Flood,
	Gas,
	Silent,
	Duress,
	Other
} AlarmTypes;


extern ROM char *AlarmStatesText[]; 			// Text for the alarm state
extern ROM char *AlarmTypesText[];  			// determines the text outputed each of the alarm types
extern char UserID[USERNAMELENGTH];				// name of the user the performed the last action (arm or diarm)
extern int iCount, i2Count;						// Counters that are used in a number of places, centraly defined to reduce memory use
extern char RejectedPassword[USERPASSLENGTH];	// Hold the last rejected password. Used in the user web page for RFID tags
extern unsigned int LastZoneAlarmed;			// The last zone to change to alarmed state.  used in SMTP.c
extern BatteryStates BatteryState;				// Defines the state of the battery
extern unsigned int VoltageBattery; 			// Voltage of the battery
extern unsigned int VoltageSupply;				// Voltage of the power supply (12v)

/// When checking the zone state in EOL, these are the possible results
typedef enum _Zone_States {
	StateNothing = 0,		///< Nothing has changed since the last check
	StateNormal,			///< Since last check the zone has changed to normal
	StateTrigger,			///< Since last check the zone has changes to Triggered
	StateTamper,			///< Since last check the zone has changes to Tampered
	StateBypass,			///< In bypassed state
	StateIsolate,			///< In Isolated state
	StateAlarm
} Zone_States;


/// Stores all of the configuration for ech of the zones
typedef struct {
	unsigned int IsTriggered:1;			///< If the zone in a triggered state
	unsigned int IsNO:1;				///< Is the zone Normally Open (fales = Normally Closed)
	unsigned int IsEntry:1;				///< Is this zone an entry and exit point
	unsigned int IsIsolated:1;			///< Is zone isolated
	unsigned int IsBypassed:1;			///< Is zone Bypassed
	unsigned int IsArmed:1;				///< Is Zone Armed
	unsigned int IsAlarmed:1;			///< Did the zone cause the alarm to go off
	unsigned int IsChangeFlagged:1;		///< Used for state changes
	unsigned int IsTampered:1;			///< Is the zone in a tampered state
	Zone_States State;					///< Current state of the zone
	ZoneTypes ZoneType:3;				///< xPL Type of zone (perimeter, interior or 24 hour)
	AlarmTypes ZoneAlarmType:4;			///< xPL Alarm type (eg fire, flood, gas etc)
	unsigned short int AreaList:AREACOUNT;		///< Bit field list of the areas that this zone is a member of
	unsigned int BounceDelay:8;			///< Timer for reducing impact of a bouncing IO lines
} ZoneConfigs;
extern ZoneConfigs ZoneConfig[ZONECOUNT];


/// Stores each Area configuration information
typedef struct {
	unsigned int IsTriggered:1;			///< Is the Area triggered
	unsigned int IsIsolated:1;			///< Is area isolated
	unsigned int IsBypassed:1;			///< Is area Bypassed
	unsigned int IsArmed:1;				///< Is area Armed
	unsigned int IsAlarmed:1;			///< Did a zone in this area cause the alarm to go off
	unsigned int IsChangeFlagged:1;		///< Used for state changes
	unsigned int IsTampered:1;			///< Has a zone in this area been tampered
	Zone_States State;					///< The current state of the area
	AlarmTypes ZoneAlarmType:4;			///< xPL Alarm type (eg fire, flood, gas etc)
} AreaConfigs;

extern AreaConfigs AreaConfig[AREACOUNT];


/// General system Configuration entries
typedef struct {
	unsigned int xPL_Heartbeat_Interval;	///< Interval of the xPL heatbeat message in seconds
	char xPL_My_Instance_ID[18];			///< xPL instance name of this device
	short int ExitDelay;					///< Number of seconds you have to exit from an entry/exit zone
	short int EntryDelay;					///< Number of seconds you have to disarm alarm from an entry/exit zone before alarm goes off
	short int AlarmingDuration;				///< Number of seconds that the alarm will ring for
	short int Alarming24Duration;			///< Number of seconds that the alarm will ring for a 24 hour alarm type
	short int SirenDelay24h;				///< Number of seconds before the external siren is activated for 24h alarm type
	short int SirenDelay;					///< Number of seconds before the external siren is activated
	char SMTP_To[80];						///< Email address to send messages to
	char SMTP_Server[20];					///< POP3 server
	char SMTP_User[40];						///< POP3 User name
	char SMTP_Password[20];					///< POP3 Password
	int SMTP_Port;							///< Port number
	unsigned int Use_AES:1;					///< Use AES encryption on the user feild in the xPL message
	//unsigned int SMTP_UseSSL:1;			// Use SSL for sending email. Removed due to application size issues (sigh)
	unsigned int SMTP_Active:1;				///< Disable emails
	unsigned int SMTP_OnlyInitalAlarm:1;	///< Only send emails on the inital alarm
	unsigned int SMTP_ArmDisarm:1;			///< Send emails on all arm and disarm events
	char NTP_Server[35];					///< NTP Server name
} xPLConfigs;

extern xPLConfigs xPLConfig;




// ################## Task Buffer ##################

/// The type of task that has been buffered for processing
typedef enum _TaskBuffers {
	TB_Empty = 0,					///< Task is empty
	TB_SendConfigCurrent,
	TB_SendConfigList,
	TB_ArmFailed,
	TB_SendArmed,
	TB_SendDisarm,
	TB_Panic,
	TB_SendGatewayInfo,
	TB_SendZoneList,
	TB_SendAreaList,
	TB_SendZoneInfo,
	TB_SendAreaInfo,
	TB_Isolate,
	TB_Bypass,
	TB_Enable,
	TB_SendAreaStateMsg,
	TB_SendInvalidUser,
	TB_SendInvalidUserTimestamp,
	TB_SendGatewayChanged,
	TB_SendError,
	TB_SendGateStat,
	TB_SendAreaStat,
	TB_SendZoneStat,
	TB_SendBatteryStat,
	TB_SendACStat
} TaskBuffers;


/// Defines a task in the xPL messages to be transmitted task buffer
typedef struct {
	TaskBuffers TaskBuffered;				///< Type of task
	rom char * ErrorMessage;				///< Error message passed to SendError
	unsigned int FlaggedZones[ZONECOUNT]; 	///< An array of the Zones that have been flagged for this message
	unsigned int FlaggedAreas[AREACOUNT]; 	///< An array of the Areas that have been flagged for this message
	Zone_States NewZoneState;				///< Used by SendAreaStateMsg to know what changes have been made			
	int TaskState;							///< 0 means completed, > 0 means more things to do. Value > 0 can be used by task to track state
} xPLTasks;


/// Task buffer for sending xPL messages
typedef struct {
	unsigned int CurentTask;				///< End up being the last task that was processed
	unsigned int FreePosition;				///< Task number that is free to create a new task in
	int PositionCounter;					///< Keeps track of what position we were in so it can return back to the next spot. Ie cycle through all zone and send multiple messages (SendAreaStateMsg)
	TICK GatewayChangedTimer;				///< Provides a delay in sending the xPL gateway changed message
	xPLTasks Task[XPLBUFFERSIZE];			///< an array of the tasks, see SMTPTaskStack
} xPLTaskLists;

/// The Task buffer for sending xPL messages
extern xPLTaskLists xPLTaskList;

//---------- SMTP Email task buffer ----------------

/// Type of email message to be sent
typedef enum _SMTP_SendTypes {
	SMTP_Test =0,
	SMTP_Arm,
	SMTP_Disarm,
	SMTP_Alarmed,
	SMTP_Reset,
	SMTP_InternetRestored
} SMTP_SendTypes;

/// State of the email stack
typedef enum _SMTP_States {
	SMTP_SM_InQ =0,
	SMTP_SM_Working,
	SMTP_SM_Done
} SMTP_States;

/// Task buffer for queuing up emails to be sent
typedef struct {
	SMTP_SendTypes SMTP_SendType;		///< Type of email to be sent
	SMTP_States SMTP_State;				///< Current state of the task (State Machine)
} SMTPTaskStack;

/// Overall Task buffer, mostly accessed to create a new tasks
typedef struct {
	unsigned int CurentTask;			///< Task number being processed by the stack at the moment
	unsigned int FreePosition;			///< Task number that is free to create a new task in
	SMTPTaskStack Task[SMTPSMSIZE];		///< an array of the tasks, see SMTPTaskStack
} SMTPTaskLists;

extern SMTPTaskLists SMTPTaskList;
extern int SMPTTestResult; 				///< used for testing email


/// state machine for NTP process, see SNTP Customised.c
typedef enum {
	SM_HOME = 0,
	SM_NAME_RESOLVE,
	SM_ARP_START_RESOLVE,
	SM_ARP_RESOLVE,
	SM_ARP_START_RESOLVE2,
	SM_ARP_RESOLVE2,
	SM_ARP_START_RESOLVE3,
	SM_ARP_RESOLVE3,
	SM_ARP_RESOLVE_FAIL,
	SM_UDP_SEND,
	SM_UDP_RECV,
	SM_SHORT_WAIT,
	SM_WAIT
} SNTPStates;
extern SNTPStates SNTPState;
extern int NTPTestResponse; // stores the response from a SNTP test

//******************************************************************************
//**  P R O T O T Y P E S ******************************************************
//******************************************************************************


void xPLFactoryRestart(void);
void ClearArmingFlags (void);
void SetArming(char * TheValue, BOOL IsZone);
void Disarm (void);
void IsolateZone (int TheZone);
void IsolateArea (int TheArea);
void bypassZone (int TheZone);
void bypassArea (int TheArea);
void EnableZone (int TheZone);
void EnableArea (int TheArea);

void Arm (int short TimeDelay) ;
void ArmHome (int short TimeDelay) ;
void ArmAway (int short TimeDelay) ;
void ArmLatchkey (int short TimeDelay) ;
void Panic (void) ;

// SecuritySystem.c
BOOL isZoneMemberOfArea(int ZoneID, int AreaID);

// Authenticate.c
void ModifyUser (BYTE *ptr);
BOOL Authenticate (char * Password);

// HTTPSecurity.c
int DecodeIntFromGET(BYTE *GETptr, rom char * DebugMesg) ;

//SecurityXPLSender.c  The sending of messages
int addxPLMessage (TaskBuffers TaskBuffered);
void SendAreaInfo (int TheArea);
void SendZoneInfo (int TheZone);
void SendAreaList (void);
void SendZoneList (void);
void SendDisarm (void);
void SendError (rom char * ErrorMessage);
void SendArmed (void);
void SendGatewayInfo (void);
void SendAlarmingMsg (int TheZone);
int  SendAreaStateMsg (Zone_States CurrentZoneState, int TheZone, int StartPoint);
void SendZoneStateMsg (Zone_States MessageType, int TheZone);
void SendConfigList (void) ;
void SendConfigCurrent (void) ;
void SendIPValue(IP_ADDR IPVal) ;
void SendGatewayChanged (void) ;
void SendGateStat (void) ;
void SendZoneStat (int TheZone) ;
void SendAreaStat (int TheArea) ;

// SMTP.c
HTTP_IO_RESULT HTTPPostEmail(void) ;
HTTP_IO_RESULT HTTPPostSMTPConfig(void) ;
void SMTPTaskProcess(void) ;
void SMTPQueueEmail (SMTP_SendTypes Email_Type) ;

// Battery.c
void BatteryTask(void) ;

// EOL.c
Zone_States IsZoneTriggered (int ZoneID);

#endif // __SECURITYH__
