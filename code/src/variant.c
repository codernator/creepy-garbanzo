/***************************************************************************
*   Original Diku Mud copyright(C) 1990, 1991 by Sebastian Hammer,         *
*   Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
*                                                                              *
*   Merc Diku Mud improvments copyright(C) 1992, 1993 by Michael           *
*   Chastain, Michael Quan, and Mitchell Tse.                              *
*	                                                                       *
*   In order to use any part of this Merc Diku Mud, you must comply with   *
*   both the original Diku license in 'license.doc' as well the Merc	   *
*   license in 'license.txt'.  In particular, you may not remove either of *
*   these copyright notices.                                               *
*                                                                              *
*   Much time and thought has gone into this software and you are          *
*   benefitting.  We hope that you share your changes too.  What goes      *
*   around, comes around.                                                  *
***************************************************************************/

/***************************************************************************
*   ROM 2.4 is copyright 1993-1998 Russ Taylor                             *
*   ROM has been brought to you by the ROM consortium                      *
*       Russ Taylor(rtaylor@hypercube.org)                                 *
*       Gabrielle Taylor(gtaylor@hypercube.org)                            *
*       Brian Moore(zump@rom.org)                                          *
*   By using this code, you have agreed to follow the terms of the         *
*   ROM license, in the file Rom24/doc/rom.license                         *
***************************************************************************/

/***************************************************************************
*	includes
***************************************************************************/
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#if defined(WIN32)
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#endif

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
		valid = FALSE;
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
		valid = TRUE;
		break;
	}

	return valid;
}
