///@file SecuritySystem.c
///@brief Main of security 

#include "TCPIP Stack/TCPIP.h"
#include "xPL.h"
#include "SecuritySystem.h"


//******************************************************************************
/** D E C L A R A T I O N S **************************************************/
//******************************************************************************

/*    D A T A B A N K   1     */
#pragma udata xPLvars1

/// Reflects the state of the alarm
Alarm_States AlarmState;
/// The previous state of the alarm system, used when state change fails
Alarm_States PreviousAlarmState;
/// Zone Entery timer
TICK EntryTimer;
/// Triggered Entery Zone that set off the timer
TICK TriggeredEntryZone;
/// Exit timer - 0 means off
TICK ExitTimer;
/// Delay used so we are not always checking the status of the zones
TICK NextZoneCheckDelay;
/// Times how long the alarm will sound for
TICK AlarmingTimer;
/// Times the delay before acivating the external siren
TICK SirenTimer;
/// Timer checking the battery status
TICK BatteryCheckTimer = 0;
/// Counters that are used in a number of places, centraly defined to reduce memory use
int iCount, i2Count;
/// Name of user
char UserID[USERNAMELENGTH];
/// The last zone to change to alarmed state.  used in SMTP.c
unsigned int LastZoneAlarmed;



/// determines the text outputed in the xPL message for each of the alarm types
ROM char *AlarmTypesText[] =
{
	"buglary",
	"fire",
	"flood",
	"gas",
	"silent",
	"duress",
	"other",
	""
};


/// determines the text outputed in the xpl message for each of the zone types
ROM char *ZoneTypesText[] =
{
	"perimeter",
	"interior",
	"24hour",
	""
};


/// Security command Types
static ROM char *SecurityCommandText[] =
{
	"arm",
	"arm-home",
	"arm-away",
	"arm-latchkey",
	"disarm",
	"panic",
	"isolate",
	"bypass",
	"enable"
};

/// Types of security commands received
typedef enum _SecurityCommands {
	Command_arm = 0,
	Command_armhome,
	Command_armaway,
	Command_armlatchkey,
	Command_disarm,
	Command_panic,
	Command_isolate,
	Command_bypass,
	Command_enable
} SecurityCommands;


/// Types of security requests received
typedef enum _SecurityRequests {
	Request_gateinfo = 0,
	Request_zonelist,
	Request_arealist,
	Request_zoneinfo,
	Request_areainfo,
	Request_gatestat,
	Request_zonestat,
	Request_areastat
} SecurityRequests;


/// xPL feild values for the different security requests
static ROM char *SecurityRequestsText[] =
{
	"gateinfo",
	"zonelist",
	"arealist",
	"zoneinfo",
	"areainfo",
	"gatestat",
	"zonestat",
	"areastat"
};

extern DWORD dwLastUpdateTick; // SNTP Customised.c used in NTPLastUpdate


/*    D A T A B A N K   2     */
#pragma udata xPLvars2

/// The Task buffer for sending xPL messages
xPLTaskLists xPLTaskList;


/*    D A T A B A N K   3     */
#pragma udata xPLvars3


/// Configuration entries
xPLConfigs xPLConfig;


/*    D A T A B A N K   4     */
#pragma udata xPLvars4

/// Stores each Zones configuration information
ZoneConfigs ZoneConfig[ZONECOUNT];

/*    D A T A B A N K   5     */
#pragma udata xPLvars5

/// Stores each Area configuration information
AreaConfigs AreaConfig[AREACOUNT];



//******************************************************************************
/** P R I V A T E  P R O T O T Y P E S ***************************************/
//******************************************************************************


static void LoadDefaultConfig (void);
static BOOL IOTasks(void) ;
static void StateChangeToArmFailed (void);
static void StateChangeToArm (BOOL isLatchKey);
static void TaskAlarmed (void);
static void TaskAlarming (void);
static void StateChangeToAlarmed (void);
static void StateChangeToAlarming (int ZoneID);

static Zone_States ArmingZoneState (int TheZone);



#pragma code
/*********************************************************************
 *********************************************************************
 Task related
 *********************************************************************
 ********************************************************************/



//*********************************************************************
/**
 Function:        SchemaInit
 PreCondition:    UDP initalised, XPL initalised
 Side Effects:    None
 Overview:
 General initalisiation procedure for any xPL schema(s).
 Set inital variables and system state after startup. Assumes TCIP is available and xPL configured.
 Returns FALSE if the default config options were loaded

 @param           None
 @result          BOOL, is the device configured
 */
BOOL SchemaInit (void) {

	BOOL ReturnResult = TRUE;
	char TempVal[4];
	BYTE * ptr;


	//Load configuration data
   	if (xPLLoadEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig)) == FALSE) {
	   	//EE data incorrect
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" # NOTE# ZoneConfig Loaded default Security Settings\r\n");
#endif
		ReturnResult = FALSE;
	}


 	if (xPLLoadEEConfig(&xPLConfig, EEDATASTART_XPL, sizeof(xPLConfig)) == FALSE) {
	   	//EE data incorrect
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" # NOTE# xPLConfig Loaded default Security Settings\r\n");
#endif
		ReturnResult = FALSE;
	}


   	if (xPLLoadEEConfig(AreaConfig, EEDATASTART_AREA, sizeof(AreaConfig)) == FALSE) {
		//EE data incorrect
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" # NOTE# AreaConfig Loaded default Security Settings\r\n");
#endif
		ReturnResult = FALSE;
	}

	if (ReturnResult == FALSE) LoadDefaultConfig();

