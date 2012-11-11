///@file SecurityXPLSender.c
///@brief Sends all security xPL messages
#include "TCPIP Stack/TCPIP.h"
#include "xPL.h"
#include "SecuritySystem.h"


//*********************************************************************
/**
Adds a new xPL message into the queue for transmittion.
Caller populates IsChangeFlagged flag on the zone and area that should be applied.
Note: Additional values should be populated manually 

@param TaskBuffered, type of xPL message that will be sent
@result returns the task number used to populate additional task info
*/
int addxPLMessage (TaskBuffers TaskBuffered) {

	int iCounter, Result;

	#if defined(DEBUG_UART)
		char TempString[3]; // used for debug only
		putrsUART((ROM char*)"xPL message placed in queue \r\n  - task position");
		uitoa(xPLTaskList.FreePosition,  TempString);
		putsUART(TempString);
		putrsUART((ROM char*)"\r\n");
	#endif

	// set new properties of the task
	xPLTaskList.Task[xPLTaskList.FreePosition].TaskBuffered = TaskBuffered;
	Result = xPLTaskList.FreePosition;
	xPLTaskList.Task[Result].TaskState = 0;

	// populate zone array from the current IsChangeFlagged values
	for(iCounter =0; iCounter < ZONECOUNT; iCounter++)
		 xPLTaskList.Task[xPLTaskList.FreePosition].FlaggedZones[iCounter] = ZoneConfig[iCounter].IsChangeFlagged;

	// populate area array from the current IsChangeFlagged values
	for(iCounter =0; iCounter < AREACOUNT; iCounter++)
		 xPLTaskList.Task[xPLTaskList.FreePosition].FlaggedAreas[iCounter] = AreaConfig[iCounter].IsChangeFlagged;

	ClearArmingFlags();

	// Increaments the Free position, ready for the next task.
	if (xPLTaskList.FreePosition < (XPLBUFFERSIZE-1))
		xPLTaskList.FreePosition++;
	else if(xPLTaskList.FreePosition >= (XPLBUFFERSIZE-1))
		xPLTaskList.FreePosition =0;
		
return Result;
}





/*********************************************************************
 *********************************************************************
 The sending of messages
 *********************************************************************
 ********************************************************************/
	


//*********************************************************************
 /** 
 
  Create and send the xPL message for a change in state within a zone. 
  This only covers Tamper, Trigger and normal events, not alarmed
  
  @param MessageType only support Tamper, Trigger and normal events
  @param TheZone
 */
 
void SendZoneStateMsg (Zone_States MessageType, int TheZone) {

	char strZoneID[3];
	
	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendZoneStateMsg, no MAC transmit port\r\n");
#endif
	}
	else {
		
		itoa(TheZone, strZoneID);

		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zone);
			
			
		if (MessageType == StateTrigger) {
			UDPPutROMString((ROM BYTE*) "event=alert\nzone=");
			UDPPutArray(strZoneID,strlen(strZoneID));
			UDPPutROMString((ROM BYTE*) "\n}\n");

		}
		else if (MessageType == StateTamper) {
			UDPPutROMString((ROM BYTE*) "event=tamper\nzone=");
			UDPPutArray(strZoneID,strlen(strZoneID));
			UDPPutROMString((ROM BYTE*) "\n}\n");
			
		}
		else if (MessageType == StateNormal) {
			UDPPutROMString((ROM BYTE*) "event=normal\nzone=");
			UDPPutArray(strZoneID,strlen(strZoneID));
			UDPPutROMString((ROM BYTE*) "\n}\n");
			
		}

		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)
}// SendZoneStateMsg

//*********************************************************************
 /** 
 
  Create and send the xPL message for a change in state within a area. 
  This only covers Tamper, Trigger and normal events, not alarmed.
  This needs to be called until the result = 0
  
  @param MessageType only support Tamper, Trigger and normal events
  @param TheZone, yes zone to send area messages
  @param StartPoint, is the starting area, used to resume from previous point
  @result 0 = no message sent, 1 = message sent & must call again
 */
 
 
