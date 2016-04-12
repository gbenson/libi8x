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

struct i8x_note
{
  I8X_OBJECT_FIELDS;

  char *srcname;	/* Name of the file this note came from.  */
  ssize_t srcoffset;	/* Offset in the file this note came from.  */

  size_t encoded_size;	/* Size of encoded data, in bytes.  */
  char *encoded;	/* Encoded data.  */

  struct i8x_list *chunks;  /* Linked list of chunks.  */

  size_t strings_size;	/* Size of string table, in bytes.  */
  const char *strings;	/* String table.  */
};

static i8x_err_e
i8x_note_locate_chunks (struct i8x_note *note)
{
  struct i8x_readbuf *rb;
  i8x_err_e err;

  err = i8x_list_new (i8x_note_get_ctx (note), true, &note->chunks);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_new_from_note (note, &rb);
  if (err != I8X_OK)
    return err;

  while (i8x_rb_bytes_left (rb) > 0)
    {
      struct i8x_chunk *chunk;

      err = i8x_chunk_new_from_readbuf (rb, &chunk);
      if (err != I8X_OK)
	break;

      /* Note consumers MUST treat chunks of zero size
	 as if they were not present.  */
      if (i8x_chunk_get_encoded_size (chunk) == 0)
	{
	  i8x_chunk_unref (chunk);
	  continue;
	}

      err = i8x_list_append_chunk (note->chunks, chunk);
      chunk = i8x_chunk_unref (chunk);
      if (err != I8X_OK)
	break;
    }

  rb = i8x_rb_unref (rb);

  return err;
}

static i8x_err_e
i8x_note_init (struct i8x_note *note, const char *buf, size_t bufsiz,
	       const char *srcname, ssize_t srcoffset)
{
  i8x_err_e err;

  if (srcname != NULL)
    {
      note->srcname = strdup (srcname);

      if (note->srcname == NULL)
	return i8x_out_of_memory (i8x_note_get_ctx (note));
    }

  note->srcoffset = srcoffset;

  note->encoded_size = bufsiz;
  note->encoded = malloc (bufsiz);
  if (note->encoded == NULL)
    return i8x_out_of_memory (i8x_note_get_ctx (note));

  memcpy (note->encoded, buf, bufsiz);

  err = i8x_note_locate_chunks (note);
  if (err != I8X_OK)
    return err;

  return err;
}

static void
i8x_note_unlink (struct i8x_object *ob)
{
  struct i8x_note *note = (struct i8x_note *) ob;

  note->chunks = i8x_list_unref (note->chunks);
}

static void
i8x_note_free (struct i8x_object *ob)
{
  struct i8x_note *note = (struct i8x_note *) ob;

  if (note->srcname != NULL)
    free (note->srcname);

  if (note->encoded != NULL)
    free (note->encoded);
}

const struct i8x_object_ops i8x_note_ops =
  {
    "note",			/* Object name.  */
    sizeof (struct i8x_note),	/* Object size.  */
    i8x_note_unlink,		/* Unlink function.  */
    i8x_note_free,		/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_note_new_from_buf (struct i8x_ctx *ctx, const char *buf,
		       size_t bufsiz, const char *srcname,
		       ssize_t srcoffset, struct i8x_note **note)
{
  struct i8x_note *n;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_note_ops, &n);
  if (err != I8X_OK)
    return err;

  err = i8x_note_init (n, buf, bufsiz, srcname, srcoffset);
  if (err != I8X_OK)
    {
      n = i8x_note_unref (n);

      return err;
    }

  *note = n;

  return I8X_OK;
}

I8X_EXPORT const char *
i8x_note_get_src_name (struct i8x_note *note)
{
  return note->srcname;
}

I8X_EXPORT ssize_t
i8x_note_get_src_offset (struct i8x_note *note)
{
  return note->srcoffset;
}

I8X_EXPORT size_t
i8x_note_get_encoded_size (struct i8x_note *note)
{
  return note->encoded_size;
}

I8X_EXPORT const char *
i8x_note_get_encoded (struct i8x_note *note)
{
  return note->encoded;
}

I8X_EXPORT struct i8x_list *
i8x_note_get_chunks (struct i8x_note *note)
{
  return note->chunks;
}

I8X_EXPORT i8x_err_e
i8x_note_get_unique_chunk (struct i8x_note *note, uintmax_t type_id,
			   bool must_exist, struct i8x_chunk **chunkp)
{
  struct i8x_chunk *found = NULL;
  struct i8x_listitem *li;

  i8x_list_foreach (note->chunks, li)
    {
      struct i8x_chunk *chunk = i8x_listitem_get_chunk (li);

      if (i8x_chunk_get_type_id (chunk) != type_id)
	continue;

      if (found != NULL)
	return i8x_chunk_unhandled_error (chunk);

      found = chunk;
    }

  if (found == NULL && must_exist)
    return i8x_note_error (note, I8X_NOTE_UNHANDLED, NULL);

  *chunkp = found;

  return I8X_OK;
}

static i8x_err_e
i8x_note_locate_strings (struct i8x_note *note)
{
  struct i8x_chunk *chunk;
  const char *strings;
  i8x_err_e err;

  err = i8x_note_get_unique_chunk (note, I8_CHUNK_STRINGS,
				   true, &chunk);
  if (err != I8X_OK)
    return err;

  if (i8x_chunk_get_version (chunk) != 1)
    return i8x_chunk_version_error (chunk);

  note->strings_size = i8x_chunk_get_encoded_size (chunk);
  strings = i8x_chunk_get_encoded (chunk);

  if (strings[note->strings_size - 1] != '\0')
    return i8x_note_error (note, I8X_NOTE_CORRUPT, strings);

  note->strings = strings;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_rb_read_offset_string (struct i8x_readbuf *rb, const char **result)
{
  struct i8x_note *note = i8x_rb_get_note (rb);
  const char *offset_ptr;
  size_t offset;
  i8x_err_e err;

  offset_ptr = i8x_rb_get_ptr (rb);

  err = i8x_rb_read_uleb128 (rb, &offset);
  if (err != I8X_OK)
    return err;

  if (note->strings == NULL)
    {
      err = i8x_note_locate_strings (note);
      if (err != I8X_OK)
	return err;
    }

  if (offset >= note->strings_size)
    return i8x_note_error (note, I8X_NOTE_CORRUPT, offset_ptr);

  *result = note->strings + offset;

  return I8X_OK;
}
