/* Copyright (C) 2016-17 Red Hat, Inc.
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
#include "funcref-private.h"

static i8x_err_e
i8x_funcref_init (struct i8x_funcref *ref, const char *signature,
		  struct i8x_type *type)
{
  ref->signature = strdup (signature);
  if (ref->signature == NULL)
    return i8x_out_of_memory (i8x_funcref_get_ctx (ref));

  if (strstr (signature, "::__") != NULL)
    ref->is_private = true;

  ref->type = i8x_type_ref (type);

  ref->num_args = i8x_list_size (i8x_type_get_ptypes (type));
  ref->num_rets = i8x_list_size (i8x_type_get_rtypes (type));

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

  if (ref->signature != NULL)
    free (ref->signature);
}

static const struct i8x_object_ops i8x_funcref_ops =
  {
    "funcref",				/* Object name.  */
    sizeof (struct i8x_funcref),	/* Object size.  */
    i8x_funcref_unlink,			/* Unlink function.  */
    i8x_funcref_free,			/* Free function.  */
  };

i8x_err_e
i8x_funcref_new (struct i8x_ctx *ctx, const char *signature,
		 struct i8x_type *type, struct i8x_funcref **ref)
{
  struct i8x_funcref *r;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_funcref_ops, &r);
  if (err != I8X_OK)
    return err;

  err = i8x_funcref_init (r, signature, type);
  if (err != I8X_OK)
    {
      r = i8x_funcref_unref (r);

      return err;
    }

  dbg (ctx, "funcref %p is %s\n", r, signature);

  *ref = r;

  return I8X_OK;
}

I8X_EXPORT const char *
i8x_funcref_get_signature (struct i8x_funcref *ref)
{
  return ref->signature;
}

I8X_EXPORT bool
i8x_funcref_is_private (struct i8x_funcref *ref)
{
  return ref->is_private;
}

I8X_EXPORT size_t
i8x_funcref_get_num_params (struct i8x_funcref *ref)
{
  return ref->num_args;
}

I8X_EXPORT size_t
i8x_funcref_get_num_returns (struct i8x_funcref *ref)
{
  return ref->num_rets;
}

void
i8x_func_register (struct i8x_func *func)
{
  struct i8x_funcref *ref = i8x_func_get_funcref (func);

  ref->regcount++;

  if (ref->regcount != 1)
    func = NULL;

  ref->unique = func;
}

void
i8x_func_unregister (struct i8x_func *func)
{
  struct i8x_funcref *ref = i8x_func_get_funcref (func);

  ref->regcount--;

  if (ref->regcount == 1)
    {
      struct i8x_ctx *ctx = i8x_funcref_get_ctx (ref);
      struct i8x_listitem *li;

      func = NULL;
      i8x_list_foreach (i8x_ctx_get_functions (ctx), li)
	{
	  func = i8x_listitem_get_func (li);

	  if (i8x_func_get_funcref (func) == ref)
	    break;

	  func = NULL;
	}
    }
  else
    func = NULL;

  ref->unique = func;
}

void
i8x_funcref_reset_is_resolved (struct i8x_funcref *ref)
{
  struct i8x_func *resolved = ref->unique;
  struct i8x_code *interp_impl = NULL;
  i8x_nat_fn_t *native_impl = NULL;

  if (resolved != NULL)
    {
      interp_impl = i8x_func_get_interp_impl (resolved);
      native_impl = i8x_func_get_native_impl (resolved);

      i8x_assert ((interp_impl != NULL) ^ (native_impl != NULL));
    }

  ref->resolved = resolved;
  ref->interp_impl = interp_impl;
  ref->native_impl = native_impl;

}

void
i8x_funcref_mark_unresolved (struct i8x_funcref *ref)
{
  ref->resolved = NULL;
  ref->interp_impl = NULL;
  ref->native_impl = NULL;
}

I8X_EXPORT bool
i8x_funcref_is_resolved (struct i8x_funcref *ref)
{
  return ref->resolved != NULL;
}

I8X_EXPORT struct i8x_type *
i8x_funcref_get_type (struct i8x_funcref *ref)
{
  return ref->type;
}
