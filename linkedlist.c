/* Viziru Luciana - 332CA */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"
#include "util.h"

typedef struct node node;

/* struct for list node */
struct node
{
	node *next;		/* pointer to next node in list */
	void *value;	/* node info */
};

/* struct for list */
struct list
{
	node *head;		/* pointer to first node in list */
	int size;		/* list size */
};


/* return reference to an empty list */
list* create_list(void)
{
	list *mylist = malloc(sizeof(list));

	DIE(mylist == NULL, "List failed to create");

	mylist->size = 0;
	mylist->head = NULL;
	return mylist;
}

/* insert node to end of a list */
void insert(list *mylist, void *value, int info_size)
{
	node *new_node, *current;

	/* if node is already in list, do nothing */
	if (find(mylist, value, info_size)) {
		return;
	}

	/* alloc and initialize node */
	new_node = malloc(sizeof(node));
	DIE(new_node == NULL, "node failed to create");

	new_node->next = NULL;
	new_node->value = value;

	current = mylist->head;

	/* empty list, put new_node as head node */
	if (current == NULL) {
		mylist->head = new_node;
	}
	else {
		/* iterate to end of list */
		while (current->next != NULL) {
			current = current->next;
		}
		/* set new_node as last node*/
		current->next = new_node;
	}
	/* update list size */
	mylist->size++;
}

/* remove a node from list */
void remove_value(list *mylist, void *value, int info_size)
{
	node *current, *tmp;

	/* if nothing to remove, do nothing */
	if (!find(mylist, value, info_size)) {
		return;
	}

	current = mylist->head;
	/*if first node is to be removed*/
	if (memcmp(current->value, value, info_size) == 0) {
		mylist->head = current->next;
		free(current->value);
		free(current);
		/* update list size and return */
		mylist->size--;
		return;
	}

	/*iterate to desired node and remove*/
	while (memcmp(current->next->value, value, info_size) != 0) {
		current = current->next;
	}
	/* current->next is the node to remove */
	tmp = current->next->next;	/* save reference to next node*/
	free(current->next->value);
	free(current->next);
	current->next = tmp;	/* update links */
}

/* check if value exists in list */
int find(list *mylist, void *value, int info_size)
{
	node *current = mylist->head;
	/* iterate through list and check values */
	while (current != NULL) {

		if (memcmp(current->value, value, info_size) == 0) {
			return 1;
		}
		current = current->next;
	}

	return 0;
}

/* return an array of list values - char* */
void** get_values(list *mylist)
{
	node *current;
	int i;
	void **values = malloc(mylist->size * sizeof(void*));
	DIE(values == NULL, "malloc failed");
	current = mylist->head;
	i = 0;
	/* iterate through list and add values to array */
	while (current != NULL) {
		values[i] = current->value;
		current = current->next;
		i++;
	}
	return values;
}

/* list size getter */
int get_size(list *mylist)
{
	if (mylist == NULL)
		return -1;
	return mylist->size;
}

/* delete all elements in list */
void empty_list(list *mylist)
{
	node* current = mylist->head;
	node* next;
	/*free every node*/
	while (current != NULL) {
		next = current->next;
		free(current->value);
		free(current);
		current = next;
	}

	mylist->head = NULL;
	mylist->size = 0;
}

/* destroy list */
void destruct_list(list *mylist)
{
	if (mylist->size > 0) {
		empty_list(mylist);
	}
	free(mylist);
}