int SendAreaStateMsg (Zone_States CurrentZoneState, int TheZone, int StartPoint) {

	int iCounter, TheArea;
	char TheAreaTxt[2];
	BOOL FirstArea = TRUE;
	BOOL UpdateRequired = FALSE;


	// Check to see if this zone is a member of any area(s), if not no update is required
	// if it we save the area id for later
	for (iCounter = StartPoint; iCounter <= AREACOUNT-1; iCounter++) {
		if (isZoneMemberOfArea(TheZone, iCounter) == TRUE)  {
			
			// if tampered we only update the area state if the state is normal, triggered or tampered
			if (CurrentZoneState == StateTamper) {
				if ((AreaConfig[iCounter].State == StateNormal) || (AreaConfig[iCounter].State == StateTrigger) || (AreaConfig[iCounter].State == StateTamper)) {
					UpdateRequired = TRUE;
					TheArea = iCounter;
					break; 
				}	
			}	
			
			// if triggered we only update the area state if the state is normal or triggered
			if (CurrentZoneState == StateTrigger) {
				if ((AreaConfig[iCounter].State == StateNormal) || (AreaConfig[iCounter].State == StateTrigger)) {
					UpdateRequired = TRUE;
					TheArea = iCounter;
					break; // \todo check this does not stop multiple areas from working
				}	
			}	
			
			// if normal we only update is area is normal
			if (CurrentZoneState == StateNormal) {
				if (AreaConfig[iCounter].State == StateNormal) {
					UpdateRequired = TRUE;
					TheArea = iCounter;
					break; 
				}	
			}	
			
		}// isZoneMemberOfArea
	}// iCounter

	if (UpdateRequired == FALSE) {
		// Do nothing
		return 0;
	}


	//Send the message for this area
	if (!UDPIsPutReady(XPLSocket)) { //Check if we have a MAC transmit port, if not then shit yourself
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendAreaStateMsg, no MAC transmit port\r\n");
#endif
	}

	else {
		// Generate xPL head with type security.zone
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Area);

		// Trigger event
		if (CurrentZoneState == StateTrigger) 
			UDPPutROMString((ROM BYTE*) "event=alert\narea=");
		
		// Tamper event
		if (CurrentZoneState == StateTamper) 
			UDPPutROMString((ROM BYTE*) "event=tamper\narea=");
		
		// Normal event
		if (CurrentZoneState == StateNormal) 
			UDPPutROMString((ROM BYTE*) "event=normal\narea=");
		
		itoa(TheArea, TheAreaTxt);
		UDPPutArray(TheAreaTxt,strlen(TheAreaTxt));
	
		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)

return ++TheArea;

} // SendAreaStateMsg

/*********************************************************************
 Function:        SendAlarmingMsg
 PreCondition:    None
 Input:           ZoneID
 Output:          None
 Side Effects:    None
 Overview:
/// Create and send the xPL message for when the alarm is triggered
/// \todo what should be in the panic message ??
 ********************************************************************/
