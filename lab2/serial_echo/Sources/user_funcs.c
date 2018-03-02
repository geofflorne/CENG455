#include "user_funcs.h"
#include "os_tasks.h"

// THIS FUNCTION IS NOT FULLY FINISHED!
// add self to the list of tasks with read permission
int OpenR(_queue_id stream_no) {
	_task_id task_id = _task_get_id();

	if (_mutex_lock(&read_mutex) == MQX_OK) {
		//read lock gained
		int i = 0;
		for(; task_read_permissions[i]; i++) { // check  to see if already have permission
			printf("idx %i: %i\n", i, task_read_permissions[i]);
			if (task_read_permissions[i] == task_id) {
				_mutex_unlock(&read_mutex);
				return 0; // already have permission
			}
		}

		// if not past end of array and current index is empty
		if (i < NUM_USER_TASKS && !task_read_permissions[i]) { // give read permission to task
			task_read_permissions[i] = task_id;

			_mutex_unlock(&read_mutex);
			printf("task %i got read permission\n");
			return 1; // TRUE
		}
	}

	printf("max read permissions already given. Need to wait until another task relinquishes\n");
	_mutex_unlock(&read_mutex);
	return 0;
}

// COMMENTED CODE FOR OpenW() that is not yet finished!
//int _getline(char* string) {
//	user_msg_ptr = _msgq_receive(user_qid, 0);
//	return 0;
//}

// try to grab write permission for the task with id task_id (pass own id in here)
// if can't gain permission, then exit
// write mutex acts as write permissions
// since only one task can access
_queue_id OpenW() {
	_mqx_uint mutex_lock_result = _mutex_lock(&write_mutex);
	if (mutex_lock_result == MQX_OK) {
		if (task_write_permission != NO_TASK) { // something is already holding the write permission. return 0 (error)
			printf("write permission already held by task %i\n", task_write_permission);
			_mutex_unlock(&write_mutex);
			return 0;
		}

		task_write_permission = _task_get_id();

		_mutex_unlock(&write_mutex);

		return HANDLER_QUEUE; // if lock succeeds (no task is using write channel then return handler queue id
	}

	printf("error obtaining write lock for task %i error: %d \n", _task_get_id(), mutex_lock_result);
	return 0; // lock could not be obtained. return 0
}

void clearString(char *inputString) {
	for(int i = 0; i < strlen(inputString); i++) inputString[i] = '\0';
}

int _putline(_queue_id qid, char* string) {
	// Send a message to the handler task containing a string for the handler to
	// send to the output queue for printing

	_task_id task_id = _task_get_id();
	char stringWithNewline[strlen(string) + 1];
	clearString(stringWithNewline);

	// Append newline to string
	strcat(stringWithNewline, string);
	strcat(stringWithNewline, "\n");

	if (_mutex_lock(&write_mutex) == MQX_OK) { // Try to grab lock for accessing the write permission locker
		if (task_write_permission != task_id) { // another task is holding the write permission.
			printf("write permission held by other task %i when trying to putline\n", task_write_permission);
			return 0; // FALSE
		}

		_mutex_unlock(&write_mutex); // We're done examining the write permission locker

		// Initialize message to send
		MESSAGE_T_PTR msg_ptr;
		msg_ptr = (MESSAGE_T_PTR)_msg_alloc(message_pool);
		bool result;
		if (msg_ptr == NULL) {
			printf("\nCould not allocate a message in _putline() in task%i\n", task_id);
			_task_block();
		}

		// Set message data
		msg_ptr->HEADER.TARGET_QID = _msgq_get_id(0, qid);
		msg_ptr->HEADER.SIZE = sizeof(MESSAGE_HEADER_STRUCT) + strlen((char *)msg_ptr->DATA) + 1;
		msg_ptr->DATA = stringWithNewline;

		// Send string to handler task's queue
		result = _msgq_send(msg_ptr);
		if (result != TRUE) {
		  printf("\nCould not send a message\n");
	      _task_block();
		}

		return 1; // TRUE
	}

	printf("unable to obtain write lock in _putline\n");
	return 0;
}

int Close() {
	// Relinquish permission for the write stream

	_task_id task_id = _task_get_id(); // Get the id of the invoking task
	if (_mutex_lock(&write_mutex) == MQX_OK) { // Grab the lock for modifying the write permission tracker
		if (task_write_permission != task_id) {
			 // a task is already holding the write permission (only one allowed at a time)
			printf("write permission held by other task %i\n", task_write_permission);
			return 0; // FALSE
		}

		task_write_permission = NO_TASK; // Release write permission

		_mutex_unlock(&write_mutex);

		return 1; // TRUE
	}

	return 0; // FALSE
}
