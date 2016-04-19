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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "libi8x-private.h"

/**
 * SECTION:libi8x
 * @short_description: libi8x context
 *
 * The context contains the default values for the library user,
 * and is passed to all library operations.
 */

/**
 * i8x_ctx:
 *
 * Opaque object representing the library context.
 */
struct i8x_ctx
{
  I8X_OBJECT_FIELDS;

  i8x_log_fn_t *log_fn;
  int log_priority;
  bool logging_started;

  bool use_debug_interpreter_default;

  struct i8x_note *error_note;	/* Note that caused the last error.  */
  const char *error_ptr;	/* Pointer into error_note.  */

  struct i8x_list *funcrefs;	/* List of interned function references.  */
  struct i8x_list *symrefs;	/* List of interned symbol references.  */
  struct i8x_list *functypes;	/* List of interned function types.  */

  struct i8x_list *functions;	/* List of registered functions.  */

  /* User-supplied function called when a function becomes available.  */
  i8x_func_cb_t *func_avail_observer_fn;

  /* User-supplied function called when a function is about to become
     unavailable.  */
  i8x_func_cb_t *func_unavail_observer_fn;

  /* The three core types.  */
  struct i8x_type *integer_type;
  struct i8x_type *pointer_type;
  struct i8x_type *opaque_type;

  /* The interpreters' dispatch tables.  */
  void **dispatch_std;
  void **dispatch_dbg;
};

void
i8x_ctx_log (struct i8x_ctx *ctx,
	     int priority, const char *file, int line, const char *fn,
	     const char *format, ...)
{
  va_list args;

  if (!ctx->logging_started)
    {
      ctx->logging_started = true;

      /* These messages are deferred from i8x_ctx_new to allow the
	 caller to install a custom logger and set the priority if
	 they require it.  */
      dbg (ctx, "ctx %p created\n", ctx);
      dbg (ctx, "log_priority=%d\n", ctx->log_priority);
    }

  va_start (args, format);
  ctx->log_fn (ctx, priority, file, line, fn, format, args);
  va_end (args);
}

static void
log_stderr (struct i8x_ctx *ctx,
	    int priority, const char *file, int line, const char *fn,
	    const char *format, va_list args)
{
  fprintf (stderr, "libi8x: %s: ", fn);
  vfprintf (stderr, format, args);
}

static int
log_priority (const char *str)
{
  char *endptr;
  int priority;

  priority = strtol (str, &endptr, 10);
  if (endptr[0] == '\0' || isspace (endptr[0]))
    return priority;
  if (strncmp (str, "err", 3) == 0)
    return LOG_ERR;
  if (strncmp (str, "info", 4) == 0)
    return LOG_INFO;
  if (strncmp (str, "debug", 5) == 0)
    return LOG_DEBUG;

  return 0;
}

static bool
strtobool (const char *str)
{
  if (str[0] == '\0' || str[0] == '0')
    return false;
  if (isdigit (str[0]))
    return true;
  if (strncasecmp (str, "yes", 3) == 0)
    return true;
  if (strncasecmp (str, "true", 4) == 0)
    return true;

  return false;
}

static i8x_err_e
i8x_ctx_init (struct i8x_ctx *ctx)
{
  i8x_err_e err;

  err = i8x_list_new (ctx, true, &ctx->functions);
  if (err != I8X_OK)
    return err;

  err = i8x_list_new (ctx, false, &ctx->funcrefs);
  if (err != I8X_OK)
    return err;

  err = i8x_list_new (ctx, false, &ctx->symrefs);
  if (err != I8X_OK)
    return err;

  err = i8x_list_new (ctx, false, &ctx->functypes);
  if (err != I8X_OK)
    return err;

  err = i8x_type_new_coretype (ctx, I8_TYPE_INTEGER, &ctx->integer_type);
  if (err != I8X_OK)
    return err;

  err = i8x_type_new_coretype (ctx, I8_TYPE_POINTER, &ctx->pointer_type);
  if (err != I8X_OK)
    return err;

  err = i8x_type_new_coretype (ctx, I8_TYPE_OPAQUE, &ctx->opaque_type);
  if (err != I8X_OK)
    return err;

  return err;
}

