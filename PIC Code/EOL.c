///@file EOL.c
///@brief End Of Line detection, hardware interface
#include "TCPIP Stack/TCPIP.h"
#include "SecuritySystem.h"
#include "xPL.h"


Zone_States CheckEOL (int ZoneID, WORD* ADval) ;

//*********************************************************************
/**

 Determine if the EOL has been triggered. If so sends triggeres alarm once disarmed it will not set the alarm off again until armed again. 
 This is the hardwaare component to checking the zone status, mostly called by IsZoneTriggered()
 \todo ensure arm fails when EOL is triggered
 
 @param  ZoneID, the zone number to be checked
 @param WORD* ADval, outputs the analogue reading of the sensor, used for HTTPPrint
 @result Zone_States, the outcome of the checked zone sensor
 
 ********************************************************************/

Zone_States CheckEOL (int ZoneID, WORD* ADval) {
	
	char AN0String[7];
	char DebugTemp[7];
	int PlexerDevice, BinaryValue;
	
	// Disable all analogue multiplexer
	PLEXERA_S_IO = 1; // High = disabled		
	PLEXERB_S_IO = 1;
	PLEXERC_S_IO = 1;
	
	// Select the analogue multiplexer
	PlexerDevice = ZoneID / 8;
	BinaryValue = ZoneID - ((ZoneID / 8) * 8); // The value can now be expressed as a 3 bit value
	
	
	if (PlexerDevice == 0) {
		
		PLEXERA_S_IO = 0; //enable this device
		PLEXERA_2_IO = ((BinaryValue / 4 > 0)?TRUE:FALSE);
		BinaryValue -= (BinaryValue / 4) * 4;		
		PLEXERA_1_IO = ((BinaryValue / 2 > 0)?TRUE:FALSE);
		BinaryValue -= (BinaryValue / 2) * 2;		
		PLEXERA_0_IO = ((BinaryValue > 0)?TRUE:FALSE);
		
	} // PlexerDevice ==0
	
	
	else if (PlexerDevice == 1) {
		PLEXERB_S_IO = 0; //enable this device
		PLEXERB_2_IO = BinaryValue / 4;
		BinaryValue -= (BinaryValue / 4) * 4;
		PLEXERB_1_IO = BinaryValue / 2;
		BinaryValue -= (BinaryValue / 2) * 2;
		PLEXERB_0_IO = BinaryValue;
	} // PlexerDevice == 1
	
	else if (PlexerDevice == 2) {
		PLEXERC_S_IO = 0; //enable this device
		PLEXERC_2_IO = BinaryValue / 4;
		BinaryValue -= (BinaryValue / 4) * 4;
		PLEXERC_1_IO = BinaryValue / 2;
		BinaryValue -= (BinaryValue / 2) * 2;
		PLEXERC_0_IO = BinaryValue;
	} // PlexerDevice == 2
	
	else {
#if defined(DEBUG_UART)
		putrsUART((ROM char*)" !!! CheckEOL is out of range !!!!! ");
#endif
		*ADval = 0;
		return StateNormal;
	}
	
	
	
	//60 ns delay is required for the Multiplexer to swith analogue channels
	//Nop();Nop();Nop();Nop();Nop();
	
	// Select A/D channel AN4
	ADCON0 = 0b00010000;	// ADON = On(1), GO/DONE = Idle (0), AN4 selected (0100), not used (0), calibration off (0)
	ADCON0bits.ADON = 1;
    ADCON0bits.GO = 1;
	
    // Wait until A/D conversion is done
    while(ADCON0bits.GO);
	
    // Convert 10-bit value into ASCII string
    *ADval = (WORD)ADRES;
    uitoa(*ADval, AN0String);
 	
	if (ZoneID == 1) {
		memset(LCDText, '\0', 32);
		if (strlen(AN0String) < (unsigned int)4 ) strcatpgm2ram(AN0String, (rom char *) " ");
		if (strlen(AN0String) < (unsigned int)4 ) strcatpgm2ram(AN0String, (rom char *) " ");
		if (strlen(AN0String) < (unsigned int)4 ) strcatpgm2ram(AN0String, (rom char *) " ");
		
		strcat(LCDText, AN0String);
		strcatpgm2ram(LCDText, (rom char *) "->");
	}	
	
	if ( (*ADval >= (WORD)(EOLNORMAL - EOLTOLERANCE)) && (*ADval <= (WORD)(EOLNORMAL + EOLTOLERANCE))) {
		if (ZoneID == 1) {
			strcatpgm2ram(LCDText, (rom char *) "Normal");
			LCDUpdate();
		}	
		// Need to consider if the zone is Normaly Open or Normaly Closed
		if (ZoneConfig[ZoneID].IsNO == FALSE)
			return StateNormal;
		else
			return StateTrigger;
	}
	
	else if ( (*ADval >= (WORD)(EOLTRIGGER - EOLTOLERANCE)) && (*ADval <= (WORD)(EOLTRIGGER + EOLTOLERANCE))) {
		if (ZoneID == 1) {	
			strcatpgm2ram(LCDText, (rom char *) "Trigger");
			LCDUpdate();
		}	
		// Need to consider if the zone is Normaly Open or Normaly Closed
		if (ZoneConfig[ZoneID].IsNO == FALSE)
			return StateTrigger;
		else
			return StateNormal;

	}
	
	else {
		
		if (ZoneID == 1) {
			strcatpgm2ram(LCDText, (rom char *) "Tamper");
			LCDUpdate();
		}	
		return StateTamper;
	}	
	
	
    
	
} // CheckEOL


