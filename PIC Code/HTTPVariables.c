///@file HTTPVariables.c
///@brief All HTML variables are defined here
#define __CUSTOMHTTPAPP_C

#include "TCPIP Stack/TCPIP.h"
#include "SecuritySystem.h"
#include "xPL.h"

 //******************************************************************************
 /** D E C L A R A T I O N S **************************************************/
 //******************************************************************************


extern HTTP_CONN curHTTP;
extern HTTP_STUB httpStubs[MAX_HTTP_CONNECTIONS];
extern BYTE curHTTPID;
extern DWORD dwLastUpdateTick; // SNTP Customised.c used in NTPLastUpdate


/// Sticky status message variable.
/// This is used to indicated whether or not the previous POST operation was 
/// successful.  The application uses these to store status messages when a 
/// POST operation redirects.  This lets the application provide status messages
/// after a redirect, when connection instance data has already been lost.
BOOL lastSuccess = FALSE;

/// Stick status message variable.  See lastSuccess for details.
BOOL lastFailure = FALSE;

/// used for int conversions
static BYTE Tempdigits[4]; 


/// Text used for the current state of the alarm
ROM char *AlarmStatesText[] =
	{
		"Disarmed",
		"Alarming",
		"Alarming 24",
		"Alarmed",
		"Alarmed 24",
		"Armed LatchKey",
		"Armed"
	};  // max 14 characters hard coded(!!)

#if defined STACK_USE_DYNAMICDNS_CLIENT 
// Interface for DynDNS.c
extern  HTTP_IO_RESULT HTTPPostDDNSConfig(void);
#endif // DYNDNS


 //******************************************************************************
 /** P R I V A T E  P R O T O T Y P E S ***************************************/
 //******************************************************************************

static void HTTPPrintIP(IP_ADDR ip);


//******************************************************************************
//							DYNAMIC VARIABLES
//
//******************************************************************************


/// \todo the variable used in this fuction is never set (lastFailure)
/// Used to determine if the last post was a successful or not
void HTTPPrint_status_fail(void)
{
	if(lastFailure)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"block");
	else
		TCPPutROMString(sktHTTP, (ROM BYTE*)"none");
	lastFailure = FALSE;
}

//******************************************************************************

void HTTPPrint_rebootaddr(void)
{// This is the expected address of the board upon rebooting
	
	TCPPutArray(sktHTTP, curHTTP.data, strlen((char*)curHTTP.data));	
	return;
}

	

//******************************************************************************
void HTTPPrint_config_hostname(void)
{
	TCPPutArray(sktHTTP, AppConfig.NetBIOSName, strlen((char*)AppConfig.NetBIOSName));
	return;
}

//******************************************************************************

void HTTPPrint_reboot(void)
{
	// This is not so much a print function, but causes the board to reboot
	// when the configuration is changed.  If called via an AJAX call, this
	// will gracefully reset the board and bring it back online immediately
	Reset();
}

//******************************************************************************

// Used for Printing an IP address
void HTTPPrintIP(IP_ADDR ip)
{
	BYTE digits[4];
	BYTE i;
	
	for(i = 0; i < 4; i++)
	{
		if(i != 0)
			TCPPut(sktHTTP, '.');
		uitoa(ip.v[i], digits);
		TCPPutArray(sktHTTP, digits, strlen((char*)digits));
	}
}

//******************************************************************************

void HTTPPrint_config_ip(void)
{
	HTTPPrintIP(AppConfig.MyIPAddr);
	return;
}

//******************************************************************************

void HTTPPrint_config_gw(void)
{
	HTTPPrintIP(AppConfig.MyGateway);
	return;
}

//******************************************************************************

void HTTPPrint_config_subnet(void)
{
	HTTPPrintIP(AppConfig.MyMask);
	return;
}

//******************************************************************************

void HTTPPrint_config_dns1(void)
{
	HTTPPrintIP(AppConfig.PrimaryDNSServer);
	return;
}