void SendAlarmingMsg (int TheZone) {

	BOOL isFirstZone = TRUE;
	char TempBuffer[3];


	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendAlarmingMsg, no MAC transmit port\r\n");
#endif
	}
	else {

		//Generate xPL head with type security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=alarm");
		if (TheZone != 99 ) { // Zone 99 is for Panic
			UDPPutROMString((ROM BYTE*) "\nzone-list=");
			itoa(TheZone, TempBuffer);
			UDPPutArray(TempBuffer,strlen(TempBuffer));
			UDPPutROMString((ROM BYTE*) "\narea-list=");

			for (iCount = 0; iCount < AREACOUNT; iCount++) {
				if (isZoneMemberOfArea(TheZone, iCount) == TRUE) {
					if (!isFirstZone) UDPPut(',');
					isFirstZone = FALSE;
					UDPPut('0'+iCount);
				}// isZoneMemberOfArea
			}// for iCount

			//alarm-type=Alarm type of the zone triggered
			UDPPutROMString((ROM BYTE*) "\nalarm-type=");
			iCount = ZoneConfig[TheZone].ZoneAlarmType;
			UDPPutROMString(AlarmTypesText[iCount]);

		} // TheZone != 99 ie Panic



		UDPPutROMString((ROM BYTE*) "\n}\n");


#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendAlarmingMsg Message Sent\r\n");

		putrsUART((ROM char*)"Zone: ");
		itoa(TheZone, xPLMsgKey);
		putsUART(xPLMsgKey);

		if (TheZone == 99 )
			putrsUART((ROM char*)" Panic Alarm");
		else {
			putrsUART((ROM char*)"ZoneAlarmType: ");
			itoa(ZoneConfig[TheZone].ZoneAlarmType, xPLMsgKey);
			putsUART(xPLMsgKey);
			putrsUART((ROM char*)"Text: ");
			putrsUART(AlarmTypesText[iCount]);
		}
		putrsUART((ROM char*)"\r\n");


#endif


		// Jobs not over until you have wiped and flushed
		UDPFlush();
	} // UDPIsPutReady(XPLSocket)

} // SendAlarmingMsg

/*********************************************************************
 Function:        SendGatewayInfo
 PreCondition:    None
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message for gateway info
 ********************************************************************/
void SendGatewayInfo (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendGatewayInfo, no MAC transmit port\r\n");
#endif
	}
	else {

		//Generate xPL head with type security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateinfo);
		UDPPutROMString((ROM BYTE*) "protocol=xPL\n");
		UDPPutROMString((ROM BYTE*) "description=Alarm system designed for Home Automation\n");
		UDPPutROMString((ROM BYTE*) "version=Alpha release\n");
		UDPPutROMString((ROM BYTE*) "author=Jarrod Neven\n");
		UDPPutROMString((ROM BYTE*) "info-url=www.neven.info\n");
		UDPPutROMString((ROM BYTE*) "zone-count=");
		itoa(ZONECOUNT, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		UDPPut('\n');
		UDPPutROMString((ROM BYTE*) "area-count=");
		itoa(AREACOUNT, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		UDPPut('\n');
		UDPPutROMString((ROM BYTE*) "gateway-commands=arm|arm-home|arm-away|arm-latchkey|disarm|panic|isolate|bypass|enable\n");
		UDPPutROMString((ROM BYTE*) "zone-commands=arm|isolate|bypass|enable\n");
		UDPPutROMString((ROM BYTE*) "area-commands=arm,isolate,bypass,enable\n");

		UDPPutROMString((ROM BYTE*) "}\n");

		// Now transmit it.
		UDPFlush();


#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendGatewayInfo Message Sent\r\n");
#endif

	} // UDPIsPutReady(XPLSocket)


} //SendGatewayInfo


/*********************************************************************
 Function:        SendGatewayReady
 PreCondition:    None
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message to state the gateway is ready for business
 ********************************************************************/
void SendGatewayReady (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendGatewayInfo, no MAC transmit port\r\n");
#endif
	}
	else {

		//Generate xPL head with type security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=ready\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendGatewayReady Message Sent\r\n");
#endif

	} // UDPIsPutReady(XPLSocket)


} //SendGatewayReady

/*********************************************************************
 Function:        SendArmed
 PreCondition:    None
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message for when the alarm is armed
 ********************************************************************/
