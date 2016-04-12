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

struct i8x_list
{
  I8X_OBJECT_FIELDS;

  /* The head of the list.  */
  struct i8x_listitem *head;

  /* True if listitems should own references to their objects.  */
  bool manage_references;
};

struct i8x_listitem
{
  I8X_OBJECT_FIELDS;

  struct i8x_listitem *next, *prev;

  struct i8x_object *ob;
};

I8X_COMMON_OBJECT_FUNCTIONS (listitem);

static struct i8x_list *
i8x_listitem_get_list (struct i8x_listitem *li)
{
  return (struct i8x_list *)
    i8x_ob_get_parent ((struct i8x_object *) li);

}

static void
i8x_list_unlink (struct i8x_object *ob)
{
  struct i8x_list *list = (struct i8x_list *) ob;

  list->head = i8x_listitem_unref (list->head);
}

static void
i8x_listitem_unlink (struct i8x_object *ob)
{
  struct i8x_listitem *li = (struct i8x_listitem *) ob;
  struct i8x_list *list = i8x_listitem_get_list (li);

  if (list->manage_references)
    li->ob = i8x_ob_unref (li->ob);

  if (li->next != list->head)
    li->next = i8x_listitem_unref (li->next);
}

const struct i8x_object_ops i8x_list_ops =
  {
    "list",				/* Object name.  */
    sizeof (struct i8x_list),		/* Object size.  */
    i8x_list_unlink,			/* Unlink function.  */
    NULL,				/* Free function.  */
  };

const struct i8x_object_ops i8x_listitem_ops =
  {
    "listitem",				/* Object name.  */
    sizeof (struct i8x_listitem),	/* Object size.  */
    i8x_listitem_unlink,		/* Unlink function.  */
    NULL,				/* Free function.  */
  };

static i8x_err_e
i8x_listitem_new (struct i8x_list *list,
		  struct i8x_object *ob,
		  struct i8x_listitem **li)
{
  struct i8x_listitem *l;
  i8x_err_e err;

  err = i8x_ob_new (list, &i8x_listitem_ops, &l);
  if (err != I8X_OK)
    return err;

  if (list->manage_references)
    ob = i8x_ob_ref (ob);

  l->ob = ob;

  *li = l;

  return I8X_OK;
}

i8x_err_e
i8x_list_new (struct i8x_ctx *ctx, bool manage_references,
	      struct i8x_list **list)
{
  struct i8x_list *l;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_list_ops, &l);
  if (err != I8X_OK)
    return err;

  l->manage_references = manage_references;

  err = i8x_listitem_new (l, NULL, &l->head);
  if (err != I8X_OK)
    {
      l = i8x_list_unref (l);

      return err;
    }

  l->head->next = l->head->prev = l->head;

  *list = l;

  return I8X_OK;
}

i8x_err_e
i8x_list_append (struct i8x_list *list, struct i8x_object *ob)
{
  struct i8x_listitem *head = list->head;
  struct i8x_listitem *last = head->prev;
  struct i8x_listitem *item;
  i8x_err_e err;

  err = i8x_listitem_new (list, ob, &item);
  if (err != I8X_OK)
    return err;

  item->prev = last;
  item->next = list->head;
  last->next = item;
  head->prev = item;

  return I8X_OK;
}

static struct i8x_listitem *
i8x_list_get_listitem (struct i8x_list *list, struct i8x_object *ob)
{
  struct i8x_listitem *li;

  i8x_list_foreach (list, li)
    if (li->ob == ob)
      return li;

  return NULL;
}

void
i8x_list_remove (struct i8x_list *list, struct i8x_object *ob)
{
  struct i8x_listitem *item = i8x_list_get_listitem (list, ob);

  item->prev->next = item->next;
  item->next->prev = item->prev;

  item->next = NULL;
  item = i8x_listitem_unref (item);
}

I8X_EXPORT struct i8x_listitem *
i8x_list_get_first (struct i8x_list *list)
{
  if (list == NULL)
    return NULL;

  return i8x_list_get_next (list, list->head);
}

I8X_EXPORT struct i8x_listitem *
i8x_list_get_next (struct i8x_list *list, struct i8x_listitem *li)
{
  li = li->next;
  if (li == list->head)
    return NULL;

  return li;
}

I8X_EXPORT struct i8x_object *
i8x_listitem_get_object (struct i8x_listitem *li)
{
  return li->ob;
}
