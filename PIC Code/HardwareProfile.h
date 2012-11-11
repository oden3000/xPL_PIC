///@file HardwareProfile.h
///@brief Defines the hardware (IO lines)
/*********************************************************************
 *
 *	Hardware specific definitions
 *
 *********************************************************************

Outputs:
- External Siren
- Internal Siren
- Strobe

Inputs:
- Battery voltage ADC
- Power supply ADC
- Reset button 
- Factory Reset button

12 IO lines for analogue switch
3 ADC lines

Uart

IO = 16
ADC = 5

ADCON0 LED & Helpers.c (GenerateRandomDWORD)
ADCON1 LED
ADCON2 Voltage on Power supply
ADCON3 Voltage on Battery
ADCON4 Demultiplexer 


 ********************************************************************/
#ifndef __HARDWARE_PROFILE_H
#define __HARDWARE_PROFILE_H

// Choose which hardware profile to compile for here.  See 
// the hardware profiles below for meaning of various options.  
#define PICDEMNET2

// Set configuration fuses (but only once)
#if defined(THIS_IS_STACK_APPLICATION)
	#if defined(__18CXX)
//		#if defined(__EXTENDED18__)
			#pragma config XINST=ON
//		#elif !defined(HI_TECH_C)
//			#pragma config XINST=OFF
//		#endif
	#endif	
	// PICDEM.net 2 or any other PIC18F97J60 family device
	#pragma config WDT=OFF, FOSC2=ON, FOSC=HSPLL, ETHLED=ON

#endif // Prevent more than one set of config fuse definitions

// Clock frequency value.
// This value is used to calculate Tick Counter value
#define GetSystemClock()		(41666667ul)      // Hz
#define GetInstructionClock()	(GetSystemClock()/4)
#define GetPeripheralClock()	GetInstructionClock()


// Hardware mappings
// PICDEM.net 2 (PIC18F97J60 + ENC28J60)
	
	
	// Outputs
	#define INT_SIREN_TRIS			(TRISJbits.TRISJ5)
	#define INT_SIREN_IO			(PORTJbits.RJ5)
	#define EXT_SIREN_TRIS			(TRISJbits.TRISJ6)
	#define EXT_SIREN_IO			(PORTJbits.RJ6)
	#define STROBE_TRIS				(TRISJbits.TRISJ7)
	#define STROBE_IO				(PORTJbits.RJ7)
	#define SLA_CHARGER_TRIS		(TRISJbits.TRISJ4)
	#define SLA_CHARGER_IO			(LATJbits.LATJ4)  // High activates the charger

	// Analogue 3 bits plus and enable 
	#define PLEXERA_S_TRIS			(TRISBbits.TRISB3)
	#define	PLEXERA_S_IO			(PORTBbits.RB3)  // High = disabled
	#define PLEXERA_2_TRIS			(TRISBbits.TRISB2)
	#define	PLEXERA_2_IO			(PORTBbits.RB2)
	#define PLEXERA_1_TRIS			(TRISBbits.TRISB1)
	#define	PLEXERA_1_IO			(PORTBbits.RB1)
	#define PLEXERA_0_TRIS			(TRISBbits.TRISB0)
	#define	PLEXERA_0_IO			(PORTBbits.RB0)


	#define PLEXERB_S_TRIS			(TRISBbits.TRISB7)
	#define	PLEXERB_S_IO			(PORTBbits.RB7)  // High = disabled
	#define PLEXERB_2_TRIS			(TRISBbits.TRISB6)
	#define	PLEXERB_2_IO			(PORTBbits.RB6)
	#define PLEXERB_1_TRIS			(TRISBbits.TRISB5)
	#define	PLEXERB_1_IO			(PORTBbits.RB5)
	#define PLEXERB_0_TRIS			(TRISBbits.TRISB4)
	#define	PLEXERB_0_IO			(PORTBbits.RB4)


	#define PLEXERC_S_TRIS			(TRISEbits.TRISE2)
	#define	PLEXERC_S_IO			(PORTEbits.RE2)  // High = disabled
	#define PLEXERC_2_TRIS			(TRISEbits.TRISE2)
	#define	PLEXERC_2_IO			(PORTEbits.RE2)
	#define PLEXERC_1_TRIS			(TRISEbits.TRISE1)
	#define	PLEXERC_1_IO			(PORTEbits.RE1)
	#define PLEXERC_0_TRIS			(TRISEbits.TRISE0)
	#define	PLEXERC_0_IO			(PORTEbits.RE0)



	// Factory reset button is RE0
	#define BUTTON0_TRIS		(TRISEbits.TRISE0)
	#define	BUTTON0_IO			(PORTEbits.RE0)

	
	// Inputs
	#define SUPPLY_ACTIVE_TRIS		(TRISDbits.TRISD0)
	#define SUPPLY_ACTIVE_IO		(PORTDbits.RD0)	 // Goes high when external power supply is active

	// A/D ports
	#define AN2_TRIS	(TRISAbits.TRISA2)
	#define AN3_TRIS	(TRISAbits.TRISA3)
	#define AN4_TRIS	(TRISAbits.TRISA4)