void SendArmed (void) {

	BOOL isFirst;
	char TempVal[3];

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendArmed, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=armed\n");

		// Zone list
		UDPPutROMString((ROM BYTE*) "zone-list=");
		isFirst = TRUE;
		for (iCount = 0; iCount < ZONECOUNT; iCount++) {
			if(ZoneConfig[iCount].IsArmed == TRUE) {
				if (!isFirst) UDPPut(',');
				isFirst = FALSE;
				itoa(iCount, TempVal);
				UDPPutArray(TempVal,strlen(TempVal));
			}// isArmed
		}// for iCount
		UDPPutROMString((ROM BYTE*) "\n");

		// area list
		UDPPutROMString((ROM BYTE*) "area-list=");
		isFirst = TRUE;
		for (iCount = 0; iCount < AREACOUNT; iCount++) {
			if(AreaConfig[iCount].IsArmed == TRUE) {
				if (!isFirst) UDPPut(',');
				isFirst = FALSE;
				itoa(iCount, TempVal);
				UDPPutArray(TempVal,strlen(TempVal));
			}// isArmed
		}// for iCount
		UDPPutROMString((ROM BYTE*) "\n");


		if (UserID[0] != '\0') {
			UDPPutROMString((ROM BYTE*) "user=");
			UDPPutString(UserID);
			UDPPutROMString((ROM BYTE*) "\n");
		}

		UDPPutROMString((ROM BYTE*) "}\n");


		// Now transmit it.
		UDPFlush();

		// no more need for UserID to be populated
		memset(UserID,'\0',USERNAMELENGTH);


#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendArmed Message Sent\r\n");
#endif


	} //UDPIsPutReady(XPLSocket)
} // SendArmed

/*********************************************************************
 Function:        SendError
 PreCondition:    None
 Input:           Error text
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message to communicate error events
 ********************************************************************/
void SendError (rom char * ErrorMessage) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendError, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=error");
		UDPPutROMString((ROM BYTE*) "error=");

		UDPPutROMString((ROM BYTE*) ErrorMessage);

		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

	} //UDPIsPutReady(XPLSocket)

} // SendError
/*********************************************************************
 Function:        SendDisarm
 PreCondition:    None
 Input:           UserID
 Output:          None
 Side Effects:    None
 Overview:
/// Create and send the xPL message to communicate that the alarm has ben disarmed
 ********************************************************************/
void SendDisarm (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendDisarm, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Gateway);
		UDPPutROMString((ROM BYTE*) "event=disarmed\nuser=");
		UDPPutString(UserID);
		UDPPutROMString((ROM BYTE*) "\n}\n");
		// Now transmit it.
		UDPFlush();


		// no more need for UserID to be populated
		memset(UserID,'\0',USERNAMELENGTH);

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendDisarm Message Sent\r\n");
#endif



	}//UDPIsPutReady(XPLSocket)

} // SendDisarm

/*********************************************************************
 Function:        SendZoneList
 PreCondition:    None
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
/// Create and send the xPL message of Zone List
 ********************************************************************/

void SendZoneList (void) {

	BOOL isFirstZone = TRUE;

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendZoneList, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zonelist);
		UDPPutROMString((ROM BYTE*) "zone-list=");

		for (iCount = 0; iCount < ZONECOUNT; iCount++) {
			if (!isFirstZone) UDPPut(',');
			isFirstZone = FALSE;
			itoa(iCount, xPLMsgKey);
			UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		}// for iCount

		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)

} // SendZoneList


/*********************************************************************
 Function:        SendAreaList
 PreCondition:    None
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message of Area List
 ********************************************************************/

void SendAreaList (void) {

	int short iCount;
	BOOL isFirstZone = TRUE;

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendAreaList, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Arealist);
		UDPPutROMString((ROM BYTE*) "area-list=");

		for (iCount = 0; iCount < AREACOUNT; iCount++) {
			if (!isFirstZone) UDPPut(',');
			isFirstZone = FALSE;
			UDPPut('0'+iCount);
		}// for iCount

		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)

} // SendAreaList


/*********************************************************************
 Function:        SendZoneInfo
 PreCondition:    None
 Input:           ZoneID
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message of Zone Info
 ********************************************************************/
/*
 security.zoneinfo
 zone=id
 zone-type= determine the zone type and translate it to text from the bit map
 alarm-type= determine the alarm type and translate to text from the zone bit map
 area-count= count of the areas listed in the zone bit map
 area-list= list all of the areas in the zone bit map
 */

