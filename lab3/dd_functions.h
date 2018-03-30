/*
 * dd_functions.h
 *
 *  Created on: Mar 6, 2018
 *      Author: geoff147
 */

#include <mqx.h>
#include "os_tasks.h"

_task_id dd_tcreate(uint32_t parameter);
uint32_t dd_delete(uint32_t task_id);
uint32_t dd_return_active_list(struct Node **list);
uint32_t dd_return_overdue_list(struct Node **list);
uint16_t get_client_qid();