#if defined(DEBUG_UART)

	ultoa ((unsigned int) sizeof(AppConfig), TempVal);
	putrsUART((ROM char*)"IP AppConfig  size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_CONFIG, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_XPL - (unsigned int)EEDATASTART_CONFIG, TempVal);
	putsUART(TempVal);

	putrsUART((ROM char*)"\r\n");

	ultoa ((unsigned int) sizeof(xPLConfig), TempVal);
	putrsUART((ROM char*)"Security Schema xPLConfig size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_XPL, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_ZONE - (unsigned int)EEDATASTART_XPL, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)"\r\n");

	ultoa ((unsigned int) sizeof(ZoneConfig), TempVal);
	putrsUART((ROM char*)"Security Schema ZoneConfig size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_ZONE, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_AREA - (unsigned int)EEDATASTART_ZONE, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)"\r\n");

	ultoa ((unsigned int) sizeof(AreaConfig), TempVal);
	putrsUART((ROM char*)"Security Schema AreaConfig size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_AREA, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_USER - (unsigned int)EEDATASTART_AREA, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)"\r\n");


	ultoa ((unsigned int) USERCOUNT * (USERNAMELENGTH + USERPASSLENGTH) , TempVal);
	putrsUART((ROM char*)"User size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_USER, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_DEVICEDESC - (unsigned int)EEDATASTART_USER, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)"\r\n");


	ultoa ((unsigned int) DEVICEDESCLENGTH * (ZONECOUNT + AREACOUNT) , TempVal);
	putrsUART((ROM char*)"Device desc size: ");
	putsUART(TempVal);
	putrsUART((ROM char*)" EEStarts:");
	ultoa ((unsigned int) EEDATASTART_DEVICEDESC, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)" EESize: ");
	ultoa ((unsigned int) EEDATASTART_MPFS - (unsigned int)EEDATASTART_DEVICEDESC, TempVal);
	putsUART(TempVal);
	putrsUART((ROM char*)"\r\n");


	//EEDATASTART_MPFS



#endif


	//Clear timers
	EntryTimer = 0;
	TriggeredEntryZone = 0;
	ExitTimer = 0;
	NextZoneCheckDelay = TickGet() + (TICK_SECOND / ZONECHECKDELAY);
	xPLTaskList.GatewayChangedTimer = 0;
	dwLastUpdateTick =0;


	//Set initial alarm state, arming delay and previous arm state
	AlarmState = Disarmed;
	PreviousAlarmState = Disarmed;
	LastZoneAlarmed = 0;
	BatteryState = Battery_Disconnected;
	memset(RejectedPassword, '\0', USERPASSLENGTH);
	NTPTestResponse = NTP_TEST_DONE; // result of NTP test

	//initalise hardware
	// outputs
	STROBE_TRIS = 0;
	STROBE_IO = 0;
	EXT_SIREN_TRIS = 0;
	EXT_SIREN_IO = 0;
	INT_SIREN_TRIS = 0;
	INT_SIREN_IO = 0;
	SLA_CHARGER_TRIS = 0;
	SLA_CHARGER_IO = 0;

	// Analogue Demulplexer
	PLEXERA_0_TRIS = 0;
	PLEXERA_1_TRIS = 0;
	PLEXERA_2_TRIS = 0;
	PLEXERA_S_TRIS = 0;

	PLEXERA_0_IO = 0;
	PLEXERA_1_IO = 0;
	PLEXERA_2_IO = 0;


	//inputs
	SUPPLY_ACTIVE_TRIS = 1;



	// SMTP state machine, send out an email stating device has been restarted

	SMTPTaskList.CurentTask = 0;
	SMTPTaskList.FreePosition = 1;
	SMTPTaskList.Task[0].SMTP_SendType = SMTP_Reset;
	SMTPTaskList.Task[0].SMTP_State = SMTP_SM_InQ;
	SMPTTestResult = SMTP_TEST_DONE;

	for (iCount = 1; iCount < SMTPSMSIZE; iCount++) {
		SMTPTaskList.Task[iCount].SMTP_SendType = SMTP_Test;
		SMTPTaskList.Task[iCount].SMTP_State = SMTP_SM_Done;
	}

	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsTriggered = FALSE;
		ZoneConfig[iCount].IsArmed = FALSE;
		ZoneConfig[iCount].IsAlarmed = FALSE;
		ZoneConfig[iCount].IsChangeFlagged = FALSE;
		ZoneConfig[iCount].BounceDelay = 0;
		ZoneConfig[iCount].IsTampered = FALSE;
		ZoneConfig[iCount].IsIsolated = FALSE;	// should not continue state after a restart
	} // for

	for (iCount = 0; iCount < AREACOUNT; iCount++) {
		AreaConfig[iCount].IsTriggered = FALSE;
		AreaConfig[iCount].IsArmed = FALSE;
		AreaConfig[iCount].IsAlarmed = FALSE;
		AreaConfig[iCount].IsChangeFlagged = FALSE;
		AreaConfig[iCount].IsIsolated = FALSE; // should not continue state after a restart
		AreaConfig[iCount].IsTampered = FALSE;
	} //for

	// xPL task buffer
	xPLTaskList.FreePosition = 0;
	xPLTaskList.CurentTask =0;

	// Set the TRIS flags to input for all of the A/D pins, page 331 chapter 21.0 of data sheet
	AN2_TRIS = 1;
	AN3_TRIS = 1;
 	AN4_TRIS = 1;

	// Setup the A/D channels, page 329 of data sheet
	ADCON0 = 0b00001001;	// ADON = On(1), GO/DONE = Idle (0), AN1 selected (1000)
	ADCON1 = 0b00001010;	// upto AN4 enabled (1010), VCFG0 (Voltage ref) = Vdd/Vss (00)
	ADCON2 = 0b10111110;	// Fosc/64 (~21.0kHz) (110), 20TAD acquisition time (111), Right justify (0)

	// Do a calibration A/D conversion
	ADCON0bits.ADCAL = 1;
    ADCON0bits.GO = 1;
	while(ADCON0bits.GO);
	ADCON0bits.ADCAL = 0;



	//Queue reset email
	if (xPLConfig.SMTP_Active == TRUE)
		SMTPQueueEmail (SMTP_Reset);

	//Send gateway message
	SendGatewayReady();

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Security Schema initalised, Gateway message sent.\r\n");
#endif

	return ReturnResult;

}

//*********************************************************************
/**
 Function        LoadDefaultConfig
 PreCondition:    None
 Side Effects:    Populates ZoneConfig and AreaConfig
 Overview:
 Override any values on ZoneConfig and AreaConfig with default values
 @param           None
 @result          None

*/

void LoadDefaultConfig (void) {


	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsTriggered = FALSE;
		ZoneConfig[iCount].IsNO = TRUE;
		ZoneConfig[iCount].IsEntry = FALSE;
		ZoneConfig[iCount].IsIsolated = FALSE;
		ZoneConfig[iCount].IsBypassed = FALSE;
		ZoneConfig[iCount].IsArmed = FALSE;
		ZoneConfig[iCount].IsAlarmed = FALSE;
		ZoneConfig[iCount].IsChangeFlagged = FALSE;
		ZoneConfig[iCount].ZoneType = Perimiter;
		ZoneConfig[iCount].ZoneAlarmType = Buglary;
		ZoneConfig[iCount].AreaList = 0;
		ZoneConfig[iCount].BounceDelay = 0;
	} // for

	for (iCount = 0; iCount < AREACOUNT; iCount++) {
		AreaConfig[iCount].IsTriggered = FALSE;
		AreaConfig[iCount].IsTriggered = FALSE;
		AreaConfig[iCount].IsIsolated = FALSE;
		AreaConfig[iCount].IsBypassed = FALSE;
		AreaConfig[iCount].IsArmed = FALSE;
		AreaConfig[iCount].IsAlarmed = FALSE;
		AreaConfig[iCount].IsChangeFlagged = FALSE;
		AreaConfig[iCount].ZoneAlarmType = Buglary;
	} //for

	ZoneConfig[0].AreaList = 0b11111111;
	ZoneConfig[1].AreaList = 0b00000010;
	ZoneConfig[2].AreaList = 0b00000100;
	ZoneConfig[3].AreaList = 0b00001000;
	ZoneConfig[4].AreaList = 0b00010000;
	ZoneConfig[5].AreaList = 0b00100000;
	ZoneConfig[6].AreaList = 0b01000000;
	ZoneConfig[7].AreaList = 0b10000000;

	for (iCount = 6; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsBypassed = TRUE;
	}



	xPLConfig.Alarming24Duration = 18;
	xPLConfig.ExitDelay = 5;
	xPLConfig.EntryDelay = 7;
	xPLConfig.AlarmingDuration = 15;
	xPLConfig.SirenDelay = 10;
	xPLConfig.SirenDelay24h = 12;
	memset(xPLConfig.NTP_Server, '\0',sizeof(xPLConfig.NTP_Server));
	memcpypgm2ram(xPLConfig.NTP_Server, (ROM void*)"pool.ntp.org", 12);
	xPLConfig.Use_AES = FALSE;


	// Email
	memset(xPLConfig.SMTP_Server,'\0', sizeof(xPLConfig.SMTP_Server));
	memset(xPLConfig.SMTP_To,'\0', sizeof(xPLConfig.SMTP_To));
	memset(xPLConfig.SMTP_User,'\0', sizeof(xPLConfig.SMTP_User));
	memset(xPLConfig.SMTP_Password,'\0', sizeof(xPLConfig.SMTP_Password));
	xPLConfig.SMTP_Port = 21;
	//xPLConfig.SMTP_UseSSL = FALSE;
	xPLConfig.SMTP_Active = FALSE;
	xPLConfig.SMTP_ArmDisarm = TRUE;
	xPLConfig.SMTP_OnlyInitalAlarm = TRUE;



} //LoadDefaultConfig


//********************************************************************
/**
 Function        xPLFactoryRestart
 PreCondition:    None
 Side Effects:    Kills the starting character of the EEProm config data
 Overview:
 Performs a factory restart of the device, clearing everything
 @param           None
 @result          None

*/
void xPLFactoryRestart(void) {
	char TempVal[9];

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"##### Factory restart in progress #####\r\n");
#endif

	// I dont think this will work as we do not call the StackTasks to send the message

	// if we have a good socket send the message, if not then do not bother
	if (UDPIsPutReady(XPLSocket)) {
		// Send header
		XPLSendHeader(xPL_STATUS, TRUE,Class_HBeat,Type_END);
		UDPPutROMString((ROM BYTE*) "interval=0\n}\n");
		// Now transmit it.
		UDPFlush();
	}


    XEEBeginWrite(EEDATASTART_ZONE); // Zone config
    XEEWrite(0x00); // 0x00 will cause the xPLLoadEEConfig function to fail and revert to default settings.
    XEEEndWrite();

    XEEBeginWrite(EEDATASTART_AREA); // area config
    XEEWrite(0x00); // 0x00 will cause the xPLLoadEEConfig function to fail and revert to default settings.
    XEEEndWrite();

    XEEBeginWrite(EEDATASTART_XPL); // device config
    XEEWrite(0x00); // 0x00 will cause the xPLLoadEEConfig function to fail and revert to default settings.
    XEEEndWrite();

    XEEBeginWrite(EEDATASTART_CONFIG); // IP address, this is a part of the TCIP stack
    XEEWrite(0x00); // 0x00 will cause the xPLLoadEEConfig function to fail and revert to default settings.
    XEEEndWrite();

    while(XEEIsBusy());

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Clearing config data\r\n");
#endif

	// Blank everything
    for (iCount = EEDATASTART_XPL; iCount < (MPFS_RESERVE_BLOCK - 2); iCount++) {

   		XEEBeginWrite(iCount);
	 	XEEWrite('.');
    	XEEEndWrite();
	   	while(XEEIsBusy());
    }

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"All config data has been deleted. \r\n");
	putrsUART((ROM char*)"Restarting device.............\r\n");
#endif


    // perform a Hardware restart
	Reset();

}

//********************************************************************
/**
 Function        SchemaTasks
 PreCondition:    None
 Side Effects:    None
 Overview:
 General task manager for any implemented xPL Schema(s)
 
 \todo add a timer that prevents the xPL task from blocking access to IOTasks();
 
 @param           None
 @result          True if no task has ran

*/

