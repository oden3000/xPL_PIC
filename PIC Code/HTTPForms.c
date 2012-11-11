///@file HTTPForms.c
///@brief Processes data from HTML forms
#define __CUSTOMHTTPAPP_C

#include "TCPIP Stack/TCPIP.h"
#include "SecuritySystem.h"
#include "xPL.h"

//******************************************************************************
/** D E C L A R A T I O N S **************************************************/
//******************************************************************************



//******************************************************************************
/** P R I V A T E  P R O T O T Y P E S ***************************************/
//******************************************************************************

static HTTP_IO_RESULT HTTPPostConfig(void);
static void SaveDeviceDescription(int StartAddress, BYTE *p);


//******************************************************************************
//							GET SUBMISSION
//******************************************************************************
/// Processes all HTTP GETS

HTTP_IO_RESULT HTTPExecuteGet(void) {

	BYTE *ptr;
	BYTE filename[20];
	int DeviceID = 0;
	int short ExitDelay;

	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, 20);
	/*
	 #if defined(DEBUG_UART)
	 putrsUART((ROM char*)"HTTP Get received from file:");
	 putsUART(filename);
	 putrsUART((ROM char*)"\r\n");
	 #endif
	 */


	// ######### Zone configuration #########
	if(!memcmppgm2ram(filename, "admin/zoneconfig.htm", 20)) {

		if(AlarmState == Disarmed) {


			//ID of the zone
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"id");
			if(*ptr)
				DeviceID = DecodeIntFromGET(ptr,(ROM BYTE *)"Zone ID");

			// Bypass the zone
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"bypass");
			if(*ptr == '1')  {
				ClearArmingFlags();
				ZoneConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Bypass);
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Bypass of Zone\r\n");
#endif
				return HTTP_IO_DONE;
			}

			// Isolate the zone
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"isolate");
			if(*ptr == '1')  {
				ClearArmingFlags();
				ZoneConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Isolate);
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Isolate\r\n");
#endif
				return HTTP_IO_DONE;
			}


			// Enable the zone
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"enable");
			if(*ptr == '1')  {
				ClearArmingFlags();
				ZoneConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Enable);
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"web enable\r\n");
#endif
				return HTTP_IO_DONE;
			}

			// Normaly Open
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"no");

			if(*ptr == '1')  {
				ZoneConfig[DeviceID].IsNO = TRUE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Normaly Open = True\r\n");
#endif
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}
			if(*ptr == '0') {
				ZoneConfig[DeviceID].IsNO = FALSE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Normaly Open = False\r\n");
#endif
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}


			// Entry Zone
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"entry");

			if(*ptr == '1') {
				ZoneConfig[DeviceID].IsEntry = TRUE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Entry = True\r\n");
#endif
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}
			if(*ptr == '0')  {
				ZoneConfig[DeviceID].IsEntry = FALSE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Entry = False\r\n");
#endif
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}


			// Zone type ( perimeter,interior,24hour)
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"type");
			if((*ptr)&& (*ptr != 'a')) {
				ZoneConfig[DeviceID].ZoneType = DecodeIntFromGET(ptr, (ROM BYTE *)"Zone type");
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}//if(prt)


			// Zone alarm type ( fire, flood gas)
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"alarmtype");
			if((*ptr)&& (*ptr != 'a')) {
				ZoneConfig[DeviceID].ZoneAlarmType = DecodeIntFromGET(ptr, (ROM BYTE *)"Zone alarm type");
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}//if(prt)



			// Zones area list as a single digit integer
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"arealist");
			if((*ptr)&& (*ptr != 'a')) {
				ZoneConfig[DeviceID].AreaList = DecodeIntFromGET(ptr, (ROM BYTE *)"Zone area list");
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
				xPLSaveEEConfig(ZoneConfig, EEDATASTART_ZONE, sizeof(ZoneConfig));	//now save the changes
				return HTTP_IO_DONE;
			}//if(prt)


			// Zones description
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"desc");
			if((*ptr)&& (*ptr != '\0')) {

				iCount = EEDATASTART_DEVICEDESC;
				if (DeviceID != 0)
					iCount += ((int)DeviceID * DEVICEDESCLENGTH) + 1;

				SaveDeviceDescription(iCount, ptr);
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
				return HTTP_IO_DONE;
			}//if(prt)

			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/zoneconfig.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;


		} //	if(AlarmState == Disarmed) {
		else {
			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/disarmconfig_error.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;
		} // AlarmState != Disarmed

	}// zoneconfig.htm

	// ######### Area configuration #########
	else if (!memcmppgm2ram(filename, "admin/areaconfig.htm", 20)) {

		if(AlarmState == Disarmed) {

			//ID of the Area
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"id");
			if(*ptr)
				DeviceID = DecodeIntFromGET(ptr,(ROM BYTE *)"Area ID");

			// Bypass the area
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"bypass");
			if(*ptr == '1')  {
				ClearArmingFlags();
				AreaConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Bypass);
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Bypass of Area\r\n");
#endif
			}


			// Isolate the area
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"isolate");
			if(*ptr == '1')  {
				ClearArmingFlags();
				AreaConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Isolate);
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Isolate Area\r\n");
#endif
			}

			// Enable the area
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"enable");
			if (*ptr == '1')  {
				ClearArmingFlags();
				AreaConfig[DeviceID].IsChangeFlagged = TRUE;
				addxPLMessage (TB_Enable);
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Enable Area\r\n");
#endif
			}


			// Area alarm type ( perimeter,interior,24hour)
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"alarmtype");
			if ((*ptr)&& (*ptr != 'a')) {
				AreaConfig[DeviceID].ZoneAlarmType = DecodeIntFromGET(ptr, (ROM BYTE *)"Zone Alarm Type");
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
			}//if(prt)

			// Area description
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"desc");
			if((*ptr) && (*ptr != '\0')) {

				iCount = (int)EEDATASTART_DEVICEDESC + ((int)DEVICEDESCLENGTH * (int)ZONECOUNT) +1;
				if (DeviceID != 0)
					iCount += DeviceID * (int)DEVICEDESCLENGTH;

				SaveDeviceDescription( iCount, ptr);
				xPLTaskList.GatewayChangedTimer = TickGet() + (GATEWAYCHANGEDDELAY * TICK_SECOND);
			}//if(prt)


			//now save the changes
			xPLSaveEEConfig(AreaConfig, EEDATASTART_AREA, sizeof(AreaConfig));

			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/areaconfig.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;
			return HTTP_IO_DONE;


		} //	if(AlarmState == Disarmed)
		else {
			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/disarmconfig_error.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;
		} // AlarmState != Disarmed


	} // areaconfig.htm

	//######### Alarm Control #############
	else if(!memcmppgm2ram(filename, "control.htm", 11)) {

		// Delay
		ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"delay");

		if(*ptr == 'd') {
			// load default value
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Exit delay, default\r\n");
#endif
			ExitDelay = xPLConfig.ExitDelay;
		} //d

		else if(*ptr)
			ExitDelay = DecodeIntFromGET(ptr,(ROM BYTE *) "Exit delay");

		else
			ExitDelay = xPLConfig.ExitDelay;

		// Control
		ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"control");

		if(*ptr == 'd') {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Disarm Alarm system\r\n");
#endif
			Disarm();
		} //d

		if(*ptr == 'h') {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Arm Home\r\n");
#endif
			ArmHome (ExitDelay);
		} //h


		if(*ptr == 'a') {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Arm Away\r\n");
#endif
			ArmAway (ExitDelay);
		} //a

		if(*ptr == 'l') {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Arm Latch key\r\n");
#endif
			ArmLatchkey (ExitDelay);

		} //l

		if(*ptr == 'p') {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Panic\r\n");
#endif
			Panic ();
		} //p


		// Clean the URL
		strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/control.htm");
		curHTTP.httpStatus = HTTP_REDIRECT;
		return HTTP_IO_DONE;

	} //control.htm

	//######### Alarm Configuration #############
	else if(!memcmppgm2ram(filename, "admin/alarmconfig.ht", 20)) {

		if(AlarmState == Disarmed) {


			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"TestNTP");
			if(*ptr =='1') {
				if (NTPTestResponse == NTP_TEST_DONE) {
					NTPTestResponse = NTP_TEST_PROGRESS;
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" - TEST done state\r\n");
#endif
					if (SNTPState == SM_WAIT || SNTPState == SM_SHORT_WAIT){
						SNTPState = SM_HOME;
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" - TEST SM_HOMe\r\n");
#endif
					}
					else {
						NTPTestResponse == NTP_TEST_TIMEOUT;
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" - TEST timeout from start\r\n");
#endif
					}
				}

				if (NTPTestResponse == NTP_TEST_PROGRESS)
					return HTTP_IO_WAITING;

