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

#include <stdio.h>
#include <string.h>

#include <i8x/libi8x.h>
#include "libi8x-private.h"

struct i8x_funcsig
{
  I8X_OBJECT_FIELDS;

  char *provider;
  char *name;
  char *encoded_ptypes; /* XXX store decoded? */
  char *encoded_rtypes;

  char *fullname;
};

static i8x_err_e
i8x_fs_init (struct i8x_funcsig *fs, const char *provider,
	     const char *name, const char *ptypes, const char *rtypes)
{
  int fullname_size;

  fs->provider = strdup (provider);
  if (fs->provider == NULL)
    return i8x_out_of_memory (i8x_fs_get_ctx (fs));

  fs->name = strdup (name);
  if (fs->name == NULL)
    return i8x_out_of_memory (i8x_fs_get_ctx (fs));

  fs->encoded_ptypes = strdup (ptypes);
  if (fs->encoded_ptypes == NULL)
    return i8x_out_of_memory (i8x_fs_get_ctx (fs));

  fs->encoded_rtypes = strdup (rtypes);
  if (fs->encoded_rtypes == NULL)
    return i8x_out_of_memory (i8x_fs_get_ctx (fs));

  fullname_size = (strlen (provider)
		   + 2   /* "::"  */
		   + strlen (name)
		   + 1   /* "("  */
		   + strlen (ptypes)
		   + 1   /* ")"  */
		   + strlen (rtypes)
		   + 1); /* '\0'  */

  fs->fullname = malloc (fullname_size);
  if (fs->fullname == NULL)
    return i8x_out_of_memory (i8x_fs_get_ctx (fs));
  snprintf (fs->fullname, fullname_size,
	    "%s::%s(%s)%s", provider, name, ptypes, rtypes);

  return I8X_OK;
}

static void
i8x_fs_free (struct i8x_object *ob)
{
  struct i8x_funcsig *fs = (struct i8x_funcsig *) ob;

  if (fs->provider != NULL)
    free (fs->provider);

  if (fs->name != NULL)
    free (fs->name);

  if (fs->encoded_ptypes != NULL)
    free (fs->encoded_ptypes);

  if (fs->encoded_rtypes != NULL)
    free (fs->encoded_rtypes);

  if (fs->fullname != NULL)
    free (fs->fullname);
}

const struct i8x_object_ops i8x_fs_ops =
  {
    "funcsig",				/* Object name.  */
    sizeof (struct i8x_funcsig),	/* Object size.  */
    NULL,				/* Unlink function.  */
    i8x_fs_free,			/* Free function.  */
  };

I8X_EXPORT i8x_err_e
i8x_fs_new (struct i8x_ctx *ctx, const char *provider,
	    const char *name, const char *ptypes,
	    const char *rtypes, struct i8x_funcsig **fs)
{
  struct i8x_funcsig *f;
  i8x_err_e err;

  err = i8x_ob_new (ctx, &i8x_fs_ops, &f);
  if (err != I8X_OK)
    return err;

  err = i8x_fs_init (f, provider, name, ptypes, rtypes);
  if (err != I8X_OK)
    {
      i8x_fs_unref (f);

      return err;
    }

  *fs = f;

  return I8X_OK;
}

i8x_err_e
i8x_fs_new_from_readbuf (struct i8x_readbuf *rb,
			 struct i8x_funcsig **fs)
{
  const char *provider, *name, *ptypes, *rtypes;
  i8x_err_e err;

  err = i8x_rb_read_offset_string (rb, &provider);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &name);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &ptypes);
  if (err != I8X_OK)
    return err;

  err = i8x_rb_read_offset_string (rb, &rtypes);
  if (err != I8X_OK)
    return err;

  err = i8x_fs_new (i8x_rb_get_ctx (rb), provider,
		    name, ptypes, rtypes, fs);

  return err;
}

I8X_EXPORT const char *
i8x_fs_get_fullname (struct i8x_funcsig *fs)
{
  return fs->fullname;
}
