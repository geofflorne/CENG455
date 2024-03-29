#include <mqx.h>
#include <message.h>
#include <mutex.h>
#include <bsp.h>

#define NO_TASK -1
#define NUM_USER_TASKS 2

typedef struct read_permission {
	_task_id task_id;
	_queue_id queue_id;
} read_permission;

MUTEX_STRUCT write_mutex;
MUTEX_ATTR_STRUCT write_mutex_attr;
MUTEX_STRUCT read_mutex;
MUTEX_ATTR_STRUCT read_mutex_attr;

int task_write_permission; // task_read_permission == 1 means task 1 can write
read_permission task_read_permissions[NUM_USER_TASKS]; // if id is in task_read_permissions it means task id can read
int OpenR(_queue_id stream_no);
_queue_id OpenW();
int _putline(_queue_id qid, char* string);
int Close();
int _getline(char** string, _queue_id user_queu_id);