//*********************************************************************
/**
 Determine if there has been a state change compared to the previously know state. Regular called to track the state of the zone.
 Updates the ZoneConfig and AreaConfig states (Triggered & Tampered). Include bounce timer to prevent zones from bouncing from one state to another
 and flooding the network all the time.

 @param  ZoneID, the zone being checked
 @result Zone_States, the outcome of the checked zone
 ********************************************************************/

Zone_States IsZoneTriggered (int ZoneID)  { 
	
	
	Zone_States TheZoneState, PreviousZoneState;
	WORD Ignore;
	
	//ensure zone is not bypassed or isolated, if so return nothing
	if ((ZoneConfig[ZoneID].IsIsolated) || (ZoneConfig[ZoneID].IsBypassed)) 
		return StateNothing;
	
	// What is the previous state
	if (ZoneConfig[ZoneID].IsTampered) PreviousZoneState = StateTamper;
	else if (ZoneConfig[ZoneID].IsTriggered) PreviousZoneState = StateTrigger;
	else PreviousZoneState = StateNormal;
	
	// Determine the state of the zone
	TheZoneState = CheckEOL(ZoneID, &Ignore);
	
	//If the state is the same as the previous check and bounce timer not active return nothing.
	if  ((PreviousZoneState == TheZoneState ) && (ZoneConfig[ZoneID].BounceDelay == (unsigned) 0))  
		return StateNothing;
	
	// *** TAMPER ***
	
	// Zone is Tampered
	if (TheZoneState == StateTamper) {
		ZoneConfig[ZoneID].IsTampered = TRUE;
		ZoneConfig[ZoneID].BounceDelay = 0; // Will cause a loop always returning StateTamper if left out
		
		// find out what Areas the zone is a part of and set them to tampered
		for(iCount = 0; iCount < AREACOUNT; iCount++) {
			if (isZoneMemberOfArea(ZoneID, iCount)) {
				AreaConfig[iCount].IsTampered = TRUE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Area ");
				uitoa(iCount, xPLMsgKey);
				putsUART(xPLMsgKey);
				putrsUART((ROM char*)" is Tampered. \r\n");
#endif
			}// isZoneMemberOfArea(ZoneID, iCount)
		}// iCount	
		return StateTamper;
	} // StateTamper
	
	
	// Return from Tamper
	if (PreviousZoneState == StateTamper) {
		ZoneConfig[ZoneID].IsTampered = FALSE;
		ZoneConfig[ZoneID].BounceDelay = 0; 
		
		// find out what Areas the zone is a part and set tamper to false if no other zones in the area are tampered
		for(iCount = 0; iCount < AREACOUNT; iCount++) {
			if (isZoneMemberOfArea(ZoneID, iCount)) {
				for(i2Count=0; i2Count < ZONECOUNT; i2Count++) {
					if ((i2Count != ZoneID) && isZoneMemberOfArea(i2Count, iCount)) {
						if( ZoneConfig[i2Count].IsTampered == TRUE) {
							i2Count = 1;
							break;
						}// is triggered
					}// is member of the zone		 
				}//i2Count, Area in question
				if (i2Count >= ZONECOUNT) { // no zones in the area found to be triggered
					AreaConfig[iCount].IsTampered = FALSE;	
#if defined(DEBUG_UART)
					putrsUART((ROM char*)" Area ");
					uitoa(iCount, xPLMsgKey);
					putsUART(xPLMsgKey);
					putrsUART((ROM char*)" is not tampered. \r\n");
#endif
				}// i2Count >= ZONECOUNT	
			}// isZoneMemberOfArea(ZoneID, iCount)
		}// iCount		
		
		// When changing from Tampered to Triggered this will generate a Triggered event after the tamper cleared event
		ZoneConfig[ZoneID].IsTriggered = FALSE;
		// Return as normal even if triggered to ensure message sequence is correct
		return StateNormal;
		
	}	
	
	
	// *** BOUNCE / TRIGGER CLEARED ***
	
	//Every time we return from Triggered to Normal there is a delay of .5 Second, this limits bouncing speed.
	//If BounceTimer not expired return Nothing. If we are bouncing it will only send 2 msg per 1/2 second. Do not change previous state 
	if (ZoneConfig[ZoneID].BounceDelay > (unsigned) 1) {
		ZoneConfig[ZoneID].BounceDelay--;
		if (TheZoneState == StateTrigger)
			ZoneConfig[ZoneID].BounceDelay = 0;	
		return StateNothing;
	}	 
	
	//if BounceTimer is expired clear BounceTimer
	if (ZoneConfig[ZoneID].BounceDelay == (unsigned) 1) {
		
		// if state = Trigger then return nothing as we have bounced back (Trigger, Normal Trigger)
		if (TheZoneState == StateTrigger) {
			ZoneConfig[ZoneID].BounceDelay = 0;
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Bounce prevented. Zone ");
			uitoa(ZoneID, xPLMsgKey);
			putsUART(xPLMsgKey);
			putrsUART((ROM char*)" remains triggered.\r\n");
#endif
			return StateNothing;
		}
		
		// if state = normal return Normal and set the state flag on the zone
		if (TheZoneState == StateNormal) {
			ZoneConfig[ZoneID].BounceDelay = 0;
			ZoneConfig[ZoneID].IsTriggered = FALSE;
			ZoneConfig[ZoneID].IsTampered = FALSE;
			
#if defined(DEBUG_UART)
			putrsUART((ROM char*)" Bounce Cleared. Zone ");
			uitoa(ZoneID, xPLMsgKey);
			putsUART(xPLMsgKey);
			putrsUART((ROM char*)" is normal. \r\n");
#endif
			
			// find out what Areas the zone is a part and set triggered to false if no other zones in the area
			// are also triggered at the same time
			for(iCount = 0; iCount < AREACOUNT; iCount++) {
				if (isZoneMemberOfArea(ZoneID, iCount)) {
					for(i2Count=0; i2Count < ZONECOUNT; i2Count++) {
						if ((i2Count != ZoneID) && isZoneMemberOfArea(i2Count, iCount)) {
							if( ZoneConfig[i2Count].IsTriggered == TRUE) {
								i2Count = 1;
								break;
							}// is triggered
						}// is member of the zone		 
					}//i2Count, Area in question
					if (i2Count >= ZONECOUNT) { // no zones in the area found to be triggered
						AreaConfig[iCount].IsTriggered = FALSE;	
#if defined(DEBUG_UART)
						putrsUART((ROM char*)" Area ");
						uitoa(iCount, xPLMsgKey);
						putsUART(xPLMsgKey);
						putrsUART((ROM char*)" is normal. \r\n");
#endif
					}// i2Count >= ZONECOUNT	
				}// isZoneMemberOfArea(ZoneID, iCount)
			}// iCount		
			
			return StateNormal;
		}
	}	
	
	
	//If current state Normal, set the BounceTimer, return Nothing
	if (TheZoneState == StateNormal) {
		ZoneConfig[ZoneID].BounceDelay = BOUNCEDELAY;
#if defined(DEBUG_UART)
		if (ZoneID == 0) putrsUART((ROM char*)" Bounce Set. \r\n");
#endif
		return StateNothing;
	}
	
	// *** TRIGGER ***
	
	//The only thing left should be a normal trigger, return trigger and set the state flag on the zone
	if (TheZoneState == StateTrigger) {
		ZoneConfig[ZoneID].BounceDelay = 0;
		ZoneConfig[ZoneID].IsTriggered = TRUE;
		
#if defined(DEBUG_UART)
		putrsUART((ROM char*)"Zone ");
		uitoa(ZoneID, xPLMsgKey);
		putsUART(xPLMsgKey);
		putrsUART((ROM char*)"triggered. ");
#endif
		
		// find out what Areas the zone is a part of and set them to triggered
		for(iCount = 0; iCount < AREACOUNT; iCount++) {
			if (isZoneMemberOfArea(ZoneID, iCount)) {
				AreaConfig[iCount].IsTriggered = TRUE;
#if defined(DEBUG_UART)
				putrsUART((ROM char*)" Area ");
				uitoa(iCount, xPLMsgKey);
				putsUART(xPLMsgKey);
				putrsUART((ROM char*)" is Triggered. \r\n");
#endif
			}// isZoneMemberOfArea(ZoneID, iCount)
		}// iCount	
		
		return StateTrigger;
	}	
	
} //IsZoneTriggered

//******************************************************************************
/// Prints the analogue value read from the zone, calls CheckEOL() to determine the value
void HTTPPrint_EOLValue (WORD num) {
	
	char EOLOutput[5];
	Zone_States Ignore;
	WORD EOLValue;
	
	// Set a flag to indicate not finished
	curHTTP.callbackPos = 1;
	
	// Make sure there's enough output space
	if(TCPIsPutReady(sktHTTP) < (unsigned int)5)
		return;
	
	Ignore = CheckEOL(num, &EOLValue);
	uitoa(EOLValue, EOLOutput);
	
	TCPPutArray(sktHTTP, (BYTE*)EOLOutput, (WORD) strlen(EOLOutput));
	
	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
	
}

//******************************************************************************