#if defined(DEBUG_UART)
				putrsUART((ROM char*)" - fuck a duck\r\n");
#endif
				// redirect the to result of the test email
				strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/NTPtest.htm");
				curHTTP.httpStatus = HTTP_REDIRECT;
				return HTTP_IO_DONE;
			}


#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Alarm is being updated\r\n");
#endif


			// Instance ID

			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"Ins");

			if(*ptr) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"instance \r\n");
#endif
				for(iCount = 0;((*ptr != '\0') && (iCount < sizeof(xPLConfig.xPL_My_Instance_ID))); iCount++) {
					if(*ptr >= 'A' && *ptr <= 'Z')
						*ptr -= 'A' - 'a'; //Convert to lower case
					if((*ptr < '0') || (*ptr > 'z') || (*ptr > '9' && *ptr < 'a')) *ptr = '0'; // only accept 0 - 9 and a - z
					xPLConfig.xPL_My_Instance_ID[iCount] = *ptr;
					ptr++;
				}// for
				xPLConfig.xPL_My_Instance_ID[iCount] = '\0';

			} //ptr

			// Heart Beat interval
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"Int");
			if(*ptr)
				xPLConfig.xPL_Heartbeat_Interval = DecodeIntFromGET(ptr, (ROM BYTE *)"Heart Beat interval");

			// Exit delay
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"ExD");
			if(*ptr)
				xPLConfig.ExitDelay = DecodeIntFromGET(ptr, (ROM BYTE *)"Exit Delay");

			// Entry Delay
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"EnD");
			if(*ptr)
				xPLConfig.EntryDelay = DecodeIntFromGET(ptr, (ROM BYTE *)"Entry Delay");

			// Alarming Duration
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"ADu");
			if(*ptr)
				xPLConfig.AlarmingDuration = DecodeIntFromGET(ptr, (ROM BYTE *)"Alarming Duration");

			// Alarming 24hour Duration
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"AD2");
			if(*ptr)
				xPLConfig.Alarming24Duration = DecodeIntFromGET(ptr, (ROM BYTE *)"Alarming 24hour Duration");

			// Siren Delay 24h
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"SD2");
			if(*ptr)
				xPLConfig.SirenDelay24h = DecodeIntFromGET(ptr, (ROM BYTE *)"Siren Delay 24h");

			// Heart Beat interval
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"SDe");
			if(*ptr)
				xPLConfig.SirenDelay = DecodeIntFromGET(ptr, (ROM BYTE *)"Siren Delay");

			// NTP Server name
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"NTP");
			if(*ptr) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"NTP Server \r\n");
