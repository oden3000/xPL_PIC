This in an xPL based alarm system build on a PIC18f97j60 with the Microchip TCIP stack.

An xPL framework has been isolated from the alarm system implementation (refereed to as a schema), so it can be used to create other xPL devices with easy. 

Code has been written with the MICROCHIP IDE and documented with Doxygen. The xPL framework has been heavily tested and the security is very close to completion but development has stopped.

# Stack features: 
* Handles heart beat and config events 
* Processing incoming xPL messages 
* Passes only relevant messages to the schema to process 
* Provides common functions for processing xPL messages 
* Generation of the xPL headers, making sending messages extremely simple 
* Cooperative multi tasking framework that ensures UDP sent / received correctly 
* Load and save data from EEprom

# Security schema features 
* Speaks the xPL security schema, all of it 
* 8 Areas, 24 Zones 
* 10 character description field for Zones and Areas 
* Perimiter, Interior and TwentyFourHour types supported 
* HTTPS (512 bit) web page for configuration, control and monitoring 
* Optional delay before external siren is activated, helps prevent false alarms 
* 10 users with password long enough to support RFID tages 
* Support authentication on xPL messages 
* Sending emails via SMTP gateway for alarm events (configurable) 
1. SMTP_Test - Generate test email from the web page
1. SMTP_Arm - when armed 
1. SMTP_Disarm - alarm disarmed 
1. SMTP_Alarmed - when the alarm goes off
1. SMTP_Reset - power is turned off and back on again
1. SMTP_InternetRestored - internet connection is restored 
* Configurable supervised End of Line resistor values and tolerance 
* Arm home, arm away 
* Double bounce filter to prevent dirty contacts (in relays and reed switches) from sending multiple state change messages 
* SLA battery charger with voltage indication

## Limitations
* NIC is only 10 base half duplex 
* No dialer (PSTN) device support 
* Passwords are sent as plain text via xPL messages. 
* TODO: AES encryption has not been implemented on the user field of the message.

# Overview of xPL framework

## main.c
Used for basic hardware and config initialisation initialisation.

## xPL.c  Functions 
* XPLInit - initialises the xPL stack and generated the initial heart beat message and xPL config if not configured 
* xPLTasks - There are 3 tasks here that need to run in this order 
1. SchemaTasks, the implementation related tasks. Needs to be called first to flush out transmit buffer 
1. UDP input buffer, processing of an incoming xPL message 
1. xPL heart beat message, timely send out heart beat related messages 
* XPLProcessUDPMsg - Processes a received xPL message and determines what to do with it. If the message was not heart beat or config relate it will be passed to SchemaProcessUDPMsg() for further processing.

# To create a new implementation

1. update these variables in xpl.c 
1. 1. xPLschemaClassText[] - xPL Schema's (class) that are supported by this application.
1. 1.  xPLschemaTypeText[] - xPL Schema's (type) that are supported by this application. Add what ever you 
want to implement here 
1. 1. Device name
1. create a SchemaTasks function to handle what ever your thingy does 
1. create a SchemaProcessUDPMsg function process incoming xPL messages 
1. create a SchemaInit function to initialise the schema
1. Add an include statement so that xPL.c can find the schema