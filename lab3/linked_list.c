#include "linked_list.h"
int size = 0;

Node *create_node(uint32_t tid, uint32_t deadline, uint32_t creation_time, uint32_t task_type) {
	Node *new_node = malloc(sizeof(Node));
	new_node->prev = NULL;
	new_node->next = NULL;
	new_node->tid = tid;
	new_node->deadline = deadline;
	new_node->task_type = task_type;
	new_node->creation_time = creation_time; // GET CURR TIME
	return new_node;
}

// does NOT free the popped node! This is so that we can just add it to another queue if needed
Node *delete_head(Node *head) {
	size--;
	if (head == NULL) {
		return NULL;
	}

	Node *new_head = head->next;
	if (new_head != NULL) {
		new_head->prev = NULL;
	}
	return new_head;
}

// insert a Node into the list in sorted order
Node *insert(Node *head, Node *new_node) {
	size++;
	if (head == NULL) {
		new_node->next = NULL;
		new_node->prev = NULL;
		return new_node;
	}
	if (abs_deadline(new_node) < abs_deadline(head)) { // new node has shortest deadline. make it the new head
		new_node->next = head;
		head->prev = new_node;
		return new_node;
	}

	Node *prev = head;
	Node *curr = head->next;

	// iterate until we find a node with larger deadline than new_node, or reach the end
	while(1) {
		if (curr == NULL) { // we are at the end. insert new node here
			prev->next = new_node;
			new_node->prev = prev;
			break;
		}
		if (abs_deadline(new_node) < abs_deadline(curr)) {
			prev->next = new_node;
			new_node->prev = prev;
			new_node->next = curr;
			curr->prev = new_node;
			break;
		}
		prev = curr;
		curr = curr->next;
	}
	return head;
}

uint32_t abs_deadline(Node *node) {
	return node->creation_time + node->deadline;
}