static void
i8x_ctx_unlink (struct i8x_object *ob)
{
  struct i8x_ctx *ctx = (struct i8x_ctx *) ob;

  ctx->error_note = i8x_note_unref (ctx->error_note);

  ctx->functions = i8x_list_unref (ctx->functions);

  ctx->funcrefs = i8x_list_unref (ctx->funcrefs);
  ctx->symrefs = i8x_list_unref (ctx->symrefs);
  ctx->functypes = i8x_list_unref (ctx->functypes);

  ctx->integer_type = i8x_type_unref (ctx->integer_type);
  ctx->pointer_type = i8x_type_unref (ctx->pointer_type);
  ctx->opaque_type = i8x_type_unref (ctx->opaque_type);
}

static void
i8x_ctx_free (struct i8x_object *ob)
{
  struct i8x_ctx *ctx = (struct i8x_ctx *) ob;

  if (ctx->dispatch_std != NULL)
    free (ctx->dispatch_std);

  if (ctx->dispatch_dbg != NULL)
    free (ctx->dispatch_dbg);
}

const struct i8x_object_ops i8x_ctx_ops =
  {
    "ctx",			/* Object name.  */
    sizeof (struct i8x_ctx),	/* Object size.  */
    i8x_ctx_unlink,		/* Unlink function.  */
    i8x_ctx_free,		/* Free function.  */
  };

/**
 * i8x_ctx_new:
 *
 * Create i8x library context. This reads the i8x configuration
 * and fills in the default values.
 *
 * The initial refcount is 1, and needs to be decremented to
 * release the resources of the i8x library context.
 *
 * Returns: a new i8x library context
 **/
I8X_EXPORT i8x_err_e
i8x_ctx_new (struct i8x_ctx **ctx)
{
  const char *env;
  struct i8x_ctx *c;
  i8x_err_e err;

  err = i8x_ob_new (NULL, &i8x_ctx_ops, &c);
  if (err != I8X_OK)
    return err;

  c->log_fn = log_stderr;
  c->log_priority = LOG_ERR;

  env = secure_getenv ("I8X_LOG");
  if (env != NULL)
    i8x_ctx_set_log_priority (c, log_priority (env));

  env = secure_getenv ("I8X_DEBUG");
  if (env != NULL)
    c->use_debug_interpreter_default = strtobool (env);

  err = i8x_ctx_init (c);
  if (err != I8X_OK)
    {
      c = i8x_ctx_unref (c);

      return err;
    }

  *ctx = c;

  return I8X_OK;
}

/**
 * i8x_ctx_set_log_fn:
 * @ctx: i8x library context
 * @log_fn: function to be called for logging messages
 *
 * The built-in logging writes to stderr. It can be
 * overridden by a custom function, to plug log messages
 * into the user's logging functionality.
 *
 **/
I8X_EXPORT void
i8x_ctx_set_log_fn (struct i8x_ctx *ctx, i8x_log_fn_t *log_fn)
{
  ctx->log_fn = log_fn;
}

/**
 * i8x_ctx_get_log_priority:
 * @ctx: i8x library context
 *
 * Returns: the current logging priority
 **/
I8X_EXPORT int
i8x_ctx_get_log_priority (struct i8x_ctx *ctx)
{
  return ctx->log_priority;
}

/**
 * i8x_ctx_set_log_priority:
 * @ctx: i8x library context
 * @priority: the new logging priority
 *
 * Set the current logging priority. The value controls which messages
 * are logged.
 **/
I8X_EXPORT void
i8x_ctx_set_log_priority (struct i8x_ctx *ctx, int priority)
{
  ctx->log_priority = priority;
}

bool
i8x_ctx_get_use_debug_interpreter_default (struct i8x_ctx *ctx)
{
  return ctx->use_debug_interpreter_default;
}


