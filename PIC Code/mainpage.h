// This is a Doxygen file, not a part of the code.

/** @file mainpage.h
* @brief Alarm Cabinent Documentation page
* @mainpage Project overview
* 
* This application has been developed to work with the microchip TCIP stack.
*  
* 
* @section a1 xPL Framework 
* 
* The xPL framework provides the following feature:
* - Parsing all inbound xPL messages and converts them into a manageable data structure
* - Only passes relevant xPL messages to the device being implemented
* - Providing a consistent and easy method of transmitting all xPL messages
* - Handling all heart beat functionality
* - Greatly simplifying the device configuration process
* 
* The device being implemented only needs to have the following methods exposed:
* - BOOL SchemaInit (void) - Returns true if the device has configuration, false sets the device into an un configured state
* - BOOL SchemaTasks (void) - Is called with each co operative multi tasking cycle, return True if a messages has been placed in the outbound queue
* - void SchemaProcessUDPMsg (void) - This is called by the xPL framework to process an inbound xPL message
* 
* The xPL framework is initialised from the main.c file by opening a UDP socket, waiting for the network to stabilise and then calling the xPLInit() function. 
* From main.c you call the StackTask() as a part of the TCPIP framework and maybe StackApplications(); if you are in the mood for running other services that are a part of
* the Microchip stack. After these functions are called then the xPL stack task is called.
* 
\code
* void main(void) { 
* 	while(1) { 
* 		StackTask(); 
* 		StackApplications(); 
* 		XPLTasks(); 
* 	} 
* } 
* 
\endcode 

Inside XPLTasks() there are 3 other tasks that need to run in this order, 
* - Implemented schema related tasks - SchemaTasks()
* - UDP input buffer, for processing inbound messages
* - xPL heart beat message generation \n \n
Remember the Microchip TCP framework StackTask() needs to be called to flush the transmit buffer each time, hence only one can run per task run! The order is determined by importance, with the most important function being executed first.
 \n
 \n
* @section a2 Useful tools when implementing an xPL device:
* 
- XPLSendHeader() \n
 Used to create any xPL message. Populates the header then adds class type and opens the body. \n
 
- xPLSaveEEConfig() \n
 Accepts configuration data (in the form of a pointer) and saves it to EE Prom.  Used in the SchemaInit() procedure.
 
- xPLLoadEEConfig() \n
 Loads data from EE Prom into the selected config pointer.  Used in the SchemaInit() procedure.
 
- SaveKVP() \n
 Loads a Key and a Value from the UDP message. Used in processing messages in SchemaProcessUDPMsg()

- xPLFindArrayInKVP() \n
 Searches though an array looking for a match to TheValue. When a match is found  it will return the position in the array. Used in processing messages in SchemaProcessUDPMsg()
* 

@section a3 Security 

Functions of the secrutiy schema are called via three entry points \n
1) HTTPExecuteGet(), the result of posting data on the HTTP form. \n
2) SchemaProcessUDPMsg(), receiving xPL data. \n
3) SchemaTasks(), xPL framework calling the secuirty_system.c task function\n

When sending an xPL message, it is placed into xPLTaskList where the function SchemaTasks() then excitues the fuction once a UDP socket is available.
The same function decides when to check the input pins for changes in the alarm sensore, IOTasks() and battery related function BatteryTask().

Data Structure: \n
xPLConfigs is all of the confiration data for the device. This data is saved and loaded out of the EE Prom \n
AreaConfigs and  ZoneConfigs hold tempery state information on individual areas and zones. \n



*/