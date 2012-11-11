///@file Battery.c
///@brief Backup battery management

#include "TCPIP Stack/TCPIP.h"

#include "SecuritySystem.h"
#include "xPL.h"

#define ADC_Battery 1	///< ADC port for the battey voltage sensor
#define ADC_Supply 2	///< ADC port for the supply voltage sensor


 //******************************************************************************
 /** D E C L A R A T I O N S **************************************************/
 //******************************************************************************

/// Text used for the current state of the alarm
ROM char *BatteryStatesText[] =
	{
		"Disconnected",
		"Charging",
		"Fully charged",
		"In use"
	};

BatteryStates BatteryState;		///< Keeps track of the battery state
unsigned int VoltageBattery; 	///< Battery voltage
unsigned int VoltageSupply; 	///< 12v supply voltage



 //******************************************************************************
 /** P R I V A T E  P R O T O T Y P E S ***************************************/
 //******************************************************************************

WORD FetchADC(int ADCPort) ;

//******************************************************************************
/// Prints the supply voltage variable, not real time
void HTTPPrint_VoltageSupply(void) {
	char Tempdigits[8];

	uitoa(VoltageSupply,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
	return;
}

//******************************************************************************
/// Prints the battery voltage variable, not real time
void HTTPPrint_VoltageBattery(void) {
	char Tempdigits[8];

	uitoa(VoltageBattery,  Tempdigits);
	TCPPutArray(sktHTTP, Tempdigits, strlen((char*)Tempdigits));
	return;
}

//******************************************************************************
/// Prints the battery status state, not real time
void HTTPPrint_BatteryStatus(void) {
	if(TCPIsPutReady(sktHTTP) < strlenpgm((ROM char*) BatteryStatesText[BatteryState])) {
		curHTTP.callbackPos = 0x01;
		return;
	}

	TCPPutROMArray(sktHTTP, BatteryStatesText[BatteryState], strlenpgm((ROM char*)BatteryStatesText[BatteryState]));

	// Indicate that we're done
	curHTTP.callbackPos = 0x00;
	return;
}

//*******************************************************************************
/**
Performs Analogue to Digital (ADC) conversion on the selected port number.
If it is for the battery it will disconnect the charger first so the voltage is read correctly

\todo convert value into a voltage reading
\todo Might have to delay reading abit to discharge any caps??

@param  ADCPort, ACD port number
@result WORD, outputs the value as a voltage
*/

WORD FetchADC(int ADCPort) {



	WORD ADval;

    if (ADCPort == ADC_Battery) {
		//ADCON3 Voltage on Battery

		// Make sure the charger is disabled first
		SLA_CHARGER_IO = 0;
		// add delay here

		// Select A/D channel
		ADCON0 = 0b00001100;	// -> ADON = On(1), GO/DONE = Idle (0), AN3 selected (0011)
		ADCON0bits.ADON = 1;

		 // AN0 should already be set up as an analog input
    	ADCON0bits.GO = 1;

    // Wait until A/D conversion is done
    while(ADCON0bits.GO);



	}

	else if (ADCPort == ADC_Supply) {
		//ADCON2 Voltage on Power supply

		// Select A/D channel
		ADCON0 = 0b00001000;	// -> ADON = On(1), GO/DONE = Idle (0), AN2 selected (0010)
		ADCON0bits.ADON = 1;

		// Wait until A/D conversion is done
	    ADCON0bits.GO = 1;
	    while(ADCON0bits.GO);
	}


	else {
		#if defined(DEBUG_UART)
			putsUART((ROM char*) '0' + ADCPort);
			putrsUART((ROM char*)" <- There has been an error in the battery \\ power reading \r\n");
		#endif

}
  // Convert 10-bit value into ASCII string
  ADval = (WORD)ADRES;
  //ADval *= (WORD)10;
  //ADval /= (WORD)102;


return ADval;

}
//******************************************************************************
/// Task process for managing the battery state

void BatteryTask(void) {

BatteryStates PreviousState;

// Chech the voltages
SLA_CHARGER_IO = 0;
VoltageBattery = FetchADC(ADC_Battery);
VoltageSupply = FetchADC(ADC_Supply);

PreviousState = BatteryState;

// Check to see if we are running from battery
if (SUPPLY_ACTIVE_IO == 0) {
	SLA_CHARGER_IO = 0; // make sure battery charger is off
	BatteryState = Battery_InUse;
	if (PreviousState != Battery_InUse)
		addxPLMessage (TB_SendACStat);
	return;
}


if (VoltageBattery <= BATTERY_DISCONNECTED) BatteryState = Battery_Disconnected;
else if (VoltageBattery >= BATTERY_LOW) BatteryState = Battery_Charged;
else if (VoltageBattery <  BATTERY_LOW) {
	SLA_CHARGER_IO = 1;
	BatteryState = Battery_Charging;
}

// Send a state change message
if (PreviousState != BatteryState) {
	if (PreviousState==Battery_InUse)
		addxPLMessage (TB_SendACStat);
	else
		addxPLMessage (TB_SendBatteryStat);
}

} //BatteryTask
//******************************************************************************