BOOL SchemaTasks(void) {

	BOOL ReturnResult;
	int iCounter, DeviceSelected, CurrentTask;

	// If there are no tasks in queue check to see if the battery state should be checked
	if (xPLTaskList.FreePosition == xPLTaskList.CurentTask) {
		if (BatteryCheckTimer < TickGet()) {
			BatteryCheckTimer = TickGet() + TICK_SECOND;
			BatteryTask();
		}
	}


	// Is there a task to work on?
	if (xPLTaskList.FreePosition == xPLTaskList.CurentTask) {
		// as there are no messages to be sent, check the IOTasks to see if the state of the zones has changed
		ReturnResult = IOTasks();
		return ReturnResult;  // Returns is a message was sent by the IOTasks
		
		// todo add a timer that prevents the xPL task from blocking access to IOTasks();
	}

	// If the current task has not completed make sure it has the chance to run again.
	if (xPLTaskList.Task[xPLTaskList.CurentTask].TaskState == 0) {
		// updates the current Task, remember the current task will end up being the last task processed once this has run
		if (xPLTaskList.CurentTask < (XPLBUFFERSIZE-1))
			xPLTaskList.CurentTask++;
		else if(xPLTaskList.CurentTask >= (XPLBUFFERSIZE-1))
			xPLTaskList.CurentTask =0;
	}
	CurrentTask = xPLTaskList.CurentTask;	

	// populate IsChangeFlagged from the zone array for the task to be processed
	for(iCounter =0; iCounter < ZONECOUNT; iCounter++)
		 ZoneConfig[iCounter].IsChangeFlagged = xPLTaskList.Task[xPLTaskList.CurentTask].FlaggedZones[iCounter];

	// populate IsChangeFlagged from the area array for the task to be processed
		for(iCounter =0; iCounter <= AREACOUNT; iCounter++)
			AreaConfig[iCounter].IsChangeFlagged = xPLTaskList.Task[xPLTaskList.CurentTask].FlaggedAreas[iCounter];

	switch (xPLTaskList.Task[xPLTaskList.CurentTask].TaskBuffered) {

		case TB_SendConfigList:
			SendConfigList(); //very simple
			break;

		case TB_ArmFailed:
			StateChangeToArmFailed(); // needs zones or areas populated
			break;

		case TB_SendArmed:
			SendArmed(); // extracts data from the zones & area directly
			if(xPLConfig.SMTP_ArmDisarm == TRUE)
				SMTPQueueEmail (SMTP_Arm); // queue an email notification
			break;

		case TB_SendDisarm:
			SendDisarm();
			if(xPLConfig.SMTP_ArmDisarm == TRUE)
				SMTPQueueEmail (SMTP_Disarm);
			break;

		case TB_Panic:
			SendAlarmingMsg(99);
			SMTPQueueEmail (SMTP_Alarmed);
			break;

		case TB_SendGatewayInfo:
			SendGatewayInfo();
			break;

		case TB_SendZoneList:
			SendZoneList();
			break;

		case TB_SendAreaList:
			SendAreaList();
			break;

		case TB_SendGateStat:
			SendGateStat();
			break;

		case TB_SendAreaStat:
			for(iCounter =0; iCounter < AREACOUNT; iCounter++)
				if (xPLTaskList.Task[CurrentTask].FlaggedAreas[iCounter] == 1)
					DeviceSelected = iCounter;
			SendAreaStat (DeviceSelected);
			break;

		case TB_SendZoneStat:
			for(iCounter =0; iCounter < ZONECOUNT; iCounter++)
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			SendZoneStat (DeviceSelected);
			break;

		case TB_SendZoneInfo:
			for (iCount = 0; iCount < ZONECOUNT; iCount++) 
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			SendZoneInfo(DeviceSelected);
			break;

		case TB_SendAreaInfo:
			for(iCounter =0; iCounter < AREACOUNT; iCounter++)
				if (xPLTaskList.Task[CurrentTask].FlaggedAreas[iCounter] == 1)
					DeviceSelected = iCounter;
			SendAreaInfo(DeviceSelected);
			break;

		case TB_Isolate:
			for (iCount = 0; iCount < ZONECOUNT; iCount++) 
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			IsolateZone(DeviceSelected);
			break;
			
		case TB_Bypass:
			for (iCount = 0; iCount < ZONECOUNT; iCount++) 
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			bypassZone(DeviceSelected);
			break;
			
		case TB_Enable:
			for (iCount = 0; iCount < ZONECOUNT; iCount++) 
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			EnableZone(DeviceSelected);
			break;

	
		case TB_SendAreaStateMsg:
			for (iCount = 0; iCount < ZONECOUNT; iCount++) 
				if (xPLTaskList.Task[CurrentTask].FlaggedZones[iCounter] == 1)
					DeviceSelected = iCounter;
			xPLTaskList.Task[CurrentTask].TaskState = SendAreaStateMsg (xPLTaskList.Task[CurrentTask].NewZoneState, DeviceSelected, xPLTaskList.Task[CurrentTask].TaskState);
			break;

		case TB_SendInvalidUser:
			SendError( (ROM char *)"user invalid");
			break;

		case TB_SendInvalidUserTimestamp:
			SendError( (ROM char *)"Timestamp out of range. Check system time");
			break;

		case TB_SendGatewayChanged:
			SendGatewayChanged();
			xPLTaskList.GatewayChangedTimer = 0;
			break;

		case TB_SendError:
			SendError(xPLTaskList.Task[CurrentTask].ErrorMessage);
			break;

		case TB_SendBatteryStat:
			SendBatteryStat();
			break;

		case TB_SendACStat:
			SendACStat();
			break;


	} // switch


	return ReturnResult;

} //SchemaTasks


//********************************************************************
/**
 Function        IOTasks
 PreCondition:    None
 Side Effects:    Zone and Area states, sends UDP
 Overview:
 This procedure moniters the statis of zones and areas, sending triggers or alarms of a state changes
 It also calls the functions that control the current alarm state (alarming / alarmed)

 \todo  when exiting from sending a message need to return to next position. Low task

 @result BOOL, no UDP message sent (false means message sent)

*/
BOOL IOTasks (void) {

	int CheckingZone, xPLTaskID;
	Zone_States ZoneResult, PreviousZoneState;


	//If EntryTimer set and state is Disarmed, clear entry timer
	if ((EntryTimer != (unsigned)0) && (AlarmState == Disarmed)) EntryTimer = 0;

	//If entry timer has expired then Change to Alarming and sent ZoneID of the zone that triggered the entry timer
	if ((TickGet() > EntryTimer) && (EntryTimer != (unsigned)0) )   {
		StateChangeToAlarming(TriggeredEntryZone);
		return FALSE;
	}

	//Loop though all Zones's only when needed to
	if (TickGet() > NextZoneCheckDelay) {

		for (CheckingZone = 0; CheckingZone < ZONECOUNT; CheckingZone++) {

			// What is the previous state
			if (ZoneConfig[CheckingZone].IsTampered) PreviousZoneState = StateTamper;
			else if (ZoneConfig[CheckingZone].IsTriggered) PreviousZoneState = StateTrigger;
			else PreviousZoneState = StateNormal;

			// determine the current state
			ZoneResult = IsZoneTriggered(CheckingZone);

			//If the result is nothing then do nothing
			if (ZoneResult == StateNothing)
				goto EndLoop;

			//If returning from Tamper send ZoneState and AreaState messages
			if ((ZoneResult == StateNormal) && (PreviousZoneState ==  StateTamper)){
				SendZoneStateMsg(StateNormal,CheckingZone);
				xPLTaskID = addxPLMessage (TB_SendAreaStateMsg);
				xPLTaskList.Task[xPLTaskID].NewZoneState = StateNormal;
				return FALSE;
			}

			//If Tamper send ZoneState and AreaState messages and sound alarm
			if (ZoneResult == StateTamper) {
				// set the alarm off when any zone is tampered
				StateChangeToAlarming(CheckingZone);
				SendZoneStateMsg(StateTamper,CheckingZone);
				xPLTaskID = addxPLMessage (TB_SendAreaStateMsg); 
				xPLTaskList.Task[xPLTaskID].NewZoneState = StateTamper;
				return FALSE;
			}


			//If returning from triggered send ZoneState and AreaState messages
			if ((ZoneResult == StateNormal) && (PreviousZoneState ==  StateTrigger)){
				SendZoneStateMsg(StateNormal,CheckingZone);
				xPLTaskID = addxPLMessage (TB_SendAreaStateMsg);
				xPLTaskList.Task[xPLTaskID].NewZoneState = StateNormal; 
				return FALSE;
			}

			//If triggered and 24H, set the alarm off
			if ((ZoneResult == StateTrigger) && (ZoneConfig[CheckingZone].ZoneType == TwentyFourHour)) {
				StateChangeToAlarming(CheckingZone);
				return FALSE;
			}

			//If Triggered and not Armed send trigger message
			if ((ZoneResult == StateTrigger) && (!ZoneConfig[CheckingZone].IsArmed)) {
				SendZoneStateMsg(StateTrigger,CheckingZone);
				xPLTaskID = addxPLMessage (TB_SendAreaStateMsg);
				xPLTaskList.Task[xPLTaskID].NewZoneState = StateTrigger;
				return FALSE;
			}


			//If Triggered and Zone is Armed
			if ((ZoneResult == StateTrigger) && (ZoneConfig[CheckingZone].IsArmed)) {
				// If the zone is an entry point and Exit time is not Zero, set (Triggered Entery Zone) = ZoneID and send trigger messages
				if ((ExitTimer != (unsigned)0) && (ZoneConfig[CheckingZone].IsEntry)) {
					SendZoneStateMsg (StateTrigger,CheckingZone);
					xPLTaskID = addxPLMessage (TB_SendAreaStateMsg);
					xPLTaskList.Task[xPLTaskID].NewZoneState = StateTamper;
					return FALSE;
				}
				//If the zone is an entry point and the Entry timer is not set, then set it and send trigger messages
				if ((EntryTimer == (unsigned)0) && (ZoneConfig[CheckingZone].IsEntry)) {
					EntryTimer = TickGet() + (TICK_SECOND * xPLConfig.EntryDelay);
					TriggeredEntryZone = CheckingZone;
					SendZoneStateMsg (StateTrigger,CheckingZone);
					xPLTaskID = addxPLMessage (TB_SendAreaStateMsg);
					xPLTaskList.Task[xPLTaskID].NewZoneState = StateTrigger;
					return FALSE;
				}
				//Now we set the alarm off
				StateChangeToAlarming(CheckingZone);
				return FALSE;
			}
		EndLoop:

			TRUE; // what a funny little hack ;)

		}//Loop ends here



		// Set the time of the next zone check
		NextZoneCheckDelay = TickGet() + (TICK_SECOND / ZONECHECKDELAY);


	} // end of delay

	//If Exit timer active and time passed, then set it to zero.
	if ((ExitTimer !=(unsigned)0) && (TickGet() > ExitTimer)) ExitTimer = 0;


	//If in alarming state, call the alarmed task manager
	if ((AlarmState == Alarming) || ( AlarmState == Alarming24))  TaskAlarming();

	//If in alarmed state call the alarmed task
	if ((AlarmState == Alarmed) || ( AlarmState == Alarmed24))  TaskAlarmed();

	return TRUE;

} //IOTasks


