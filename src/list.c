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

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_list
{
  I8X_LISTITEM_OBJECT_FIELDS;

  bool manage_references;	/* True if the list should reference
				   the items it contains.  */
};

static void
i8x_list_unlink (struct i8x_object *ob)
{
  struct i8x_list *list = (struct i8x_list *) ob;
  struct i8x_listitem *item, *next;

  if (!list->manage_references)
    return;

  for (item = list->li.next;
       item != (struct i8x_listitem *) list;
       item = next)
    {
      next = item->next;
      item = i8x_listitem_unref (item);
    }
}

const struct i8x_object_ops i8x_list_ops =
  {
    "list",				/* Object name.  */
    sizeof (struct i8x_list),		/* Object size.  */
    i8x_list_unlink,			/* Unlink function.  */
    NULL,				/* Free function.  */
  };

i8x_err_e
i8x_list_new (struct i8x_ctx *ctx, bool reference_items,
	      struct i8x_list **list)
{
  struct i8x_list *l;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_list_ops, &l);
  if (err != I8X_OK)
    return err;

  l->li.prev = l->li.next = (struct i8x_listitem *) l;
  l->manage_references = reference_items;

  *list = l;

  return I8X_OK;
}

void
i8x_list_append (struct i8x_list *headp, struct i8x_listitem *item)
{
  struct i8x_listitem *head = (struct i8x_listitem *) headp;
  struct i8x_listitem *last = head->prev;

  /* Ensure this item is not already in a list.  */
  i8x_assert (item->prev == NULL && item->next == NULL);

  if (headp->manage_references)
    item = i8x_listitem_ref (item);

  item->prev = last;
  item->next = head;
  last->next = item;
  head->prev = item;
}

void
i8x_list_remove (struct i8x_list *list, struct i8x_listitem *item)
{
  item->prev->next = item->next;
  item->next->prev = item->prev;

  if (list->manage_references)
    item = i8x_listitem_unref (item);
}
