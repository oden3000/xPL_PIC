///@file SMTP.c
///@brief All things Email

#define __SMTPDEMO_C

#include "TCPIP Stack/TCPIP.h"
#include "MainDemo.h"
#include "SecuritySystem.h"
#include "xPL.h"


 //******************************************************************************
 /** D E C L A R A T I O N S **************************************************/
 //******************************************************************************

/// Text values of config option 'when to send emails'
static ROM char *WhenToSendEmailText[] =
	{
		"No Emails",
		"Latchkey Only",
		"Inital Alarm",
		"All Alarms",
		"Every Event"
	};

/// State machine for sending out emails
SMTPTaskLists SMTPTaskList;

/// testing email
int SMPTTestResult;

 //******************************************************************************
 /** P R I V A T E  P R O T O T Y P E S ***************************************/
 //******************************************************************************


//*****************************************************************************
/// Prints who the email is addressed to
void HTTPPrint_SMTP_To (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(xPLConfig.SMTP_To))
		return;

	TCPPutArray(sktHTTP, xPLConfig.SMTP_To, strlen(xPLConfig.SMTP_To));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

//*****************************************************************************
/// Prints the POP3 sever
void HTTPPrint_SMTP_Server (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(xPLConfig.SMTP_Server))
		return;

	TCPPutArray(sktHTTP, xPLConfig.SMTP_Server, strlen(xPLConfig.SMTP_Server));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}
//*****************************************************************************
/// Prints the user name to login to the POP3 server
void HTTPPrint_SMTP_User (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(xPLConfig.SMTP_User))
		return;

	TCPPutArray(sktHTTP, xPLConfig.SMTP_User, strlen(xPLConfig.SMTP_User));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}
//*****************************************************************************
/// Prints the password to login to the POP3 server
void HTTPPrint_SMTP_Password (void) {

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(xPLConfig.SMTP_Password))
		return;

	TCPPutArray(sktHTTP, xPLConfig.SMTP_Password, strlen(xPLConfig.SMTP_Password));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}
//*****************************************************************************
/// Port number that the POP3 server runs from
void HTTPPrint_SMTP_Port (void) {

	char Tempdigits[6];

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(Tempdigits))
		return;

	uitoa(xPLConfig.SMTP_Port,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}
/*****************************************************************************
void HTTPPrint_SMTP_UseSSL (void) {

	char Tempdigits[3];

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < sizeof(Tempdigits))
		return;

	uitoa(xPLConfig.SMTP_UseSSL,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}
*/
//*****************************************************************************
/// Prints if email service is active
void HTTPPrint_SMTP_Active (void) {

	TCPPut(sktHTTP, (xPLConfig.SMTP_Active?'1':'0'));
	return;
}
//*****************************************************************************
/// Prints if emails will only be sent on the first trigger of the alarm
void HTTPPrint_SMTP_OnlyInitalAlarm (void) {

	TCPPut(sktHTTP, (xPLConfig.SMTP_OnlyInitalAlarm?'1':'0'));
	return;
}
//*****************************************************************************
/// Prints if emails should be sent on disarm
void HTTPPrint_SMTP_ArmDisarm (void) {

	TCPPut(sktHTTP, (xPLConfig.SMTP_ArmDisarm?'1':'0'));
	return;
}

//*****************************************************************************
/// Prnits test data for P0P3 results
void HTTPPrint_SMTPTestOutput (void) {



	char Tempdigits[6];

	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;

	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < 17)
		return;

	if (SMPTTestResult == SMTP_SUCCESS)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Successful");
	else if (SMPTTestResult == SMTP_RESOLVE_ERROR)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"DNS error");
	else if (SMPTTestResult == SMTP_CONNECT_ERROR)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Connection error");
	else if (SMPTTestResult == 554)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Transaction failed");		
	else if (SMPTTestResult == 550)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Requested action not taken: mailbox unavailable");
    else if (SMPTTestResult == 552)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Requested mail action aborted: exceeded storage allocation");		
    else if (SMPTTestResult == 553)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Requested action not taken: mailbox name not allowed");		
    else if (SMPTTestResult == 530)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Access denied");		
    else if (SMPTTestResult == 535)
		TCPPutROMString(sktHTTP, (ROM BYTE*)"SMTP Authentication unsuccessful/Bad username or password");	
		
	else {
		uitoa(SMPTTestResult,  Tempdigits);
		TCPPutROMString(sktHTTP, (ROM BYTE*)"Error# ");
		TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
	}

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	SMPTTestResult = SMTP_TEST_DONE;

	return;

}



/*********************************************************************
  Function:        HTTPPostSMTPConfig
  PreCondition:    curHTTP is loaded
  Input:           None
  Output:          HTTP_IO_DONE on success, HTTP_IO_NEED_DATA if more data is requested
  Side Effects:    None
  Overview:
 ********************************************************************/
