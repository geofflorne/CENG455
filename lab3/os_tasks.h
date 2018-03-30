/* ###################################################################
**     Filename    : os_tasks.h
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
** @file os_tasks.h
** @version 01.00
** @brief
**         This is user's event module.
**         Put your event handler code here.
*/         
/*!
**  @addtogroup os_tasks_module os_tasks module documentation
**  @{
*/         

#ifndef __os_tasks_H
#define __os_tasks_H
/* MODULE os_tasks */

#include "fsl_device_registers.h"
#include "clockMan1.h"
#include "pin_init.h"
#include "osa1.h"
#include "mqx_ksdk.h"
#include "uart1.h"
#include "fsl_mpu1.h"
#include "fsl_hwtimer1.h"
#include "MainTask.h"
#include "DDSchedulerTask.h"
#include "myUART.h"
#include "MonitorTask.h"
#include "UserTask.h"
#include "PeriodicGeneratorTask.h"
#include "AperiodicGeneratorTask.h"
#include "message.h"
#include "mutex.h"
#include "linked_list.h"

#ifdef __cplusplus
extern "C" {
#endif 

TIME_STRUCT time;

MQX_TICK_STRUCT start_tick, end_tick, diff_tick;
MQX_TICK_STRUCT user_start, user_end, user_diff;

uint32_t user_ticks;

MUTEX_STRUCT dd_mutex;
MUTEX_ATTR_STRUCT dd_mutex_attr;

#define SCHEDULER_QUEUE 8
#define APERIODIC_QUEUE 9
#define MONITOR_QUEUE 10

#define CREATE  0
#define DELETE  1
#define GET_ACTIVE  2
#define	GET_OVERDUE 3

#define MIN_USER_TASK_PRIORITY 15

_pool_id message_pool;

_task_id monitor_task_id;

QUEUE_STRUCT task_queue;
QUEUE_STRUCT *task_queue_ptr;

QUEUE_STRUCT overdue_queue;
QUEUE_STRUCT *overdue_queue_ptr;

typedef struct aperiodic_message {
	MESSAGE_HEADER_STRUCT HEADER;
	char			  	  DATA;
}APERIODIC_MESSAGE, *APERIODIC_MESSAGE_PTR;

typedef struct scheduler_message {
	MESSAGE_HEADER_STRUCT HEADER;
	uint32_t DEADLINE;
	uint32_t ACTION;
	_task_id T_ID;
	Node *TASK_LIST;
	Node *OVERDUE_LIST;
}SCHEDULER_MESSAGE, *SCHEDULER_MESSAGE_PTR;

typedef struct monitor_queue {
	MESSAGE_HEADER_STRUCT HEADER;
	char			  	  DATA;
}MONITOR_MESSAGE, *MONITOR_MESSAGE_PTR;

typedef struct task_gen_list {
	uint32_t task_index;
	uint32_t run_time;
	uint32_t deadline;
	uint32_t period;
	TIME_STRUCT start_time;
} TASK_GEN_LIST, * TASK_GEN_LIST_PTR;


/*
** ===================================================================
**     Callback    : dd_scheduler_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void dd_scheduler_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : monitor_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void monitor_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : user_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void user_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : aperiodic_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void aperiodic_generator_task(os_task_param_t task_init_data);

/*
** ===================================================================
**     Callback    : periodic_generator_task
**     Description : Task function entry.
**     Parameters  :
**       task_init_data - OS task parameter
**     Returns : Nothing
** ===================================================================
*/
void periodic_generator_task(os_task_param_t task_init_data);

bool is_overdue(Node *node);

void busy_delay(uint32_t miliseconds);

uint32_t get_time_in_ms();

/* END os_tasks */

#ifdef __cplusplus
}  /* extern "C" */
#endif 

#endif 
/* ifndef __os_tasks_H*/
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
