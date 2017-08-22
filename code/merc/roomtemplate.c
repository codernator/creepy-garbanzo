#include "merc.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "serialize.h"


/*@dependent@*/struct extra_descr_data *roomtemplate_addextra(struct room_index_data *template, /*@observer@*/const char *keyword, /*@observer@*/const char *description)
{
    struct extra_descr_data *extra;

    extra = extradescrdata_new();
    assert(extra->next == NULL);
    assert(extra->keyword == NULL);
    assert(extra->description == NULL);
    extra->keyword = strdup(keyword);
    extra->description = strdup(description);
    extra->next = template->extra_descr->next;
    template->extra_descr->next = extra;
    return extra;
}

/*@dependent@*//*@null@*/struct extra_descr_data *roomtemplate_findextra(struct room_index_data *template, /*@observer@*/const char *keyword)
{
    struct extra_descr_data *extra;

    for (extra = template->extra_descr->next; extra != NULL; extra = extra->next)
        if (is_name(keyword, extra->keyword))
            return extra;

    return NULL;
}

bool roomtemplate_deleteextra(struct room_index_data *template, /*@observer@*/const char *keyword)
{
    struct extra_descr_data *prev;
    struct extra_descr_data *curr;

    if (template->extra_descr->next == NULL)
        return false;

    prev = template->extra_descr;
    curr = prev->next;
    while (curr != NULL && !is_name(keyword, curr->keyword)) {
        prev = curr;
        curr = prev->next;
    }

    if (curr == NULL)
        return false;

    assert(prev->next == curr);
    prev->next = curr->next;

    // TODO figure out how to properly annotate or rewrite the contents of this method.
    /*@-dependenttrans@*/
    /*@-usereleased@*/
    /*@-compdef@*/
    curr->next = NULL;
    extradescrdata_free(curr);
    return true;
    /*@+compdef@*/
    /*@+usereleased@*/
    /*@+compdef@*/
}

void olc_roomtemplate_getdescription(void *owner, char *target, size_t maxlen)
{
    struct room_index_data *roomtemplate;
    roomtemplate = (struct room_index_data *)owner;
    (void)strncpy(target, roomtemplate->description, maxlen);
    return;
}

void olc_roomtemplate_setdescription(void *owner, const char *text)
{
    struct room_index_data *roomtemplate;
    roomtemplate = (struct room_index_data *)owner;
    free_string(roomtemplate->description);
    roomtemplate->description = str_dup(text);
    return;
 }