/// 	This function reads an input parameter from the POSTed data, and writes that string to the configuration values for SMTP

HTTP_IO_RESULT HTTPPostSMTPConfig(void) {

    BYTE temp[16];
    BYTE *ptr;

    // Define state machine values
    #define SM_SMTP_START			(0u)
    #define SM_SMTP_TO				(1u)
    #define SM_SMTP_SERVER			(2u)
    #define SM_SMTP_USER			(3u)
    #define SM_SMTP_PASSWORD		(4u)
    #define SM_SMTP_PORT			(5u)
    #define SM_SMTP_SSL				(6u)
    #define SM_SMTP_ACTIVE			(7u)
    #define SM_SMTP_INITALALARM		(8u)
    #define SM_SMTP_ARMDISARM		(9u)
    #define SM_SMTP_SUCCESS			(10u)

    switch(curHTTP.smPost)
    {
        case SM_SMTP_START:
            // Read the next variable name.  If a complete name is
            // not found, request more data.  This function will
            // automatically truncate invalid data to prevent
            // buffer overflows.

            if(curHTTP.byteCount == 0) {
				curHTTP.smPost = SM_SMTP_SUCCESS;
				break;
			}

            if(HTTPReadPostName(temp,16) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;

            if(!strcmppgm2ram((char*)temp, (ROM char*)"to"))
                curHTTP.smPost = SM_SMTP_TO;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"server"))
                curHTTP.smPost = SM_SMTP_SERVER;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"user"))
                curHTTP.smPost = SM_SMTP_USER;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"pw"))
                curHTTP.smPost = SM_SMTP_PASSWORD;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"port"))
                curHTTP.smPost = SM_SMTP_PORT;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"ssl"))
                curHTTP.smPost = SM_SMTP_SSL;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"active"))
                curHTTP.smPost = SM_SMTP_ACTIVE;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"initalarm"))
                curHTTP.smPost = SM_SMTP_INITALALARM;

            else if(!strcmppgm2ram((char*)temp, (ROM char*)"armdisarm"))
                curHTTP.smPost = SM_SMTP_ARMDISARM;


            else {
    			// Something has gone wrong
	 	   		#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Finished POST early due to unexpected name: ");
					putsUART(temp);
					putrsUART((ROM char*)"\r\n");
				#endif

    			return HTTP_IO_DONE;
    			}
    		break;


        case SM_SMTP_TO:
            memset(xPLConfig.SMTP_To,'\0',sizeof(xPLConfig.SMTP_To));
            if(HTTPReadPostValue(xPLConfig.SMTP_To ,sizeof(xPLConfig.SMTP_To) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;

        case SM_SMTP_SERVER:
			memset(xPLConfig.SMTP_Server,'\0',sizeof(xPLConfig.SMTP_Server));
       		if(HTTPReadPostValue(xPLConfig.SMTP_Server ,sizeof(xPLConfig.SMTP_Server) + 2 ) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;


        case SM_SMTP_USER:
			memset(xPLConfig.SMTP_User,'\0',sizeof(xPLConfig.SMTP_User));
       		if(HTTPReadPostValue(xPLConfig.SMTP_User ,sizeof(xPLConfig.SMTP_User) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;

        case SM_SMTP_PASSWORD:
        	memset(xPLConfig.SMTP_Password,'\0',sizeof(xPLConfig.SMTP_Password));
       		if(HTTPReadPostValue(xPLConfig.SMTP_Password ,sizeof(xPLConfig.SMTP_Password) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;

        case SM_SMTP_PORT:
       		if(HTTPReadPostValue(temp ,sizeof(temp) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            xPLConfig.SMTP_Port = atoi(temp);
	        curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;

/*        case SM_SMTP_SSL:
       		if(HTTPReadPostValue(temp ,sizeof(temp)) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            if(temp[0] == '1')
            	xPLConfig.SMTP_UseSSL = TRUE;
            if(temp[0] == '0')
            	xPLConfig.SMTP_UseSSL = FALSE;
	        curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;
 */
        case SM_SMTP_ACTIVE:
       		if(HTTPReadPostValue(temp ,sizeof(temp) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            if(temp[0] == '1')
            	xPLConfig.SMTP_Active = TRUE;
            if(temp[0] == '0')
            	xPLConfig.SMTP_Active = FALSE;
	        curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;

        case SM_SMTP_INITALALARM:
       		if(HTTPReadPostValue(temp ,sizeof(temp) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            if(temp[0] == '1')
            	xPLConfig.SMTP_OnlyInitalAlarm = TRUE;
            if(temp[0] == '0')
            	xPLConfig.SMTP_OnlyInitalAlarm = FALSE;
	        curHTTP.smPost = SM_SMTP_START; // Return to read a new name
            break;


        case SM_SMTP_ARMDISARM:
       		if(HTTPReadPostValue(temp ,sizeof(temp) + 2) == HTTP_READ_INCOMPLETE)
                return HTTP_IO_NEED_DATA;
            if(temp[0] == '1')
            	xPLConfig.SMTP_ArmDisarm = TRUE;
            if(temp[0] == '0')
            	xPLConfig.SMTP_ArmDisarm = FALSE;
	        curHTTP.smPost = SM_SMTP_START; // Return to read a new name
        //    break;

		// The POST is only accepted after the SM_SMTP_ARMDISARM, it will fail if this is missing
		// dont like this design but it should never be an issue


	     case SM_SMTP_SUCCESS:
	   		#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Finished POST correctly\r\n");
			#endif

			// now we save all of it
			xPLSaveEEConfig(&xPLConfig, EEDATASTART_XPL, sizeof(xPLConfig));

	        return HTTP_IO_DONE; // we are complete


    } // switch(curHTTP.smPost)

    return HTTP_IO_WAITING;

} //  HTTPPostSMTPConfig

/*********************************************************************
  Function:        SMTPTaskProcess
  PreCondition:    None
  Input:           None
  Output:          None
  Side Effects:    None
  Overview:

 ********************************************************************/
///	Managers the scheduled sending of emails

void SMTPTaskProcess(void) {

#if defined(STACK_USE_SMTP_CLIENT)

static BYTE RAMStringBody[50];
char DeviceDesc[DEVICEDESCLENGTH];
int ReturnResult;

#if defined(DEBUG_UART)
	char TempString[4]; // prints the status of the SMTP task machine
#endif

// Manages the Current Task and keeps it up to date.
if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State == SMTP_SM_Done) {
	if (SMTPTaskList.FreePosition == SMTPTaskList.CurentTask) {
		return;
	}
	if (SMTPTaskList.CurentTask < (SMTPSMSIZE-1))
		SMTPTaskList.CurentTask++;
	else if(SMTPTaskList.CurentTask >= (SMTPSMSIZE-1))
		SMTPTaskList.CurentTask =0;
	return;
}

//Check to see if emails are active
if (xPLConfig.SMTP_Active == FALSE) {
	// Do nothing in this case
	#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Doing nothing with SMTP task as SMTP is not active\r\n");
	#endif
	SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State = SMTP_SM_Done;
	return;
}

/*  Sample code
	// Increaments the Free position, ready for the next task.
	if (SMTPTaskList.FreePosition < (SMTPSMSIZE-1))
		SMTPTaskList.FreePosition++;
	else if(SMTPTaskList.FreePosition >= (SMTPSMSIZE-1))
		SMTPTaskList.FreePosition =0;
*/


	switch(SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State ) {

 		case SMTP_SM_InQ:

			if(SMTPBeginUsage()) {

				// Note that these strings must stay allocated in
				// memory until SMTPIsBusy() returns FALSE.  To
				// guarantee that the C compiler does not reuse this
				// memory, you must allocate the strings as static.

				memset(RAMStringBody, '\0',50);

				SMTPClient.To.szRAM = xPLConfig.SMTP_To;
				SMTPClient.From.szROM = (ROM BYTE*) "\"Alarm System\" <no-reply@redspartan.com.au>";
				SMTPClient.ROMPointers.From = 1;
				SMTPClient.Server.szRAM = xPLConfig.SMTP_Server;
				SMTPClient.Username.szRAM = xPLConfig.SMTP_User;
				SMTPClient.Password.szRAM = xPLConfig.SMTP_Password;
				SMTPClient.ServerPort = xPLConfig.SMTP_Port;
				//#if defined(STACK_USE_SSL)
				//	SMTPClient.UseSSL = FALSE; //xPLConfig.SMTP_UseSSL;
				//#endif

				if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_Test) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"This is a test email";
					SMTPClient.ROMPointers.Subject = 1;
					SMTPClient.Body.szROM = (ROM BYTE*)"If you see this message the test worked\r\nHave a nice day :)\r\n";
					SMTPClient.ROMPointers.Body = 1;
				}

				else if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_Arm) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"System Armed";
					SMTPClient.ROMPointers.Subject = 1;
					memcpypgm2ram(RAMStringBody, (ROM void*)"Armed by user:",14);
					strcat(RAMStringBody, UserID);
					SMTPClient.Body.szRAM = RAMStringBody;
				}
				else if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_Disarm) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"System Disarmed";
					SMTPClient.ROMPointers.Subject = 1;
					memcpypgm2ram(RAMStringBody, (ROM void*)"Disarmed by user:",17);
					strcat(RAMStringBody, UserID);
					SMTPClient.Body.szRAM = RAMStringBody;
				}

				else if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_Alarmed) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"Alarm has been triggered";
					SMTPClient.ROMPointers.Subject = 1;
					memcpypgm2ram(RAMStringBody, (ROM void*)"Zone number:",12);
					uitoa(LastZoneAlarmed,  DeviceDesc);
					strcat(RAMStringBody, DeviceDesc);
					memcpypgm2ram(RAMStringBody, (ROM void*)"\r\n",2);

					memcpypgm2ram(RAMStringBody, (ROM void*)"Zone description:",17);
					i2Count = DEVICEDESCLENGTH;
					if (LastZoneAlarmed != 0)
						i2Count += (LastZoneAlarmed) * DEVICEDESCLENGTH + 1;
					XEEReadArray ((DWORD) i2Count, DeviceDesc , (BYTE) DEVICEDESCLENGTH);
					strncat(RAMStringBody, DeviceDesc, DEVICEDESCLENGTH);
					memcpypgm2ram(RAMStringBody, (ROM void*)"\r\n",2);

					SMTPClient.Body.szRAM = RAMStringBody;
				}

				else if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_Reset) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"Device restarted";
					SMTPClient.ROMPointers.Subject = 1;
					SMTPClient.Body.szROM = (ROM BYTE*)"The device has been restarted. It is not known why. \r\nCurrently Disarmed";
					SMTPClient.ROMPointers.Body = 1;

				}

				else if (SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_SendType == SMTP_InternetRestored) {
					SMTPClient.Subject.szROM = (ROM BYTE*)"Internet has been restored";
					SMTPClient.ROMPointers.Subject = 1;
					SMTPClient.Body.szROM = (ROM BYTE*)"Connection to the net was lost, it is back now.";
					SMTPClient.ROMPointers.Body = 1;
				}

				else {
					SMTPClient.Subject.szROM = (ROM BYTE*)"Internal error with SMTP";
					SMTPClient.ROMPointers.Subject = 1;
					SMTPClient.Body.szROM = (ROM BYTE*)"You are seeing this message because something is broken. Get Help!";
					SMTPClient.ROMPointers.Body = 1;
				}


				// inform the SMTP Stack that email is ready for consumption
				SMTPSendMail();

				// inc the task state
				SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State = SMTP_SM_Working;

				#if defined(DEBUG_UART)
					putrsUART((ROM char*)"email built\r\n");
				#endif

			}


			else {
				// Something went wrong
				SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State = SMTP_SM_Done;
				#if defined(DEBUG_UART)
					putrsUART((ROM char*)"email failed to start\r\n");
				#endif

			}
			break;

		case SMTP_SM_Working:
			if(!SMTPIsBusy()) {

				SMTPTaskList.Task[SMTPTaskList.CurentTask].SMTP_State = SMTP_SM_Done;

				#if defined(DEBUG_UART)
					putrsUART((ROM char*)"email completed\r\n");
				#endif

				ReturnResult = (int)SMTPEndUsage();

				SMPTTestResult = ReturnResult;

				#if defined(DEBUG_UART)
					putrsUART((ROM char*)"email Response code: ");
					ultoa (ReturnResult, TempString);
					putsUART(TempString);
					putrsUART((ROM char*)"\r\n");
				#endif

			}///SMTPIsBusy
			else {
				// Email in progress

			}

			break;

	} //switch


#endif //#if defined(STACK_USE_SMTP_CLIENT)
} //void SMTPTaskProcess(void)



/*********************************************************************
  Function:        SMTPQueueEmail
  PreCondition:    none
  Input:           Email type
  Output:          None
  Side Effects:    None
  Overview:

 ********************************************************************/

 /// Places a new email in the SMTP queue to be processed (sent)
 void SMTPQueueEmail (SMTP_SendTypes Email_Type) {

	char TempString[3];

	// If emails are not active, drop out and do nothing
	if(xPLConfig.SMTP_Active == FALSE)
		return;

	#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Email placed in queue \r\n  - SMTP task position");
		uitoa(SMTPTaskList.FreePosition,  TempString);
		putsUART(TempString);
		putrsUART((ROM char*)"\r\n");
	#endif

	SMTPTaskList.Task[SMTPTaskList.FreePosition].SMTP_SendType = Email_Type;
	SMTPTaskList.Task[SMTPTaskList.FreePosition].SMTP_State = SMTP_SM_InQ;

	// Increaments the Free position, ready for the next task.
	if (SMTPTaskList.FreePosition < (SMTPSMSIZE-1))
		SMTPTaskList.FreePosition++;
	else if(SMTPTaskList.FreePosition >= (SMTPSMSIZE-1))
		SMTPTaskList.FreePosition =0;


} //SMTPQueueEmail




