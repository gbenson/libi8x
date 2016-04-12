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

#include "libi8x-private.h"

static struct i8x_object *i8x_ob_unref_parent (struct i8x_object *ob);

enum ref_sense_e
{
  RS_FORWARD,
  RS_BACK
};

static struct i8x_object *
i8x_ob_ref_1 (struct i8x_object *ob, enum ref_sense_e sense)
{
  if (ob == NULL)
    return NULL;

  ob->refcount[sense]++;

  return ob;
}

static struct i8x_object *
i8x_ob_unref_1 (struct i8x_object *ob, enum ref_sense_e sense)
{
  struct i8x_ctx *ctx;

  if (ob == NULL)
    return NULL;

  ob->refcount[sense]--;
  if (ob->refcount[RS_FORWARD] > 0 || ob->is_moribund)
    return NULL;

  ob->is_moribund = true;

  if (ob->ops->unlink_fn != NULL)
    ob->ops->unlink_fn (ob);

  ctx = i8x_ctx_ref (i8x_ob_get_ctx (ob));
  ob->parent = i8x_ob_unref_parent (ob->parent);

  if (ob->refcount[RS_BACK] > 0)
    warn (ctx, "%s %p released with references\n", ob->ops->name, ob);
  else
    info (ctx, "%s %p released\n", ob->ops->name, ob);

  ctx = i8x_ctx_unref (ctx);

  if (ob->ops->free_fn != NULL)
    ob->ops->free_fn (ob);
  free (ob);

  return NULL;
}

/**
 * i8x_ob_ref:
 * @ob: i8x object
 *
 * Take a reference to the i8x object.
 *
 * Returns: the passed i8x object
 **/
I8X_EXPORT struct i8x_object *
i8x_ob_ref (struct i8x_object *ob)
{
  return i8x_ob_ref_1 (ob, RS_FORWARD);
}

/**
 * i8x_ob_unref:
 * @ob: i8x object
 *
 * Drop a reference of the i8x object.
 *
 **/
I8X_EXPORT struct i8x_object *
i8x_ob_unref (struct i8x_object *ob)
{
  return i8x_ob_unref_1 (ob, RS_FORWARD);
}

static struct i8x_object *
i8x_ob_ref_parent (struct i8x_object *ob)
{
  return i8x_ob_ref_1 (ob, RS_BACK);
}

static struct i8x_object *
i8x_ob_unref_parent (struct i8x_object *ob)
{
  return i8x_ob_unref_1 (ob, RS_BACK);
}

i8x_err_e
i8x_ob_new (void *pp, const struct i8x_object_ops *ops, void *ob)
{
  struct i8x_object *parent = (struct i8x_object *) pp;
  struct i8x_ctx *ctx = parent == NULL ? NULL : i8x_ob_get_ctx (parent);
  struct i8x_object *o;

  o = calloc (1, ops->size);
  if (o == NULL)
    return i8x_out_of_memory (ctx);

  if (ctx != NULL)
    info (ctx, "%s %p created\n", ops->name, o);

  o->ops = ops;
  o->parent = i8x_ob_ref_parent (parent);

  *(struct i8x_object **) ob = i8x_ob_ref (o);

  return I8X_OK;
}

struct i8x_object *
i8x_ob_get_parent (struct i8x_object *ob)
{
  return ob->parent;
}

struct i8x_object *
i8x_ob_cast (struct i8x_object *ob, const struct i8x_object_ops *ops)
{
  if (ob->ops == ops)
    return ob;
  else
    return NULL;
}

I8X_EXPORT struct i8x_ctx *
i8x_ob_get_ctx (struct i8x_object *ob)
{
  if (ob == NULL)
    return NULL;

  while (ob->parent != NULL)
    ob = ob->parent;

  return (struct i8x_ctx *) ob;
}

/**
 * i8x_ob_get_userdata:
 * @ob: i8x object
 *
 * Retrieve stored data pointer from the object.
 *
 * Returns: stored userdata
 **/
I8X_EXPORT void *
i8x_ob_get_userdata (struct i8x_object *ob)
{
  return ob->userdata;
}

/**
 * i8x_ob_set_userdata:
 * @ob: i8x object
 * @userdata: data pointer
 *
 * Store custom @userdata in the object.
 **/
I8X_EXPORT void
i8x_ob_set_userdata (struct i8x_object *ob, void *userdata,
		     i8x_userdata_cleanup_fn *cleanup)
{
  i8x_assert (userdata != NULL || cleanup == NULL);

  ob->userdata = userdata;
  ob->userdata_cleanup = cleanup;
}
