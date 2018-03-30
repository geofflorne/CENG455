/*
 * dd_functions.c
 *
 *  Created on: Mar 6, 2018
 *      Author: geoff147
 */

#include "dd_functions.h"

_task_id dd_tcreate(uint32_t parameter){
//	When a user task is created using dd_tcreate the scheduler must put the task id (and other information)
//	into the task list and then sort the list.
//	_task_id dd_tcreate(uint_32 template_index, uint_32 deadline)
//	This primitive, creates a deadline driven scheduled task. It follows the steps outlined below
//	1. Opens a queue
//	2. Creates the task specified and assigns it the minimum priority possible
//	3. Composes a create_task_message and sends it to the DD-scheduler
//	4. Waits for a reply at the queue it created above
//	5. Once the reply is received, it obtains it
//	6. Destroys the queue
//	7. Returns to the invoking task
//	template_index is the template index of the task to be created.
//	deadline is the number of clock ticks to the task's deadline.
//	It returns the task_id of the created task, or an error. The error is either an MQX task creation error or a
//	DD-scheduler specific error (to be determined).


	TASK_GEN_LIST_PTR task_params = (TASK_GEN_LIST_PTR)parameter;
	_queue_id client_qid = _msgq_open(MSGQ_FREE_QUEUE, 0);
	if (!client_qid) {
		printf("Could not open message queue in dd_tcreate: %d\n", _task_get_error());
		_task_block();
	}
	_task_id tid = _task_create_blocked(0, task_params->task_index, parameter); //_task_create_blocked
	_mqx_uint priority, temp;
	_task_get_priority(tid, &priority);
	temp = priority;
	priority = 25; //set this to highest priority possible
	_task_set_priority(tid, priority, &temp);

	SCHEDULER_MESSAGE_PTR msg_ptr;
	msg_ptr = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
	if(msg_ptr == NULL) {
		//error;
		printf("could not aloc message in dd_tcreate: %u\n",_task_get_error());
		return 0;
	}

	msg_ptr->HEADER.SOURCE_QID = client_qid;
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULER_QUEUE);
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULER_MESSAGE);
	msg_ptr->T_ID = tid;
	msg_ptr->DEADLINE = task_params->deadline;
	msg_ptr->ACTION = CREATE;

	_msgq_send(msg_ptr);

	msg_ptr =  _msgq_receive(client_qid, 0);
	_msgq_close(client_qid);
	_msg_free(msg_ptr);

	return tid;
}

uint32_t dd_delete(uint32_t task_id){
//	When the function dd_delete is called, the specified task is deleted.
//	This primitive deletes the task specified. It parallels the structure of the dd_tcreate as outlined above.
	_queue_id client_qid = _msgq_open(MSGQ_FREE_QUEUE, 0);
	if (!client_qid) {
		printf("Could not open delete message queue in dd_delete %d\n", _task_get_error());
		_task_block();
	}

	SCHEDULER_MESSAGE_PTR msg_ptr;
	msg_ptr = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);

	if(msg_ptr == NULL) {
		//error;
		printf("could not aloc message in dd_delete: %u\n", _task_get_error());
		return 0;
	}

	msg_ptr->HEADER.SOURCE_QID = client_qid;
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULER_QUEUE);
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULER_MESSAGE);
	msg_ptr->T_ID = task_id;
	msg_ptr->ACTION = DELETE;


	_msgq_send(msg_ptr);

	msg_ptr =  _msgq_receive(client_qid, 0);
	_msgq_close(client_qid);
	_msg_free(msg_ptr);

	if(_task_destroy(task_id) != MQX_OK) {
		//error
		printf("oopsie whoopsie!\n");
		return 0;
	}

	return 1;
}

uint32_t dd_return_active_list(struct Node **list){
//	This primitive requests the DD-scheduler for the list of active tasks and returns this information to the
//	requestor.
//	The designer must decide whether the list should be copied and sent to the requestor, or simply a
//	pointer pointing to the start of the list could suffice. Analyze the alternatives and justify your
//	implementation choice.
	_queue_id client_qid = _msgq_open(MSGQ_FREE_QUEUE, 0);
	if (!client_qid) {
		printf("\nCould not open active list message queue\n");
		_task_block();
	}
	SCHEDULER_MESSAGE_PTR msg_ptr, msg_ptr_receive;
	msg_ptr = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
	msg_ptr_receive = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
	if(msg_ptr == NULL || msg_ptr_receive == NULL) {
		printf("oopsie whoopsie!\n");
		return 0;
	}

	msg_ptr->HEADER.SOURCE_QID = client_qid;
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULER_QUEUE);
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULER_MESSAGE);
	msg_ptr->ACTION = GET_ACTIVE;

	_msgq_send(msg_ptr);
	msg_ptr_receive = _msgq_receive(client_qid, 0);

    *list = msg_ptr_receive->TASK_LIST;

	_msg_free(msg_ptr_receive);
	_msgq_close(client_qid);

	return 1;
}

uint32_t dd_return_overdue_list(struct Node **list){
//	This primitive requests the DD-scheduler for the list of overdue tasks and returns this information to the
//	requestor. Similar in structure as the dd_return_active_list primitive.
	_queue_id client_qid = _msgq_open(MSGQ_FREE_QUEUE, 0);
	if (!client_qid) {
		printf("\nCould not open overdue list message queue\n");
		_task_block();
	}
	SCHEDULER_MESSAGE_PTR msg_ptr, msg_ptr_receive;
	msg_ptr = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
	msg_ptr_receive = (SCHEDULER_MESSAGE_PTR) _msg_alloc(message_pool);
	if(msg_ptr == NULL || msg_ptr_receive == NULL) {
		//error;
		printf("oopsie whoopsie!\n");
		return 0;
	}

	msg_ptr->HEADER.SOURCE_QID = client_qid;
	msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, SCHEDULER_QUEUE);
	msg_ptr->HEADER.SIZE = sizeof(SCHEDULER_MESSAGE);
	msg_ptr->ACTION = GET_OVERDUE;

	_msgq_send(msg_ptr);
	msg_ptr_receive = _msgq_receive(client_qid, 0);
    *list = msg_ptr_receive->OVERDUE_LIST;

	_msg_free(msg_ptr_receive);
	_msgq_close(client_qid);

	return 1;
}





