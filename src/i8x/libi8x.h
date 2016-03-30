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
#include <stdint.h>
#include <stdlib.h>

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
struct i8x_object;
struct i8x_readbuf;

/*
 * i8x_object
 *
 * XXX don't call these directly, use the inline functions provided by
 * I8X_COMMON_OBJECT_FUNCTIONS et al.
 */
typedef void i8x_userdata_cleanup_fn (void *userdata);

struct i8x_object *i8x_ob_ref (struct i8x_object *ob);
struct i8x_object *i8x_ob_unref (struct i8x_object *ob);
struct i8x_ctx *i8x_ob_get_ctx (struct i8x_object *ob);
void *i8x_ob_get_userdata (struct i8x_object *ob);
void i8x_ob_set_userdata (struct i8x_object *ob,
			  void *userdata,
			  i8x_userdata_cleanup_fn *userdata_cleanup);

#define I8X_NOPARENT_OBJECT_FUNCTIONS(TYPE, PREFIX)			\
  static inline struct i8x_ ## TYPE * __attribute__ ((always_inline))	\
  i8x_ ## PREFIX ## _ref (struct i8x_ ## TYPE *x)			\
  {									\
    return (struct i8x_ ## TYPE *)					\
      i8x_ob_ref ((struct i8x_object *) x);				\
  }									\
									\
  static inline struct i8x_ ## TYPE * __attribute__ ((always_inline))	\
  i8x_ ## PREFIX ## _unref (struct i8x_ ## TYPE *x)			\
  {									\
    return (struct i8x_ ## TYPE *)					\
      i8x_ob_unref ((struct i8x_object *) x);				\
  }									\
									\
  static inline void * __attribute__ ((always_inline))			\
  i8x_ ## PREFIX ## _get_userdata (struct i8x_ ## TYPE *x)		\
  {									\
    return i8x_ob_get_userdata ((struct i8x_object *) x);		\
  }									\
									\
  static inline void __attribute__ ((always_inline))			\
  i8x_ ## PREFIX ## _set_userdata (struct i8x_ ## TYPE *x,		\
				   void *userdata,			\
				   i8x_userdata_cleanup_fn *cleanup)	\
  {									\
    i8x_ob_set_userdata ((struct i8x_object *) x, userdata, cleanup);	\
  }

#define I8X_COMMON_OBJECT_FUNCTIONS_PREFIX(TYPE, PREFIX)		\
  I8X_NOPARENT_OBJECT_FUNCTIONS (TYPE, PREFIX)				\
									\
  static inline struct i8x_ctx * __attribute__ ((always_inline))	\
  i8x_ ## PREFIX ## _get_ctx (struct i8x_ ## TYPE *x)			\
  {									\
    return i8x_ob_get_ctx ((struct i8x_object *) x);			\
  }

#define I8X_CONTEXT_OBJECT_FUNCTIONS(TYPE) \
  I8X_NOPARENT_OBJECT_FUNCTIONS (TYPE, TYPE)

#define I8X_COMMON_OBJECT_FUNCTIONS(TYPE) \
  I8X_COMMON_OBJECT_FUNCTIONS_PREFIX (TYPE, TYPE)

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

I8X_CONTEXT_OBJECT_FUNCTIONS (ctx);

i8x_err_e i8x_ctx_new (struct i8x_ctx **ctx);
void i8x_ctx_set_log_fn (struct i8x_ctx *ctx, i8x_log_fn_t *log_fn);
int i8x_ctx_get_log_priority (struct i8x_ctx *ctx);
void i8x_ctx_set_log_priority (struct i8x_ctx *ctx, int priority);
const char *i8x_ctx_strerror_r (struct i8x_ctx *ctx, i8x_err_e code,
				char *buf, size_t bufsiz);

/*
 * i8x_note
 *
 * access to notes of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (note);

i8x_err_e i8x_note_new_from_mem (struct i8x_ctx *ctx,
				 const char *buf, size_t bufsiz,
				 const char *srcname, ssize_t srcoffset,
				 struct i8x_note **note);
const char *i8x_note_get_src_name (struct i8x_note *note);
ssize_t i8x_note_get_src_offset (struct i8x_note *note);
size_t i8x_note_get_encoded_size (struct i8x_note *note);
const char *i8x_note_get_encoded (struct i8x_note *note);

/*
 * i8x_readbuf
 *
 * access to readbufs of i8x
 */

I8X_COMMON_OBJECT_FUNCTIONS_PREFIX (readbuf, rb);

i8x_err_e i8x_rb_new_from_note (struct i8x_note *note,
				struct i8x_readbuf **rb);
struct i8x_note *i8x_rb_get_note (struct i8x_readbuf *rb);
size_t i8x_rb_bytes_left (struct i8x_readbuf *rb);
i8x_err_e i8x_rb_read_uint8_t (struct i8x_readbuf *rb, uint8_t *result);
i8x_err_e i8x_rb_read_uleb128 (struct i8x_readbuf *rb, uintmax_t *result);
i8x_err_e i8x_rb_read_bytes (struct i8x_readbuf *rb, size_t nbytes,
			     const char **result);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_H_ */
