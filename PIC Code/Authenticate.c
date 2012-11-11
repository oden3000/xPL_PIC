///@brief Authenication, xPL and web
///@file Authenticate.c

#include "AES.h"
#include "TCPIP Stack/TCPIP.h"
#include "xPL.h"
#include "SecuritySystem.h"

#define AESBLOCKSIZE 16		/// AES Cypher block size setting
#define MAXTIMEDIFFERENCE 120 /// difference in seconds between client time and internal time


//******************************************************************************
/** D E C L A R A T I O N S **************************************************/
//******************************************************************************

char RejectedPassword[USERPASSLENGTH]; ///< Tracks that last password that was rejected, useful when adding RFID tags

//******************************************************************************
/** P R I V A T E  P R O T O T Y P E S ***************************************/
//******************************************************************************

static void LoadUserNameEEdata (BYTE *p, int UserNumber) ;
static void LoadUserPasswordEEdata (BYTE *p, int UserNumber, int ReturnLength, int StartPosition) ;
static void SaveUserPasswordEEdata (BYTE *p, int UserNumber) ;
static void SaveUserNameEEdata (BYTE *p, int UserNumber) ;


//*********************************************************************
/**
 Called when receiving an xPL message that contains a user that needs to be checked.
 Authenticates the user by decoding the message (if AES enabled) and then calls Authenticate(); to check if this is a valid user.
 Return TRUE if the user was found and updates the UserID variable with the user name.
 
 @param Password char, the supplied password
 @return BOOL if authentication worked. global of UserID is also updated in the process  
 */

BOOL xPLAuthenticate (char * Password) {
	
	unsigned char AESDecodeKey[AESBLOCKSIZE];		// AES Key
	unsigned char AESDecodeBlock[AESBLOCKSIZE];		// AES stuff
	unsigned char RawData[AESBLOCKSIZE * 2];		// 10 bytes of EPOC + 16 bytes of the admin password
	char UserPassword[17];							// The password supplied, if AES enabled this is taken out of the decrypted data
	int ByteCopy, BlockNumber;
	DWORD MessageEPOC;								// EPOC (time) contained in the message is AES is enabled
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"xPL message user authentication has started.\r\n");
#endif
	
	if (xPLConfig.Use_AES == FALSE) {
		// AES is not enabled
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" AES is disables, no encryption on user feild\r\n");
#endif
		
		if (Authenticate(Password) == FALSE) {
			addxPLMessage (TB_SendInvalidUser);
			return FALSE;
		}
		return TRUE;				
	}
	
    // Load admin password
    memset(AESDecodeKey, '\0',  sizeof(AESDecodeBlock));
    LoadUserPasswordEEdata ((BYTE *) AESDecodeKey, 0, AESBLOCKSIZE, 0);
    
    // if password is less then 16, padd it out with spaces
    while (strlen(AESDecodeKey) < AESBLOCKSIZE)
	    strcatpgm2ram(AESDecodeKey, (rom char *) " ");
    
#if defined(DEBUG_UART)
	putsUART(AESDecodeKey);
	putrsUART((ROM char*)" <-16 byte Password for decryption\r\n");
#endif
	
	// calculate the decode key
	AESCalcDecodeKey(*AESDecodeKey); 
	
#if defined(DEBUG_UART)
	putsUART(AESDecodeKey);
	putrsUART((ROM char*)" <-16 byte decryption key after AEScalcDecodeKey\r\n");
#endif
	
	// Decode the message and convert it back to pain text
	
	BlockNumber = 0;
	memset(RawData, '\0',sizeof(RawData)); 	
	while ((BlockNumber * AESBLOCKSIZE) < sizeof(RawData)) {
		memset(AESDecodeBlock, '\0', sizeof(AESDecodeBlock));
		for (ByteCopy = 0; ByteCopy < AESBLOCKSIZE; ByteCopy++) {
			AESDecodeBlock[ByteCopy] = Password[ (BlockNumber * AESBLOCKSIZE) + ByteCopy];
			if (AESDecodeBlock[ByteCopy] == '/0') {
				BlockNumber = 9; // we have found the end of the data
				break;
			}	
		}	
		
#if defined(DEBUG_UART)
		putsUART(AESDecodeBlock);
		putrsUART((ROM char*)" <-16 byte Block 2 be decoded\r\n");
#endif
		
		AESDecode(*AESDecodeBlock, *AESDecodeKey);
		
#if defined(DEBUG_UART)
		putsUART(AESDecodeBlock);
		putrsUART((ROM char*)" <-16 byte result from decode\r\n");
#endif
		
		strcat(RawData, AESDecodeBlock);
		BlockNumber++;
	} 	
	
