#if !defined(__UTILITY_H)
#define __UTILITY_H

/***************************************************************************
*	typedefs
***************************************************************************/
typedef struct node_type NODE;
typedef struct variant_type VARIANT;
typedef struct linked_list LLIST;


/***************************************************************************
*	variant type definitions
***************************************************************************/
#define VARIANT_NULL                    0
#define VARIANT_INTEGER                 1
#define VARIANT_LONG                    2
#define VARIANT_STRING                  3
#define VARIANT_CHARACTER               4
#define VARIANT_MOB_INDEX               5
#define VARIANT_ROOM_INDEX              6
#define VARIANT_OBJECT                  7
#define VARIANT_OBJECT_INDEX            8
#define VARIANT_DESCRIPTOR              9
#define VARIANT_AREA                    10

/***************************************************************************
*	states
*
*	if the memory that the variant contains has been deleted
*	by some other source, or if the variant becomes unstable
*	for some other reason, the variant will set itself to
*	an error state
***************************************************************************/
#define VARIANT_ERROR                   0
#define VARIANT_OK                              1


/***************************************************************************
*	VALIDATE_VARIANT
*
*	used to determine if a variant is of a certain type and that
*	the state is still valid
***************************************************************************/
#define VALIDATE_VARIANT(var, var_type)                         \
	((var) != NULL)                                         \
	&&                                                                      \
	(((var)->type == (var_type))            \
	 &&                                                                      \
	 ((var)->state == VARIANT_OK))           \


#define RESOLVE_VARIANT_TYPE(var, out, type)            \
	if (VALIDATE_VARIANT((var), type))                       \
	{                                                                                       \
		(out) = (char *)(var)->data;                  \
	}                                                                                       \



/***************************************************************************
*	variant_type
*
*	a type that can easily cast more than one value to
***************************************************************************/
struct variant_type {
	VARIANT *recycle;   /* for memory recycling	*/
	int type;           /* type of the VARIANT	*/
	void *data;
	int state;          /* for error state		*/
	bool valid;         /* for memory recycling	*/
};

/* non-type-safe variant functions -- scary */
void set_variant(VARIANT *variant, int type, void *data);
void *get_variant(VARIANT *variant);
bool is_valid_type(int var_type);


/***************************************************************************
*	node_type
*
*	a single node in a linked list
***************************************************************************/
struct node_type {
	NODE *		recycle;                /* used for memory recycling */
	NODE *		next;
	NODE *		prev;
	VARIANT *	data;
	char *		key;
	bool		valid;                  /* used for memory recycling */
};



/***************************************************************************
*	linked_list
*
*	a list that contains one or more nodes
***************************************************************************/
struct linked_list {
	NODE *	node_start;
	NODE *	node_end;
	LLIST * recycle;                /* used for memory recycling */
	bool	valid;                  /* used for memory recycling */
};

void            llist_add(LLIST *list, char *key, void *data, int data_type);
void            llist_delete(LLIST *list, char *key);
VARIANT *llist_get(LLIST *list, char *key);

/***************************************************************************
*	variant recycling functions
***************************************************************************/
VARIANT *new_variant();
void            free_variant(VARIANT *var);

/***************************************************************************
*	node recycling functions
***************************************************************************/
NODE *new_node();
void            free_node(NODE *node);

/***************************************************************************
*	linked list recycling functions
***************************************************************************/
LLIST *new_list();
void            free_list(LLIST *list);


#endif  /* __UTILITY_H */
