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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_note
{
  struct i8x_ctx *ctx;
  int refcount;

  const char *srcname; /* Name of the file this note came from.  */
  ssize_t srcoffset;   /* Offset in the file this note came from.  */

  size_t encoded_size; /* Size of encoded data, in bytes.  */
  char encoded[0];     /* Encoded data.  Must be last in the struct.  */
};

I8X_EXPORT struct i8x_note *
i8x_note_ref (struct i8x_note *note)
{
  if (note == NULL)
    return NULL;

  note->refcount++;

  return note;
}

I8X_EXPORT struct i8x_note *
i8x_note_unref (struct i8x_note *note)
{
  if (note == NULL)
    return NULL;

  note->refcount--;
  if (note->refcount > 0)
    return NULL;

  info (note->ctx, "note %p released\n", note);
  i8x_unref (note->ctx);
  free (note);

  return NULL;
}

I8X_EXPORT struct i8x_ctx *
i8x_note_get_ctx (struct i8x_note *note)
{
  return note->ctx;
}

I8X_EXPORT i8x_err_e
i8x_note_new_from_mem (struct i8x_ctx *ctx, const char *buf,
		       size_t bufsiz, const char *srcname,
		       ssize_t srcoffset, struct i8x_note **note)
{
  struct i8x_note *n;
  size_t srcnamesiz = 0;

  if (srcname != NULL && *srcname != '\0')
    srcnamesiz = strlen (srcname) + 1;

  /* Allocate all the memory at once.  */
  n = malloc (sizeof (struct i8x_note) + bufsiz + srcnamesiz);
  if (n == NULL)
    return i8x_set_error (ctx, I8X_OUT_OF_MEMORY, NULL, -1);
  memset (n, 0, sizeof (struct i8x_note));

  n->ctx = i8x_ref (ctx);
  n->refcount = 1;

  n->encoded_size = bufsiz;
  memcpy (n->encoded, buf, bufsiz);

  if (srcnamesiz != 0)
    {
      memcpy (n->encoded + bufsiz, srcname, srcnamesiz);
      n->srcname = n->encoded + bufsiz;
    }
  n->srcoffset = srcoffset;

  info (ctx, "note %p created\n", n);
  *note = n;

  return I8X_OK;
}
