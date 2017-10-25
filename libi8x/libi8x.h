/* Copyright (C) 2016-17 Red Hat, Inc.
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes.  */

typedef enum
{
  /* No error.  */
  I8X_OK = 0,

  /* Errors analogous to errno values.  */
  I8X_ENOMEM = -99,
  I8X_EINVAL,

  /* Note rejection reasons.  */
  I8X_NOTE_CORRUPT = -199,
  I8X_NOTE_UNHANDLED,
  I8X_NOTE_INVALID,

  /* Runtime errors.  */
  I8X_UNRESOLVED_FUNCTION = -299,
  I8X_STACK_OVERFLOW,
  I8X_RELOC_FAILED,
  I8X_READ_MEM_FAILED,
  I8X_DIVIDE_BY_ZERO,
  I8X_NATCALL_FAILED,
  I8X_NATCALL_BAD_FUNCREF_RET,
}
i8x_err_e;

const char *i8x_strerror_r (i8x_err_e code, char *buf, size_t buflen);

/* Values for i8x_ctx_new's "flags" argument.  In addition to these,
   the bottom 3 bits can be used to pass a logging message priority.
   Priorities have the same values and meaning as the severities
   defined in section 6.2.1 of RFC 5424 ("The Syslog Protocol").
   You can "#include <syslog.h>" for definitions of these values.  */

#define I8X_LOG_TRACE	0x08	/* Log bytecodes as they execute.  */
#define I8X_DBG_MEM	0x10	/* Debug memory allocation.  */

/* Forward declarations.  */

struct i8x_ctx;
struct i8x_func;
struct i8x_funcref;
struct i8x_inf;
struct i8x_list;
struct i8x_listitem;
struct i8x_note;
struct i8x_object;
struct i8x_reloc;
struct i8x_type;
struct i8x_xctx;

/* Runtime values.  */

union i8x_value
{
  void *p;			/* Pointer values.  */
  intptr_t i;			/* Signed integer values.  */
  uintptr_t u;			/* Unsigned integer values.  */
  struct i8x_funcref *f;	/* Function values.  */
};

/* Function types.  */

typedef void i8x_cleanup_fn_t (void *userdata);

typedef void i8x_log_fn_t (struct i8x_ctx *ctx,
			   int priority,
			   const char *file, int line,
			   const char *fn,
			   const char *format, va_list args);

typedef i8x_err_e i8x_nat_fn_t (struct i8x_xctx *xctx,
				struct i8x_inf *inf,
				struct i8x_func *func,
				union i8x_value *args,
				union i8x_value *rets);

/* i8x_object functions.  Don't call these directly, use the typed
   versions provided by I8X_COMMON_OBJECT_FUNCTIONS et al.  */

struct i8x_object *i8x_ob_ref (struct i8x_object *ob);
struct i8x_object *i8x_ob_unref (struct i8x_object *ob);
const char *i8x_ob_get_classname (struct i8x_object *ob);
struct i8x_object *i8x_ob_get_parent (struct i8x_object *ob);
struct i8x_ctx *i8x_ob_get_ctx (struct i8x_object *ob);
void *i8x_ob_get_userdata (struct i8x_object *ob);
void i8x_ob_set_userdata (struct i8x_object *ob,
			  void *userdata,
			  i8x_cleanup_fn_t *userdata_cleanup);
struct i8x_object *i8x_listitem_get_object (struct i8x_listitem *item);

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
				   i8x_cleanup_fn_t *cleanup)	\
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

