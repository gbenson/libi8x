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

  /* Lists of parameter and return types, for function types.  */
  struct i8x_list *ptypes, *rtypes;
};

static bool
i8x_type_is_functype (struct i8x_type *type)
{
  return type->encoded != NULL && type->encoded[0] == I8_TYPE_FUNCTION;
}

static i8x_err_e
i8x_tld_error (struct i8x_ctx *ctx, i8x_err_e code,
	       struct i8x_note *cause_note, const char *cause_ptr)
{
  if (cause_note == NULL)
    return i8x_invalid_argument (ctx);
  else
    return i8x_note_error (cause_note, code, cause_ptr);
}

static i8x_err_e
i8x_type_list_skip_to (struct i8x_ctx *ctx, const char *ptr,
		       const char *limit, struct i8x_note *src_note,
		       char stop_char, const char **stop_char_ptr);

static i8x_err_e
i8x_functype_get_bounds (struct i8x_ctx *ctx, const char *ptr,
			 const char *limit, struct i8x_note *src_note,
			 const char **ptypes_start,
			 const char **ptypes_limit,
			 const char **rtypes_start,
			 const char **rtypes_limit)
{
  i8x_err_e err;

  /* Return types start straight after the 'F'.  */
  *rtypes_start = ++ptr;
  err = i8x_type_list_skip_to (ctx, ptr, limit, src_note,
				 '(', rtypes_limit);
  if (err != I8X_OK)
    return err;

  /* Parameter types start straight after the '('.  */
  *ptypes_start = ptr = *rtypes_limit + 1;
  err = i8x_type_list_skip_to (ctx, ptr, limit, src_note,
				 ')', ptypes_limit);
  if (err != I8X_OK)
    return err;

  return I8X_OK;
}

static i8x_err_e
i8x_tld_helper (struct i8x_ctx *ctx,
		struct i8x_note *src_note,
		const char *ptr, const char *limit,
		struct i8x_list *result,
		char stop_char, const char **stop_char_ptr)
{
  i8x_assert ((result == NULL)
	      ^ (stop_char == 0 && stop_char_ptr == NULL));

  while (ptr < limit)
    {
      struct i8x_type *type;
      char c = *ptr;

      switch (c)
	{
	case I8_TYPE_INTEGER:
	  if (result != NULL)
	    type = i8x_ctx_get_integer_type (ctx);

	  ptr++;
	  break;

	case I8_TYPE_POINTER:
	  if (result != NULL)
	    type = i8x_ctx_get_pointer_type (ctx);

	  ptr++;
	  break;

	case I8_TYPE_OPAQUE:
	  if (result != NULL)
	    type = i8x_ctx_get_opaque_type (ctx);

	  ptr++;
	  break;

	case I8_TYPE_FUNCTION:
	  {
	    const char *ptypes_start, *ptypes_limit;
	    const char *rtypes_start, *rtypes_limit;
	    i8x_err_e err;

	    err = i8x_functype_get_bounds (ctx, ptr, limit,
					   src_note, &ptypes_start,
					   &ptypes_limit, &rtypes_start,
					   &rtypes_limit);
	    if (err != I8X_OK)
	      return err;

	    if (result != NULL)
	      {
		err = i8x_ctx_get_functype (ctx, ptypes_start,
					    ptypes_limit, rtypes_start,
					    rtypes_limit, src_note,
					    &type);
		if (err != I8X_OK)
		  return err;
	      }

	    ptr = ptypes_limit + 1;
	  }
	  break;

	case '(':
	case ')':
	  if (*ptr != stop_char)
	    return i8x_tld_error (ctx, I8X_NOTE_CORRUPT, src_note, ptr);

	  *stop_char_ptr = ptr;
	  return I8X_OK;

	default:
	  return i8x_tld_error (ctx, I8X_NOTE_UNHANDLED, src_note, ptr);
	}

      if (result != NULL)
	{
	  i8x_err_e err;

	  err = i8x_list_append_type (result, type);
	  i8x_type_unref (type);
	  if (err != I8X_OK)
	    return err;
	}
    }

  if (stop_char != 0)
    return i8x_tld_error (ctx, I8X_NOTE_CORRUPT, src_note, ptr);

  return I8X_OK;
}

static i8x_err_e
i8x_type_list_skip_to (struct i8x_ctx *ctx, const char *ptr,
		       const char *limit, struct i8x_note *src_note,
		       char stop_char, const char **stop_char_ptr)
{
  return i8x_tld_helper (ctx, src_note, ptr, limit,
			 NULL, stop_char, stop_char_ptr);
}

static i8x_err_e
i8x_type_list_decode (struct i8x_ctx *ctx, struct i8x_note *src_note,
		      const char *ptr, const char *limit,
		      struct i8x_list *result)
{
  return i8x_tld_helper (ctx, src_note, ptr, limit,
			 result, 0, NULL);
}

static i8x_err_e
i8x_type_init (struct i8x_type *type, const char *encoded,
	       const char *ptypes_start, const char *ptypes_limit,
	       const char *rtypes_start, const char *rtypes_limit,
	       struct i8x_note *src_note)
{
  struct i8x_ctx *ctx = i8x_type_get_ctx (type);

  if (encoded[0] == I8_TYPE_FUNCTION)
    {
      i8x_err_e err;

      err = i8x_list_new (ctx, true, &type->ptypes);
      if (err != I8X_OK)
	return err;

      err = i8x_type_list_decode (ctx, src_note,
				  ptypes_start, ptypes_limit,
				  type->ptypes);
      if (err != I8X_OK)
	return err;

      err = i8x_list_new (ctx, true, &type->rtypes);
      if (err != I8X_OK)
	return err;

      err = i8x_type_list_decode (ctx, src_note,
				  rtypes_start, rtypes_limit,
				  type->rtypes);
      if (err != I8X_OK)
	return err;
    }

  /* Do this last so i8x_type_is_functype returns false while
     parameter and return types lists are being decoded.  This
     stops i8x_type_unlink calling i8x_ctx_forget_functype if
     i8x_type_list_decode encounters an error.  */
  type->encoded = strdup (encoded);
  if (type->encoded == NULL)
    return i8x_out_of_memory (ctx);

  return I8X_OK;
}

static void
i8x_type_unlink (struct i8x_object *ob)
{
  struct i8x_type *type = (struct i8x_type *) ob;

  if (i8x_type_is_functype (type))
    i8x_ctx_forget_functype (type);

  type->ptypes = i8x_list_unref (type->ptypes);
  type->rtypes = i8x_list_unref (type->rtypes);
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