#endif
				memset(xPLConfig.NTP_Server, '\0', sizeof(xPLConfig.NTP_Server));

				for(iCount = 0;((*ptr != '\0') && (iCount < sizeof(xPLConfig.NTP_Server))); iCount++) {
					if(*ptr >= 'A' && *ptr <= 'Z')
						*ptr -= 'A' - 'a'; //Convert to lower case
					xPLConfig.NTP_Server[iCount] = *ptr;
					ptr++;
				}// for
				xPLConfig.NTP_Server[iCount] = '\0';

			} //ptr

			// UseAES
			ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"UseAES");
			if(*ptr == '1')
				xPLConfig.Use_AES = TRUE;
			if(*ptr == '0')
				xPLConfig.Use_AES = FALSE;


			// now we save the shit
			xPLSaveEEConfig(&xPLConfig, EEDATASTART_XPL, sizeof(xPLConfig));

			// Clean the URL
			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/alarmconfig.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;
			return HTTP_IO_DONE;



		}//	if(AlarmState != Disarmed)
		else {
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Disarm the alarm before updating the config\r\n");
#endif

			strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/disarmconfig_error.htm");
			curHTTP.httpStatus = HTTP_REDIRECT;
			return HTTP_IO_DONE;
		} // AlarmState != Disarmed


	} //admin/alarmconfig.htm

	//######### User Authentication #############
	else if(!memcmppgm2ram(filename, "admin/users.htm", 14))
		ModifyUser(ptr);

	//######### Send Test Email #############
	else if(!memcmppgm2ram(filename, "email/index.htm", 15)) {

		if (SMPTTestResult == SMTP_TEST_DONE) {
			SMPTTestResult = SMTP_TEST_WAITING;
			SMTPQueueEmail(SMTP_Test);
		}

		if (SMPTTestResult == SMTP_TEST_WAITING)
			return HTTP_IO_WAITING;

		// redirect the to result of the test email
		strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/email/test.htm");
		curHTTP.httpStatus = HTTP_REDIRECT;

	}


	return HTTP_IO_DONE;


}


