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
**         dd_scheduler_task - void dd_scheduler_task(os_task_param_t task_init_data);
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
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif 


/* User includes (#include below this line is not maintained by Processor Expert) */

/*
** ===================================================================
**     Callback    : dd_scheduler_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void dd_scheduler_task(os_task_param_t task_init_data)
{
	//init mutex
	if (_mutatr_init(&dd_mutex_attr) != MQX_OK) {
		printf("Initializing dd mutex attributes failed.\n");
		_mqx_exit(0);
	}

	if (_mutex_init(&dd_mutex, &dd_mutex_attr) != MQX_OK) {
		printf("Initializing dd mutex failed.\n");
		_mqx_exit(0);
	}

	//init message queue
	SCHEDULER_MESSAGE_PTR msg_ptr, msg_ptr_resp;
	_queue_id scheduler_qid = _msgq_open(SCHEDULER_QUEUE, 0);

	if (scheduler_qid == 0) {
		  printf("\nCould not open the scheduler message queue\n");
		  _task_block();
	}

	message_pool = _msgpool_create(sizeof(SCHEDULER_MESSAGE), 1, 1, 0);
	if (message_pool == MSGPOOL_NULL_POOL_ID) {
	  printf("\nCount not create a message pool\n");
	  _task_block();
	}

	Node *active_list = NULL;
	Node *overdue_list = NULL;

	user_ticks = 0;

	//init generators
	struct task_gen_list list[] = {
			{USERTASK_TASK, 300, 800,  1000, {0,0}},
			{USERTASK_TASK, 350, 1000, 1000, {0,0}},
			{USERTASK_TASK, 50,  300,  300,  {0,0}},
			{USERTASK_TASK, 1000, 2000, 300,  {0,0}},
			{0,0,0,0,{0,0}}
	};

	monitor_task_id = _task_create(0,MONITORTASK_TASK,0);
	_task_create(0,PERIODICGENERATORTASK_TASK, (uint32_t)&list[0]);
	_task_create(0,PERIODICGENERATORTASK_TASK, (uint32_t)&list[1]);
	_task_create(0,PERIODICGENERATORTASK_TASK, (uint32_t)&list[2]);
	_task_create(0,APERIODICGENERATORTASK_TASK,(uint32_t)&list[3]);

#ifdef PEX_USE_RTOS
  while (1) {
#endif

	  // update lists
		while (is_overdue(active_list)) {
			printf("task %u is overdue. Moving it to overdue list.\n", active_list->tid);  // overdue. remove it from this list and add it to overdue list
			Node *overdue_task_node = active_list;
			active_list = delete_head(active_list);
			overdue_list = insert(overdue_list, overdue_task_node);
			if(_task_destroy(overdue_task_node->tid) != MQX_OK) { // kill that shit
				printf("unable to destroy an overdue task after removing it from the active list!\n");
				return 0;
			}
		}

		Node *curr_node = active_list;
		_mqx_uint priority = MIN_USER_TASK_PRIORITY;
		while(curr_node) {
			_mqx_uint old_priority;
			_task_get_priority(curr_node->tid, &old_priority);
			_task_set_priority(curr_node->tid, priority++, &old_priority);
			_task_ready(_task_get_td(curr_node->tid));
			curr_node = curr_node->next;
		}
		uint32_t next_deadline = active_list ? active_list->deadline : 0;
		//received schedule messages
		// but wake up before the the shortest deadline task's deadline is up
		msg_ptr = _msgq_receive(scheduler_qid, (next_deadline + 100));
		// if deadline exceeded, don't check the message contents. skip straight to next iteration.
		if (msg_ptr == NULL) { // this should not be happening so much. next_deadline too short? actual problems with the msgq_receive?
			continue; // Probably a timeout. skip to next iteration
		}

		if (msg_ptr->ACTION == CREATE){
			uint32_t tid = msg_ptr->T_ID;
			uint32_t deadline = msg_ptr->DEADLINE;
			Node *new_node = create_node(tid, deadline, get_time_in_ms(), USERTASK_TASK);
			active_list = insert(active_list, new_node);
		}
		else if (msg_ptr->ACTION == DELETE) {
			active_list = delete_head(active_list);
		}

		_queue_id source_qid = msg_ptr->HEADER.SOURCE_QID;
//		_msg_free(msg_ptr);
//
//		msg_ptr = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
//		if(msg_ptr == NULL) {
//			//error;
//			printf("could not aloc message in scheduler\n");
//			return 0;
//		}

		msg_ptr_resp = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
		msg_ptr_resp->HEADER.TARGET_QID = source_qid;
		msg_ptr_resp->HEADER.SIZE = sizeof(SCHEDULER_MESSAGE);
		msg_ptr_resp->TASK_LIST = active_list;
		msg_ptr_resp->OVERDUE_LIST = overdue_list;

		_msgq_send(msg_ptr_resp);
		_msg_free(msg_ptr);

#ifdef PEX_USE_RTOS
  }
#endif
}

/*
** ===================================================================
**     Callback    : monitor_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void monitor_task(os_task_param_t task_init_data)
{
	char message[255];
	Node *active, *overdue, *curr_node;
	uint32_t util, over, idle_ticks = 0;
	MONITOR_MESSAGE_PTR msg_ptr;
	_time_get_ticks(&start_tick);
	_queue_id monitor_qid = _msgq_open(MONITOR_QUEUE, 0);

	if (monitor_qid == 0) {
		  printf("\nCould not open the aperiodic message queue\n");
		  _task_block();
	}

#ifdef PEX_USE_RTOS
  while (1) {
#endif
		msg_ptr = _msgq_receive_ticks(monitor_qid, 1);
		if (msg_ptr == NULL) {
			idle_ticks++;
			continue;
		 }

		 dd_return_active_list(&active);
		 dd_return_overdue_list(&overdue);
		_msg_free(msg_ptr);


		_time_get_ticks(&end_tick);
		_time_diff_ticks(&end_tick, &start_tick , &diff_tick);
		util = 100 * (diff_tick.TICKS[0] - idle_ticks) / (diff_tick.TICKS[0]);
		over = 100 * (diff_tick.TICKS[0] - user_ticks) / (diff_tick.TICKS[0]);
		over = over > 100 ? 0 : over;
		sprintf(message, "Utilization: %u%% Overhead: %u%%\n\r",util, over);
		UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);
		_time_get_ticks(&start_tick);
		user_ticks = 0;
		idle_ticks = 0;

		sprintf(message, "\nActive List:\n\r");
		UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);

		int count = 1;
		while(active != NULL){
			sprintf(message, "%d:\tTask ID: %u\tDeadline: %u\tTask type: %u\tCreation time: %u\n\r",
					count++, active->tid, active->deadline, active->task_type, active->creation_time);

			UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);
			active = active->next;
		}

		sprintf(message, "\nOverdue List:\n\r");
		UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);

		count = 1;
		while(overdue != NULL){
			sprintf(message, "%d:\tTask ID: %u\tDeadline: %u\tTask type: %u\tCreation time: %u\n\r",
					count++, overdue->tid, overdue->deadline, overdue->task_type, overdue->creation_time);

			UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);
			overdue = overdue->next;
		}

		sprintf(message, "\n\r ========================================= \n\r");
		UART_DRV_SendDataBlocking(myUART_IDX, &message, strlen(message) + 1, 1000);

		_mqx_uint piroirty;
		_task_get_priority(monitor_task_id, &piroirty);
		_task_set_priority(monitor_task_id, 31, &piroirty);
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
	TASK_GEN_LIST_PTR task_params = (TASK_GEN_LIST_PTR)task_init_data;
	_task_id tid = _task_get_id();
	printf("user task %u started\n", tid);
	MQX_TICK_STRUCT user_start, user_end, user_diff;
	_time_get_ticks(&user_start);
	busy_delay(task_params->run_time);
	_time_get_ticks(&user_end);
	_time_diff_ticks(&user_end, &user_start, &user_diff);
	user_ticks += user_diff.TICKS[0];
	printf("user task %u done\n", tid);
	dd_delete(tid);
}

/*
** ===================================================================
**     Callback    : periodic_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void periodic_generator_task(os_task_param_t task_init_data)
{

	TASK_GEN_LIST_PTR task_params = (TASK_GEN_LIST_PTR)task_init_data;

#ifdef PEX_USE_RTOS
  while (1) {
#endif
	dd_tcreate(task_init_data);
	_time_delay(task_params->period);
#ifdef PEX_USE_RTOS   
  }
#endif


}

/*
** ===================================================================
**     Callback    : aperiodic_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void aperiodic_generator_task(os_task_param_t task_init_data)
{
	APERIODIC_MESSAGE_PTR msg_ptr;
	_queue_id aperiodic_qid = _msgq_open(APERIODIC_QUEUE, 0);

	if (aperiodic_qid == 0) {
		  printf("\nCould not open the aperiodic message queue\n");
		  _task_block();
	}

  while (1) {
	msg_ptr = _msgq_receive(aperiodic_qid, 0); // block until a message arrives in the input queue
	if (msg_ptr == NULL) {
		printf("\nCould not receive a message\n");
		_task_block();
	 }

	_msg_free(msg_ptr);

	printf("generated aperiodic task %u\n", dd_tcreate(task_init_data));

  }
}

bool is_overdue(Node *node) {
	if(node == NULL) return false;
	return(get_time_in_ms() > abs_deadline(node));
}

void busy_delay(uint32_t miliseconds) {
	uint32_t start;
	start = get_time_in_ms();
	while(get_time_in_ms() - start < miliseconds){}
}

uint32_t get_time_in_ms(){
	_time_get(&time);
	return time.SECONDS * 1000 + time.MILLISECONDS;
}