I8X_EXPORT void
i8x_ctx_set_func_available_cb (struct i8x_ctx *ctx,
			       i8x_func_cb_t *func_avail_cb_fn)
{
  ctx->func_avail_observer_fn = func_avail_cb_fn;
}

I8X_EXPORT void
i8x_ctx_set_func_unavailable_cb (struct i8x_ctx *ctx,
				 i8x_func_cb_t *func_unavail_cb_fn)
{
  ctx->func_unavail_observer_fn = func_unavail_cb_fn;
}

void
i8x_ctx_fire_availability_observer (struct i8x_func *func,
				    bool is_available)
{
  struct i8x_ctx *ctx;
  i8x_func_cb_t *callback;

  if (i8x_func_is_native (func))
    return;

  if (i8x_func_is_private (func))
    return;

  ctx = i8x_func_get_ctx (func);
  callback = is_available
    ? ctx->func_avail_observer_fn
    : ctx->func_unavail_observer_fn;

  if (callback != NULL)
    callback (func);
}

i8x_err_e
i8x_ctx_set_error (struct i8x_ctx *ctx, i8x_err_e code,
		   struct i8x_note *cause_note, const char *cause_ptr)
{
  i8x_assert (code != I8X_OK);

  if (ctx != NULL)
    {
      ctx->error_note = i8x_note_unref (ctx->error_note);
      ctx->error_note = i8x_note_ref (cause_note);

      ctx->error_ptr = cause_ptr;
    }

  return code;
}

static const char *
error_message_for (i8x_err_e code)
{
  switch (code)
    {
    case I8X_OK:
      return _("No error");

    case I8X_ENOMEM:
      return _("Out of memory");

    case I8X_EINVAL:
      return _("Invalid argument");

    case I8X_NOTE_CORRUPT:
      return _("Corrupt note");

    case I8X_NOTE_UNHANDLED:
      return _("Unhandled note");

    case I8X_NOTE_INVALID:
      return _("Invalid note");

    case I8X_STACK_OVERFLOW:
      return _("Stack overflow");

    default:
      return NULL;
    }
}

static void __attribute__ ((format (printf, 3, 4)))
xsnprintf (char **bufp, char *limit, const char *format, ...)
{
  char *buf = *bufp;
  size_t bufsiz = limit - buf;
  va_list args;

  va_start (args, format);
  vsnprintf (buf, bufsiz, format, args);
  va_end (args);

  buf[bufsiz - 1] = '\0';
  *bufp = buf + strlen (buf);
}

I8X_EXPORT const char *
i8x_ctx_strerror_r (struct i8x_ctx *ctx, i8x_err_e code,
		    char *buf, size_t bufsiz)
{
  char *ptr = buf;
  char *limit = ptr + bufsiz;
  const char *prefix = NULL;
  ssize_t offset = -1;
  const char *msg = error_message_for (code);

  if (ctx != NULL && ctx->error_note != NULL)
    {
      prefix = i8x_note_get_src_name (ctx->error_note);
      offset = i8x_note_get_src_offset (ctx->error_note);

      if (offset >= 0 && ctx->error_ptr != NULL)
	offset += ctx->error_ptr
	          - i8x_note_get_encoded (ctx->error_note);
    }

  if (prefix == NULL)
    prefix = PACKAGE;

  xsnprintf (&ptr, limit, "%s", prefix);

  if (offset >= 0)
    xsnprintf (&ptr, limit, "[0x%lx]", offset);

  xsnprintf (&ptr, limit, ": ");

  if (msg == NULL)
    xsnprintf (&ptr, limit, _("unhandled error %d"), code);
  else
    xsnprintf (&ptr, limit, "%s", msg);

  return buf;
}

struct i8x_type *
i8x_ctx_get_integer_type (struct i8x_ctx *ctx)
{
  return ctx->integer_type;
}

struct i8x_type *
i8x_ctx_get_pointer_type (struct i8x_ctx *ctx)
{
  return ctx->pointer_type;
}

