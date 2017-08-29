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

  struct i8x_note *error_note;	/* Note that caused the last error.  */
  const char *error_ptr;	/* Pointer into error_note.  */

  struct i8x_list *funcrefs;	/* List of interned function references.  */
  struct i8x_list *functypes;	/* List of interned function types.  */

  struct i8x_list *functions;	/* List of registered functions.  */

  /* User-supplied function called when a function becomes available.  */
  i8x_notify_fn_t *func_avail_observer_fn;

  /* User-supplied function called when a function is about to become
     unavailable.  */
  i8x_notify_fn_t *func_unavail_observer_fn;

  /* The three core types.  */
  struct i8x_type *integer_type;
  struct i8x_type *pointer_type;
  struct i8x_type *opaque_type;

  /* Special type internal to the validator.  */
  struct i8x_type *int_or_ptr_type;

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

struct i8x_logprio
{
  const char *name;
  int priority;
};

/* Lifted from syslog.h.  */
struct i8x_logprio prioritynames[] =
{
  { "alert", LOG_ALERT },
  { "crit", LOG_CRIT },
  { "debug", LOG_DEBUG },
  { "emerg", LOG_EMERG },
  { "err", LOG_ERR },
  { "info", LOG_INFO },
  { "notice", LOG_NOTICE },
  { "panic", LOG_EMERG },
  { "trace", LOG_TRACE },
  { "warn", LOG_WARNING },
  { NULL, -1 }
};

static int
strtoprio (const char *str)
{
  struct i8x_logprio *pn;
  char *endptr;
  int numeric;

  numeric = strtol (str, &endptr, 10);
  if (endptr[0] == '\0' || isspace (endptr[0]))
    return numeric;

  for (pn = prioritynames; pn->name != NULL; pn++)
    {
      if (strncasecmp (str, pn->name, strlen (pn->name)) == 0)
	return pn->priority;
    }

  return 0;
}

static bool
strtobool (const char *str)
{
  char *endptr;
  int numeric;

  numeric = strtol (str, &endptr, 10);
  if (endptr[0] == '\0' || isspace (endptr[0]))
    return numeric != 0;

  if (strncasecmp (str, "yes", 3) == 0)
    return true;
  if (strncasecmp (str, "true", 4) == 0)
    return true;

  return false;
}

static i8x_err_e
i8x_ctx_make_dispatch_table (struct i8x_ctx *ctx, bool is_debug,
			     void ***tablep)
{
  size_t table_size = i8x_ctx_get_dispatch_table_size (ctx);
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

  err = i8x_type_new_coretype (ctx, I8X_TYPE_INTPTR, &ctx->int_or_ptr_type);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_make_dispatch_table (ctx, false, &ctx->dispatch_std);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_make_dispatch_table (ctx, true, &ctx->dispatch_dbg);
  if (err != I8X_OK)
    return err;

  return err;
}