//******************************************************************************

void HTTPPrint_config_dns2(void)
{
	HTTPPrintIP(AppConfig.SecondaryDNSServer);
	return;
}

//******************************************************************************

void HTTPPrint_config_mac(void)
{
	BYTE i;
	
	if(TCPIsPutReady(sktHTTP) < 18)
	{//need 17 bytes to write a MAC
		curHTTP.callbackPos = 0x01;
		return;
	}	
	
	// Write each byte
	for(i = 0; i < 6; i++)
	{
		if(i != 0)
			TCPPut(sktHTTP, ':');
		TCPPut(sktHTTP, btohexa_high(AppConfig.MyMACAddr.v[i]));
		TCPPut(sktHTTP, btohexa_low(AppConfig.MyMACAddr.v[i]));
	}
	
	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}



//******************************************************************************

void HTTPPrint_AlarmState  (void) {

	if(TCPIsPutReady(sktHTTP) < strlenpgm((ROM char*) AlarmStatesText[AlarmState]))
	{//need 14 bytes to write largest state name
		curHTTP.callbackPos = 0x01;
		return;
	}

	TCPPutROMArray(sktHTTP, AlarmStatesText[AlarmState], strlenpgm((ROM char*)AlarmStatesText[AlarmState]));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

//******************************************************************************
void HTTPPrint_TypeList (void) {
	
	if(TCPIsPutReady(sktHTTP) < strlenpgm((ROM char*)ZoneTypesText) + 10 )  { //hard coded values, arh well...
		curHTTP.callbackPos = 0x01;
		return;
	}
	

	for(iCount = 0; ZoneTypesText[iCount][0] != '\0' && iCount < 10; iCount++) {
	
		if (iCount != 0) TCPPutROMArray(sktHTTP, (ROM BYTE*)",", 1);	
		TCPPutROMArray(sktHTTP, (ROM BYTE*)"'", 1);
		TCPPutROMArray(sktHTTP, ZoneTypesText[iCount], strlenpgm((ROM char*)ZoneTypesText[iCount]));
		TCPPutROMArray(sktHTTP, (ROM BYTE*)"'", 1);
	}
	curHTTP.callbackPos = 0x00;	
}	

//******************************************************************************

void HTTPPrint_AlarmTypeList (void) {
	
	if(TCPIsPutReady(sktHTTP) < sizeof(AlarmTypesText) +10) {
		curHTTP.callbackPos = 0x01;
		return;
	}
	
	
	for(iCount = 0; AlarmTypesText[iCount][0] != '\0' && iCount < 10; iCount++) {
	
		if (iCount != 0) TCPPutROMArray(sktHTTP, (ROM BYTE*)",", 1);	
		TCPPutROMArray(sktHTTP, (ROM BYTE*)"'", 1);
		TCPPutROMArray(sktHTTP, AlarmTypesText[iCount], strlenpgm((ROM char*)AlarmTypesText[iCount]));
		TCPPutROMArray(sktHTTP, (ROM BYTE*)"'", 1);
	}	
	
	curHTTP.callbackPos = 0x00;	
}	

//******************************************************************************

void HTTPPrint_InstanceName (void) {
	
	TCPPutArray(sktHTTP, xPLConfig.xPL_My_Instance_ID, strlen(xPLConfig.xPL_My_Instance_ID));	
}
//******************************************************************************
void HTTPPrint_HeartBeatTime (void) {

	uitoa(xPLConfig.xPL_Heartbeat_Interval,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_ExitDelay (void) {

	uitoa(xPLConfig.ExitDelay,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_EntryDelay (void) {

	uitoa(xPLConfig.EntryDelay,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_AlarmingDuration (void) {

	uitoa(xPLConfig.AlarmingDuration,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_Alarming24Duration (void) {

	uitoa(xPLConfig.Alarming24Duration,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_SirenDelay24h (void) {

	uitoa(xPLConfig.SirenDelay24h,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}
//******************************************************************************
void HTTPPrint_SirenDelay (void) {

	uitoa(xPLConfig.SirenDelay,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));	
}




//******************************************************************************

// 						ZONES

//******************************************************************************







//******************************************************************************
void HTTPPrint_ZoneCount  (void) {
	
	uitoa(ZONECOUNT,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
	return;
}

//******************************************************************************

void HTTPPrint_ZonesBypassed (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsBypassed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesIsolated (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsIsolated?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesTriggered (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsTriggered?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesArmed (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsArmed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesAlarmed (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsAlarmed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesEntry (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsEntry?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_ZonesNO (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsNO?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}



//******************************************************************************

void HTTPPrint_ZonesType (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		
		uitoa(ZoneConfig[iCount-1].ZoneType,  Tempdigits);
		TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesAlarmType (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		uitoa(ZoneConfig[iCount-1].ZoneAlarmType,  Tempdigits);
		TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesAreaList (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		uitoa(ZoneConfig[iCount-1].AreaList,  Tempdigits);
		TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
		}// for iCount

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZoneDesc (WORD num) {

	char DeviceDesc[DEVICEDESCLENGTH];
	
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < DEVICEDESCLENGTH)
		return;
		
	iCount = EEDATASTART_DEVICEDESC;
	if (num != 0)
		iCount += ((int)num * DEVICEDESCLENGTH) + 1;		

	XEEReadArray ((DWORD) iCount, DeviceDesc , (BYTE) DEVICEDESCLENGTH); 
	TCPPutArray(sktHTTP, DeviceDesc, DEVICEDESCLENGTH);

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_ZonesTampered (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = ZONECOUNT + ZONECOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= ZONECOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
			TCPPut(sktHTTP, (ZoneConfig[iCount-1].IsTampered?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************
// 						AREAS
//******************************************************************************


//******************************************************************************

void HTTPPrint_AreaCount  (void) {
	
	uitoa(AREACOUNT,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
	
	return;
}


//******************************************************************************

void HTTPPrint_AreasBypassed (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (AreaConfig[iCount-1].IsBypassed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_AreasIsolated (void) {



	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (AreaConfig[iCount-1].IsIsolated?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}




//******************************************************************************

void HTTPPrint_AreasTriggered (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (AreaConfig[iCount-1].IsTriggered?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_AreasArmed (void) {



	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (AreaConfig[iCount-1].IsArmed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_AreasAlarmed (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		TCPPut(sktHTTP, (AreaConfig[iCount-1].IsAlarmed?'1':'0'));
		}// for iCount


	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************

void HTTPPrint_AreasAlarmType (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT + AREACOUNT;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount >1) TCPPut(sktHTTP,',');
		uitoa(AreaConfig[iCount-1].ZoneAlarmType,  Tempdigits);
		TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
		}// for iCount

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_AreaDesc (WORD num) {

	char DeviceDesc[DEVICEDESCLENGTH];
	
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < DEVICEDESCLENGTH)
		return;

	iCount = (int)EEDATASTART_DEVICEDESC + ((int)DEVICEDESCLENGTH * (int)ZONECOUNT) + 1;
	if (num != 0)
		iCount += (int)num * (int)DEVICEDESCLENGTH ;

	XEEReadArray ((DWORD) iCount, DeviceDesc , (BYTE) DEVICEDESCLENGTH); 
	TCPPutArray(sktHTTP, DeviceDesc, DEVICEDESCLENGTH);

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}


//******************************************************************************

void HTTPPrint_AreaTampered (void) {


	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	iCount = AREACOUNT * 2;
	if(TCPIsPutReady(sktHTTP) < iCount)
		return;

	for (iCount = 1; iCount <= AREACOUNT; iCount++) {
		if (iCount > 1) TCPPut(sktHTTP,',');
			TCPPut(sktHTTP, (AreaConfig[iCount-1].IsTampered?'1':'0'));
		}// for iCount

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;

}

//******************************************************************************