void SendZoneInfo (int TheZone) {

	int short iTotal;
	BOOL isFirstZone = TRUE;

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendZoneInfo, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Zoneinfo);
		UDPPutROMString((ROM BYTE*) "zone=");
		itoa(TheZone, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
		UDPPutROMString((ROM BYTE*) "\nzone-type=");
		UDPPutROMString((ROM BYTE*) ZoneTypesText[ZoneConfig[TheZone].ZoneType]);
		UDPPutROMString((ROM BYTE*) "\nalarm-type=");
		UDPPutROMString((ROM BYTE*) AlarmTypesText[ZoneConfig[TheZone].ZoneAlarmType]);
		UDPPutROMString((ROM BYTE*) "\narea-list=");

		iTotal = 0;
		for (iCount = 0; iCount < AREACOUNT; iCount++) {
			if (isZoneMemberOfArea(TheZone, iCount) == TRUE) {
				if (!isFirstZone) UDPPut(',');
				isFirstZone = FALSE;
				UDPPut('0'+ iCount);
				iTotal++;
			} // isZoneMemberOfArea(TheZone, iCount)
		}// for iCount

		UDPPutROMString((ROM BYTE*) "\narea-count=");
		itoa(iTotal, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));

		UDPPutROMString((ROM BYTE*) "\nroom=");

		iCount = EEDATASTART_DEVICEDESC;
		if (TheZone != 0)
			iCount += ((int)TheZone * DEVICEDESCLENGTH) + 1;

		XEEReadArray ((DWORD) iCount, xPLMsgKey , (BYTE) DEVICEDESCLENGTH);
		TCPPutArray(sktHTTP, xPLMsgKey, DEVICEDESCLENGTH);

		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)

} // SendZoneInfo


/*********************************************************************
 Function:        SendAreaInfo
 PreCondition:    None
 Input:           AreaID
 Output:          None
 Side Effects:    None
 Overview:
 /// Create and send the xPL message of Area Info
 ********************************************************************/

/*
 security.areainfo

 area=id
 zone-count= a count of the zones that are listed in this area
 zone-list= list of zones in this area
 */

void SendAreaInfo (int TheArea) {

	int short iTotal;
	BOOL isFirstZone = TRUE;

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendAreaInfo, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Security,Type_Areainfo);
		UDPPutROMString((ROM BYTE*) "area=");
		UDPPut('0'+ TheArea);
		UDPPutROMString((ROM BYTE*) "\nzone-list=");

		iTotal = 0;
		for (iCount = 0; iCount < ZONECOUNT; iCount++) {
			if (isZoneMemberOfArea(iCount, TheArea) == TRUE) {
				if (!isFirstZone) UDPPut(',');
				isFirstZone = FALSE;
				itoa(iCount, xPLMsgKey);
				UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));
				iTotal++;
			} // ZoneConfig
		}// for iCount

		UDPPutROMString((ROM BYTE*) "\nzone-count=");
		itoa(iTotal, xPLMsgKey);
		UDPPutArray(xPLMsgKey,strlen(xPLMsgKey));

		UDPPutROMString((ROM BYTE*) "\nroom=");

		iCount = (int)EEDATASTART_DEVICEDESC + ((int)DEVICEDESCLENGTH * (int)ZONECOUNT) + 1;
		if (TheArea != 0)
			iCount += (int)TheArea * (int)DEVICEDESCLENGTH ;

		XEEReadArray ((DWORD) iCount, xPLMsgKey , (BYTE) DEVICEDESCLENGTH);
		TCPPutArray(sktHTTP, xPLMsgKey, DEVICEDESCLENGTH);

		UDPPutROMString((ROM BYTE*) "}\n");

		// Now transmit it.
		UDPFlush();

	}//UDPIsPutReady(XPLSocket)

} // SendAreaInfo



/*********************************************************************
 Function:        SendConfigList
 PreCondition:    Config request message must have been received
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Constructs the config list
 ********************************************************************/