//******************************************************************************
/**
 Writes the device description to the EE Prom
 */
void SaveDeviceDescription(int StartAddress, BYTE *p) {

	for(iCount = 0; iCount < DEVICEDESCLENGTH; iCount++) {
		XEEBeginWrite(StartAddress + iCount);
		if(iCount > strlen(p))
			XEEWrite('\0');
		else
			XEEWrite(p[iCount]);
		XEEEndWrite();
	   	while(XEEIsBusy());
	}

} // SaveDeviceDescription

//******************************************************************************
/// Decodes and int value from a HTTP buffer (GET)
int DecodeIntFromGET(BYTE *GETptr, rom char * DebugMesg) {

	char Tempdigits[4];

	memset(Tempdigits,'\0',sizeof(Tempdigits));

	for(iCount = 0;(*GETptr != '\0') && (iCount < sizeof(Tempdigits) -1); iCount++) {
		if((*GETptr < '0') || (*GETptr > '9')) *GETptr = '0'; // only accept 0 - 9
		Tempdigits[iCount] = *GETptr;
		GETptr++;
	}// for

#if defined(DEBUG_UART)
	putrsUART(DebugMesg);
	putrsUART((ROM char*)" = ");
	putsUART(Tempdigits);
	putrsUART((ROM char*) "\r\n");
#endif

	if (atoi(Tempdigits) >= 1) return atoi(Tempdigits);
	return 0;
}


//******************************************************************************
//
//							POST SUBMISSION
//
//******************************************************************************
/// Processes HTTP POST's

