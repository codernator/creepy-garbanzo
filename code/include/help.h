typedef struct help_data HELP_DATA;

/* Help table types. */
struct help_data {
    /*@owned@*//*@null@*//*@partial@*/HELP_DATA *next;
    /*@dependent@*//*@null@*//*@partial@*/HELP_DATA *prev;

    int level;
    int trust;
    /*@owned@*/char *keyword;
    /*@owned@*/char *text;
    /*@owned@*/char *category;
};

struct helpdata_iterator {
    /*@observer@*/HELP_DATA *current;
};

/* help.c */
//TODO - this is maybe overly encapsulated. Maybe it should be 
//   show_help(desc, helpdata_find(topic, argument));
void show_help(/*@observer@*/DESCRIPTOR_DATA *descriptor, /*@observer@*/const char *topic, /*@observer@*/const char *argument);
bool is_help(/*@observer@*/const char *argument);
int count_helps();
/*@dependent@*/HELP_DATA *helpdata_new();
void helpdata_free(/*@owned@*/HELP_DATA *helpdata);
KEYVALUEPAIR_ARRAY *helpdata_serialize(const HELP_DATA *helpdata);
/*@dependent@*/HELP_DATA *helpdata_deserialize(const KEYVALUEPAIR_ARRAY *data);
/*@observer@*//*@null@*/HELP_DATA *help_lookup(const char *keyword);

/*@only@*//*@null@*/struct helpdata_iterator *helpdata_iteratorstart();
/*@only@*//*@null@*/struct helpdata_iterator *helpdata_iteratornext(/*@only@*/struct helpdata_iterator *);