void SendConfigList (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendConfigList, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_List);
		UDPPutROMString((ROM BYTE*) "reconf=newconf\n");
		UDPPutROMString((ROM BYTE*) "option=interval\n");
		UDPPutROMString((ROM BYTE*) "option=ip\n");
		UDPPutROMString((ROM BYTE*) "option=subnet\n");
		UDPPutROMString((ROM BYTE*) "}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendConfigList sent");
#endif


	}//UDPIsPutReady(XPLSocket)

} // SendAreaList


/*********************************************************************
 Function:        SendConfigCurrent
 PreCondition:    Config Current message must have been received
 Input:           None
 Output:          None
 Side Effects:    None
 Overview:
 /// Constructs the config list
 ********************************************************************/

void SendConfigCurrent (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendConfigCurrent, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Current);

		UDPPutROMString((ROM BYTE*) "newconf=");
		for (iCount = 0; ! xPLConfig.xPL_My_Instance_ID[iCount] == '\0'; iCount++) {
			UDPPut(xPLConfig.xPL_My_Instance_ID[iCount]);
		}
		UDPPutROMString((ROM BYTE*) "\n");

		UDPPutROMString((ROM BYTE*) "interval=");
		memset(xPLMsgKey,'\0',xPLMSGKEYLENGTH);
		uitoa((WORD)xPLConfig.xPL_Heartbeat_Interval, xPLMsgKey);
		UDPPutString(xPLMsgKey);
		UDPPutROMString((ROM BYTE*) "\n");

		UDPPutROMString((ROM BYTE*) "ip=");
		SendIPValue(AppConfig.MyIPAddr);
		UDPPutROMString((ROM BYTE*) "\n");

		UDPPutROMString((ROM BYTE*) "subnet=");
		SendIPValue(AppConfig.MyMask);
		UDPPutROMString((ROM BYTE*) "\n");


		UDPPutROMString((ROM BYTE*) "}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendConfigCurrent sent");
#endif

	}//UDPIsPutReady(XPLSocket)

} // SendAreaList


/*********************************************************************
 Function:        SendIPValue
 PreCondition:    XPLSendHeader must have been called
 Input:           None
 Output:          UDP data
 Side Effects:    None
 Overview:
 /// Sends the string value of an IP address to the UDP output buffer
 ********************************************************************/

void SendIPValue(IP_ADDR IPVal) {
    BYTE IPDigit[4];
	BYTE i;

	for(i = 0; i < sizeof(IP_ADDR); i++) {
	    uitoa((WORD)IPVal.v[i], IPDigit);
		UDPPutString(IPDigit);
		if(i == sizeof(IP_ADDR)-1)
			break;
		UDPPutROMString((ROM char *)".");
	}

} // SendIPValue

/*********************************************************************
 Function:        SendGatewayChanged
 PreCondition:    None
 Input:           None
 Output:          UDP data
 Side Effects:    None
 Overview:
/// Send the xpl message to say that the device has changed its config
 ********************************************************************/
void SendGatewayChanged (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendGatewayChanged, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Gateway);

		UDPPutROMString((ROM BYTE*) "event=changed\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendGatewayChanged sent");
#endif

	}//UDPIsPutReady(XPLSocket)


} //SendGatewayChanged

/*********************************************************************
 Function:        SendBatteryStat
 PreCondition:    None
 Input:           None
 Output:          UDP data
 Side Effects:    None
 Overview:
 /// Send the xpl message to announce a change in the battery state
 ********************************************************************/
void SendBatteryStat (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendBatteryStat, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Gateway);

		if (VoltageBattery <  (unsigned int) BATTERY_LOW)
			UDPPutROMString((ROM BYTE*) "event=low-battery\n}\n");
		else
			UDPPutROMString((ROM BYTE*) "event=battery-ok\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendBatteryStat sent\r\n");
#endif

	}//UDPIsPutReady(XPLSocket)
} //SendBatteryStat

