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
#include "extern-private.h"

static i8x_err_e
i8x_funcref_init (struct i8x_funcref *ref, const char *fullname,
		  struct i8x_type *type)
{
  ref->fullname = strdup (fullname);
  if (ref->fullname == NULL)
    return i8x_out_of_memory (i8x_funcref_get_ctx (ref));

  if (strstr (fullname, "::__") != NULL)
    ref->is_private = true;

  ref->type = i8x_type_ref (type);

  return I8X_OK;
}

static void
i8x_funcref_unlink (struct i8x_object *ob)
{
  struct i8x_funcref *ref = (struct i8x_funcref *) ob;

  ref->type = i8x_type_unref (ref->type);

  i8x_ctx_forget_funcref (ref);
}

static void
i8x_funcref_free (struct i8x_object *ob)
{
  struct i8x_funcref *ref = (struct i8x_funcref *) ob;

  if (ref->fullname != NULL)
    free (ref->fullname);
}

const struct i8x_object_ops i8x_funcref_ops =
  {
    "funcref",				/* Object name.  */
    sizeof (struct i8x_funcref),	/* Object size.  */
    i8x_funcref_unlink,			/* Unlink function.  */
    i8x_funcref_free,			/* Free function.  */
  };

i8x_err_e
i8x_funcref_new (struct i8x_ctx *ctx, const char *fullname,
		 struct i8x_type *type, struct i8x_funcref **ref)
{
  struct i8x_funcref *r;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_funcref_ops, &r);
  if (err != I8X_OK)
    return err;

  err = i8x_funcref_init (r, fullname, type);
  if (err != I8X_OK)
    {
      r = i8x_funcref_unref (r);

      return err;
    }

  dbg (ctx, "funcref %p is %s\n", r, fullname);

  *ref = r;

  return I8X_OK;
}

struct i8x_funcref *
i8x_object_as_funcref (struct i8x_object *ob)
{
  return (struct i8x_funcref *) i8x_ob_cast (ob, &i8x_funcref_ops);
}

I8X_EXPORT const char *
i8x_funcref_get_fullname (struct i8x_funcref *ref)
{
  return ref->fullname;
}

I8X_EXPORT bool
i8x_funcref_is_private (struct i8x_funcref *ref)
{
  return ref->is_private;
}

void
i8x_funcref_register_func (struct i8x_funcref *ref,
			   struct i8x_func *func)
{
  ref->regcount++;

  if (ref->regcount != 1)
    func = NULL;

  ref->int_resolved = func;
}

void
i8x_funcref_unregister_func (struct i8x_funcref *ref,
			     struct i8x_func *func)
{
  ref->regcount--;

  if (ref->regcount == 1)
    i8x_not_implemented ();
  else
    func = NULL;

  ref->int_resolved = func;
}

void
i8x_funcref_reset_is_resolved (struct i8x_funcref *ref)
{
  ref->ext_resolved = ref->int_resolved;
}

void
i8x_funcref_mark_unresolved (struct i8x_funcref *ref)
{
  ref->ext_resolved = NULL;
}

I8X_EXPORT bool
i8x_funcref_is_resolved (struct i8x_funcref *ref)
{
  return ref->ext_resolved != NULL;
}

struct i8x_type *
i8x_funcref_get_type (struct i8x_funcref *ref)
{
  return ref->type;
}
