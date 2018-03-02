/* ###################################################################
**     Filename    : os_tasks.c
**     Project     : serial_echo
**     Processor   : MK64FN1M0VLL12
**     Component   : Events
**     Version     : Driver 01.00
**     Compiler    : GNU C Compiler
**     Date/Time   : 2018-01-30, 15:47, # CodeGen: 1
**     Abstract    :
**         This is user's event module.
**         Put your event handler code here.
**     Settings    :
**     Contents    :
**         serial_task - void serial_task(os_task_param_t task_init_data);
**
** ###################################################################*/
/*!
** @file os_tasks.c
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup os_tasks_module os_tasks module documentation
**  @{
*/         
/* MODULE os_tasks */

#include "Cpu.h"
#include "Events.h"
#include "rtos_main_task.h"
#include "os_tasks.h"
#include "funcs.h"
#include <stdio.h>
#include "user_funcs.h"

#ifdef __cplusplus
extern "C" {
#endif 


/* User includes (#include below this line is not maintained by Processor Expert) */

/*
** ===================================================================
**     Callback    : serial_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void serial_task(os_task_param_t task_init_data)
{
}

/*
** ===================================================================
**     Callback    : handler_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void handler_task(os_task_param_t task_init_data) // grab chars from handler queue and send to uart for printing
{
	// initialize read and write permission mutexes
	if (_mutatr_init(&write_mutex_attr) != MQX_OK) {
		printf("Initializing write mutex attributes failed.\n");
		_mqx_exit(0);
	}
	if (_mutex_init(&write_mutex, &write_mutex_attr) != MQX_OK) {
		printf("Initializing write mutex failed.\n");
		_mqx_exit(0);
	}
	if (_mutatr_init(&read_mutex_attr) != MQX_OK) {
		printf("Initializing read mutex attributes failed.\n");
		_mqx_exit(0);
	}
	if (_mutex_init(&read_mutex, &read_mutex_attr) != MQX_OK) {
		printf("Initializing read mutex failed.\n");
		_mqx_exit(0);
	}
	printf("read/write mutexes initialized\n");

	task_write_permission = NO_TASK; // Initialize write permission tracker

	output_buf_idx = 0; // initialize output buffer index

	printf("handler task Created! \n\r");

	MESSAGE_T_PTR msg_ptr;
	_queue_id handler_qid = _msgq_open(HANDLER_QUEUE, 0); // open handler queue for reading

	if (handler_qid == 0) {
		  printf("\nCould not open the handler message queue\n");
		  _task_block();
	}

	/* create message pool */
   message_pool = _msgpool_create(sizeof(MESSAGE_T), 1, 0, 0);
   if (message_pool == MSGPOOL_NULL_POOL_ID) {
	  printf("\nCount not create a message pool\n");
	  _task_block();
   }
  
#ifdef PEX_USE_RTOS
  while (1) { // task loop
#endif

    msg_ptr = _msgq_receive(handler_qid, 0); // block until a message arrives in the input queue
    if (msg_ptr == NULL) {
   		printf("\nCould not receive a message\n");
   		_task_block();
   	 }

    char * message  = msg_ptr->DATA;
    
    // read all characters from the message
    // and update the output buffer and terminal display accordingly
	int i = 0;
    while(message[i] != '\0'){
    	unsigned char c = message[i];
    	updateOutputBufferAndDisplay(c);
    	i++;
    }
    _msg_free(msg_ptr); // free the received message
    
#ifdef PEX_USE_RTOS   
  }
#endif    
}

/*
** ===================================================================
**     Callback    : user_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void user_task(os_task_param_t task_init_data)
{

	/*
	 * The commented code below is for the OpenR() implementation that is unfinished as of now.
	 *
	 * Please ignore it.
	 */

	 //send to terminal using senduart function
	 //send chars to terminal char by char
//	 MESSAGE_T_PTR user_msg_ptr;
//
//	 _queue_id user_qid = _msgq_open(USER_QUEUE_1, 0);
//
//	 if (user_qid == 0) {
//	 	printf("\nCould not open the user task message queue\n");
//	 	_task_block();
//	 }
//
//	 // create a message pool
//	 user_msg_pool_1 = _msgpool_create(sizeof(MESSAGE_T), 1, 0, 0);
//
//	 if (user_msg_pool_1 == MSGPOOL_NULL_POOL_ID) {
//		 printf("\nCount not create a message pool\n");
//	 	 _task_block();
//	 }
//
//	 if (OpenR(user_qid) != 1) {
//		 printf("already had read permission or request for read not granted\n");
//	 }
//
//	 char *line;
//	 _getline(line);
//
// END OF COMMENTED OpenR() CODE

	// Try to obtain permission for the write stream
	_queue_id handler_queue_id = OpenW();
	 if (handler_queue_id == 0) {
		 printf("not able to obtain write stream\n");
		 _task_block();
	 }
	 else{ // Obtained write permission.
	     _putline(handler_queue_id, "IM USER TASK!!!"); // Write to the write stream!
		 Close();
	 }
  
#ifdef PEX_USE_RTOS
  while (1) {
#endif
    
#ifdef PEX_USE_RTOS   
  }
#endif

}