struct i8x_type *
i8x_ctx_get_opaque_type (struct i8x_ctx *ctx)
{
  return ctx->opaque_type;
}

/* Internal version of i8x_ctx_get_funcref with an extra source note
   argument for error-reporting.  If the source note is not NULL then
   rtypes and ptypes MUST be pointers into the note's buffer or any
   resulting error messages will contain nonsense offsets.  */

i8x_err_e
i8x_ctx_get_funcref_with_note (struct i8x_ctx *ctx,
			       const char *provider, const char *name,
			       const char *ptypes, const char *rtypes,
			       struct i8x_note *src_note,
			       struct i8x_funcref **refp)
{
  struct i8x_listitem *li;
  struct i8x_funcref *ref;
  struct i8x_type *functype;
  size_t fullname_size;
  char *fullname;
  i8x_err_e err = I8X_OK;

  /* Build the full name.  */
  fullname_size = (strlen (provider)
		   + 2   /* "::"  */
		   + strlen (name)
		   + 1   /* '('  */
		   + strlen (ptypes)
		   + 1   /* ')'  */
		   + strlen (rtypes)
		   + 1); /* '\0'  */
  fullname = malloc (fullname_size);
  if (fullname == NULL)
    return i8x_out_of_memory (ctx);

  snprintf (fullname, fullname_size,
	    "%s::%s(%s)%s", provider, name, ptypes, rtypes);

  /* If we have this reference already then return it.  */
  i8x_list_foreach (ctx->funcrefs, li)
    {
      ref = i8x_listitem_get_funcref (li);

      if (strcmp (i8x_funcref_get_fullname (ref), fullname) == 0)
	{
	  *refp = i8x_funcref_ref (ref);

	  goto cleanup;
	}
    }

  /* It's a new reference that needs creating.  */
  err = i8x_ctx_get_functype (ctx,
			      ptypes, ptypes + strlen (ptypes),
			      rtypes, rtypes + strlen (rtypes),
			      src_note, &functype);
  if (err != I8X_OK)
    goto cleanup;

  err = i8x_funcref_new (ctx, fullname, functype, &ref);
  i8x_type_unref (functype);
  if (err != I8X_OK)
    goto cleanup;

  err = i8x_list_append_funcref (ctx->funcrefs, ref);
  if (err != I8X_OK)
    {
      ref = i8x_funcref_unref (ref);

      goto cleanup;
    }

  *refp = ref;

 cleanup:
  free (fullname);

  return err;
}

I8X_EXPORT i8x_err_e
i8x_ctx_get_funcref (struct i8x_ctx *ctx, const char *provider,
		     const char *name, const char *ptypes,
		     const char *rtypes, struct i8x_funcref **refp)
{
  /* Do not allow clients to reference private functions.  */
  if (strncmp (name, "__", 2) == 0)
    return i8x_invalid_argument (ctx);

  return i8x_ctx_get_funcref_with_note (ctx, provider, name,
					ptypes, rtypes, NULL, refp);
}

void
i8x_ctx_forget_funcref (struct i8x_funcref *ref)
{
  struct i8x_ctx *ctx = i8x_funcref_get_ctx (ref);

  i8x_list_remove_funcref (ctx->funcrefs, ref);
}

i8x_err_e
i8x_ctx_get_symref (struct i8x_ctx *ctx, const char *name,
		    struct i8x_symref **refp)
{
  struct i8x_listitem *li;
  struct i8x_symref *ref;
  i8x_err_e err;

  /* If we have this reference already then return it.  */
  i8x_list_foreach (ctx->symrefs, li)
    {
      ref = i8x_listitem_get_symref (li);

      if (strcmp (i8x_symref_get_name (ref), name) == 0)
	{
	  *refp = i8x_symref_ref (ref);

	  return I8X_OK;
	}
    }

  /* It's a new reference that needs creating.  */
  err = i8x_symref_new (ctx, name, &ref);
  if (err != I8X_OK)
    return err;

