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

#ifndef _LIBI8X_PRIVATE_H_
#define _LIBI8X_PRIVATE_H_

#include <syslog.h>
#include <stdbool.h>

#include <i8x/libi8x.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations.  */

struct i8x_ext;
struct i8x_list;
struct i8x_symref;

/* Errors.  */

i8x_err_e i8x_ctx_set_error (struct i8x_ctx *ctx, i8x_err_e code,
			     struct i8x_note *cause_note,
			     const char *cause_ptr);

#define i8x_out_of_memory(ctx) \
  i8x_ctx_set_error (ctx, I8X_OUT_OF_MEMORY, NULL, NULL)

#define i8x_note_error(note, code, ptr) \
  i8x_ctx_set_error (i8x_note_get_ctx (note), code, note, ptr)

/* Assertions.  */

#define i8x_assert(expr) \
  ((void) ((expr) ? 0 : \
	   (i8x_assert_fail (__FILE__, __LINE__, __FUNCTION__, \
			     #expr), 0)))

#define i8x_assert_fail(file, line, function, assertion) \
  i8x_internal_error (file, line, function, \
		      _("Assertion '%s' failed."), assertion)

#define i8x_not_implemented() \
  do { \
    i8x_internal_error (__FILE__, __LINE__, __FUNCTION__, \
			_("Not implemented.")); \
  } while (0)

void i8x_internal_error (const char *file, int line,
			 const char *function, const char *format, ...)
  __attribute__ ((__noreturn__, format (printf, 4, 5)));

/* Logging.  */

static inline void __attribute__ ((always_inline, format (printf, 2, 3)))
i8x_log_null (struct i8x_ctx *ctx, const char *format, ...) {}

#define i8x_log_cond(ctx, prio, arg...) \
  do { \
    if (i8x_ctx_get_log_priority (ctx) >= prio) \
      i8x_ctx_log (ctx, prio, __FILE__, __LINE__, __FUNCTION__, ## arg); \
  } while (0)

#ifdef ENABLE_LOGGING
#  ifdef ENABLE_DEBUG
#    define dbg(ctx, arg...) i8x_log_cond (ctx, LOG_DEBUG, ## arg)
#  else
#    define dbg(ctx, arg...) i8x_log_null (ctx, ## arg)
#  endif
#  define info(ctx, arg...) i8x_log_cond (ctx, LOG_INFO, ## arg)
#  define notice(ctx, arg...) i8x_log_cond (ctx, LOG_NOTICE, ## arg)
#  define warn(ctx, arg...) i8x_log_cond (ctx, LOG_WARNING, ## arg)
#  define err(ctx, arg...) i8x_log_cond (ctx, LOG_ERR, ## arg)
#else
#  define dbg(ctx, arg...) i8x_log_null (ctx, ## arg)
#  define info(ctx, arg...) i8x_log_null (ctx, ## arg)
#  define notice(ctx, arg...) i8x_log_null (ctx, ## arg)
#  define warn(ctx, arg...) i8x_log_null (ctx, ## arg)
#  define err(ctx, arg...) i8x_log_null (ctx, ## arg)
#endif

#ifndef HAVE_SECURE_GETENV
#  ifdef HAVE___SECURE_GETENV
#    define secure_getenv __secure_getenv
#  else
#    error neither secure_getenv nor __secure_getenv is available
#  endif
#endif

#define I8X_EXPORT __attribute__ ((visibility ("default")))

void i8x_ctx_log (struct i8x_ctx *ctx,
		  int priority, const char *file, int line,
		  const char *fn, const char *format, ...)
  __attribute__ ((format (printf, 6, 7)));

/* Placeholder for NLS.  */

#define _(string) string

/* Object system.  */

struct i8x_object_ops
{
  const char *name;
  size_t size;
  void (*unlink_fn) (struct i8x_object *ob);
  void (*free_fn) (struct i8x_object *ob);
};

struct i8x_object
{
  const struct i8x_object_ops *ops;
  struct i8x_object *parent;
  int refcount[2];
  bool is_moribund;
  void *userdata;
  i8x_userdata_cleanup_fn *userdata_cleanup;
};

#define I8X_OBJECT_FIELDS struct i8x_object _ob

/*
 * i8x_listitem
 *
 * access to listitems of i8x
 */
struct i8x_listitem
{
  I8X_OBJECT_FIELDS;

  struct i8x_listitem *next;
  struct i8x_listitem *prev;
};

#define I8X_LISTITEM_OBJECT_FIELDS struct i8x_listitem li

#define I8X_LISTITEM_OBJECT_FUNCTIONS_PREFIX(TYPE, PREFIX)		\
  static inline struct i8x_ ## TYPE * __attribute__ ((always_inline))	\
  i8x_ ## PREFIX ## _list_get_next (struct i8x_list *head,		\
				    struct i8x_ ## TYPE *item)		\
  {									\
    struct i8x_listitem *next = ((struct i8x_listitem *) item)->next;	\
									\
    if (next == (struct i8x_listitem *) head)				\
      return NULL;							\
									\
    return (struct i8x_ ## TYPE *) next;				\
  }									\
									\
  static inline struct i8x_ ## TYPE * __attribute__ ((always_inline))	\
  i8x_ ## PREFIX ## _list_get_first (struct i8x_list *head)		\
  {									\
    return i8x_ ## PREFIX ## _list_get_next (				\
      head, (struct i8x_ ## TYPE *) head);				\
  }									\
									\
  static inline void __attribute__ ((always_inline))			\
  i8x_ ## PREFIX ## _list_append (struct i8x_list *head,		\
				  struct i8x_ ## TYPE *item)		\
  {									\
    i8x_list_append (head, (struct i8x_listitem *) item);		\
  }									\
									\
  static inline void __attribute__ ((always_inline))			\
  i8x_ ## PREFIX ## _list_remove (struct i8x_list *head,		\
				  struct i8x_ ## TYPE *item)		\
  {									\
    i8x_list_remove (head, (struct i8x_listitem *) item);		\
  }

#define I8X_LISTITEM_OBJECT_FUNCTIONS(TYPE) \
  I8X_LISTITEM_OBJECT_FUNCTIONS_PREFIX (TYPE, TYPE)

I8X_COMMON_OBJECT_FUNCTIONS (listitem);

/*
 * i8x_list
 *
 * access to lists of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (list);

i8x_err_e i8x_list_new (struct i8x_ctx *ctx,
			bool reference_items,
			struct i8x_list **list);
void i8x_list_append (struct i8x_list *head, struct i8x_listitem *item);
void i8x_list_remove (struct i8x_list *head, struct i8x_listitem *item);

/*
 * i8x_ext
 *
 * access to exts of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (ext);
I8X_LISTITEM_OBJECT_FUNCTIONS (ext);

#define i8x_ext_list_foreach(item, list)		\
  for (item = i8x_ext_list_get_first (list);		\
       item != NULL;					\
       item = i8x_ext_list_get_next (list, item))

i8x_err_e i8x_ext_new_from_readbuf (struct i8x_readbuf *rb,
				    struct i8x_ext **ext);

/*
 * i8x_symref
 *
 * access to symrefs of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (symref);
I8X_LISTITEM_OBJECT_FUNCTIONS (symref);

#define i8x_symref_list_foreach(item, list)		\
  for (item = i8x_symref_list_get_first (list);		\
       item != NULL;					\
       item = i8x_symref_list_get_next (list, item))

i8x_err_e i8x_symref_new (struct i8x_ctx *ctx, const char *name,
			  struct i8x_symref **ref);
const char *i8x_symref_get_name (struct i8x_symref *ref);

/* i8x_chunk private functions.  */

i8x_err_e i8x_chunk_list_new_from_readbuf (struct i8x_readbuf *rb,
					   struct i8x_chunk **chunk_list);
struct i8x_chunk *i8x_chunk_get_next (struct i8x_chunk *chunk);
i8x_err_e i8x_chunk_version_error (struct i8x_chunk *chunk);

#define i8x_chunk_list_foreach(chunk, first_chunk)  \
  for (chunk = first_chunk;			    \
       chunk != NULL;				    \
       chunk = i8x_chunk_get_next (chunk))

/* i8x_ctx private functions.  */

void i8x_ctx_forget_funcref (struct i8x_funcref *ref);
i8x_err_e i8x_ctx_get_symref (struct i8x_ctx *ctx,
			      const char *name,
			      struct i8x_symref **ref);
void i8x_ctx_forget_symref (struct i8x_symref *ref);

/* i8x_func private functions.  */

I8X_LISTITEM_OBJECT_FUNCTIONS (func);

#define i8x_func_list_foreach(item, list)		\
  for (item = i8x_func_list_get_first (list);		\
       item != NULL;					\
       item = i8x_func_list_get_next (list, item))

/* i8x_funcref private functions.  */

I8X_LISTITEM_OBJECT_FUNCTIONS (funcref);

#define i8x_funcref_list_foreach(item, list)		\
  for (item = i8x_funcref_list_get_first (list);	\
       item != NULL;					\
       item = i8x_funcref_list_get_next (list, item))

i8x_err_e i8x_funcref_new (struct i8x_ctx *ctx, const char *fullname,
			   const char *ptypes, const char *rtypes,
			   struct i8x_funcref **ref);
void i8x_funcref_register_func (struct i8x_funcref *ref,
				struct i8x_func *func);
void i8x_funcref_unregister_func (struct i8x_funcref *ref,
				  struct i8x_func *func);

/* i8x_object private functions.  */

i8x_err_e i8x_ob_new (void *parent, const struct i8x_object_ops *ops,
		      void *ob);
struct i8x_object *i8x_ob_get_parent (struct i8x_object *ob);

/* i8x_readbuf private functions.  */

const char *i8x_rb_get_ptr (struct i8x_readbuf *rb);
i8x_err_e i8x_rb_read_funcref (struct i8x_readbuf *rb,
			       struct i8x_funcref **ref);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_PRIVATE_H_ */