HTTP_IO_RESULT HTTPExecutePost(void) {

	// Resolve which function to use and pass along
	BYTE filename[20];

	// Load the file name
	// Make sure BYTE filename[] above is large enough for your longest name
	MPFSGetFilename(curHTTP.file, filename, 20);

#if defined(DEBUG_UART)
	putrsUART((ROM char*)"HTTP POST received from file:");
	putsUART(filename);
	putrsUART((ROM char*)"\r\n");
#endif


	if(!memcmppgm2ram(filename, "admin/ipconfig.htm", 18))
		return HTTPPostConfig();

#if defined STACK_USE_DYNAMICDNS_CLIENT

	if(!memcmppgm2ram(filename, "admin/dyndns.htm", 18))
		return HTTPPostDDNSConfig();

#endif // DYNDNS

	if(!strcmppgm2ram((char*)filename, "email/index.htm"))
		return HTTPPostSMTPConfig();

	return HTTP_IO_DONE;
}

//*********************************************************************
/**
 * Function:        HTTP_IO_RESULT HTTPPostConfig(void)
 *
 * PreCondition:    curHTTP is loaded
 *
 * Input:           None
 *
 * Output:          HTTP_IO_DONE on success
 *					HTTP_IO_NEED_DATA if more data is requested
 *					HTTP_IO_WAITING if waiting for asynchronous process
 *
 * Side Effects:    None
 *
 * Overview:        This function reads an input parameter from
 *					the POSTed data, and writes that string to the
 *					AppConfig (NIC related config).
 *
 * Note:            None
 ********************************************************************/

#define HTTP_POST_CONFIG_MAX_LEN	(HTTP_MAX_DATA_LEN - sizeof(AppConfig) - 3)

