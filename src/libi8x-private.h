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
#include <i8x/libi8x.h>

/* Encoded types.  */

#define I8_TYPE_INTEGER	 'i'
#define I8_TYPE_POINTER	 'p'
#define I8_TYPE_OPAQUE	 'o'
#define I8_TYPE_FUNCTION 'F'

/* Forward declarations.  */

struct i8x_symref;
struct i8x_type;

/* Errors.  */

#define i8x_out_of_memory(ctx) \
  i8x_ctx_set_error (ctx, I8X_ENOMEM, NULL, NULL)

#define i8x_invalid_argument(ctx) \
  i8x_ctx_set_error (ctx, I8X_EINVAL, NULL, NULL)

#ifdef __cplusplus
extern "C" {
#endif

i8x_err_e i8x_note_error (struct i8x_note *note, i8x_err_e code,
			  const char *cause_ptr);
i8x_err_e i8x_rb_error (struct i8x_readbuf *rb, i8x_err_e code,
			const char *cause_ptr);

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

/* Branch prediction.  */

#if __GNUC__ >= 3
# define __i8x_unlikely(cond)	__builtin_expect ((cond), 0)
# define __i8x_likely(cond)	__builtin_expect ((cond), 1)
#else
# define __i8x_unlikely(cond)	(cond)
# define __i8x_likely(cond)	(cond)
#endif

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

/* Linked lists.  */

i8x_err_e i8x_list_append (struct i8x_list *list, struct i8x_object *ob);
void i8x_list_remove (struct i8x_list *list, struct i8x_object *ob);

#define I8X_LIST_FUNCTIONS_PREFIX(TYPE, PREFIX)			\
  static inline i8x_err_e __attribute__ ((always_inline))	\
  i8x_list_append_ ## PREFIX (struct i8x_list *list,		\
			      struct i8x_ ## TYPE *ob)		\
  {								\
    return i8x_list_append (list, (struct i8x_object *) ob);	\
  }								\
								\
  static inline void __attribute__ ((always_inline))		\
  i8x_list_remove_ ## PREFIX (struct i8x_list *list,		\
			      struct i8x_ ## TYPE *ob)		\
  {								\
    i8x_list_remove (list, (struct i8x_object *) ob);		\
  }

#define I8X_LIST_FUNCTIONS(TYPE) \
  I8X_LIST_FUNCTIONS_PREFIX (TYPE, TYPE)

/*
 * i8x_code
 *
 * access to codes of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (code);

i8x_err_e i8x_code_new_from_func (struct i8x_func *func,
				  struct i8x_code **code);

/*
 * i8x_symref
 *
 * access to symrefs of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (symref);
I8X_LIST_FUNCTIONS (symref);
I8X_LISTABLE_OBJECT_FUNCTIONS (symref);

i8x_err_e i8x_symref_new (struct i8x_ctx *ctx, const char *name,
			  struct i8x_symref **ref);
struct i8x_symref *i8x_object_as_symref (struct i8x_object *ob);
const char *i8x_symref_get_name (struct i8x_symref *ref);

/*
 * i8x_type
 *
 * access to types of i8x
 */
I8X_COMMON_OBJECT_FUNCTIONS (type);
I8X_LIST_FUNCTIONS (type);
I8X_LISTABLE_OBJECT_FUNCTIONS (type);

i8x_err_e i8x_type_new_coretype (struct i8x_ctx *ctx,
				 char encoded,
				 struct i8x_type **type);
i8x_err_e i8x_type_new_functype (struct i8x_ctx *ctx,
				 const char *encoded,
				 const char *ptypes_start,
				 const char *ptypes_limit,
				 const char *rtypes_start,
				 const char *rtypes_limit,
				 struct i8x_note *src_note,
				 struct i8x_type **type);
const char *i8x_type_get_encoded (struct i8x_type *type);
struct i8x_list *i8x_type_get_ptypes (struct i8x_type *type);
struct i8x_list *i8x_type_get_rtypes (struct i8x_type *type);

/* i8x_chunk private functions.  */

I8X_LIST_FUNCTIONS (chunk);

i8x_err_e i8x_chunk_new_from_readbuf (struct i8x_readbuf *rb,
				      struct i8x_chunk **chunk);
i8x_err_e i8x_chunk_unhandled_error (struct i8x_chunk *chunk);
i8x_err_e i8x_chunk_version_error (struct i8x_chunk *chunk);

/* i8x_ctx private functions.  */

struct i8x_type *i8x_ctx_get_integer_type (struct i8x_ctx *ctx);
struct i8x_type *i8x_ctx_get_pointer_type (struct i8x_ctx *ctx);
struct i8x_type *i8x_ctx_get_opaque_type (struct i8x_ctx *ctx);
bool i8x_ctx_get_use_debug_interpreter_default (struct i8x_ctx *ctx);
i8x_err_e i8x_ctx_set_error (struct i8x_ctx *ctx, i8x_err_e code,
			     struct i8x_note *cause_note,
			     const char *cause_ptr);
i8x_err_e i8x_ctx_get_funcref_with_note (struct i8x_ctx *ctx,
					 const char *provider,
					 const char *name,
					 const char *encoded_ptypes,
					 const char *encoded_rtypes,
					 struct i8x_note *src_note,
					 struct i8x_funcref **ref);
void i8x_ctx_forget_funcref (struct i8x_funcref *ref);
i8x_err_e i8x_ctx_get_symref (struct i8x_ctx *ctx,
			      const char *name,
			      struct i8x_symref **ref);
void i8x_ctx_forget_symref (struct i8x_symref *ref);
i8x_err_e i8x_ctx_get_functype (struct i8x_ctx *ctx,
				const char *encoded_ptypes_start,
				const char *encoded_ptypes_limit,
				const char *encoded_rtypes_start,
				const char *encoded_rtypes_limit,
				struct i8x_note *src_note,
				struct i8x_type **type);
void i8x_ctx_forget_functype (struct i8x_type *type);
void i8x_ctx_fire_availability_observer (struct i8x_func *func,
					 bool is_available);
size_t i8x_ctx_get_dispatch_table_size (struct i8x_ctx *ctx);
i8x_err_e i8x_ctx_get_dispatch_tables (struct i8x_ctx *ctx,
				       void ***dispatch_std,
				       void ***dispatch_dbg);
i8x_err_e i8x_ctx_init_dispatch_table (struct i8x_ctx *ctx,
				       void **table, size_t table_size,
				       bool is_debug);

/* i8x_func private functions.  */

I8X_LIST_FUNCTIONS (func);
I8X_LISTABLE_OBJECT_FUNCTIONS (func);

bool i8x_func_all_deps_resolved (struct i8x_func *func);
void i8x_func_fire_availability_observers (struct i8x_func *func);
struct i8x_code *i8x_func_get_interp_impl (struct i8x_func *func);
i8x_nat_fn_t *i8x_func_get_native_impl (struct i8x_func *func);

/* i8x_funcref private functions.  */

I8X_LIST_FUNCTIONS (funcref);
I8X_LISTABLE_OBJECT_FUNCTIONS (funcref);

i8x_err_e i8x_funcref_new (struct i8x_ctx *ctx, const char *fullname,
			   struct i8x_type *functype,
			   struct i8x_funcref **ref);
struct i8x_funcref *i8x_object_as_funcref (struct i8x_object *ob);
void i8x_funcref_register_func (struct i8x_funcref *ref,
				struct i8x_func *func);
void i8x_funcref_unregister_func (struct i8x_funcref *ref,
				  struct i8x_func *func);
void i8x_funcref_reset_is_resolved (struct i8x_funcref *ref);
void i8x_funcref_mark_unresolved (struct i8x_funcref *ref);
struct i8x_type *i8x_funcref_get_type (struct i8x_funcref *ref);

/* i8x_list private functions.  */

i8x_err_e i8x_list_new (struct i8x_ctx *ctx,
			bool manage_references,
			struct i8x_list **list);

/* i8x_object private functions.  */

i8x_err_e i8x_ob_new (void *parent, const struct i8x_object_ops *ops,
		      void *ob);
struct i8x_object *i8x_ob_get_parent (struct i8x_object *ob);
struct i8x_object *i8x_ob_cast (struct i8x_object *ob,
				const struct i8x_object_ops *ops);

/* i8x_readbuf private functions.  */

const char *i8x_rb_get_ptr (struct i8x_readbuf *rb);
i8x_err_e i8x_rb_read_funcref (struct i8x_readbuf *rb,
			       struct i8x_funcref **ref);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_PRIVATE_H_ */