#if defined(DEBUG_UART)
	putsUART(RawData);
	putrsUART((ROM char*)" <-Decrypted output\r\n");
#endif
	
#if defined(STACK_USE_SNTP_CLIENT)
	
	// Determine the time difference between this messgae and the system time.
	// AESDecodeKey is now going to be used as a temp string	
	memset(AESDecodeKey, '\0', sizeof(AESDecodeKey));
	memcpy(AESDecodeKey, RawData, 10); // 10 character EPOC
	MessageEPOC = atoi(AESDecodeKey);
	
	if (SNTPGetUTCSeconds() > MessageEPOC) {
		if ((SNTPGetUTCSeconds() - MessageEPOC) > MAXTIMEDIFFERENCE) {
			// Time stamp is to old
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Time stamp is to old, message rejected\r\n");
#endif
			
			// Send xPL message that there was an error with the time stamp
			addxPLMessage (TB_SendInvalidUserTimestamp);
			return FALSE;
		}
	}
	else {
		if ((MessageEPOC - SNTPGetUTCSeconds()) > MAXTIMEDIFFERENCE) {
			// Time stamp is to far in the future
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Time stamp is to far in the future, message rejected\r\n");
#endif
			
			// Send xPL message that there was an error with the time stamp
			addxPLMessage (TB_SendInvalidUserTimestamp);
			return FALSE;		
		}
	}
	
#endif //#if defined(STACK_USE_SNTP_CLIENT)

	
	// Lastly we check to see if the password is valid
	memset(UserPassword, '\0',sizeof(UserPassword));
	for (iCount=0; (iCount+10 < sizeof(RawData) && RawData[iCount+10] != '\0'); iCount++) {
		UserPassword[iCount] = RawData[iCount + 10];
	}// for	
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Decrypted password is:");
	putsUART(UserPassword);
	putrsUART((ROM char*)", passing to authentication module\r\n");
#endif
	
	if (Authenticate(UserPassword) == FALSE) {
		addxPLMessage (TB_SendInvalidUser);
		return FALSE;
	}
	
	return TRUE;				
	
	
}//xPLAuthenticate


//*********************************************************************
/**
 Authenticates the user by checking the password. Populates the users name into UserID. Return TRUE if the user was found.
 \todo add config option to reject users from different subnet
 @param Password char
 @return BOOL if authentication worked. global of UserID is also updated in the process  
 */

BOOL Authenticate (char * Password) {
	
    char UserPassword[USERPASSLENGTH+1];	
	memset(UserID, '\0',USERNAMELENGTH);
	
	// ensure we have something in the password to check.
	if (*Password == '\0') {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" INVALID, FAILED authentication\r\n");
#endif
		return FALSE;
	}
	
	
	// Override password
	if (!strcmppgm2ram((char*)Password, (ROM char*)"boo")) {
#if defined(DEBUG_UART)
    	putrsUART((ROM char*)"Override password used\r\n");
#endif
		strcpypgm2ram((char*)UserID, (ROM void*)"Boo");
		return TRUE;
	}	
	
	
	
	for(iCount = 0; iCount < USERCOUNT; iCount++) {
	    // cycle through all of the passwords until we find it
	    LoadUserPasswordEEdata ((BYTE *) UserPassword, iCount, USERPASSLENGTH, 0);
	    if (!strcmp((char*)Password, UserPassword)) {
		    LoadUserNameEEdata (UserID, iCount);
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" User:");
			putsUART(UserID);
			putrsUART((ROM char*)" has passed authentication\r\n");
#endif
		    return TRUE;
			break;
		}// UserName
	}// iCount
	
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" User:");
	putsUART(Password);
	putrsUART((ROM char*)" has FAILED authentication\r\n");
#endif
	
	
	return FALSE;
	
}