/*********************************************************************
 Function:        SendACStat
 PreCondition:    None
 Input:           None
 Output:          UDP data
 Side Effects:    None
 Overview:
///  Send the xpl message to announce a change in the battery state
 ********************************************************************/
void SendACStat (void) {

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendACStat, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Gateway);

		if (BatteryState == Battery_InUse)
			UDPPutROMString((ROM BYTE*) "event=ac-fail\n}\n");
		else
			UDPPutROMString((ROM BYTE*) "event=ac-restored\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendACStat sent");
#endif

	}//UDPIsPutReady(XPLSocket)
} //SendACStat


/*********************************************************************
 Function:        SendGateStat
 PreCondition:    None
 Input:           None
 Output:          UDP data
 Side Effects:    None
 Overview:
 /// Send the xpl message relating to the state of the gateway
 ********************************************************************/
void SendGateStat (void) {

	char TempVal[3];
	BOOL isFirstArea;

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendGateStat, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Gatestat);

		// AC power state
		UDPPutROMString((ROM BYTE*) "ac-fail=");
		if (BatteryState == Battery_InUse)
			UDPPutROMString((ROM BYTE*) "true\n");
		else
			UDPPutROMString((ROM BYTE*) "false\n");

		// internal battery state
		UDPPutROMString((ROM BYTE*) "low-battery=");
		if (VoltageBattery <  (unsigned int) BATTERY_LOW)
			UDPPutROMString((ROM BYTE*) "true\n");
		else
			UDPPutROMString((ROM BYTE*) "false\n");

		// Alarm state
		UDPPutROMString((ROM BYTE*) "status=");
		if ((AlarmState == Armed) || (AlarmState == Armed_LatchKey) )
			UDPPutROMString((ROM BYTE*) "armed\n");
		else if ( AlarmState == Disarmed)
			UDPPutROMString((ROM BYTE*) "disarmed\n");
		else { // must be in an alarming state
			UDPPutROMString((ROM BYTE*) "alarm\n");
			uitoa(LastZoneAlarmed,  TempVal);
			UDPPutROMString((ROM BYTE*) "zone-list=");
			UDPPutString(xPLMsgKey);
			UDPPutROMString((ROM BYTE*) "\n");
			UDPPutROMString((ROM BYTE*) "area-list=");

			isFirstArea = TRUE;
			for (iCount = 0; iCount < AREACOUNT; iCount++) {
				if (isZoneMemberOfArea(LastZoneAlarmed, iCount) == TRUE) {
					if (!isFirstArea) UDPPut(',');
					isFirstArea = FALSE;
					UDPPut('0'+iCount);
				}// isZoneMemberOfArea
			}// for iCount

			UDPPutROMString((ROM BYTE*) "\n");
			UDPPutROMString((ROM BYTE*) "alarm-type=");
			UDPPutROMString(AlarmTypesText[ZoneConfig[LastZoneAlarmed].ZoneAlarmType]);
			UDPPutROMString((ROM BYTE*) "\n");
		}// alarm state

		UDPPutROMString((ROM BYTE*) "}\n");
		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendGateStat sent");
#endif

	}//UDPIsPutReady(XPLSocket)


} //SendGateStat



/*********************************************************************
 Function:        SendAreaStat
 PreCondition:    None
 Input:           Area ID
 Output:          UDP data
 Side Effects:    None
 Overview:
/// Send the xpl message relating to the state of the area
 ********************************************************************/
