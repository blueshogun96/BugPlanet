#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"


/*
 * Usage notes:
 * When iterating through a linked list with the possibility of deleting nodes,
 * it's important get the next node BEFORE you delete anything! I found this out
 * the hard way, and it was a hassle.
 */

/* Deletion callback function */
void (*delete_func)(void*);


/* Add a node at the beginning of the list */
void list_add_beginning( struct node_t** head, void* data )
{
	struct node_t* temp;

	temp = ( struct node_t* ) malloc( sizeof( struct node_t ) );
	temp->data = data;

	if( (*head) == NULL )
	{
		(*head) = temp;
		(*head)->next = NULL;
	}
	else
	{
		temp->next = (*head);
		(*head) = temp;
	}
}

/* Add list node to the end */
void list_add_end( struct node_t** head, void* data )
{
	struct node_t* temp1;
	struct node_t* temp2;

	temp1 = (struct node_t*) malloc( sizeof( struct node_t ) );
	temp1->data = data;

	temp2 = (*head);

	if( (*head) == NULL )
	{
		(*head) = temp1;
		(*head)->next = NULL;
	}
	else
	{
		while( temp2->next != NULL )
			temp2 = temp2->next;

		temp1->next = NULL;
		temp2->next = temp1;
	}
}

/* Add a new node at a specific position */
void list_add_at( struct node_t** head, void* data, int loc )
{
	int i;
	struct node_t *temp, *prev_ptr, *cur_ptr;

	cur_ptr = (*head);

	if( loc > (list_length( head )+1) || loc <= 0 )
	{
	}
	else
	{
		if( loc == 1 )
		{
			list_add_beginning(head, data);
		}
		else
		{
			for( i = 1; i < loc; i++ )
			{
				prev_ptr = cur_ptr;
				cur_ptr = cur_ptr->next;
			}

			temp = (struct node_t*) malloc( sizeof( struct node_t ) );
			temp->data = data;

			prev_ptr->next = temp;
			temp->next = cur_ptr;
		}
	}
}


/* Returns the number of elements in the list */
int list_length( struct node_t** head )
{
	struct node_t* cur_ptr;
	int count= 0;

	cur_ptr = (*head);

	while( cur_ptr != NULL )
	{
		cur_ptr = cur_ptr->next;
		count++;
	}

	return count;
}


/* Delete a node from the list */
int list_delete( struct node_t** head, void* data )
{
	struct node_t *prev_ptr, *cur_ptr;

	cur_ptr = (*head);

	while( cur_ptr != NULL )
	{
		if( cur_ptr->data == data )
		{
			if( cur_ptr == (*head) )
			{
				(*head) = cur_ptr->next;
				if( delete_func ) delete_func( cur_ptr->data );
				free( cur_ptr );
				return 1;
			}
			else
			{
				prev_ptr->next = cur_ptr->next;
				if( delete_func ) delete_func( cur_ptr->data );
				free( cur_ptr );
				return 1;
			}
		}
		else
		{
			prev_ptr = cur_ptr;
			cur_ptr = cur_ptr->next;
		}
	}

	return 0;
}

/* Deletes a node from the given position */
int list_delete_loc( struct node_t** head, int loc )
{
	struct node_t *prev_ptr, *cur_ptr;
	int i;

	cur_ptr = (*head);

	if( loc > list_length( head ) || loc <= 0 )
	{
	}
	else
	{
		if( loc == 1 )
		{
			(*head) = cur_ptr->next;
			if( delete_func ) delete_func( cur_ptr->data );
			free(cur_ptr);
			return 1;
		}
		else
		{
			for( i = 1; i < loc; i++ )
			{
				prev_ptr = cur_ptr;
				cur_ptr = cur_ptr->next;
			}

			prev_ptr->next = cur_ptr->next;
			if( delete_func ) delete_func( cur_ptr->data );
			free(cur_ptr);
		}
	}

	return 0;
}

/* Deletes every node in the list */
void list_clear( struct node_t** head )
{
	struct node_t* cur_ptr, *temp;

	cur_ptr = (*head);

	while( cur_ptr )
	{
		temp = cur_ptr->next;
		if( delete_func ) delete_func( cur_ptr->data );
		free(cur_ptr);
		cur_ptr = temp;
	}
    
    (*head) = NULL;
}

/* Retrieve data from the selected node */
void* list_get_node_data( struct node_t** head, int loc )
{
	struct node_t* cur_ptr;
	int i = 1;

	cur_ptr = (*head);

	if( loc < 1 )
		return NULL;

	while( i < loc )
	{
		cur_ptr = cur_ptr->next;
		i++;
	}
    
    if( !cur_ptr )
        return NULL;
	
	return cur_ptr->data;
}
		
/* Node deletion callback */
void set_deletion_callback( void (*func)(void*) )
{
	delete_func = func;
}
