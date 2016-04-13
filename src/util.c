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
#include "libi8x-private.h"

void
i8x_internal_error (const char *file, int line,
		    const char *function, const char *format, ...)
{
  va_list args;

  fprintf (stderr, "libi8x: %s:%d: %s: internal error: ",
	   file, line, function);

  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);

  fputc ('\n', stderr);

  abort ();
}

i8x_err_e
i8x_note_error (struct i8x_note *note, i8x_err_e code, const char *ptr)
{
  return i8x_ctx_set_error (i8x_note_get_ctx (note), code, note, ptr);
}

i8x_err_e
i8x_rb_error (struct i8x_readbuf *rb, i8x_err_e code, const char *ptr)
{
  return i8x_note_error (i8x_rb_get_note (rb), code, ptr);
}