//*********************************************************************
/**
 Dynamic variable that prints the user password that was last rejected
  ********************************************************************/

void HTTPPrint_RejectPassword(void) {
	
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < USERPASSLENGTH)
		return;
    
    TCPPutArray(sktHTTP, RejectedPassword, USERPASSLENGTH);
    
    // Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
	
	
	
}//HTTPPrint_UserName


//*********************************************************************
/**
 Dynamic variable that prints the user name to the web page
 ********************************************************************/

void HTTPPrint_UserName(WORD num) {
	
	char UserName[USERNAMELENGTH+1];
    
    UserName[USERNAMELENGTH] = '\0';
    
    // Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < USERNAMELENGTH)
		return;
    
    LoadUserNameEEdata ((BYTE *)UserName, num) ; 
    
    TCPPutArray(sktHTTP, UserName, strlen(UserName));
    
    // Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
	
	
}//HTTPPrint_UserName

//*********************************************************************
/**
 Dynamic variable that prints the user password to the web page
 ********************************************************************/

void HTTPPrint_UserPassword(WORD num) {
	
    char UserPassword[USERPASSLENGTH+1];
    
    UserPassword[USERPASSLENGTH] = '\0';
    
    // Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < USERPASSLENGTH)
		return;
    
    LoadUserPasswordEEdata ((BYTE *)UserPassword, num,USERPASSLENGTH, 0) ;
    
    TCPPutArray(sktHTTP, UserPassword, strlen(UserPassword));
    
    // Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

//*********************************************************************
/**
 Takes the user number and loads the password of that user into the pointer. The length can be limited for easy use in HTTPPrint
 @param  *p,  updates pointer with the output
 @param  UserNumber, the user number to return
 @param  ReturnLength, the number of characters to return
 @param  StartPosition, start returning characters after this position
 @return Null, returns values with *p

 ********************************************************************/

void LoadUserPasswordEEdata (BYTE *p, int UserNumber, int ReturnLength, int StartPosition) {
	
	int StartAddress;
	
	// make sure request is not out of range
	if (UserNumber >= USERCOUNT) {
		*p = '\0';
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"User ID:");
		uitoa(UserNumber,  xPLMsgKey);
		putsUART(xPLMsgKey);
		putrsUART((ROM char*)"\r\n");
		putrsUART((ROM char*)"#ERROR# User out of range,LoadUserPasswordEEdata \r\n");
#endif
		return;
	}	
	
	// find the begining of the users password
    StartAddress = (unsigned int) EEDATASTART_USER + (unsigned int) (((USERNAMELENGTH + USERPASSLENGTH) * UserNumber) + USERNAMELENGTH);
    StartAddress += StartPosition;
	
	
 	XEEReadArray ((DWORD) StartAddress, p , (BYTE) ReturnLength); 
	
}

//*********************************************************************
/**
 Takes the user number and loads the name of that user into the pointer
 Note, It will read the full size of the feild.

 @param  *p, output feed into this pointer
 @param  UserNumber, user number to be outputed

 ********************************************************************/

void LoadUserNameEEdata (BYTE *p, int UserNumber) {
	
	int StartAddress;
	char DebugOutput[3];
	
	// make sure request is not out of range
	if (UserNumber >= USERCOUNT) {
		*p = '\0';
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"User ID:");
		uitoa(UserNumber,  DebugOutput);
		putsUART(DebugOutput);
		putrsUART((ROM char*)"\r\n");
		putrsUART((ROM char*)"#ERROR# User out of range,LoadUserNameEEdata \r\n");
#endif
		return;
	}	
	
	// find the begining of the users password
    StartAddress = (unsigned int) EEDATASTART_USER + (unsigned int) ((USERNAMELENGTH + USERPASSLENGTH) * UserNumber);
    
    XEEReadArray ((DWORD) StartAddress, p , (BYTE) USERNAMELENGTH); 
	
	
}


//*********************************************************************
/** Saves the data stored in the pointer into the password feild of the
 user held in EE prom Note, It will write the full size of the feild.
 @param  *p, data to be saved
 @param  UserNumber, user number to save data to
 ********************************************************************/

