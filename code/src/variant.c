#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "merc.h"
#include "recycle.h"
#include "utility.h"



/***************************************************************************
*	variant memory recycling
***************************************************************************/
static VARIANT *variant_free;

/***************************************************************************
*	new_variant
*
*	create a new variant structure
***************************************************************************/
VARIANT *new_variant()
{
	static VARIANT z_variant;
	VARIANT *variant;

	if (variant_free == NULL) {
		variant = alloc_perm((unsigned int)sizeof(*variant));
	} else {
		variant = variant_free;
		variant_free = variant_free->recycle;
	}

	*variant = z_variant;
	VALIDATE(variant);
	return variant;
}


/***************************************************************************
*	free_variant
*
*	free a variant structure
***************************************************************************/
void free_variant(VARIANT *variant)
{
	if (!IS_VALID(variant))
		return;

	if (variant->type == VARIANT_STRING) {
		if (variant->data != NULL)
			free_string(variant->data);
	}

	variant->data = NULL;
	variant->type = VARIANT_NULL;
	INVALIDATE(variant);

	variant->recycle = variant_free;
	variant_free = variant;
}



/***************************************************************************
*	non-type-safe functions
*
*	set and get functions rely on void data types
***************************************************************************/
/***************************************************************************
*	set_variant
*
*	set the variant based on the type passed in
***************************************************************************/
void set_variant(VARIANT *variant, int var_type, void *data)
{
	variant->type = var_type;
	variant->state = (is_valid_type(var_type)) ? VARIANT_OK : VARIANT_ERROR;
	variant->data = data;
}

/***************************************************************************
*	get_variant
*
*	get the variant data based on its type
*	return NULL if there is an error
***************************************************************************/
void *get_variant(VARIANT *variant)
{
	return (variant->state == VARIANT_OK) ? variant->data : NULL;;
}

/***************************************************************************
*	is_valid_type
*
*	do we have a valid variant type?
***************************************************************************/
bool is_valid_type(int var_type)
{
	bool valid;


	switch (var_type) {
	default:
		valid = false;
		break;
	case VARIANT_INTEGER:
	case VARIANT_LONG:
	case VARIANT_STRING:
	case VARIANT_CHARACTER:
	case VARIANT_MOB_INDEX:
	case VARIANT_ROOM_INDEX:
	case VARIANT_OBJECT:
	case VARIANT_OBJECT_INDEX:
	case VARIANT_DESCRIPTOR:
	case VARIANT_AREA:
		valid = true;
		break;
	}

	return valid;
}