// note: BUTTON0_IO used to rest

	// I/O pins
	#define LED0_TRIS			(TRISJbits.TRISJ0)
	#define LED0_IO				(LATJbits.LATJ0)
	#define LED1_TRIS			(TRISJbits.TRISJ1)
	#define LED1_IO				(LATJbits.LATJ1)
	#define LED2_TRIS			(TRISJbits.TRISJ2)
	#define LED2_IO				(LATJbits.LATJ2)
	#define LED3_TRIS			(TRISJbits.TRISJ3)
	#define LED3_IO				(LATJbits.LATJ3)
	#define LED4_TRIS			(TRISJbits.TRISJ4)
	#define LED4_IO				(LATJbits.LATJ4)
	#define LED5_TRIS			(TRISJbits.TRISJ5)
	#define LED5_IO				(LATJbits.LATJ5)
	#define LED6_TRIS			(TRISJbits.TRISJ6)
	#define LED6_IO				(LATJbits.LATJ6)
	#define LED7_TRIS			(TRISJbits.TRISJ7)
	#define LED7_IO				(LATJbits.LATJ7)
	#define LED_GET()			(LATJ)
	#define LED_PUT(a)			(LATJ = (a))


	// ENC28J60 I/O pins
	#define ENC_RST_TRIS		(TRISDbits.TRISD2)	// Not connected by default
	#define ENC_RST_IO			(LATDbits.LATD2)
//	#define ENC_CS_TRIS			(TRISDbits.TRISD3)	// Uncomment this line if you wish to use the ENC28J60 on the PICDEM.net 2 board instead of the internal PIC18F97J60 Ethernet module
	#define ENC_CS_IO			(LATDbits.LATD3)
	#define ENC_SCK_TRIS		(TRISCbits.TRISC3)
	#define ENC_SDI_TRIS		(TRISCbits.TRISC4)
	#define ENC_SDO_TRIS		(TRISCbits.TRISC5)
	#define ENC_SPI_IF			(PIR1bits.SSPIF)
	#define ENC_SSPBUF			(SSP1BUF)
	#define ENC_SPISTAT			(SSP1STAT)
	#define ENC_SPISTATbits		(SSP1STATbits)
	#define ENC_SPICON1			(SSP1CON1)
	#define ENC_SPICON1bits		(SSP1CON1bits)
	#define ENC_SPICON2			(SSP1CON2)

	// 25LC256 I/O pins
	#define EEPROM_CS_TRIS		(TRISDbits.TRISD7)
	#define EEPROM_CS_IO		(LATDbits.LATD7)
	#define EEPROM_SCK_TRIS		(TRISCbits.TRISC3)
	#define EEPROM_SDI_TRIS		(TRISCbits.TRISC4)
	#define EEPROM_SDO_TRIS		(TRISCbits.TRISC5)
	#define EEPROM_SPI_IF		(PIR1bits.SSPIF)
	#define EEPROM_SSPBUF		(SSPBUF)
	#define EEPROM_SPICON1		(SSP1CON1)
	#define EEPROM_SPICON1bits	(SSP1CON1bits)
	#define EEPROM_SPICON2		(SSP1CON2)
	#define EEPROM_SPISTAT		(SSP1STAT)
	#define EEPROM_SPISTATbits	(SSP1STATbits)

	// LCD I/O pins
	#define LCD_DATA_TRIS		(TRISE)
	#define LCD_DATA_IO			(LATE)
	#define LCD_RD_WR_TRIS		(TRISHbits.TRISH1)
	#define LCD_RD_WR_IO		(LATHbits.LATH1)
	#define LCD_RS_TRIS			(TRISHbits.TRISH2)
	#define LCD_RS_IO			(LATHbits.LATH2)
	#define LCD_E_TRIS			(TRISHbits.TRISH0)
	#define LCD_E_IO			(LATHbits.LATH0)


	// UART mapping functions for consistent API names across 8-bit and 16 or 
	// 32 bit compilers.  For simplicity, everything will use "UART" instead 
	// of USART/EUSART/etc.
	#define BusyUART()				BusyUSART()
	#define CloseUART()				CloseUSART()
	#define ConfigIntUART(a)		ConfigIntUSART(a)
	#define DataRdyUART()			DataRdyUSART()
	#define OpenUART(a,b,c)			OpenUSART(a,b,c)
	#define ReadUART()				ReadUSART()
	#define WriteUART(a)			WriteUSART(a)
	#define getsUART(a,b,c)			getsUSART(b,a)
	#define putsUART(a)				putsUSART(a)
	#define getcUART()				ReadUSART()
	#define putcUART(a)				WriteUSART(a)
	#define putrsUART(a)			putrsUSART((far rom char*)a)


#endif