//********************************************************************
/**
 
 Used to change the alarm state, sends out required messages and chages
 any IO lines such as siren and strobe
 
 @param           ZoneID
 @result          IO changes

*/

void StateChangeToAlarming (int ZoneID) {

	int AreaCount;

	// Tell the dubugger
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"State change to alarming\r\n");
#endif

	//Ensure entry and exit timers are cleared
	EntryTimer = 0;
	ExitTimer = 0;


	// Start the strobe and internal siren
	if (ZoneConfig[ZoneID].ZoneType != Silent) {
		STROBE_IO = 1;
		INT_SIREN_IO = 1;
	}

	// update the zone state
	ZoneConfig[ZoneID].IsAlarmed = TRUE;
	ZoneConfig[ZoneID].State = StateAlarm;
	
	// Update the area state
	for (AreaCount = 0; AreaCount < AREACOUNT; AreaCount++) {
		if (isZoneMemberOfArea(ZoneID, AreaCount) == TRUE)
			AreaConfig[AreaCount].IsAlarmed = TRUE;
			
			// if area is not isolted or bypassed
			if ((AreaConfig[AreaCount].State != StateBypass) && (AreaConfig[AreaCount].State != StateIsolate)) 
				AreaConfig[AreaCount].State = StateAlarm;	
	}

	//Queue email
	LastZoneAlarmed = ZoneID;
	if (xPLConfig.SMTP_OnlyInitalAlarm == FALSE)
		SMTPQueueEmail (SMTP_Alarmed);

	// Dont want to reset the alarm timers each time something is triggered
	if ((AlarmState !=Alarming24) &&  (AlarmState !=Alarming))  {

		//Queue email email
		if (xPLConfig.SMTP_OnlyInitalAlarm == TRUE)
			SMTPQueueEmail (SMTP_Alarmed);

		if (ZoneConfig[ZoneID].ZoneType != TwentyFourHour) {
			AlarmState = Alarming;//Change the current state to alarming
			SirenTimer = TickGet() + (TICK_SECOND * xPLConfig.SirenDelay); //check to see if external siren should be activated
			AlarmingTimer =  TickGet() + (TICK_SECOND * xPLConfig.AlarmingDuration);
		}
		else {
			AlarmState = Alarming24; //Change the current state to alarming
			SirenTimer = TickGet() + (TICK_SECOND * xPLConfig.SirenDelay24h);
			AlarmingTimer =  TickGet() + (TICK_SECOND * xPLConfig.Alarming24Duration);
		}
	} // !=Alarming

	// If the zone type is silent then do not allow the siren timer
	if (ZoneConfig[ZoneID].ZoneType == Silent)
		SirenTimer = 0;

	// Send the alarming message
	SendAlarmingMsg(ZoneID);

}
//********************************************************************
/**
 Function        StateChangeToAlarmed
 PreCondition:    None
 Side Effects:    None
 Overview:
 Used to change the alarmed state
 @param           None
 @result          IO changes

*/
void StateChangeToAlarmed (void) {

	// Tell the debugger
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"State changed to Alarmed, siren and strobe off\r\n");
#endif

	//Clear timers
	AlarmingTimer = 0;
	SirenTimer = 0;

	//Stop sirens
	EXT_SIREN_IO = 0;
	INT_SIREN_IO = 0;

	//Change internal state to alarmed
	if (AlarmState == Alarming24) AlarmState = Alarmed24;
	if (AlarmState == Alarming) AlarmState = Alarmed;

}
//********************************************************************
/**
 Function        TaskAlarming
 PreCondition:    None
 Side Effects:    None
 Overview:
 Used to change the IO outputs while in the alarming state
 @param           None
 @result          IO changes

*/

void TaskAlarming (void) {

	static int DebugTimer; // Only prints updates on new second
	int SecondsLeft;
	char DebugOut[6];

	// Time delay before activating the external siren
	if ((SirenTimer != (unsigned) 0) && (TickGet() > SirenTimer))  SirenTimer = 0;

	//Set sirens
	if (SirenTimer == (unsigned) 0) {
		EXT_SIREN_IO = 1;
		INT_SIREN_IO = 1;
	}

	// has the siren gone on for long enough?
	if (TickGet() > AlarmingTimer) {
		StateChangeToAlarmed();
		DebugTimer = 0;
		AlarmingTimer = 0;
	}

	// Debug output
#if defined(DEBUG_UART)
	SecondsLeft = (AlarmingTimer - TickGet()) / TICK_SECOND;
	if (DebugTimer != SecondsLeft) {
		DebugTimer = SecondsLeft;
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Alarming count down: ");
		uitoa(DebugTimer,  DebugOut);
		putsUART(DebugOut);
		putrsUART((ROM char*)"\r\n");
#endif
	}
#endif


}
//********************************************************************
/**
 Function        TaskAlarmed
 PreCondition:    None
 Side Effects:    None
 Overview:
 Used to change the IO outputs while in the alarmed state
 @param           None
 @result          IO changes

*/
void TaskAlarmed (void) {

	//nothing to do here


}
//********************************************************************
/**
 Function        ClearArmingFlags
 PreCondition:    None
 Side Effects:    Arming flags
 Overview:
 utility used to remove all of the arming flags
 @param           None
 @result          None

*/
void ClearArmingFlags (void) {

	//Cycle through all zones and clear arming flags
	for (iCount =0; iCount > ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsChangeFlagged = FALSE;

	}


	//Cycle through all areas and clear arming flags
	for (iCount =0; iCount < AREACOUNT; iCount++) {
		AreaConfig[iCount].IsChangeFlagged = FALSE;

	}


}


//********************************************************************
/**
 Function        Arm
 PreCondition:    Arming flags must be set
 Side Effects:    Arm device
 Overview:
 Changes that alarm state to Arm based on the arming flags

 @param           exit delay
 @result          None
*/

void Arm (int short TimeDelay) {
	int Counter;

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Arm\r\n");
#endif

	// Check for a zone list or area list, if missing send error with description of fault
	for (iCount = 0, i2Count = 0; iCount < AREACOUNT; iCount++) {
		if (AreaConfig[iCount].IsChangeFlagged == TRUE)
			i2Count++;
	} //for

	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		if (ZoneConfig[iCount].IsChangeFlagged == TRUE)
			i2Count++;
	} //for

	if (i2Count == 0) {
		i2Count = addxPLMessage (TB_SendError);
		xPLTaskList.Task[i2Count].ErrorMessage = "Nothing selected to arm";
	}
	else
		ExitTimer = TickGet() + (TimeDelay * TICK_SECOND);
	StateChangeToArm (FALSE);

} // Arm

//********************************************************************
/**
 Function        ArmHome
 PreCondition:    None
 Side Effects:    Arm device
 Overview:
 Changes that alarm state to Arm Home
 @param           exit delay
 @result          None

*/