static void
i8x_ctx_unlink (struct i8x_object *ob)
{
  struct i8x_ctx *ctx = (struct i8x_ctx *) ob;

  i8x_ctx_clear_last_error (ctx);

  ctx->functions = i8x_list_unref (ctx->functions);

  ctx->funcrefs = i8x_list_unref (ctx->funcrefs);
  ctx->functypes = i8x_list_unref (ctx->functypes);

  ctx->integer_type = i8x_type_unref (ctx->integer_type);
  ctx->pointer_type = i8x_type_unref (ctx->pointer_type);
  ctx->opaque_type = i8x_type_unref (ctx->opaque_type);

  ctx->int_or_ptr_type = i8x_type_unref (ctx->int_or_ptr_type);
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

static const struct i8x_object_ops i8x_ctx_ops =
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
i8x_ctx_new (int flags, i8x_log_fn_t *log_fn, struct i8x_ctx **ctx)
{
  const char *env;
  struct i8x_ctx *c;
  i8x_err_e err;

  err = i8x_ob_new (NULL, &i8x_ctx_ops, &c);
  if (err != I8X_OK)
    return err;

  /* Set up logging at the earliest opportunity.  */
  if (log_fn != NULL)
    c->log_fn = log_fn;
  else
    c->log_fn = log_stderr;

  if (flags & I8X_LOG_TRACE)
    c->log_priority = LOG_TRACE;
  else
    c->log_priority = LOG_PRI (flags);

  env = secure_getenv ("I8X_LOG");
  if (env != NULL)
    i8x_ctx_set_log_priority (c, strtoprio (env));

  /* Announce ourselves.  */
  dbg (c, "using %s\n", PACKAGE_STRING);

  /* Now log the message i8x_ob_new deferred to us.  */
  dbg (c, "ctx %p created\n", c);

  if (flags & I8X_DBG_MEM)
    c->_ob.use_debug_allocator = true;

  env = secure_getenv ("I8X_DBG_MEM");
  if (env != NULL && strtobool (env))
    c->_ob.use_debug_allocator = true;

  dbg (c, "use_debug_allocator=%d\n", c->_ob.use_debug_allocator);

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

I8X_EXPORT void
i8x_ctx_set_func_available_cb (struct i8x_ctx *ctx,
			       i8x_notify_fn_t *func_avail_cb_fn)
{
  ctx->func_avail_observer_fn = func_avail_cb_fn;
}

I8X_EXPORT void
i8x_ctx_set_func_unavailable_cb (struct i8x_ctx *ctx,
				 i8x_notify_fn_t *func_unavail_cb_fn)
{
  ctx->func_unavail_observer_fn = func_unavail_cb_fn;
}

void
i8x_ctx_fire_availability_observer (struct i8x_func *func,
				    bool is_available)
{
  struct i8x_ctx *ctx = i8x_func_get_ctx (func);

  info (ctx, "%s became %s\n", i8x_func_get_signature (func),
	is_available ? "available" : "unavailable");

  i8x_notify_fn_t *callback = is_available
    ? ctx->func_avail_observer_fn
    : ctx->func_unavail_observer_fn;

  if (callback != NULL)
    callback (func);
}

I8X_EXPORT void
i8x_ctx_clear_last_error (struct i8x_ctx *ctx)
{
  ctx->error_note = i8x_note_unref (ctx->error_note);
}

i8x_err_e
i8x_ctx_set_error (struct i8x_ctx *ctx, i8x_err_e code,
		   struct i8x_note *cause_note, const char *cause_ptr)
{
  i8x_assert (code != I8X_OK);

  if (ctx != NULL)
    {
      i8x_ctx_clear_last_error (ctx);

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

    case I8X_UNRESOLVED_FUNCTION:
      return _("Unresolved function");

    case I8X_STACK_OVERFLOW:
      return _("Stack overflow");

    case I8X_RELOC_FAILED:
      return _("Relocation failed");

    case I8X_READ_MEM_FAILED:
      return _("Read memory failed");

    case I8X_DIVIDE_BY_ZERO:
      return _("Division by zero");

    case I8X_NATCALL_FAILED:
      return _("Native call failed");

    case I8X_NATCALL_BAD_FUNCREF_RET:
      return _("Native call returned invalid function reference");

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
i8x_strerror_r (i8x_err_e code, char *buf, size_t bufsiz)
{
  char *ptr = buf;
  char *limit = ptr + bufsiz;
  const char *msg = error_message_for (code);

  if (msg == NULL)
    xsnprintf (&ptr, limit, _("Unknown error %d"), code);
  else
    xsnprintf (&ptr, limit, "%s", msg);

  return buf;
}

I8X_EXPORT const char *
i8x_ctx_get_last_error_src_name (struct i8x_ctx *ctx)
{
  if (ctx == NULL || ctx->error_note == NULL)
    return NULL;

  return i8x_note_get_src_name (ctx->error_note);
}

I8X_EXPORT ssize_t
i8x_ctx_get_last_error_src_offset (struct i8x_ctx *ctx)
{
  if (ctx == NULL || ctx->error_note == NULL)
    return -1;

  ssize_t offset = i8x_note_get_src_offset (ctx->error_note);

  if (offset >= 0 && ctx->error_ptr != NULL)
    offset += ctx->error_ptr - i8x_note_get_encoded (ctx->error_note);

  return offset;
}

I8X_EXPORT const char *
i8x_ctx_strerror_r (struct i8x_ctx *ctx, i8x_err_e code,
		    char *buf, size_t bufsiz)
{
  char *ptr = buf;
  char *limit = ptr + bufsiz;
  const char *prefix = i8x_ctx_get_last_error_src_name (ctx);
  ssize_t offset = i8x_ctx_get_last_error_src_offset (ctx);

  if (prefix == NULL)
    prefix = PACKAGE;

  xsnprintf (&ptr, limit, "%s", prefix);

  if (offset >= 0)
    xsnprintf (&ptr, limit, "[" LHEX "]", offset);

  xsnprintf (&ptr, limit, ": ");

  i8x_strerror_r (code, ptr, limit - ptr);

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

struct i8x_type *
i8x_ctx_get_int_or_ptr_type (struct i8x_ctx *ctx)
{
  return ctx->int_or_ptr_type;
}

void
i8x_ctx_get_dispatch_tables (struct i8x_ctx *ctx,
			     void ***dispatch_std,
			     void ***dispatch_dbg)
{
  *dispatch_std = ctx->dispatch_std;
  *dispatch_dbg = ctx->dispatch_dbg;
}

/* Check the provider or name for a function reference.  */

static i8x_err_e
check_funcref_namepart (struct i8x_ctx *ctx, const char *c,
			struct i8x_note *src_note, const char *optr,
			bool zero_length_ok)
{
  const char *limit = c + strlen (c);

  /* Zero-length is invalid for global functions.  */
  if (!zero_length_ok && c == limit)
    return i8x_funcref_error (ctx, I8X_NOTE_INVALID, src_note, optr);

  /* First character cannot be numeric.  */
  if (isdigit (*c))
    return i8x_funcref_error (ctx, I8X_NOTE_INVALID, src_note, c);

  /* Remaining characters must be in [A-Za-z0-9_].  */
  while (c < limit)
    {
      if (*c != '_' && !isalnum (*c))
	return i8x_funcref_error (ctx, I8X_NOTE_INVALID, src_note, c);

      c++;
    }

  return I8X_OK;
}

/* Internal version of i8x_ctx_get_funcref with an extra source note
   argument for error-reporting.  If the source note is not NULL then
   all string arguments must be pointers into the note's buffer or any
   errors set will have nonsense offsets.  */

i8x_err_e
i8x_ctx_get_funcref_with_note (struct i8x_ctx *ctx,
			       const char *provider, const char *name,
			       const char *ptypes, const char *rtypes,
			       struct i8x_note *src_note,
			       const char *prov_off_ptr,
			       const char *name_off_ptr,
			       struct i8x_funcref **refp)
{
  bool is_local = src_note == NULL && *provider == '\0';
  struct i8x_listitem *li;
  struct i8x_funcref *ref;
  struct i8x_type *functype;
  size_t signature_size;
  char *signature;
  i8x_err_e err;

  /* Ensure the provider and name are valid.  */
  err = check_funcref_namepart (ctx, provider, src_note,
				prov_off_ptr, is_local);
  if (err == I8X_OK)
    err = check_funcref_namepart (ctx, name, src_note,
				  name_off_ptr, is_local);
  if (err != I8X_OK)
    return err;

  /* Build the full name.  */
  signature_size = (strlen (provider)
		    + 2   /* "::"  */
		    + strlen (name)
		    + 1   /* '('  */
		    + strlen (ptypes)
		    + 1   /* ')'  */
		    + strlen (rtypes)
		    + 1); /* '\0'  */
  signature = malloc (signature_size);
  if (signature == NULL)
    return i8x_out_of_memory (ctx);

  snprintf (signature, signature_size,
	    "%s::%s(%s)%s", provider, name, ptypes, rtypes);

  /* Global function references are interned.  */
  if (!is_local)
    {
      i8x_list_foreach (ctx->funcrefs, li)
	{
	  ref = i8x_listitem_get_funcref (li);

	  if (strcmp (i8x_funcref_get_signature (ref), signature) == 0)
	    {
	      *refp = i8x_funcref_ref (ref);

	      goto cleanup;
	    }
	}
    }

  /* It's a new reference that needs creating.  */
  err = i8x_ctx_get_functype (ctx,
			      ptypes, ptypes + strlen (ptypes),
			      rtypes, rtypes + strlen (rtypes),
			      src_note, &functype);
  if (err != I8X_OK)
    goto cleanup;

  err = i8x_funcref_new (ctx, signature, functype, &ref);
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
  free (signature);

  return err;
}

/* Parse a function signature, storing the components in provider_p,
   name_p, ptypes_p and rtypes_p.  Note that the signature argument
   is modified.  Note also that very little checking is done; the
   caller is responsible for ensuring the returned components are
   valid.  */

static i8x_err_e
i8x_ctx_parse_signature (struct i8x_ctx *ctx, char *signature,
			 const char **provider_p, const char **name_p,
			 const char **ptypes_p, const char **rtypes_p)
{
  char *provider = signature;

  char *name = strstr (provider, "::");
  if (name == NULL)
    return i8x_invalid_argument (ctx);

  *name = '\0';
  name += 2;

  char *ptypes = strchr (name, '(');
  if (ptypes == NULL)
    return i8x_invalid_argument (ctx);

  *(ptypes++) = '\0';

  char *rtypes;
  i8x_err_e err = i8x_type_list_skip_to (ctx, ptypes,
					 ptypes + strlen (ptypes),
					 NULL, ')',
					 (const char **) &rtypes);
  if (err != I8X_OK)
    return err;

  *(rtypes++) = '\0';

  *provider_p = provider;
  *name_p = name;
  *ptypes_p = ptypes;
  *rtypes_p = rtypes;

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_ctx_get_funcref (struct i8x_ctx *ctx, const char *signature,
		     struct i8x_funcref **refp)
{
  const char *provider, *name, *ptypes, *rtypes;
  char *buf;
  i8x_err_e err;

  buf = strdup (signature);
  if (buf == NULL)
    return i8x_out_of_memory (ctx);

  err = i8x_ctx_parse_signature (ctx, buf, &provider, &name,
				 &ptypes, &rtypes);

  if (err == I8X_OK)
    err = i8x_ctx_get_funcref_with_note (ctx, provider, name,
					 ptypes, rtypes, NULL,
					 NULL, NULL, refp);
  free (buf);

  return err;
}

void
i8x_ctx_forget_funcref (struct i8x_funcref *ref)
{
  struct i8x_ctx *ctx = i8x_funcref_get_ctx (ref);

  i8x_list_remove_funcref (ctx->funcrefs, ref);
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

  i8x_func_register (func);
  i8x_ctx_resolve_funcrefs (ctx);

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_ctx_unregister_func (struct i8x_ctx *ctx, struct i8x_func *func)
{
  dbg (ctx, "unregistering func %p\n", func);
  i8x_assert (i8x_func_get_ctx (func) == ctx);

  struct i8x_listitem *li
    = i8x_list_get_item (ctx->functions, (struct i8x_object *) func);
  if (li == NULL)
    return i8x_invalid_argument (ctx);

  func = i8x_func_ref (func);
  i8x_listitem_remove (li);

  i8x_func_unregister (func);
  i8x_ctx_resolve_funcrefs (ctx);

  func = i8x_func_unref (func);

  return I8X_OK;
}

/* convenience */

I8X_EXPORT i8x_err_e
i8x_ctx_import_bytecode (struct i8x_ctx *ctx,
			 const char *buf, size_t bufsiz,
			 const char *srcname, ssize_t srcoffset,
			 struct i8x_func **func)
{
  struct i8x_note *note;
  struct i8x_func *f;
  i8x_err_e err;

  err = i8x_note_new (ctx, buf, bufsiz, srcname, srcoffset, &note);
  if (err != I8X_OK)
    return err;

  err = i8x_func_new_bytecode (note, &f);
  note = i8x_note_unref (note);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_register_func (ctx, f);
  if (err == I8X_OK && func != NULL)
    *func = f;
  else
    f = i8x_func_unref (f);

  return err;
}

/* convenience */

I8X_EXPORT i8x_err_e
i8x_ctx_import_native (struct i8x_ctx *ctx, const char *signature,
		       i8x_nat_fn_t *impl_fn, struct i8x_func **func)
{
  struct i8x_funcref *sig;
  struct i8x_func *f;
  i8x_err_e err;

  err = i8x_ctx_get_funcref (ctx, signature, &sig);
  if (err != I8X_OK)
    return err;

  if (!i8x_funcref_is_global (sig) && func == NULL)
    err = i8x_invalid_argument (ctx);
  else
    err = i8x_func_new_native (ctx, sig, impl_fn, &f);
  sig = i8x_funcref_unref (sig);
  if (err != I8X_OK)
    return err;

  err = i8x_ctx_register_func (ctx, f);
  if (err == I8X_OK && func != NULL)
    *func = f;
  else
    f = i8x_func_unref (f);

  return err;
}

I8X_EXPORT void
i8x_inf_invalidate_relocs (struct i8x_inf *inf)
{
  struct i8x_ctx *ctx = i8x_inf_get_ctx (inf);
  struct i8x_listitem *li;

  i8x_list_foreach (ctx->functions, li)
    {
      struct i8x_func *func = i8x_listitem_get_func (li);
      struct i8x_list *relocs = i8x_func_get_relocs (func);
      struct i8x_listitem *lj;

      i8x_list_foreach (relocs, lj)
	{
	  struct i8x_reloc *reloc = i8x_listitem_get_reloc (lj);

	  i8x_reloc_invalidate_for_inferior (reloc, inf);
	}
    }
}

I8X_EXPORT struct i8x_list *
i8x_ctx_get_functions (struct i8x_ctx *ctx)
{
  return ctx->functions;
}