void SendAreaStat (int TheArea) {


	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendAreaStat, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Areastat);

		UDPPutROMString((ROM BYTE*) "area=");
		UDPPut('0'+ TheArea);

		UDPPutROMString((ROM BYTE*) "\narmed=");
		if(AreaConfig[TheArea].IsArmed == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");

		UDPPutROMString((ROM BYTE*) "\nalert=");
		if(AreaConfig[TheArea].IsTriggered == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");

		UDPPutROMString((ROM BYTE*) "\nalarm=");
		if(AreaConfig[TheArea].IsAlarmed == TRUE) {

			UDPPutROMString((ROM BYTE*) "true\nalarm-type=");
			UDPPutROMString(AlarmTypesText[AreaConfig[TheArea].ZoneAlarmType]);

			i2Count =0;
			UDPPutROMString((ROM BYTE*) "\nalarm-zones=");
			for (iCount = 0; iCount < ZONECOUNT; iCount++) {
				if (isZoneMemberOfArea(iCount, TheArea) == TRUE) {
					if (i2Count != 0) UDPPutROMString((ROM BYTE*) ",");
					i2Count = 1;
					memset(xPLMsgKey,'\0',xPLMSGKEYLENGTH);
					uitoa(iCount,  xPLMsgKey);
					UDPPutString(xPLMsgKey);
				}// isZoneMemberOfArea
			}// for
		}//	IsAlarmed == TRUE


		UDPPutROMString((ROM BYTE*) "\ntamper=");
		if(AreaConfig[TheArea].IsTampered == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");


		UDPPutROMString((ROM BYTE*) "\nisolated=");
		if(AreaConfig[TheArea].IsIsolated == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");


		UDPPutROMString((ROM BYTE*) "\nbypassed=");
		if(AreaConfig[TheArea].IsBypassed == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");


		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendAreaStat sent");
#endif

	}//UDPIsPutReady(XPLSocket)


} //SendGateStat





/*********************************************************************
 Function:        SendZoneStat
 PreCondition:    None
 Input:           Area ID
 Output:          UDP data
 Side Effects:    None
 Overview:
 /// Send the xpl message relating to the state of the area
 ********************************************************************/
void SendZoneStat (int TheZone) {


	char TempVal[3];

	//Check if we have a MAC transmit port, if not then shit yourself
	if (!UDPIsPutReady(XPLSocket)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"#ERROR# SendZoneStat, no MAC transmit port\r\n");
#endif
	}
	else {
		//security.gateway
		XPLSendHeader(xPL_STATUS, TRUE,Class_Config,Type_Areastat);			

		UDPPutROMString((ROM BYTE*) "zone=");
		uitoa(TheZone,  TempVal);
		UDPPut((char *)TempVal);

		UDPPutROMString((ROM BYTE*) "\narmed=");
		if(ZoneConfig[TheZone].IsArmed == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");

		UDPPutROMString((ROM BYTE*) "\nalert=");
		if(ZoneConfig[TheZone].IsTriggered == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");

		//state
		UDPPutROMString((ROM BYTE*) "\nstate=");
		if (ZoneConfig[TheZone].IsIsolated) 
			UDPPutROMString((ROM BYTE*) "isolated");
		else if (ZoneConfig[TheZone].IsBypassed) 
			UDPPutROMString((ROM BYTE*) "bypassed");
		else 
			UDPPutROMString((ROM BYTE*) "enabled");


		UDPPutROMString((ROM BYTE*) "\nalarm=");
		if(ZoneConfig[TheZone].IsAlarmed == TRUE) {

			UDPPutROMString((ROM BYTE*) "true\nalarm-type=");
			UDPPutROMString(AlarmTypesText[ZoneConfig[TheZone].ZoneAlarmType]);
		}//	IsAlarmed == TRUE

		UDPPutROMString((ROM BYTE*) "\ntamper=");
		if(ZoneConfig[TheZone].IsTampered == TRUE)
			UDPPutROMString((ROM BYTE*) "true");
		else
			UDPPutROMString((ROM BYTE*) "false");


		UDPPutROMString((ROM BYTE*) "\n}\n");

		// Now transmit it.
		UDPFlush();

#if defined(DEBUG_UART)
		putrsUART((ROM char*)"SendZoneStat sent");
#endif

	}//UDPIsPutReady(XPLSocket)


} //SendGateStat




//******************************************************************************
//*                                The End
//******************************************************************************

