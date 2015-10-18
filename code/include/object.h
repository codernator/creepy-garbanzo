typedef struct object_iterator_filter OBJECT_ITERATOR_FILTER;
struct object_iterator_filter {
    /*@null@*/const char *name;
    /*@null@*/struct objectprototype *object_template;
};
extern const OBJECT_ITERATOR_FILTER object_empty_filter;

/** Create a new object instance, tracked on the master object list, from the specified prototype. */
/*@dependent@*/GAMEOBJECT *object_new(/*@dependent@*/struct objectprototype *prototypedata);

/** Create a new object instance, tracked on the master object list, from the specified prototype. */
/*@dependent@*/GAMEOBJECT *object_clone(/*@dependent@*/GAMEOBJECT *parent);

/** Clean up all memory used by an object and release the object. */
void object_free(/*@owned@*/GAMEOBJECT *obj);

/** Get the number of active objects tracked by the master object list. */
int object_list_count();

/** Start an iterator over the master object list, using the specified filter. */
/*@dependent@*//*@null@*/GAMEOBJECT *object_iterator_start(const OBJECT_ITERATOR_FILTER *filter);

/** Continue an iterator over the master object list, using the specified filter. */
/*@dependent@*//*@null@*/GAMEOBJECT *object_iterator(GAMEOBJECT *current, const OBJECT_ITERATOR_FILTER *filter);

/** Encapsulation of object ownership. */
/*@dependent@*//*@null@*/const char *object_ownername_get(const GAMEOBJECT *object);
void object_ownername_set(GAMEOBJECT *object, const CHAR_DATA *owner);

/** Encapsulation of object name. */
/*@dependent@*/const char *object_name_get(const GAMEOBJECT *object);
void object_name_set(GAMEOBJECT *object, /*@shared@*//*@null@*/const char *name);


bool is_situpon(/*@partial@*/GAMEOBJECT *obj);


bool is_standupon(/*@partial@*/GAMEOBJECT *obj);