void ArmHome (int short TimeDelay) {

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"ArmHome\r\n");
#endif

	// Set arming flags on all zones of type perimeter
	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		if (ZoneConfig[iCount].ZoneType == Perimiter) ZoneConfig[iCount].IsChangeFlagged = TRUE;
		if (ZoneConfig[iCount].ZoneType == Interior) ZoneConfig[iCount].IsChangeFlagged = FALSE;
	}

	ExitTimer = TickGet() + (TimeDelay * TICK_SECOND);
	StateChangeToArm(FALSE);

} // ArmHome

//********************************************************************
/**
 Function        ArmAway
 PreCondition:    None
 Side Effects:    Arm device
 Overview:
 Changes that alarm state to Arm Away
 @param           exit delay
 @result          None

*/


void ArmAway (int short TimeDelay) {

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"ArmAway\r\n");
#endif

	// Set arming flags on all zones
	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsChangeFlagged = TRUE;
	}

	ExitTimer = TickGet() + (TimeDelay * TICK_SECOND);
	StateChangeToArm (FALSE);

} //ArmAway

//********************************************************************
/**
 Function        ArmLatchkey
 PreCondition:    None
 Side Effects:    Arm device
 Overview:
 Changes that alarm state to latch Key
 @param           exit delay
 @result          None

*/

void ArmLatchkey (int short TimeDelay) {

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"ArmLatchkey\r\n");
#endif


	// Set arming flags on all zones
	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsChangeFlagged = TRUE;
	}

	ExitTimer = TickGet() + (TimeDelay * TICK_SECOND);
	StateChangeToArm (TRUE);

} // ArmLatchkey


//********************************************************************
/**
 Function        Panic
 PreCondition:    None
 Side Effects:    IO lines and state
 Overview:
 Buts the alarm into the panic state and sends xPL message
 @param           None
 @result          None

*/
void Panic (void) {


#if defined(DEBUG_UART)
	putrsUART((ROM char*)"PANIC\r\n");
#endif

	// set off the alarm
	//Ensure entry and exit timers are cleared
	EntryTimer = 0;
	ExitTimer = 0;

	// Start the strobe and internal siren
	STROBE_IO = 1;
	INT_SIREN_IO = 1;
	AlarmState = Alarming;
	SirenTimer = TickGet() + (TICK_SECOND * xPLConfig.SirenDelay); //check to see if external siren should be activated
	AlarmingTimer =  TickGet() + (TICK_SECOND * xPLConfig.AlarmingDuration);

	// Send the alarming message
	
	addxPLMessage(TB_Panic);
} // Panic

/********************************************************************
 *********************************************************************
 In response to message
 *********************************************************************
 ********************************************************************/



//********************************************************************
/**
 Function        StateChangeToArm
 PreCondition:    ExitTimer must be set
 Side Effects:    Changes Armed / arming flags and state
 Overview:
 Perform a test on all zones to ensure it is in a good state to be armed.
 If not it will generate an xPL error message.
 Converts all of the arming flags into Armed flags, then clears arming flags
 and sends the armed message

 @param           None
 @result          None

*/
void StateChangeToArm (BOOL isLatchKey) {

	BOOL isAllZonesArmed, OneOrMoreZones, ArmFailed;
	int Counter, TaskNumber, ZoneCounter;

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"State change to arm called\r\n");
#endif

	// determine if an arming zone is triggered, if so cancel the action
	ArmFailed = FALSE;
	for (Counter=0;Counter <= ZONECOUNT;Counter++) {
		if (ArmingZoneState(Counter) != StateNormal) {  // add entry & exit zones here
			// The test has failed
			if (ArmFailed == FALSE) {
				// This is the first failed zone
				TaskNumber = addxPLMessage (TB_ArmFailed); // create the task
				for (ZoneCounter=0;ZoneCounter <= ZONECOUNT; ZoneCounter++) // clear out the zone list
					xPLTaskList.Task[TaskNumber].FlaggedZones[ZoneCounter] = 0;
				ArmFailed = TRUE;
			}
			// flag this zone as errored
			xPLTaskList.Task[TaskNumber].FlaggedZones[Counter] = 1;		
			}
	}


	if (ArmFailed) { 
	#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Arm failed as a zone is incorrect\r\n");
	#endif
		goto Exit_Here; // do not arm the system
	}
			
			

	//Cycle through all of the zones
	for (iCount =0; iCount < ZONECOUNT; iCount++) {

		//skip the bypassed or disabled. Arm anything that has the arming flag
		if ((!ZoneConfig[iCount].IsIsolated) && (!ZoneConfig[iCount].IsBypassed))
		{
			if(ZoneConfig[iCount].IsChangeFlagged) ZoneConfig[iCount].IsArmed = TRUE;
		}
	}// for loop


	// cycle though the area's, if all the zones in an area are armed, make the area armed as well
	for (iCount = 0; iCount < AREACOUNT; iCount++) {
		if ( (!AreaConfig[iCount].IsIsolated) && (!AreaConfig[iCount].IsBypassed) ) {

			isAllZonesArmed = TRUE;
			OneOrMoreZones = FALSE;
			for (i2Count = 0; i2Count < ZONECOUNT; i2Count++) {
				//is this zone a member?
				if (isZoneMemberOfArea(iCount, i2Count) == TRUE) {
					// ensure it is not disable before arming
					if ((!ZoneConfig[i2Count].IsIsolated) && (!ZoneConfig[i2Count].IsBypassed)) {
						if (ZoneConfig[i2Count].IsArmed == TRUE) {
							ZoneConfig[i2Count].IsArmed = TRUE;
							OneOrMoreZones = TRUE;
						}
						else isAllZonesArmed = FALSE;
					} // Zone not isolated or bypassed
				}// IsMemberofArea

			} // for i2Count

			if (OneOrMoreZones && isAllZonesArmed)
				AreaConfig[iCount].IsChangeFlagged = TRUE;

		}// IsChangeFlagged
	}// for iCount



	//Cycle through all areas, if one is arming enable all zones in the area
	for (iCount = 0; iCount < AREACOUNT; iCount++) {
		if ((AreaConfig[iCount].IsChangeFlagged) && ((!AreaConfig[iCount].IsIsolated) && (!AreaConfig[iCount].IsBypassed)) ) {

			for (i2Count = 0; i2Count < ZONECOUNT; i2Count++) {
				//is this zone a member?
				if (isZoneMemberOfArea(iCount, i2Count) == TRUE) {
					// ensure it is not disable before arming
					if ((!ZoneConfig[i2Count].IsIsolated) && (!ZoneConfig[i2Count].IsBypassed)) ZoneConfig[i2Count].IsArmed = TRUE;
				}// IsMemberofArea

			} // for i2Count
		}// IsChangeFlagged
	}// for iCount

	// Change the state
	if (isLatchKey) AlarmState = Armed_LatchKey;
	else AlarmState = Armed;

	// sends message and clears flags
	addxPLMessage(TB_SendArmed);

Exit_Here:
 	// Greetings human
 	Counter = 0;
}

//********************************************************************
/**
 Function        StateChangeToArmFailed
 PreCondition:    None
 Side Effects:    Changes arming flags
 Overview:
 This is called when a Arming (any type) action fails.  It will clear the arming flags
 @param           None
 @result          None

*/

void StateChangeToArmFailed (void) {

	BOOL FirstZone = TRUE;


#if defined(DEBUG_UART)
	putrsUART((ROM char*)"State change to arm failed\r\n");
#endif


	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# ArmFailed, no MAC transmit port\r\n");
#endif
	}
	else {

		//Generate xPL head with type security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=error\nzone-list=");

		//Scan all zones, list those that are triggered and of status arming
		for (iCount =0; iCount < ZONECOUNT; iCount++) {
			if ((ZoneConfig[iCount].IsTriggered) && (ZoneConfig[iCount].IsChangeFlagged))
			{
				if (FirstZone == FALSE) UDPPut(',');
				itoa(iCount, xPLMsgKey);
				UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
				FirstZone = FALSE;
			}
		}

		UDPPutROMString((ROM BYTE*) "\nerror=Arm fail as zone tripped");
		UDPPutROMString((ROM BYTE*) "\n}\n");
		// Now transmit it.
		UDPFlush();
	}


}



//********************************************************************
/**
 Function        isZoneMemberOfArea
 PreCondition:    None
 Side Effects:
 Overview:
 Returns true of false is an Zone is a member to an area
 @param           Zone ID, Area ID
 @result          BOOL

*/

