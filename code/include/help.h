/* Help table types. */
struct help_data {
    /*@owned@*//*@null@*//*@partial@*/struct help_data *next;
    /*@dependent@*//*@null@*//*@partial@*/struct help_data *prev;

    unsigned int level;
    unsigned int trust;
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
/*@dependent@*/struct help_data *helpdata_new();
void helpdata_free(/*@owned@*/struct help_data *helpdata);
struct keyvaluepair_array *helpdata_serialize(const struct help_data *helpdata);
/*@dependent@*/struct help_data *helpdata_deserialize(const struct keyvaluepair_array *data);
/*@observer@*//*@null@*/struct help_data *help_lookup(const char *keyword);

/*@dependent@*//*@null@*/struct help_data *helpdata_iteratorstart();
/*@dependent@*//*@null@*/struct help_data *helpdata_iteratornext(struct help_data *);
