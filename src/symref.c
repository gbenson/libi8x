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
#include "symref-private.h"

void
i8x_symref_invalidate_for_inferior (struct i8x_symref *ref,
				    struct i8x_inferior *inf)
{
  if (ref->cached_from != inf)
    return;

  /* Set cached_from to something that definitely isn't an
     inferior, and poison the cached value too while we're
     at it.  */
  ref->cached_from = (struct i8x_inferior *) ref;
  ref->cached_value = I8X_POISON_BAD_CACHED_SYMREF;

  dbg (i8x_symref_get_ctx (ref),
       "invalidated symref %p value for inferior %p\n", ref, inf);
}

static i8x_err_e
i8x_symref_init (struct i8x_symref *ref, const char *name)
{
  ref->name = strdup (name);
  if (ref->name == NULL)
    return i8x_out_of_memory (i8x_symref_get_ctx (ref));

  i8x_symref_invalidate_for_inferior (ref, ref->cached_from);
  i8x_assert (ref->cached_from == (struct i8x_inferior *) ref);

  return I8X_OK;
}

static void
i8x_symref_unlink (struct i8x_object *ob)
{
  struct i8x_symref *ref = (struct i8x_symref *) ob;

  i8x_ctx_forget_symref (ref);
}

static void
i8x_symref_free (struct i8x_object *ob)
{
  struct i8x_symref *ref = (struct i8x_symref *) ob;

  if (ref->name != NULL)
    free (ref->name);
}

const struct i8x_object_ops i8x_symref_ops =
  {
    "symref",			/* Object name.  */
    sizeof (struct i8x_symref),	/* Object size.  */
    i8x_symref_unlink,		/* Unlink function.  */
    i8x_symref_free,		/* Free function.  */
  };

i8x_err_e
i8x_symref_new (struct i8x_ctx *ctx, const char *name,
		struct i8x_symref **ref)
{
  struct i8x_symref *s;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_symref_ops, &s);
  if (err != I8X_OK)
    return err;

  err = i8x_symref_init (s, name);
  if (err != I8X_OK)
    {
      s = i8x_symref_unref (s);

      return err;
    }

  dbg (ctx, "symref %p is %s\n", s, name);

  *ref = s;

  return I8X_OK;
}

struct i8x_symref *
i8x_object_as_symref (struct i8x_object *ob)
{
  return (struct i8x_symref *) i8x_ob_cast (ob, &i8x_symref_ops);
}

const char *
i8x_symref_get_name (struct i8x_symref *ref)
{
  return ref->name;
}
