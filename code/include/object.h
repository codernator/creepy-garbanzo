typedef struct object_iterator_filter OBJECT_ITERATOR_FILTER;
struct object_iterator_filter {
    /*@null@*/const char *name;
    /*@null@*/struct objectprototype *object_template;
};
extern const OBJECT_ITERATOR_FILTER object_empty_filter;

/** Create a new object instance, tracked on the master object list, from the specified prototype. */
/*@dependent@*/struct gameobject *object_new(/*@dependent@*/struct objectprototype *prototypedata);

/** Create a new object instance, tracked on the master object list, from the specified prototype. */
/*@dependent@*/struct gameobject *object_clone(/*@dependent@*/struct gameobject *parent);

/** Clean up all memory used by an object and release the object. */
void object_free(/*@owned@*/struct gameobject *obj);

/** Get the number of active objects tracked by the master object list. */
int object_list_count();

/** Start an iterator over the master object list, using the specified filter. */
/*@dependent@*//*@null@*/struct gameobject *object_iterator_start(const OBJECT_ITERATOR_FILTER *filter);

/** Continue an iterator over the master object list, using the specified filter. */
/*@dependent@*//*@null@*/struct gameobject *object_iterator(struct gameobject *current, const OBJECT_ITERATOR_FILTER *filter);

/** Encapsulation of object ownership. */
/*@dependent@*//*@null@*/const char *object_ownername_get(const struct gameobject *object);
void object_ownername_set(struct gameobject *object, const struct char_data *owner);

/** Encapsulation of object name. */
/*@dependent@*/const char *object_name_get(const struct gameobject *object);
void object_name_set(struct gameobject *object, /*@shared@*//*@null@*/const char *name);


bool is_situpon(/*@partial@*/struct gameobject *obj);


bool is_standupon(/*@partial@*/struct gameobject *obj);

