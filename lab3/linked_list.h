#include "mqx.h"

typedef struct Node {
	struct Node *prev;
	struct Node *next;
	uint32_t tid;
	uint32_t deadline;
	uint32_t task_type;
	uint32_t creation_time;
} Node;

Node *create_node(uint32_t tid, uint32_t deadline, uint32_t creation_time, uint32_t task_type);
Node *delete_head(Node *head);
Node *insert(Node *head, Node *new_node);
uint32_t abs_deadline(Node *node);
