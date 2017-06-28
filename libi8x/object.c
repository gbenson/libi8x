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

#include "libi8x-private.h"

#define i8x_assert_not_poisoned(ob)					\
  ((void) (ob->is_poisoned ?						\
	   i8x_internal_error (__FILE__, __LINE__, __FUNCTION__,	\
			       _("%s %p is poisoned!"),			\
			       ob->ops->name, ob) : 0))

static void
i8x_ob_poison (struct i8x_object *ob)
{
  const struct i8x_object_ops *saved_ops = ob->ops;
  uint32_t *ptr = (uint32_t *) ob;
  uint32_t *limit = (uint32_t *) (((char *) ptr) + saved_ops->size);

  while (ptr < limit)
    *(ptr)++ = I8X_POISON_RELEASED_OBJECT;

  ob->ops = saved_ops;
  ob->is_poisoned = true;
}

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

  i8x_assert_not_poisoned (ob);

  if (ob->use_debug_allocator)
    {
      /* The "ob->parent == NULL" is required to allow
	 i8x_ob_unref_1 to reference to the dying context.  */
      i8x_assert (ob->parent == NULL || !ob->is_moribund);
      i8x_assert (ob->refcount[sense] >= 0);
    }

  ob->refcount[sense]++;

  return ob;
}

static struct i8x_object *
i8x_ob_unref_1 (struct i8x_object *ob, enum ref_sense_e sense)
{
  struct i8x_ctx *ctx;

  if (ob == NULL)
    return NULL;

  i8x_assert_not_poisoned (ob);
  i8x_assert (!ob->use_debug_allocator || ob->refcount[sense] > 0);

  ob->refcount[sense]--;
  if (ob->refcount[RS_FORWARD] > 0 || ob->is_moribund)
    return NULL;

  ob->is_moribund = true;

  if (ob->ops->unlink_fn != NULL)
    ob->ops->unlink_fn (ob);

  ctx = i8x_ob_get_ctx (ob);
  ob->parent = i8x_ob_unref_parent (ob->parent);

  if (ob->refcount[RS_BACK] == 0)
    dbg (ctx, "%s %p released\n", ob->ops->name, ob);
  else if (!ob->use_debug_allocator)
    warn (ctx, "%s %p released with references\n", ob->ops->name, ob);
  else
    i8x_internal_error (__FILE__, __LINE__, __FUNCTION__,
			"%s %p released with references",
			ob->ops->name, ob);

  if (ob->userdata_cleanup != NULL)
    ob->userdata_cleanup (ob->userdata);

  if (ob->ops->free_fn != NULL)
    ob->ops->free_fn (ob);

  if (ob->use_debug_allocator)
    i8x_ob_poison (ob);
  else
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

  /* i8x_ctx_new handles this message for contexts.  */
  if (ctx != NULL)
    dbg (ctx, "%s %p created\n", ops->name, o);

  o->ops = ops;
  o->use_debug_allocator
    = parent != NULL && parent->use_debug_allocator;
  o->parent = i8x_ob_ref_parent (parent);

  *(struct i8x_object **) ob = i8x_ob_ref (o);

  return I8X_OK;
}

I8X_EXPORT struct i8x_object *
i8x_ob_get_parent (struct i8x_object *ob)
{
  i8x_assert_not_poisoned (ob);

  return ob->parent;
}

I8X_EXPORT struct i8x_ctx *
i8x_ob_get_ctx (struct i8x_object *ob)
{
  if (ob == NULL)
    return NULL;

  i8x_assert_not_poisoned (ob);

  while (ob->parent != NULL)
    {
      ob = ob->parent;
      i8x_assert_not_poisoned (ob);
    }

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
  i8x_assert_not_poisoned (ob);

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
		     i8x_cleanup_fn_t *cleanup)
{
  i8x_assert_not_poisoned (ob);
  i8x_assert (ob->userdata == NULL
	      && ob->userdata_cleanup == NULL);
  i8x_assert (userdata != NULL || cleanup == NULL);

  ob->userdata = userdata;
  ob->userdata_cleanup = cleanup;
}
