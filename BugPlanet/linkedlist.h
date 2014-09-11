#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Linked list structure */
struct node_t
{
	void* data;
	struct node_t* next;
};

/* Add a node at the beginning of the list */
void list_add_beginning( struct node_t** head, void* data );
/* Add list node to the end */
void list_add_end( struct node_t** head, void* data );
/* Add a new node at a specific position */
void list_add_at( struct node_t** head, void* data, int loc );
/* Returns the number of elements in the list */
int list_length( struct node_t** head );
/* Delete a node from the list */
int list_delete( struct node_t** head, void* data );
/* Deletes a node from the given position */
int list_delete_loc( struct node_t** head, int loc );
/* Deletes every node in the list */
void list_clear( struct node_t** head );
/* Retrieve data from the selected node */
void* list_get_node_data( struct node_t** head, int loc );
/* Node deletion callback */
void set_deletion_callback( void (*func)(void*) );

#ifdef __cplusplus
}
#endif