BOOL isZoneMemberOfArea(int TheZone, int TheArea) {

	unsigned short int Result;

#if (AREACOUNT > 8)
#error Having more then 8 areas is not possible under this design.
#endif

	if (TheArea == 0) Result = (ZoneConfig[TheZone].AreaList & 0b00000001);
	if (TheArea == 1) Result = (ZoneConfig[TheZone].AreaList & 0b00000010);
	if (TheArea == 2) Result = (ZoneConfig[TheZone].AreaList & 0b00000100);
	if (TheArea == 3) Result = (ZoneConfig[TheZone].AreaList & 0b00001000);
	if (TheArea == 4) Result = (ZoneConfig[TheZone].AreaList & 0b00010000);
	if (TheArea == 5) Result = (ZoneConfig[TheZone].AreaList & 0b00100000);
	if (TheArea == 6) Result = (ZoneConfig[TheZone].AreaList & 0b01000000);
	if (TheArea == 7) Result = (ZoneConfig[TheZone].AreaList & 0b10000000);

	/*
	 #if defined(DEBUG_UART)
	 putrsUART((ROM char*)"Is Area Triggered, Area = ");
	 putcUART((char) '0' + TheArea);

	 putrsUART((ROM char*)" Zone = ");
	 uitoa(TheZone, xPLMsgKey);
	 putsUART(xPLMsgKey);

	 putrsUART((ROM char*)" AreaList = ");
	 uitoa(ZoneConfig[TheZone].AreaList, xPLMsgKey);
	 putsUART(xPLMsgKey);

	 putrsUART((ROM char*)" Result = ");
	 uitoa(Result, xPLMsgKey);
	 putsUART(xPLMsgKey);

	 putrsUART((ROM char*)"\r\n");
	 #endif
	 */
	if (Result >= 1)  return TRUE;
	else return FALSE;


}//isZoneMemberOfArea

//********************************************************************
/**
 Function        SetArmingArea
 PreCondition:    None
 Side Effects:    Sets arming flags for areas
 Overview:
 Sets the arming flags for the area based on values in the KVP
 @param           Value from xPL message
 @result          None

*/
void SetArmingArea(char * TheValue) {



}
//********************************************************************
/**
 Function        SetArmingZone
 PreCondition:    None
 Side Effects:    Sets arming flags for zones
 Overview:
 Sets the arming flags for the zones based on values in the KVP

 @param           Value from xPL message, BOOL is zone - False is Area
 @result          None
*/
void SetArming(char * TheValue, BOOL isZone) {

	int DeviceID;
	char TempBuffer[3];


	//if 'all' then set all zones to arming
	if (strncmppgm2ram(TheValue,"all",3)) {
		if (isZone) {
			// Loop though all of the zones
			for (iCount = 0; iCount < ZONECOUNT; iCount++) {
				if ((!ZoneConfig[iCount].IsIsolated) && (!ZoneConfig[iCount].IsBypassed)) {
					ZoneConfig[iCount].IsChangeFlagged = TRUE;
				}
			} // for
		} // isZone
		else {
			// Loop though all of the Area's
			for (iCount = 0; iCount < AREACOUNT; iCount++) {
				if ((!AreaConfig[iCount].IsIsolated) && (!AreaConfig[iCount].IsBypassed)) {
					AreaConfig[iCount].IsChangeFlagged = TRUE;
				}
			} // for
		}// !isZone
	}// if (all)
	else {

		// looking for digits seperated by a commar and terminated by new line eg (01, 02 ,3,4,12\n)
		memset(TempBuffer,'\0', 3);
		for (iCount = 0, i2Count =0;iCount < xPLMSGVALUELENGTH; iCount++) {
			if (TheValue[iCount] == ' ') //ignore spaces
				if ((i2Count ==0) && (TheValue[iCount] == '0')) //ignore leading zeros
					if ((TheValue[iCount] == ',') || (TheValue[iCount] == '\n')) {
						//make sure we have data first
						if (!i2Count == 0) {
							DeviceID = atoi(TempBuffer);
							// ensure that the zone is valid, if so arm it
							if (isZone) {
								if ((DeviceID < ZONECOUNT) && (DeviceID != 0)) ZoneConfig[iCount].IsChangeFlagged = TRUE;
							}
							if (!isZone) {
								if ((DeviceID < AREACOUNT) && (DeviceID != 0)) AreaConfig[iCount].IsChangeFlagged = TRUE;
							}
							if (TheValue[iCount] == '\n') break; // signals the end of the line
							memset(TempBuffer,'\0', 3);
						}//(!i2Count == 0)
					}// ((TheValue[iCount] == ',') || (TheValue[iCount] == '\n'))
					else {
						TempBuffer[i2Count] = TheValue[iCount];
						i2Count++;
					}

		}// for

	} // else

} // SetArmingZone


//********************************************************************
/**
 Function        ArmingZoneState
 PreCondition:    Arming flags set
 Side Effects:
 Overview:
 StateNormal means everything is ok and the zone is in a good state for arming.
 @param           The zone to be checked
 @result          Zone_States, state of the zone

*/
Zone_States ArmingZoneState (int TheZone) {

	// zone is not marked as arming
	if (!ZoneConfig[TheZone].IsChangeFlagged)
		return StateNormal;
	// isolated or bypassed
	if ((ZoneConfig[TheZone].IsBypassed) || (ZoneConfig[TheZone].IsIsolated))
		return StateNormal;
	// Triggered zone is not an entry point
	if ((ZoneConfig[TheZone].IsTriggered) && (!ZoneConfig[TheZone].IsEntry))
		return StateTrigger;
	// Anything that is tampered is a no no
	if (ZoneConfig[TheZone].IsTampered)
		return StateTamper;

	// If we get this far then all is good.
	return StateNormal;
}

