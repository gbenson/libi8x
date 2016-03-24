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

#ifndef _LIBI8X_H_
#define _LIBI8X_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* error codes */

typedef enum
{
  I8X_OK = 0,
  I8X_OUT_OF_MEMORY = -1,
  I8X_NOTE_CORRUPT = -2,
  I8X_NOTE_UNHANDLED = -3,
  I8X_NOTE_INVALID = -4,
}
i8x_err_e;

/* forward declarations */

struct i8x_ctx;
struct i8x_note;

/*
 * i8x_ctx
 *
 * library user context - reads the config and system
 * environment, user variables, allows custom logging
 */
typedef void i8x_log_fn_t (struct i8x_ctx *ctx,
			   int priority,
			   const char *file, int line,
			   const char *fn,
			   const char *format, va_list args);
struct i8x_ctx *i8x_ref (struct i8x_ctx *ctx);
struct i8x_ctx *i8x_unref (struct i8x_ctx *ctx);
i8x_err_e i8x_new (struct i8x_ctx **ctx);
void i8x_set_log_fn (struct i8x_ctx *ctx, i8x_log_fn_t *log_fn);
int i8x_get_log_priority (struct i8x_ctx *ctx);
void i8x_set_log_priority (struct i8x_ctx *ctx, int priority);
void *i8x_get_userdata (struct i8x_ctx *ctx);
void i8x_set_userdata (struct i8x_ctx *ctx, void *userdata);
const char *i8x_strerror_r (struct i8x_ctx *ctx, i8x_err_e code,
			    char *buf, size_t bufsiz);

/*
 * i8x_note
 *
 * access to notes of i8x
 */
struct i8x_note *i8x_note_ref (struct i8x_note *note);
struct i8x_note *i8x_note_unref (struct i8x_note *note);
struct i8x_ctx *i8x_note_get_ctx (struct i8x_note *note);
i8x_err_e i8x_note_new_from_mem (struct i8x_ctx *ctx,
				 const char *buf, size_t bufsiz,
				 const char *srcname, ssize_t srcoffset,
				 struct i8x_note **note);
const char *i8x_note_get_src_name (struct i8x_note *note);
ssize_t i8x_note_get_src_offset (struct i8x_note *note);
const char *i8x_note_get_encoded (struct i8x_note *note);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_H_ */
