/* Copyright (C) 2016 Red Hat, Inc.
   This file is part of the Infinity Note Execution Library.

   The Infinity Note Execution Library is free software; you can
   redistribute it and/or modify it under the terms of the GNU Lesser
   General Public License as published by the Free Software
   Foundation; either version 2.1 of the License, or (at your option)
   any later version.

   The Infinity Note Execution Library is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the GNU Lesser General Public License for more
   details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Infinity Note Execution Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <string.h>
#include "libi8x-private.h"

struct i8x_type
{
  I8X_OBJECT_FIELDS;

  char *encoded;	/* The type's encoded form.  */
};

static bool
i8x_type_is_functype (struct i8x_type *type)
{
  return type->encoded != NULL && type->encoded[0] == I8_TYPE_FUNCTION;
}

static i8x_err_e
i8x_type_init (struct i8x_type *type, const char *encoded,
	       const char *ptypes_start, const char *ptypes_limit,
	       const char *rtypes_start, const char *rtypes_limit,
	       struct i8x_note *src_note)
{
  type->encoded = strdup (encoded);
  if (type->encoded == NULL)
    return i8x_out_of_memory (i8x_type_get_ctx (type));

  if (!i8x_type_is_functype (type))
    return I8X_OK;

  return I8X_OK;
}

static void
i8x_type_unlink (struct i8x_object *ob)
{
  struct i8x_type *type = (struct i8x_type *) ob;

  if (i8x_type_is_functype (type))
    i8x_ctx_forget_functype (type);
}

static void
i8x_type_free (struct i8x_object *ob)
{
  struct i8x_type *type = (struct i8x_type *) ob;

  if (type->encoded != NULL)
    free (type->encoded);
}

const struct i8x_object_ops i8x_type_ops =
  {
    "type",			/* Object name.  */
    sizeof (struct i8x_type),	/* Object size.  */
    i8x_type_unlink,		/* Unlink function.  */
    i8x_type_free,		/* Free function.  */
  };

static i8x_err_e
i8x_type_new (struct i8x_ctx *ctx, const char *encoded,
	      const char *ptypes_start, const char *ptypes_limit,
	      const char *rtypes_start, const char *rtypes_limit,
	      struct i8x_note *src_note, struct i8x_type **type)
{
  struct i8x_type *t;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_type_ops, &t);
  if (err != I8X_OK)
    return err;

  err = i8x_type_init (t, encoded, ptypes_start, ptypes_limit,
		       rtypes_start, rtypes_limit, src_note);
  if (err != I8X_OK)
    {
      t = i8x_type_unref (t);

      return err;
    }

  dbg (ctx, "type %p is %s\n", t, encoded);

  *type = t;

  return I8X_OK;
}

i8x_err_e
i8x_type_new_coretype (struct i8x_ctx *ctx, char encoded_char,
		       struct i8x_type **type)
{
  char encoded[2] = {encoded_char, '\0'};

  return i8x_type_new (ctx, encoded, NULL, NULL, NULL, NULL, NULL, type);
}

i8x_err_e
i8x_type_new_functype (struct i8x_ctx *ctx, const char *encoded,
		       const char *ptypes_start,
		       const char *ptypes_limit,
		       const char *rtypes_start,
		       const char *rtypes_limit,
		       struct i8x_note *src_note,
		       struct i8x_type **type)
{
  return i8x_type_new (ctx, encoded, ptypes_start, ptypes_limit,
		       rtypes_start, rtypes_limit, src_note, type);
}

const char *
i8x_type_get_encoded (struct i8x_type *type)
{
  return type->encoded;
}