//********************************************************************
/**
 Function        SchemaProcessUDPMsg
 PreCondition:    None
 Side Effects:    Changes Armed / arming flags
 Overview:
 Called by xPL.c when ever a message is received. Here we determine what actions are
 needed in response to a message.

 When changing the alarm state when there is no delay the arming timer is set to an expired value.
 This means the state change will not occur until the next task run occurs.
 @param           Delay
 @result          None


*/
void SchemaProcessUDPMsg (void) {

	int short TimeDelay = xPLConfig.ExitDelay;
	int TheMessageType, xPLTaskID;

	BOOL MessageIsCommand = FALSE;
	BOOL MessageIsRequest = FALSE;

	// Ensure the message is directed to me and its of the security class
	if ((xPLReceivedMessage.xPL_MsgDestination == To_Me ) && (xPLReceivedMessage.xPL_SchemaClass == Class_Security))  {

		// clear out any left over flags
		ClearArmingFlags();
		memset(UserID,'\0',USERNAMELENGTH);

		// We have to process all of the message before we can act on it as the message could be out of order. Open standards are not always standard
		while (SaveKVP(xPLMsgKey,xPLMsgValue )) {

			if (memcmppgm2ram(xPLMsgKey, "user", 4)) {
				if (!xPLAuthenticate(xPLMsgValue))
					return;
			}
			if (memcmppgm2ram(xPLMsgKey, (ROM void*)"area-list",9)) SetArming(xPLMsgValue, FALSE);
			if (memcmppgm2ram(xPLMsgKey, (ROM void*)"zone-list",9)) SetArming(xPLMsgValue, TRUE);
			if (memcmppgm2ram(xPLMsgKey, (ROM void*)"delay",5))  {
				// Process delay value. If we receive text its might be the word 'default', either way we use the default value.
				if( isdigit((unsigned char)xPLMsgValue[0] ) ==0 )
					TimeDelay = xPLConfig.ExitDelay;
				else
					TimeDelay = atoi(xPLMsgValue);
			}

			if (memcmppgm2ram(xPLMsgKey, "command",7)) {
				if (!xPLFindArrayInKVP(SecurityCommandText, 8, &TheMessageType, xPLMsgValue)) {
#if defined(DEBUG_UART)
					putrsUART((ROM char*)"#Note# command type not known\r\n");
#endif
				}
				else MessageIsCommand = TRUE;
			}
			if (memcmppgm2ram(xPLMsgKey, "request",7)) {
				if (!xPLFindArrayInKVP(SecurityRequestsText, 8, &TheMessageType, xPLMsgValue)) {
#if defined(DEBUG_UART)
					putrsUART((ROM char*)"#Note# request type not known\r\n");
#endif
				}
				else MessageIsRequest = TRUE;
			}

		} // while


		// ### Message Type BASIC

		if (xPLReceivedMessage.xPL_SchemaTypes == Type_Basic) {

			if (MessageIsCommand) {

				switch (TheMessageType) {

					case Command_arm: // Arm, needs area list or zone list
						Arm(TimeDelay);
						break;

					case Command_armhome:
						ArmHome(TimeDelay);
						break;

					case Command_armaway:
						ArmAway (TimeDelay);
						break;

					case Command_armlatchkey:
						ArmLatchkey (TimeDelay);
						break;

					case Command_disarm:
						// call Disarm and pass the userID
						Disarm();
						break;

					case Command_panic:
						Panic();
						break;

					case Command_isolate:
						addxPLMessage (TB_Isolate) ;
						break;

					case Command_bypass:
						addxPLMessage (TB_Bypass) ;
						break;

					case Command_enable:
						addxPLMessage (TB_Enable) ;
						break;

#if defined(DEBUG_UART)
					default:
						// Something has gone wrong!
						putrsUART((ROM char*)"#ERROR# Shit, something broke with Type_Basic when the message was received\r\n");
#endif


				} // switch (TheMessageType)
			} // MessageIsCommand

		} // xPLReceivedMessage.xPL_SchemaTypes == Type_Basic


		// ### Message Type REQUEST

		if (xPLReceivedMessage.xPL_SchemaTypes == Type_Request) {

			if (MessageIsRequest) {
				switch (TheMessageType) {

					case Request_gateinfo:
						// Send gateway infomation
						addxPLMessage (TB_SendGatewayInfo) ;
						break;

					case Request_zonelist:
						// Send zone list
						addxPLMessage (TB_SendZoneList) ;
						break;

					case Request_arealist:
						// Send area list
						addxPLMessage (TB_SendAreaList );
						break;

					case Request_zoneinfo:
						addxPLMessage (TB_SendZoneInfo );
						break;

					case Request_areainfo:
						addxPLMessage (TB_SendAreaInfo );
						break;

					case Request_gatestat:
						addxPLMessage (TB_SendGateStat);
						break;

					case Request_zonestat:
						addxPLMessage (TB_SendZoneStat);
						break;
		
					case Request_areastat:
						addxPLMessage (TB_SendAreaStat);
						break;

#if defined(DEBUG_UART)
						default:
						putrsUART((ROM char*)"#ERROR# Shit broken with Type_Request when msg rc'd\r\n");
#endif


				} //switch (TheMessageType)
			}// MessageIsRequest
		} // xPLReceivedMessage.xPL_SchemaTypes == Type_Request


	} // To_Me && Class_Security



	// #### Device Configuration ###

	if ((xPLReceivedMessage.xPL_MsgDestination == To_Me ) && (xPLReceivedMessage.xPL_SchemaClass == Class_Config))  {

		if(AlarmState != Disarmed) {
			//	Note: do not allowing config changes when not in the disarmed state. A restart would make the system disarmed and or remove
			//	parts of the alarm which would be a security risk.
			xPLTaskID = addxPLMessage (TB_SendError);
			xPLTaskList.Task[xPLTaskID].ErrorMessage = "Must be disarmed to update config";
		}
		else {

			if (xPLReceivedMessage.xPL_SchemaTypes == Type_List) 
				addxPLMessage(TB_SendConfigList);
				
			if (xPLReceivedMessage.xPL_SchemaTypes == Type_Current) 
				addxPLMessage (TB_SendConfigCurrent);
			
			if (xPLReceivedMessage.xPL_SchemaTypes == Type_Response) {
				while (SaveKVP(xPLMsgKey,xPLMsgValue )) {

					// Device name
					if (stricmppgm2ram(xPLMsgKey, (ROM char*) "newconf") ==0) {

						for (iCount =0; xPLMsgValue[iCount] != '\0' && iCount < sizeof(xPLConfig.xPL_My_Instance_ID); iCount++) {
							if(xPLMsgValue[iCount] >= 'A' && xPLMsgValue[iCount] <= 'Z')
								xPLMsgValue[iCount] -= 'A' - 'a'; //Convert to lower case
							if((xPLMsgValue[iCount] > 'z') || (xPLMsgValue[iCount] < '0')) 	xPLMsgValue[iCount]  = 0;
						}

						if (xPLMsgValue[0] != '\0') {
							strcpy( xPLConfig.xPL_My_Instance_ID, xPLMsgValue);
							xPLSaveEEConfig(&xPLConfig, EEDATASTART_XPL, sizeof(xPLConfig));
						}

#if defined(DEBUG_UART)
						putrsUART((ROM char*)"newconf =");
						putsUART(xPLConfig.xPL_My_Instance_ID);
						putrsUART((ROM char*)"\r\n");
#endif


					}
					if (stricmppgm2ram(xPLMsgKey, (ROM char*) "interval") ==0) {

						if(atoi(xPLMsgValue) > 1 )  {
							xPLConfig.xPL_Heartbeat_Interval = atoi(xPLMsgValue);
							xPLSaveEEConfig(&xPLConfig, EEDATASTART_XPL, sizeof(xPLConfig));
						}

#if defined(DEBUG_UART)
						putrsUART((ROM char*)"interval =");
						putcUART( '0' + xPLConfig.xPL_Heartbeat_Interval );
						putrsUART((ROM char*)"\r\n");
#endif



					}
					if (stricmppgm2ram(xPLMsgKey, (ROM char*) "ip") ==0) {

						if(StringToIPAddress(xPLMsgValue, &(AppConfig.MyIPAddr))) {
							memcpy((void*)&(AppConfig.DefaultIPAddr), (void*)&(AppConfig.MyIPAddr), sizeof(IP_ADDR));
#if defined(DEBUG_UART)
							putrsUART((ROM char*)"ip:");
							DisplayIPValue(AppConfig.MyIPAddr);
							putrsUART((ROM char*)"\r\n");
#endif

							xPLSaveEEConfig(&AppConfig, EEDATASTART_CONFIG, sizeof(AppConfig));
						}
						else {
							//
#if defined(DEBUG_UART)
							putrsUART((ROM char*)"ip errored\r\n");
#endif
						}

					} //ip
					if (stricmppgm2ram(xPLMsgKey, ( ROM char *)"subnet") ==0)  {
						if(StringToIPAddress(xPLMsgValue, &(AppConfig.MyMask))) {
							memcpy((void*)&(AppConfig.DefaultMask), (void*)&(AppConfig.MyMask), sizeof(IP_ADDR));
#if defined(DEBUG_UART)
							putrsUART((ROM char*)"Subnet:");
							DisplayIPValue(AppConfig.MyMask);
							putrsUART((ROM char*)"\r\n");
#endif

							xPLSaveEEConfig(&AppConfig, EEDATASTART_CONFIG, sizeof(AppConfig));
						}
						else {
							//
#if defined(DEBUG_UART)
							putrsUART((ROM char*)"Subnet errored\r\n");
#endif
						}
					}
				} // while


#if defined(DEBUG_UART)
				putrsUART((ROM char*)"\r\n\r\n Restart ##############");
#endif

				Reset();
			} //AlarmState != Disarmed
		}// if ((xPLReceivedMessage.xPL_MsgDestination == To_Me ) && (xPLReceivedMessage.xPL_SchemaClass == Class_Config))



	}// Config


} // SchemaProcessUDPMsg



//********************************************************************
/**
 Function        Disarm
 PreCondition:    UserID must be populated first from Authenticate.c
 Side Effects:    armed flags, state and IO
 Overview:
 Disarm the alarm.  Uses the UserID to determine who the user was
 @param           None
 @result          None

*/

void Disarm (void) {


	//Ensure entry and exit timers are cleared
	EntryTimer = 0;
	ExitTimer = 0;

	//Change any of the IO lines for sirens and strobe
	STROBE_IO = 0;
	INT_SIREN_IO = 0;
	EXT_SIREN_IO = 0;

	//Change the state to disarmed
	PreviousAlarmState = Disarmed;

	//set the previous state to disarmed as well
	AlarmState = Disarmed;

	//Cycle though all of the zones clear any  Armed, Alarmed, Arming, isolated flags
	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		ZoneConfig[iCount].IsIsolated = FALSE;
		ZoneConfig[iCount].IsArmed = FALSE;
		ZoneConfig[iCount].IsAlarmed = FALSE;
		ZoneConfig[iCount].IsChangeFlagged = FALSE;
	}// for iCount


	//cycle thought all areas clear any Armed, Alarmed, isolated flags
	for (iCount = 0; iCount < AREACOUNT; iCount++) {
		AreaConfig[iCount].IsIsolated = FALSE;
		AreaConfig[iCount].IsArmed = FALSE;
		AreaConfig[iCount].IsAlarmed = FALSE;
		AreaConfig[iCount].IsChangeFlagged = FALSE;
	}// for iCount


	//Send disarmed message
	addxPLMessage (TB_SendDisarm);

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"State change to disarm\r\n");
#endif


} // Disarm
//********************************************************************
/**
 Function        IsolateZone
 PreCondition:    None
 Side Effects:    isolated flag
 Overview:
 will set the isolated flag for the zone provided
 @param           Zone ID
 @result          None

*/

void IsolateZone (int TheZone) {

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Isolating zone\r\n");
#endif


	//Set the zone's isolated flag to true
	ZoneConfig[TheZone].IsIsolated = TRUE;
	ZoneConfig[TheZone].IsChangeFlagged = FALSE;

	// clear out the status flags
	ZoneConfig[TheZone].IsTampered = FALSE;
	ZoneConfig[TheZone].IsTriggered = FALSE;
	ZoneConfig[TheZone].BounceDelay = 0;
	ZoneConfig[TheZone].IsAlarmed = FALSE;

	//Send message

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# IsolateZone, no MAC transmit port\r\n");
#endif
	}
	else {

		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
		UDPPutROMString((ROM BYTE*) "event=isolated\nzone=");
		itoa(TheZone, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		UDPPutROMString((ROM BYTE*) "\nstate=true\n}\n");
		UDPFlush();
	}//UDPIsPutReady(XPLSocket)

	//now save the changes
	xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));

} // IsolateZone
//********************************************************************
/**
 Function        IsolateArea
 PreCondition:    None
 Side Effects:    isolated flag
 Overview:
 will set the isolated flag for the area provided
 also sets the flag for any zones that are related
 @param           Area ID
 @result          None

*/