void SaveUserPasswordEEdata (BYTE *p, int UserNumber) {
	
	int StartAddress;
	
	// make sure request is not out of range
	if (UserNumber >= USERCOUNT) {
		*p = '\0';
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"User ID:");
		uitoa(UserNumber,  xPLMsgKey);
		putsUART(xPLMsgKey);
		putrsUART((ROM char*)"\r\n");
		putrsUART((ROM char*)"#ERROR# User out of range,SaveUserPasswordEEdata \r\n");
#endif
		return;
	}	
	
	// find the begining of the users password
    StartAddress = (unsigned int) EEDATASTART_USER + (unsigned int) (((USERNAMELENGTH + USERPASSLENGTH) * UserNumber) + USERNAMELENGTH);
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Saving user password\r\n");
#endif
	
	
	for(iCount = 0; iCount < USERPASSLENGTH; iCount++) {
		//if(XEEBeginWrite(StartAddress + iCount) != XEE_SUCCESS )	
		XEEBeginWrite(StartAddress + iCount); 
	 	XEEWrite(p[iCount]);
		XEEEndWrite();
	   	while(XEEIsBusy());		
	}
	
}


//*********************************************************************
/**
 Saves the data stored in the pointer into the name feild of the
 user held in EE prom.  Note, It will write the full size of the feild.
 @param  *p, name to be saved
 @param  UserNumber, user number to save data to
 ********************************************************************/

void SaveUserNameEEdata (BYTE *p, int UserNumber) {
	
	int StartAddress;
	
	// make sure request is not out of range
	if (UserNumber >= USERCOUNT) {
		*p = '\0';
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"User ID:");
		uitoa(UserNumber,  xPLMsgKey);
		putsUART(xPLMsgKey);
		putrsUART((ROM char*)"\r\n");
		putrsUART((ROM char*)"#ERROR# User out of range,SaveUserPasswordEEdata \r\n");
#endif
		return;
	}	
	
	// find the begining of the users password
    StartAddress = (unsigned int) EEDATASTART_USER + (unsigned int) ((USERNAMELENGTH + USERPASSLENGTH) * UserNumber);
    
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Saving user name\r\n");
#endif
	
	for(iCount = 0; iCount < USERNAMELENGTH; iCount++) {
		//	if(XEEBeginWrite(StartAddress + iCount) != XEE_SUCCESS )
		XEEBeginWrite(StartAddress + iCount);
	 	XEEWrite(p[iCount]);
		XEEEndWrite();
	   	while(XEEIsBusy());		
	}
	
}

//*********************************************************************
/** 
 HTTP GET from the user admin web page, this updates the Name and Password
 of the user in EE prom
 @param  *ptr, from HTTP (GET) stack
 ********************************************************************/
void ModifyUser (BYTE *ptr) {
	
	int UserID, iCount;
	char NewUserName[USERNAMELENGTH+1];
	char NewUserPassword[USERPASSLENGTH+1];
	
	// clear out the memory just in case
	memset (NewUserName, '\0', USERNAMELENGTH+1);
	memset (NewUserPassword, '\0', USERPASSLENGTH+1);
	
	//ID of the user being updated
	ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"id");
	if(*ptr) 
		UserID = DecodeIntFromGET(ptr,  (rom char *)"User ID");
	
	// User Name
	ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"N");
	if(*ptr) {
		for(iCount = 0;iCount < USERNAMELENGTH; iCount++) {
			NewUserName[iCount] = *ptr;
			if (*ptr != '\0') ptr++;
		}// for	
	}
	
	// User Password
	ptr = HTTPGetROMArg(curHTTP.data, (ROM BYTE *)"P");
	if(*ptr) {
		for(iCount = 0;iCount < USERPASSLENGTH; iCount++) {
			NewUserPassword[iCount] = *ptr;
			//if (*ptr != '\0') 
			ptr++;
		}// for	
	}
	
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"NewUserName:");
	putsUART(NewUserName);
	putrsUART((ROM char*)" strlen:");
	uitoa(strlen(NewUserName),  xPLMsgKey);
	putsUART(xPLMsgKey);		
	putrsUART((ROM char*)" : ");
	putrsUART((ROM char*)"NewUserPassword:");
	putsUART(NewUserPassword);
	putrsUART((ROM char*)" strlen:");
	uitoa(strlen(NewUserPassword),  xPLMsgKey);
	putsUART(xPLMsgKey);
	putrsUART((ROM char*)"\r\n");
	