#define I8X_LISTABLE_OBJECT_FUNCTIONS_PREFIX(TYPE, PREFIX)		\
  static inline struct i8x_ ## TYPE * __attribute__ ((always_inline))	\
  i8x_listitem_get_ ## PREFIX (struct i8x_listitem *item)		\
  {									\
    return (struct i8x_ ## TYPE *) i8x_listitem_get_object (item);	\
  }

#define I8X_LISTABLE_OBJECT_FUNCTIONS(TYPE) \
  I8X_LISTABLE_OBJECT_FUNCTIONS_PREFIX (TYPE, TYPE)

/* i8x_ctx */

I8X_CONTEXT_OBJECT_FUNCTIONS (ctx);

i8x_err_e i8x_ctx_new (int flags, i8x_log_fn_t *log_fn,
		       struct i8x_ctx **ctx);
void i8x_ctx_clear_last_error (struct i8x_ctx *ctx);
const char *i8x_ctx_get_last_error_src_name (struct i8x_ctx *ctx);
ssize_t i8x_ctx_get_last_error_src_offset (struct i8x_ctx *ctx);
void i8x_ctx_set_log_fn (struct i8x_ctx *ctx, i8x_log_fn_t *log_fn);
int i8x_ctx_get_log_priority (struct i8x_ctx *ctx);
void i8x_ctx_set_log_priority (struct i8x_ctx *ctx, int priority);
i8x_err_e i8x_ctx_get_funcref (struct i8x_ctx *ctx,
			       const char *signature,
			       struct i8x_funcref **ref);
i8x_err_e i8x_ctx_import_bytecode (struct i8x_ctx *ctx,
				   const char *buf, size_t buflen,
				   const char *srcname, ssize_t srcoffset,
				   struct i8x_func **func);
i8x_err_e i8x_ctx_import_native (struct i8x_ctx *ctx,
				 const char *signature,
				 i8x_nat_fn_t *impl_fn,
				 struct i8x_func **func);
struct i8x_list *i8x_ctx_get_functions (struct i8x_ctx *ctx);

/* i8x_func */

I8X_COMMON_OBJECT_FUNCTIONS (func);

struct i8x_funcref *i8x_func_get_funcref (struct i8x_func *func);
struct i8x_note *i8x_func_get_note (struct i8x_func *func);
struct i8x_list *i8x_func_get_relocs (struct i8x_func *func);
i8x_err_e i8x_func_unregister (struct i8x_func *func);

#define i8x_func_get_signature(func) \
  i8x_funcref_get_signature (i8x_func_get_funcref (func))

/* i8x_funcref */

I8X_COMMON_OBJECT_FUNCTIONS (funcref);
I8X_LISTABLE_OBJECT_FUNCTIONS (funcref);

const char *i8x_funcref_get_signature (struct i8x_funcref *ref);
bool i8x_funcref_is_global (struct i8x_funcref *ref);
bool i8x_funcref_is_resolved (struct i8x_funcref *ref);
struct i8x_type *i8x_funcref_get_type (struct i8x_funcref *ref);
size_t i8x_funcref_get_num_params (struct i8x_funcref *ref);
size_t i8x_funcref_get_num_returns (struct i8x_funcref *ref);

#define i8x_funcref_get_ptypes(ref) \
  i8x_type_get_ptypes (i8x_funcref_get_type (ref))

#define i8x_funcref_get_rtypes(ref) \
  i8x_type_get_rtypes (i8x_funcref_get_type (ref))

/* i8x_inf */

I8X_COMMON_OBJECT_FUNCTIONS (inf);

typedef i8x_err_e i8x_read_mem_fn_t (struct i8x_inf *inf,
				     uintptr_t addr, size_t len,
				     void *result);

typedef i8x_err_e i8x_relocate_fn_t (struct i8x_inf *inf,
				     struct i8x_reloc *reloc,
				     uintptr_t *result);

i8x_err_e i8x_inf_new (struct i8x_ctx *ctx, struct i8x_inf **inf);
void i8x_inf_set_read_mem_fn (struct i8x_inf *inf,
			      i8x_read_mem_fn_t *read_mem_fn);
void i8x_inf_set_relocate_fn (struct i8x_inf *inf,
			      i8x_relocate_fn_t *relocate_fn);

/* i8x_list */

I8X_COMMON_OBJECT_FUNCTIONS (list);

size_t i8x_list_size (struct i8x_list *list);
struct i8x_listitem *i8x_list_get_first (struct i8x_list *list);
struct i8x_listitem *i8x_list_get_next (struct i8x_list *list,
					struct i8x_listitem *curr);

#define i8x_list_foreach(list, item)		\
  for (item = i8x_list_get_first (list);	\
       item != NULL;				\
       item = i8x_list_get_next (list, item))

#define i8x_list_foreach_indexed(list, item, index)	\
  for (index = 0, item = i8x_list_get_first (list);	\
       item != NULL;					\
       item = i8x_list_get_next (list, item), index++)

/* i8x_listitem */

I8X_COMMON_OBJECT_FUNCTIONS (listitem);

/* i8x_note */

I8X_COMMON_OBJECT_FUNCTIONS (note);

const char *i8x_note_get_src_name (struct i8x_note *note);

/* i8x_reloc */

I8X_COMMON_OBJECT_FUNCTIONS (reloc);
I8X_LISTABLE_OBJECT_FUNCTIONS (reloc);

struct i8x_func *i8x_reloc_get_func (struct i8x_reloc *reloc);
ssize_t i8x_reloc_get_src_offset (struct i8x_reloc *reloc);
uintptr_t i8x_reloc_get_unrelocated (struct i8x_reloc *reloc);

/* i8x_type */

I8X_COMMON_OBJECT_FUNCTIONS (type);
I8X_LISTABLE_OBJECT_FUNCTIONS (type);

bool i8x_type_is_functype (struct i8x_type *type);
struct i8x_list *i8x_type_get_ptypes (struct i8x_type *type);
struct i8x_list *i8x_type_get_rtypes (struct i8x_type *type);

/* i8x_xctx */

I8X_COMMON_OBJECT_FUNCTIONS (xctx);

i8x_err_e i8x_xctx_new (struct i8x_ctx *ctx, size_t stack_slots,
			struct i8x_xctx **xctx);
i8x_err_e i8x_xctx_call (struct i8x_xctx *xctx,
			 struct i8x_funcref *ref,
			 struct i8x_inf *inf,
			 union i8x_value *args,
			 union i8x_value *rets);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_H_ */