static HTTP_IO_RESULT HTTPPostConfig(void) {
	APP_CONFIG *app;
	BYTE *ptr;
	WORD len;

	// Set app config pointer to use data array
	app = (APP_CONFIG*)&curHTTP.data[HTTP_POST_CONFIG_MAX_LEN];

	// Use data[0] as a state machine.  0x01 is initialized, 0x02 is error, else uninit
	if(curHTTP.data[0] != 0x01 && curHTTP.data[0] != 0x02)
	{
		// First run, so use current config as defaults
		memcpy((void*)app, (void*)&AppConfig, sizeof(AppConfig));
		app->Flags.bIsDHCPEnabled = 0;
		curHTTP.data[0] = 0x01;
	}


	// if TCPFind == 0xffff means no result
	// curHTTP.byteCount = How many bytes have been read so far
	// TCPIsGetReady = number of bytes available in socket
	// sktHTTP = Access the current socket



	// Loop over all parameters
	while(curHTTP.byteCount) // how many bytes have been read so far
	{
		// Find end of next parameter string
		len = TCPFind(sktHTTP, '&', 0, FALSE);

		// if '&' was not found and number of bytes in socket == bytes read so far
		if(len == 0xffff && TCPIsGetReady(sktHTTP) == curHTTP.byteCount)
			len = TCPIsGetReady(sktHTTP);

		// If there's no end in sight, then ask for more data
		if(len == 0xffff)
			return HTTP_IO_NEED_DATA;

		// Read in as much data as we can
		if(len > HTTP_MAX_DATA_LEN-sizeof(AppConfig))
		{// If there's too much, read as much as possible
			curHTTP.byteCount -= TCPGetArray(sktHTTP, curHTTP.data+1, HTTP_POST_CONFIG_MAX_LEN);
			curHTTP.byteCount -= TCPGetArray(sktHTTP, NULL, len - HTTP_POST_CONFIG_MAX_LEN);
			curHTTP.data[HTTP_POST_CONFIG_MAX_LEN-1] = '\0';
		}
		else
		{// Otherwise, read as much as we wanted to
			curHTTP.byteCount -= TCPGetArray(sktHTTP, curHTTP.data+1, len);
			curHTTP.data[len+1] = '\0';
		}

		// Decode the string
		HTTPURLDecode(curHTTP.data+1);

		// Compare the string to those we're looking for
		if(!memcmppgm2ram(curHTTP.data+1, "ip\0", 3))
		{
			if(StringToIPAddress(&curHTTP.data[3+1], &(app->MyIPAddr)))
				memcpy((void*)&(app->DefaultIPAddr), (void*)&(app->MyIPAddr), sizeof(IP_ADDR));
			else
				curHTTP.data[0] = 0x02;
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "gw\0", 3))
		{
			if(!StringToIPAddress(&curHTTP.data[3+1], &(app->MyGateway)))
				curHTTP.data[0] = 0x02;
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "subnet\0", 7))
		{
			if(StringToIPAddress(&curHTTP.data[7+1], &(app->MyMask)))
				memcpy((void*)&(app->DefaultMask), (void*)&(app->MyMask), sizeof(IP_ADDR));
			else
				curHTTP.data[0] = 0x02;
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "dns1\0", 5))
		{
			if(!StringToIPAddress(&curHTTP.data[5+1], &(app->PrimaryDNSServer)))
				curHTTP.data[0] = 0x02;
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "dns2\0", 5))
		{
			if(!StringToIPAddress(&curHTTP.data[5+1], &(app->SecondaryDNSServer)))
				curHTTP.data[0] = 0x02;
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "mac\0", 4))
		{
			WORD_VAL w;
			BYTE i, mac[12];

			ptr = &curHTTP.data[4+1];

			for(i = 0; i < 12; i++)
			{// Read the MAC address

				// Skip non-hex bytes
				while( *ptr != 0x00 && !(*ptr >= '0' && *ptr < '9') && !(*ptr >= 'A' && *ptr <= 'F') && !(*ptr >= 'a' && *ptr <= 'f') )
					ptr++;

				// MAC string is over, so zeroize the rest
				if(*ptr == 0x00)
				{
					for(; i < 12; i++)
						mac[i] = '0';
					break;
				}

				// Save the MAC byte
				mac[i] = *ptr++;
			}

			// Read MAC Address, one byte at a time
			for(i = 0; i < 6; i++)
			{
				w.v[1] = mac[i*2];
				w.v[0] = mac[i*2+1];
				app->MyMACAddr.v[i] = hexatob(w);
			}
		}
		else if(!memcmppgm2ram(curHTTP.data+1, "host\0", 5))
		{
			memset(app->NetBIOSName, ' ', 15);
			app->NetBIOSName[15] = 0x00;
			memcpy((void*)app->NetBIOSName, (void*)&curHTTP.data[5+1], strlen((char*)&curHTTP.data[5+1]));
			strupr((char*)app->NetBIOSName);
		}

		// Trash the separator character
		while( TCPFind(sktHTTP, '&', 0, FALSE)  == 0 ||
			  TCPFind(sktHTTP, '\r', 0, FALSE) == 0 ||
			  TCPFind(sktHTTP, '\n', 0, FALSE) == 0	 )
		{
			curHTTP.byteCount -= TCPGet(sktHTTP, NULL);
		}

	}


	//We dont want DHCP as an option.
	app->Flags.bIsDHCPEnabled = 0;

	// Check if all settings were successful
	if(curHTTP.data[0] == 0x01)
	{// Save the new AppConfig

		ptr = (BYTE*)app;
#if defined(MPFS_USE_EEPROM)
	    XEEBeginWrite(EEDATASTART_CONFIG);
	    XEEWrite(XPLEECONFIGVERSION);
	    for (len = 0; len < sizeof(AppConfig); len++ )
	        XEEWrite(*ptr++);
	    XEEEndWrite();
        while(XEEIsBusy());
#endif

		// Set the board to reboot to the new address
		strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/reboot.htm?");
		memcpy((void*)(curHTTP.data+18), (void*)app->NetBIOSName, 16);
		ptr = curHTTP.data;
		while(*ptr != ' ' && *ptr != '\0')
			ptr++;
		*ptr = '\0';
	}
	else
	{// Error parsing IP, so don't save to avoid errors
		strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/config_error.htm");
	}

	curHTTP.httpStatus = HTTP_REDIRECT;

	return HTTP_IO_DONE;
}



//******************************************************************************

