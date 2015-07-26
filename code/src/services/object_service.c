#include "merc.h"

inline bool is_situpon(OBJ_DATA *obj) {
    return (obj->item_type == ITEM_FURNITURE)
            && (IS_SET(obj->value[2], SIT_ON)
                || IS_SET(obj->value[2], SIT_IN)
                || IS_SET(obj->value[2], SIT_AT));
}

inline bool is_standupon(OBJ_DATA *obj) {
    return (obj->item_type == ITEM_FURNITURE
		    && (IS_SET(obj->value[2], STAND_AT)
                || IS_SET(obj->value[2], STAND_ON)
                || IS_SET(obj->value[2], STAND_IN)));
}