  err = i8x_list_append_symref (ctx->symrefs, ref);
  if (err != I8X_OK)
    {
      ref = i8x_symref_unref (ref);

      return err;
    }

  *refp = ref;

  return I8X_OK;
}

void
i8x_ctx_forget_symref (struct i8x_symref *ref)
{
  struct i8x_ctx *ctx = i8x_symref_get_ctx (ref);

  i8x_list_remove_symref (ctx->symrefs, ref);
}

i8x_err_e
i8x_ctx_get_functype (struct i8x_ctx *ctx,
		      const char *ptypes_start,
		      const char *ptypes_limit,
		      const char *rtypes_start,
		      const char *rtypes_limit,
		      struct i8x_note *src_note,
		      struct i8x_type **typep)
{
  struct i8x_listitem *li;
  struct i8x_type *type;
  size_t ptypes_size = ptypes_limit - ptypes_start;
  size_t rtypes_size = rtypes_limit - rtypes_start;
  size_t encoded_size;
  char *encoded, *ptr;
  i8x_err_e err = I8X_OK;

  /* Build the encoded form.  */
  encoded_size = (1	/* I8_TYPE_FUNCTION  */
		  + rtypes_size
		  + 1   /* '('  */
		  + ptypes_size
		  + 1   /* ')'  */
		  + 1); /* '\0'  */
  ptr = encoded = malloc (encoded_size);
  if (encoded == NULL)
    return i8x_out_of_memory (ctx);

  *(ptr++) = I8_TYPE_FUNCTION;
  memcpy (ptr, rtypes_start, rtypes_size);
  ptr += rtypes_size;
  *(ptr++) = '(';
  memcpy (ptr, ptypes_start, ptypes_size);
  ptr += ptypes_size;
  *(ptr++) = ')';
  *(ptr++) = '\0';

  /* If we have this type already then return it.  */
  i8x_list_foreach (ctx->functypes, li)
    {
      type = i8x_listitem_get_type (li);

      if (strcmp (i8x_type_get_encoded (type), encoded) == 0)
	{
	  *typep = i8x_type_ref (type);

	  goto cleanup;
	}
    }

  /* It's a new type that needs creating.  */
  err = i8x_type_new_functype (ctx, encoded,
			       ptypes_start, ptypes_limit,
			       rtypes_start, rtypes_limit,
			       src_note, &type);
  if (err != I8X_OK)
    goto cleanup;

  err = i8x_list_append_type (ctx->functypes, type);
  if (err != I8X_OK)
    {
      type = i8x_type_unref (type);

      goto cleanup;
    }

  *typep = type;

 cleanup:
  free (encoded);

  return err;
}

void
i8x_ctx_forget_functype (struct i8x_type *type)
{
  struct i8x_ctx *ctx = i8x_type_get_ctx (type);

  i8x_list_remove_type (ctx->functypes, type);
}

static void
i8x_ctx_resolve_funcrefs (struct i8x_ctx *ctx)
{
  struct i8x_listitem *li;
  bool finished = false;

  /* Mark all function references as resolved or not based
     on whether they resolve to a unique registered function.
     Dependencies are ignored at this stage.  */
  i8x_list_foreach (ctx->funcrefs, li)
    {
      struct i8x_funcref *ref = i8x_listitem_get_funcref (li);

      i8x_funcref_reset_is_resolved (ref);
    }

  /* Mark functions unresolved if any of their dependencies
     are unresolved.  Repeat until nothing changes.  */
  while (!finished)
    {
      finished = true;

      i8x_list_foreach (ctx->functions, li)
	{
	  struct i8x_func *func = i8x_listitem_get_func (li);
	  struct i8x_funcref *ref = i8x_func_get_funcref (func);

	  if (!i8x_funcref_is_resolved (ref))
	    continue;

	  if (i8x_func_all_deps_resolved (func))
	    continue;

	  i8x_funcref_mark_unresolved (ref);

	  finished = false;
	}
    }

  /* Notify the user of any function availability changes.  */
  i8x_list_foreach (ctx->functions, li)
    {
      struct i8x_func *func = i8x_listitem_get_func (li);

      i8x_func_fire_availability_observers (func);
    }
}