void IsolateArea (int TheArea) {


#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Isolate Area\r\n");
#endif


	//Set the area's isolated flag to true
	AreaConfig[TheArea].IsIsolated = TRUE;
	AreaConfig[TheArea].IsChangeFlagged = FALSE;


	// clear out the status flags
	AreaConfig[TheArea].IsTampered = FALSE;
	AreaConfig[TheArea].IsTriggered = FALSE;
	AreaConfig[TheArea].IsAlarmed = FALSE;

	//Send message

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# IsolateArea, no MAC transmit port\r\n");
#endif
	}
	else {

		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Area);
		UDPPutROMString((ROM BYTE*) "area=");
		UDPPut('0'+TheArea);
		UDPPutROMString((ROM BYTE*) "\nisolated=true");
		UDPFlush();
	}//UDPIsPutReady(XPLSocket)

	//cycle through all zones and set their isolated flag to true if they are a member
	for (iCount = 0; iCount <= ZONECOUNT; iCount++) {
		if (isZoneMemberOfArea(iCount, TheArea)) {
			ZoneConfig[iCount].IsIsolated = TRUE;
		}// ZoneConfig
	}// for iCount

	//now save the changes
	xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));
	xPLSaveEEConfig(AreaConfig, EEDATASTART_AREA, sizeof(AreaConfig));

} // IsolateZone
//********************************************************************
/**
 Function        bypassZone
 PreCondition:    None
 Side Effects:    bypass flag
 Overview:
 will set the bypass flag for the zone provided
 @param           Zone ID
 @result          None

*/

void bypassZone (int TheZone) {
	// say hello
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Bypass Zone\r\n");
#endif


	//Set the zone's bypass flag to true
	ZoneConfig[TheZone].IsBypassed = TRUE;
	ZoneConfig[TheZone].IsChangeFlagged = FALSE;

	// clear out the status flags
	ZoneConfig[TheZone].IsTampered = FALSE;
	ZoneConfig[TheZone].IsTriggered = FALSE;
	ZoneConfig[TheZone].BounceDelay = 0;
	ZoneConfig[TheZone].IsAlarmed = FALSE;


	//Send message

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# bypassZone, no MAC transmit port\r\n");
#endif
	}
	else {

		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
		UDPPutROMString((ROM BYTE*) "event=bypassed\nzone=");
		itoa(TheZone, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		UDPPutROMString((ROM BYTE*) "\nstate=true\n}\n");
		UDPFlush();
	}//UDPIsPutReady(XPLSocket)

	//now save the changes
	xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));

} // bypassZone
//********************************************************************
/**
 Function        bypassArea
 PreCondition:    None
 Side Effects:    bypass flag
 Overview:
 will set the bypass flag for the area provided
 also sets the flag for any zones that are related
 @param           Area ID
 @result          None

*/

void bypassArea (int TheArea) {

	int short iCount;

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Bypass Area\r\n");
#endif


	//Set the area's bypass flag to true
	AreaConfig[TheArea].IsBypassed = TRUE;
	AreaConfig[TheArea].IsChangeFlagged = FALSE;


	// clear out the status flags
	AreaConfig[TheArea].IsTampered = FALSE;
	AreaConfig[TheArea].IsTriggered = FALSE;
	AreaConfig[TheArea].IsAlarmed = FALSE;


	//Send message

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# bypassZone, no MAC transmit port\r\n");
#endif
	}
	else {

		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
		UDPPutROMString((ROM BYTE*) "area=");
		UDPPut('0'+TheArea);
		UDPPutROMString((ROM BYTE*) "\nbypassed=true\n}\n");
		UDPFlush();
	}//UDPIsPutReady(XPLSocket)

	//cycle through all zones and set their bypass flag to true if they are a member
	for (iCount = 0; iCount < ZONECOUNT; iCount++) {
		if (isZoneMemberOfArea(iCount, TheArea) == TRUE) {
			ZoneConfig[iCount].IsBypassed = TRUE;
		}// ZoneConfig
	}// for iCount

	//now save the changes
	xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));
	xPLSaveEEConfig(AreaConfig, EEDATASTART_AREA, sizeof(AreaConfig));


} // bypassArea
//********************************************************************
/**
 Function        EnableZone
 PreCondition:    None
 Side Effects:    bypass, isolated and arming flags
 Overview:
 will clear the bypass, isolated and arming flag for the zone provided
 Should only be called as a task. Can take 2 runs to complete task
 @param           Zone ID
 @result          None

*/

void EnableZone (int TheZone) {

	// It can be isolated and bypassed at the same time.

	// If the zone is bypassed send
	// security.zone
	//  event=bypassed
	// area=id
	// state=false
	// clear the bypassed flag

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Enabling Zone\r\n");
#endif



	if (ZoneConfig[TheZone].IsBypassed) {

		//Check if we have a MAC transmit port, if not then shit yourself
		if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"#ERROR# EnableZone, no MAC transmit port\r\n");
#endif
		}
		else {

			// Generate xPL head with type security.zone
			XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
			UDPPutROMString((ROM BYTE*) "event=bypassed\nzone=");
			itoa(TheZone, xPLMsgKey);
			UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
			UDPPutROMString((ROM BYTE*) "\nstate=false\n}\n");
			UDPFlush();
		}//UDPIsPutReady(XPLSocket)

		ZoneConfig[TheZone].IsBypassed = FALSE;

		// return back after a TCIP stack task run has occured to complete the Isolated message
		if (ZoneConfig[TheZone].IsIsolated == FALSE) ZoneConfig[TheZone].IsChangeFlagged = FALSE;

	} // IsBypassed

	else if (ZoneConfig[TheZone].IsIsolated) {

		//Check if we have a MAC transmit port, if not then shit yourself
		if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"#ERROR# EnableZone, no MAC transmit port\r\n");
#endif
		}
		else {

			// Generate xPL head with type security.zone
			XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
			UDPPutROMString((ROM BYTE*) "event=isolated\nzone=");
			uitoa(TheZone, xPLMsgKey);
			UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
			UDPPutROMString((ROM BYTE*) "\nstate=false\n}\n");
			UDPFlush();
		}//UDPIsPutReady(XPLSocket)

		ZoneConfig[TheZone].IsIsolated = FALSE;
		ZoneConfig[TheZone].IsChangeFlagged = FALSE;

		//now save the changes
		xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));

	} // IsIsolated
} //EnableZone


//********************************************************************
/**
 Function        EnableArea
 PreCondition:    None
 Side Effects:    bypass, isolated flag
 Overview:
 will set the bypass flag for the area provided
 also sets the flag for any zones that are related
 @param           Area ID
 @result          None

*/

void EnableArea (int TheArea) {


	// It can be isolated and bypassed at the same time.

	// If the area is bypassed send
	// security.area
	// { event=bypassed
	// area=id
	// state=false }
	// clear the bypassed flag

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Enable Area\r\n");
#endif

	AreaConfig[TheArea].IsChangeFlagged = FALSE;

	if (AreaConfig[TheArea].IsBypassed) {

		//Check if we have a MAC transmit port, if not then shit yourself
		if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"#ERROR# EnableArea, no MAC transmit port\r\n");
#endif
		}
		else {

			// Generate xPL head with type security.zone
			XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Area);
			UDPPutROMString((ROM BYTE*) "area=");
			UDPPut('0'+ TheArea);
			UDPPutROMString((ROM BYTE*) "\nbypassed=false\n}\n");
			UDPFlush();
		}//UDPIsPutReady(XPLSocket)

		AreaConfig[TheArea].IsBypassed = FALSE;

		// cycle through all zones and set their flags to true clear if they are a member of this area
		for (iCount = 0; iCount < ZONECOUNT; iCount++) {
			if (isZoneMemberOfArea(iCount, TheArea)) {
				ZoneConfig[iCount].IsBypassed = FALSE;
			}// ZoneConfig
		}// for iCount

	} // IsBypassed


	// if area is isolated then send
	// security.area
	// { event=isolated
	// area=id
	// state=false }
	// clear the isolated flag


	if (AreaConfig[TheArea].IsIsolated) {

		//Check if we have a MAC transmit port, if not then shit yourself
		if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"#ERROR# EnableArea, no MAC transmit port\r\n");
#endif
		}
		else {

			// Generate xPL head with type security.zone
			XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Area);
			UDPPutROMString((ROM BYTE*) "area=");
			UDPPut('0'+TheArea);
			UDPPutROMString((ROM BYTE*) "\nisolated=false\n}\n");
			UDPFlush();
		}//UDPIsPutReady(XPLSocket)

		AreaConfig[TheArea].IsIsolated = FALSE;

		// cycle through all zones and set their flags to true clear if they are a member of this area
		for (iCount = 0; iCount < ZONECOUNT; iCount++) {
			if (isZoneMemberOfArea(iCount, TheArea) == TRUE) {
				ZoneConfig[iCount].IsIsolated = FALSE;
			}// ZoneConfig
		}// for iCount

	} // IsIsolated

	//now save the changes
	xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));
	xPLSaveEEConfig(AreaConfig, EEDATASTART_AREA, sizeof(AreaConfig));

}




//******************************************************************************
//*                                The End
//******************************************************************************

