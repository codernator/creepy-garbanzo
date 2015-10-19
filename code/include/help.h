typedef struct help_data HELP_DATA;

/* Help table types. */
struct help_data {
    /*@owned@*//*@null@*//*@partial@*/HELP_DATA *next;
    /*@dependent@*//*@null@*//*@partial@*/HELP_DATA *prev;

    int level;
    int trust;
    /*@only@*/char *keyword;
    /*@only@*/char *text;
    /*@only@*/char *category;
};

/* help.c */
//TODO - this is maybe overly encapsulated. Maybe it should be 
//   show_help(desc, helpdata_find(topic, argument));
void show_help(/*@observer@*/struct descriptor_data *descriptor, /*@observer@*/const char *topic, /*@observer@*/const char *argument);
bool is_help(/*@observer@*/const char *argument);
int count_helps();
/*@dependent@*/HELP_DATA *helpdata_new();
void helpdata_free(/*@owned@*/HELP_DATA *helpdata);
KEYVALUEPAIR_ARRAY *helpdata_serialize(const HELP_DATA *helpdata);
/*@dependent@*/HELP_DATA *helpdata_deserialize(const KEYVALUEPAIR_ARRAY *data);
/*@observer@*//*@null@*/HELP_DATA *help_lookup(const char *keyword);

/*@dependent@*//*@null@*/struct help_data *helpdata_iteratorstart();
/*@dependent@*//*@null@*/struct help_data *helpdata_iteratornext(struct help_data *);
