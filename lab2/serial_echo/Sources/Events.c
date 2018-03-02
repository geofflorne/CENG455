/* ###################################################################
**     Filename    : Events.c
**     Project     : serial_echo
**     Processor   : MK64FN1M0VLL12
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2018-01-30, 15:23, # CodeGen: 0
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file Events.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup Events_module Events module documentation
**  @{
*/         
/* MODULE Events */

#include "Cpu.h"
#include "Events.h"
#include "rtos_main_task.h"
#include "os_tasks.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif 


/* User includes (#include below this line is not maintained by Processor Expert) */

/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
/*
** ===================================================================
**     Callback    : myUART_RxCallback
**     Description : This callback occurs when data are received.
**     Parameters  :
**       instance - The UART instance number.
**       uartState - A pointer to the UART driver state structure
**       memory.
**     Returns : Nothing
** ===================================================================
*/
void myUART_RxCallback(uint32_t instance, void * uartState) { // ISR to handle keyboard input

	// Initialize message to send
	MESSAGE_T_PTR msg_ptr;
	msg_ptr = (MESSAGE_T_PTR)_msg_alloc(message_pool);
	bool result;
	if (msg_ptr == NULL) {
		printf("\nCould not allocate a message\n");
        _task_block();
	}

	// set message data
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, HANDLER_QUEUE);
	msg_ptr->HEADER.SIZE = sizeof(MESSAGE_HEADER_STRUCT) + strlen((char *)msg_ptr->DATA) + 1;
	msg_ptr->DATA = myRxBuff; //set data to data from ISR buffer

	//send message to handler task
	result = _msgq_send(msg_ptr);
	if (result != TRUE) {
		printf("\nCould not send a message\n");
		_task_block();
	}

	return;
/*
** ===================================================================
**     Callback    : myUART_RxCallback
**     Description : This callback occurs when data are received.
**     Parameters  :
**       instance - The UART instance number.
**       uartState - A pointer to the UART driver state structure
**       memory.
**     Returns : Nothing
** ===================================================================
*/
void myUART_RxCallback(uint32_t instance, void * uartState)
{
  /* Write your code here ... */
}

}