#endif
	
	SaveUserNameEEdata (NewUserName, UserID) ;
	
	SaveUserPasswordEEdata (NewUserPassword, UserID) ;
	
	strcpypgm2ram((char*)curHTTP.data, (ROM void*)"/admin/users.htm");
	curHTTP.httpStatus = HTTP_REDIRECT;			
	
	
	
} //ModifyUser

//*********************************************************************
/* 
 This is a part of the TCP/IP stack that is used to determin what access
 is needed for files being pulled form the HTTP2 web server. Read the MC doco
 @param * cFile, file name
 ********************************************************************/

BYTE HTTPNeedsAuth(BYTE* cFile) {
	
    // Don't use strcmp here, because cFile has additional path info
    
#if defined(DEBUG_UART)
	putrsUART((ROM char*)"Web page being loaded:");
	putsUART(cFile);
#endif
    
    // All pages that need admin access are stored in the file call admin
    if(!memcmppgm2ram((void*)cFile, (ROM void*)"admin", 5)) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" Page needs admin access\r\n");
#endif
		return HTTP_AUTH_ADMIN;
	}    
    
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" Page needs general access\r\n"); 
#endif
    return HTTP_AUTH_OTHER;
	
	
}  //HTTPNeedsAuth




/*********************************************************************
 This is a part of the TCP/IP stack that is used to if the supplied user name
 and password are valid for the page being accessed. Read the MC doco for more
 @param * cUser, user supplied user name
 @param * cPass, user supplied password
 @return 0x80 or above grants access, anything below prevents access
 ********************************************************************/

BYTE HTTPCheckAuth(BYTE* cUser, BYTE* cPass) {
	
    char UserPassword[USERPASSLENGTH+1];
    char UserName[USERPASSLENGTH+1];
    
    if (!strcmppgm2ram((char*)cPass, (ROM char*)"boo")) {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Web Override password used\r\n");
#endif
    	return 0xff;
    }	
	
    // Check if admin access is required
    if(curHTTP.isAuthorized == HTTP_AUTH_ADMIN) {
		memset(UserPassword, '\0',  USERPASSLENGTH);
		LoadUserPasswordEEdata ((BYTE *) UserPassword, 0, USERPASSLENGTH, 0);
		LoadUserNameEEdata (UserName, 0);
		if ( (!strcmp((char*)cUser, UserName)) &&
       		(!strcmp((char*)cPass, UserPassword)) ) {
			
#if defined(DEBUG_UART)
			putrsUART((ROM char*)"Web Admin user accpeted\r\n");
#endif
			return 0xff;
		}
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Web Admin password rejected\r\n");
#endif
		return 0x00;
	}// HTTP_AUTH_ADMIN
	
    else {
		for(iCount = 0; iCount < USERCOUNT; iCount++) {
		    LoadUserNameEEdata (UserName, iCount);
		    if (!strcmp((char*)cUser, UserName)) {
#if defined(DEBUG_UART)
				putrsUART((ROM char*)"Web User name accepted,looking for password");
#endif
			    LoadUserPasswordEEdata ((BYTE *) UserPassword, iCount, USERPASSLENGTH, 0);
			    if (!strcmp((char*)cPass, UserPassword)) {
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Accepted\r\n");
#endif
					return 0x80;
				}
			}// UserName
		}// iCount
	    
	} // isAuthorized
	
#if defined(DEBUG_UART)
	putrsUART((ROM char*)" rejected\r\n");
#endif
	
    return 0x00;
    
	
}


//******************************************************************************
/**

This is a debug function that will print the entire contents of the EEdata to a web page.
Handy when you can not figure what the hell is going on ;)
*/
void HTTPPrint_PrintEEProm(WORD num) {
	
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < 50)
		return;
    
    memset (xPLMsgValue, '\0', xPLMSGVALUELENGTH);
    
    XEEReadArray ((DWORD) num, xPLMsgValue , (BYTE) 50); 
    
    TCPPutArray(sktHTTP, xPLMsgValue, 50);
    
    // Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
	
	
}//HTTPPrint_UserName

//******************************************************************************
//*                                The End
//******************************************************************************
//#endif
