#include "user_funcs.h"
#include "os_tasks.h"

int OpenR(_queue_id queue_id) {
	_task_id task_id = _task_get_id();

	if (_mutex_lock(&read_mutex) == MQX_OK) {
		//read lock gained
		int i = 0;
		//printf("iterating through read permissions\n");
		for(; task_read_permissions[i].task_id; i++) { // check  to see if already have permission
			//printf("idx %i -> task_id: %i, queue_id: %i\n", i,
				//	task_read_permissions[i].task_id, task_read_permissions[i].queue_id);
			if (task_read_permissions[i].task_id == task_id) {
				_mutex_unlock(&read_mutex);
				return 0; // already have permission
			}
		}

		// if not past end of array and current index is empty
		if (i < NUM_USER_TASKS && !task_read_permissions[i].task_id) { // give read permission to task
			// new_read_permission = read_permission;
			task_read_permissions[i].task_id = task_id;
			task_read_permissions[i].queue_id = queue_id;

			_mutex_unlock(&read_mutex);
			return 1; // TRUE
		}
	}

	printf("max read permissions already given. Need to wait until another task relinquishes\n");
	_mutex_unlock(&read_mutex);
	return 0;
}

int _getline(char** string, _queue_id user_queue_id) {
	_task_id task_id = _task_get_id();
	int has_permission = 0;

	// check to see if we have read permission
	if (_mutex_lock(&read_mutex) == MQX_OK) { // read lock gained?
		for(int i = 0; i < 2; i++) { // check to see if already have permission
			if (task_read_permissions[i].task_id == task_id) { // task has permission
				has_permission = 1; // TRUE
			}
		}
		_mutex_unlock(&read_mutex);
	}else{
		printf("could not get read lock for getline()\n");
	}

	if (has_permission == 0) return 0; // NO read permission

	MESSAGE_T_PTR user_msg_ptr = _msgq_receive(user_queue_id, 0); // block waiting for a message

	*string = user_msg_ptr->DATA; // set string to the contents of the message
	_msg_free(user_msg_ptr);

	return 1; // TRUE
}

// try to grab write permission for the task with id task_id (pass own id in here)
// if can't gain permission, then exit
// write mutex acts as write permissions
// since only one task can access
_queue_id OpenW() {
	_mqx_uint mutex_lock_result = _mutex_lock(&write_mutex);
	if (mutex_lock_result == MQX_OK) {
		if (task_write_permission != NO_TASK) { // something is already holding the write permission. return 0 (error)
			//printf("write permission already held by task %i\n", task_write_permission);
			_mutex_unlock(&write_mutex);
			return 0;
		}

		task_write_permission = _task_get_id();

		_mutex_unlock(&write_mutex);

		return HANDLER_QUEUE; // if lock succeeds (no task is using write channel then return handler queue id
	}

	printf("error obtaining write lock for task %i error: %d \n", _task_get_id(), mutex_lock_result);
	if(mutex_lock_result == MQX_CANNOT_CALL_FUNCTION_FROM_ISR){
		printf("MQX_CANNOT_CALL_FUNCTION_FROM_ISR");

	}else if(mutex_lock_result == MQX_EBUSY){
		printf("MQX_EBUSY");

	}else if(mutex_lock_result == MQX_EDEADLK){
		printf("MQX_EDEADLK");

	}else if(mutex_lock_result == MQX_EINVAL){
		printf("MQX_EINVAL");
	}
	return 0; // lock could not be obtained. return 0
}

void clearString(char *inputString) {
	for(int i = 0; i < strlen(inputString); i++) inputString[i] = '\0';
}

int _putline(_queue_id qid, char* string) {
	// Send a message to the handler task containing a string for the handler to
	// send to the output queue for printing

	_task_id task_id = _task_get_id();
	char stringWithNewline[strlen(string) + 2];
	clearString(stringWithNewline);

	// Append newline to string
	strcat(stringWithNewline, string);
	strcat(stringWithNewline, "\n\r");

	if (_mutex_lock(&write_mutex) == MQX_OK) { // Try to grab lock for accessing the write permission locker
		if (task_write_permission != task_id) { // another task is holding the write permission.
			printf("write permission held by other task %i when trying to putline\n", task_write_permission);
			_mutex_unlock(&write_mutex);
			return 0; // FALSE
		}

		// Initialize message to send
		MESSAGE_T_PTR msg_ptr = (MESSAGE_T_PTR)_msg_alloc(message_pool);
		if (msg_ptr == NULL) {
			printf("\nCould not allocate a message in _putline() in task%i\n", task_id);
			_task_block();
		}

		// Set message data
		msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, qid);
		msg_ptr->HEADER.SIZE = sizeof(MESSAGE_HEADER_STRUCT) + strlen((char *)msg_ptr->DATA) + 1;
		msg_ptr->DATA = stringWithNewline;

//		printf("sending %s from putline\n", stringWithNewline);
		// Send string to handler task's queue
		bool result = _msgq_send(msg_ptr);
		if (result != TRUE) {
		  printf("\nCould not send a message to handler\n");
	      _task_block();
		}
		_mutex_unlock(&write_mutex);// We're done examining the write permission locker
		return 1; // TRUE
	}

	printf("unable to obtain write lock in _putline\n");
	return 0;
}

int Close() {
	// Relinquish permission for the write stream
	bool lock_relinquished = false;

	_task_id task_id = _task_get_id(); // Get the id of the invoking task
	if (_mutex_lock(&write_mutex) == MQX_OK) { // Grab the lock for modifying the write permission tracker
		if (task_write_permission != task_id) {
			 // a task is already holding the write permission (only one allowed at a time)
			printf("write permission held by other task %i\n", task_write_permission);
			_mutex_unlock(&write_mutex);
		}else{
			task_write_permission = NO_TASK; // Release write permission
			_mutex_unlock(&write_mutex);
			lock_relinquished =  true; // TRUE
		}
	}

	if (_mutex_lock(&read_mutex) == MQX_OK) {
		//read lock gained
		int i = 0;
		for(; task_read_permissions[i].task_id; i++) { // check  to see if already have permission
			if (task_read_permissions[i].task_id == task_id) {
				task_read_permissions[i].task_id = NULL;
				task_read_permissions[i].queue_id = NULL;
				lock_relinquished = true;
			}
		}
		_mutex_unlock(&read_mutex);
	}else{
		printf("could not get read lock for close()\n");
	}

	return lock_relinquished;
}