I8X_EXPORT i8x_err_e
i8x_ctx_register_func (struct i8x_ctx *ctx, struct i8x_func *func)
{
  i8x_err_e err;

  dbg (ctx, "registering func %p\n", func);
  i8x_assert (i8x_func_get_ctx (func) == ctx);

  err = i8x_list_append_func (ctx->functions, func);
  if (err != I8X_OK)
    return err;

  i8x_funcref_register_func (i8x_func_get_funcref (func), func);
  i8x_ctx_resolve_funcrefs (ctx);

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_ctx_unregister_func (struct i8x_ctx *ctx, struct i8x_func *func)
{
  dbg (ctx, "unregistering func %p\n", func);
  i8x_assert (i8x_func_get_ctx (func) == ctx);

  i8x_funcref_unregister_func (i8x_func_get_funcref (func), func);
  i8x_ctx_resolve_funcrefs (ctx);
  i8x_list_remove_func (ctx->functions, func);

  return I8X_OK;
}

/* convenience */

I8X_EXPORT i8x_err_e
i8x_ctx_register_native_func (struct i8x_ctx *ctx,
			      const char *provider, const char *name,
			      const char *ptypes, const char *rtypes,
			      i8x_nat_fn_t *impl_fn)
{
  struct i8x_funcref *sig;
  struct i8x_func *func;
  i8x_err_e err;

  err = i8x_ctx_get_funcref (ctx, provider, name, ptypes, rtypes, &sig);
  if (err != I8X_OK)
    return err;

  err = i8x_func_new_native (ctx, sig, impl_fn, &func);
  i8x_funcref_unref (sig);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_register_func (ctx, func);
  i8x_func_unref (func);

  return err;
}

/* convenience */
/* note it can stop mid-way! */

I8X_EXPORT i8x_err_e
i8x_ctx_register_native_funcs (struct i8x_ctx *ctx,
			       const struct i8x_native_fn *table)
{
  i8x_err_e err = I8X_OK;

  while (table->provider != NULL)
    {
      err = i8x_ctx_register_native_func (ctx,
					  table->provider,
					  table->name,
					  table->encoded_ptypes,
					  table->encoded_rtypes,
					  table->impl_fn);
      if (err != I8X_OK)
	break;

      table++;
    }

  return err;
}

static i8x_err_e
i8x_ctx_make_dispatch_table (struct i8x_ctx *ctx, size_t table_size,
			     bool is_debug, void ***tablep)
{
  void **table;
  i8x_err_e err;

  table = calloc (table_size, sizeof (void *));
  if (table == NULL)
    return i8x_out_of_memory (ctx);

  err = i8x_ctx_init_dispatch_table (ctx, table, table_size, is_debug);
  if (err != I8X_OK)
    return err;

  *tablep = table;

  return I8X_OK;
}

i8x_err_e
i8x_ctx_get_dispatch_tables (struct i8x_ctx *ctx,
			     void ***dispatch_std,
			     void ***dispatch_dbg)
{
  size_t table_size = i8x_ctx_get_dispatch_table_size (ctx);
  i8x_err_e err;

  if (ctx->dispatch_std == NULL)
    {
      err = i8x_ctx_make_dispatch_table (ctx, table_size, false,
					 &ctx->dispatch_std);
      if (err != I8X_OK)
	return err;
    }

  if (ctx->dispatch_dbg == NULL)
    {
      err = i8x_ctx_make_dispatch_table (ctx, table_size, true,
					 &ctx->dispatch_dbg);
      if (err != I8X_OK)
	return err;
    }

  *dispatch_std = ctx->dispatch_std;
  *dispatch_dbg = ctx->dispatch_dbg;

  return I8X_OK;
}
