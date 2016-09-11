/* Viziru Luciana - 332CA */

#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>

typedef struct list list;

/* return reference to an empty list */
list* create_list(void);

/* insert node to end of a list */
void insert(list *mylist, void *value, int info_size);

/* remove a node from list */
void remove_value(list *mylist, void *value, int info_size);

/* check if value exists in list */
/* ret 1 for success and 0 for fail */
int find(list *mylist, void *value, int info_size);

/* return an array of list values - char* */
void** get_values(list *mylist);

/* list size getter */
/* ret -1 for invalid list pointer */
int get_size(list *mylist);

/* delete all elements in list */
void empty_list(list *mylist);

/* destroy list */
void destruct_list(list *mylist);

#endif
