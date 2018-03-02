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
	//printf("in serial task\n");
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
	_queue_id user_to_handler_qid = _msgq_open(USER_TO_HANDLER_QUEUE, 0); // open user task to handler queue for reading

	if (handler_qid == 0) {
		  printf("\nCould not open the handler message queue\n");
		  _task_block();
	}

	/* create message pool */
   message_pool = _msgpool_create(sizeof(MESSAGE_T), 100, 0, 0);
   if (message_pool == MSGPOOL_NULL_POOL_ID) {
	  printf("\nCount not create a message pool\n");
	  _task_block();
   }
  

#ifdef PEX_USE_RTOS
  while (1) { // task loop
#endif
    msg_ptr = _msgq_receive(handler_qid, 0); // block until a message arrives in the input queue from the ISR
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

    	if (c == CARRIAGE_RETURN) {
    		// forward the output buffer to all listening tasks
    		for(int i = 0; i < NUM_USER_TASKS; i++) {
    			_queue_id queue_id = task_read_permissions[i].queue_id; // grab task's queue_id

    			// Initialize message to send
    			MESSAGE_T_PTR msg_to_user_task_ptr = (MESSAGE_T_PTR)_msg_alloc(message_pool);
    			if (msg_to_user_task_ptr == NULL) {
    				if(_task_get_error() == MQX_COMPONENT_DOES_NOT_EXIST){
    					printf("MQX_COMPONENT_DOES_NOT_EXIST\n");
    				}else if(_task_get_error() == MSGPOOL_INVALID_POOL_ID){
    					printf("MSGPOOL_INVALID_POOL_ID\n");
    				}else if(_task_get_error() == MSGPOOL_OUT_OF_MESSAGES){
    					printf("MSGPOOL_OUT_OF_MESSAGES\n");
    				}
    				printf("\nCould not allocate a message to send to user task in handler task\n");
    				_task_block();
    			}
    			// Set message data
    			msg_to_user_task_ptr->HEADER.TARGET_QID = _msgq_get_id(0, queue_id); // set target as the user task queue
    			msg_to_user_task_ptr->DATA = output_buf;
    			msg_to_user_task_ptr->HEADER.SIZE = sizeof(MESSAGE_HEADER_STRUCT) + strlen((char *)msg_to_user_task_ptr->DATA) + 1;
    			// Send string to handler task's queue
    			printf("sending to user task from handler\n");

    			bool result = _msgq_send(msg_to_user_task_ptr);

    			if (result != TRUE) {
    				printf("\nCould not send a message\n");
    				_task_block();
    			}
    		}
    		handleUserTaskMessages(user_to_handler_qid); // wait for a response from some user task
    		break;
    	}
    	else {
//    		printf("updating outputbuffer\n");
        	handleInputChar(c); // update the output buffer
    	}
    	i++;
    }
    _msg_free(msg_ptr); // free the received message

#ifdef PEX_USE_RTOS   
  }
#endif    
}

void clearOutputBuffer() {
	for(int i = 0; i < OUTPUT_BUF_LEN; i++) output_buf[i] = '\0'; // clear the output buffer
	output_buf_idx = 0;
}

void handleUserTaskMessages(_queue_id user_to_handler_qid) {
//	printf("waiting in handler for user message\n");
	MESSAGE_T_PTR msg_ptr = _msgq_receive(user_to_handler_qid, 0); // block until a message arrives from a user task
	if (msg_ptr == NULL) {
		printf("\nCould not receive a message\n");
		_task_block();
	}
	char * message  = msg_ptr->DATA; // grab the message's data
//	printf("handler received %s from task\n", message);

	int i = 0;
	while(message[i] != '\0'){
		unsigned char c = message[i];
		UART_DRV_SendDataBlocking(myUART_IDX, &c, sizeof(c), 1000); // send char to serial port
		i++;
	}
	clearOutputBuffer();
	_msg_free(msg_ptr); // free the received message
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
	 _task_id task_id = _task_get_id();
	 _queue_id user_qid = _msgq_open(USER_QUEUE_1, 0);
	 _queue_id handler_queue_id;
	 bool has_write_permission = false;
	 if (user_qid == 0) {
	 	printf("\nCould not open the user task 1 message queue\n");
	 	_task_block();
	 }
	 printf("calling openr in task 1\n");
	 if (OpenR(user_qid) != 1) {
		 printf("already had read permission or request for read not granted\n");
	 }
  
#ifdef PEX_USE_RTOS
  while (1) {
#endif
	  if(!has_write_permission){
//		  printf("calling openw in task 1\n");
		  	  handler_queue_id = OpenW();
		  	  if (handler_queue_id == 0) {
		  		  //printf("not able to obtain write stream\n");
		  	  }
		  	  else{
		  		has_write_permission = true;
		  		printf("got write permission task 1\n");
		  	  }
	  }

	  if(has_write_permission){
			  char * line;
			  _getline(&line, user_qid); // wait for a line of text from the handler task

			  // Append newline to string
			  char stringWithTaskName[strlen(line) + 2];
			  clearString(stringWithTaskName);
			  strcat(stringWithTaskName, line);
			  strcat(stringWithTaskName, " 1");

			  _putline(USER_TO_HANDLER_QUEUE, stringWithTaskName); // Write the line to the write stream!
			  Close();
			  has_write_permission = false;
	  }
	  _time_delay(1000);
    
#ifdef PEX_USE_RTOS   
  }
#endif

}

/*
** ===================================================================
**     Callback    : user_task_2
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void user_task_2(os_task_param_t task_init_data)
{
	_task_id task_id = _task_get_id();
		 _queue_id user_qid = _msgq_open(USER_QUEUE_2, 0);
		 _queue_id handler_queue_id;
		 bool has_write_permission = false;
		 if (user_qid == 0) {
		 	printf("\nCould not open the user task 2 message queue\n");
		 	_task_block();
		 }
//		 printf("calling openr in task 2\n");
		 if (OpenR(user_qid) != 1) {
			 printf("already had read permission or request for read not granted\n");
		 }

	#ifdef PEX_USE_RTOS
	  while (1) {
	#endif
		  if(!has_write_permission){
			  	  handler_queue_id = OpenW();
			  	  if (handler_queue_id == 0) {
			  		  //printf("not able to obtain write stream\n");
			  	  }
			  	  else{
			  		has_write_permission = true;
			  		printf("got write permission task 2\n");
			  	  }
		  }

		  if(has_write_permission){
				  char * line;
				  printf("getting in t2\n");
				  _getline(&line, user_qid); // wait for a line of text from the handler task
				  printf("got in t2\n");
				  // Append newline to string
				  char stringWithTaskName[strlen(line) + 2];
				  clearString(stringWithTaskName);
				  strcat(stringWithTaskName, line);
				  strcat(stringWithTaskName, " 2");
				  printf("putting in t2\n");
				  _putline(USER_TO_HANDLER_QUEUE, stringWithTaskName); // Write the line to the write stream!
		  }
		  _time_delay(1000);

	#ifdef PEX_USE_RTOS
	  }
	#endif
}
