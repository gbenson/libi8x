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

#include <i8x/libi8x.h>
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
  struct i8x_list *symrefs;	/* List of interned symbol references.  */

  struct i8x_func *first_func;  /* Linked list of registered functions.  */
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

static int
log_priority (const char *priority)
{
  char *endptr;
  int prio;

  prio = strtol (priority, &endptr, 10);
  if (endptr[0] == '\0' || isspace (endptr[0]))
    return prio;
  if (strncmp (priority, "err", 3) == 0)
    return LOG_ERR;
  if (strncmp (priority, "info", 4) == 0)
    return LOG_INFO;
  if (strncmp (priority, "debug", 5) == 0)
    return LOG_DEBUG;

  return 0;
}

static i8x_err_e
i8x_ctx_init (struct i8x_ctx *ctx)
{
  i8x_err_e err;

  err = i8x_list_new (ctx, false, &ctx->funcrefs);
  if (err != I8X_OK)
    return err;

  err = i8x_list_new (ctx, false, &ctx->symrefs);
  if (err != I8X_OK)
    return err;

  return err;
}

static void
i8x_ctx_unlink (struct i8x_object *ob)
{
  struct i8x_ctx *ctx = (struct i8x_ctx *) ob;

  ctx->error_note = i8x_note_unref (ctx->error_note);
  ctx->first_func = i8x_func_unref (ctx->first_func);

  ctx->funcrefs = i8x_list_unref (ctx->funcrefs);
  ctx->symrefs = i8x_list_unref (ctx->symrefs);
}

const struct i8x_object_ops i8x_ctx_ops =
  {
    "ctx",			/* Object name.  */
    sizeof (struct i8x_ctx),	/* Object size.  */
    i8x_ctx_unlink,		/* Unlink function.  */
    NULL,			/* Free function.  */
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

  /* environment overwrites config */
  env = secure_getenv ("I8X_LOG");
  if (env != NULL)
    i8x_ctx_set_log_priority (c, log_priority (env));

  info (c, "ctx %p created\n", c);
  dbg (c, "log_priority=%d\n", c->log_priority);

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
  info (ctx, "custom logging function %p registered\n", log_fn);
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
      return _("no error");

    case I8X_OUT_OF_MEMORY:
      return _("out of memory");

    case I8X_NOTE_CORRUPT:
      return _("corrupt note");

    case I8X_NOTE_UNHANDLED:
      return _("unhandled note");

    case I8X_NOTE_INVALID:
      return _("invalid note");

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

I8X_EXPORT i8x_err_e
i8x_ctx_get_funcref (struct i8x_ctx *ctx, const char *provider,
		     const char *name, const char *ptypes,
		     const char *rtypes, struct i8x_funcref **refp)
{
  struct i8x_funcref *ref;
  size_t fullname_size;
  char *fullname;
  i8x_err_e err;

  /* Build the full name.  */
  fullname_size = (strlen (provider)
		   + 2   /* "::"  */
		   + strlen (name)
		   + 1   /* '('  */
		   + strlen (ptypes)
		   + 1   /* ')'  */
		   + strlen (rtypes)
		   + 1); /* '\0'  */
  fullname = alloca (fullname_size);
  snprintf (fullname, fullname_size,
	    "%s::%s(%s)%s", provider, name, ptypes, rtypes);

  /* If we have this reference already then return it.  */
  i8x_funcref_list_foreach (ref, ctx->funcrefs)
    {
      if (strcmp (i8x_funcref_get_fullname (ref), fullname) == 0)
	{
	  *refp = i8x_funcref_ref (ref);

	  return I8X_OK;
	}
    }

  /* It's a new reference that needs creating.  */
  err = i8x_funcref_new (ctx, fullname, ptypes, rtypes, &ref);
  if (err != I8X_OK)
    return err;

  i8x_funcref_list_append (ctx->funcrefs, ref);

  *refp = ref;

  return I8X_OK;
}

void
i8x_ctx_forget_funcref (struct i8x_funcref *ref)
{
  struct i8x_ctx *ctx = i8x_funcref_get_ctx (ref);

  i8x_funcref_list_remove (ctx->funcrefs, ref);
}

i8x_err_e
i8x_ctx_get_symref (struct i8x_ctx *ctx, const char *name,
		    struct i8x_symref **refp)
{
  struct i8x_symref *ref;
  i8x_err_e err;

  /* If we have this reference already then return it.  */
  i8x_symref_list_foreach (ref, ctx->symrefs)
    {
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

  i8x_symref_list_append (ctx->symrefs, ref);

  *refp = ref;

  return I8X_OK;
}

void
i8x_ctx_forget_symref (struct i8x_symref *ref)
{
  struct i8x_ctx *ctx = i8x_symref_get_ctx (ref);

  i8x_symref_list_remove (ctx->symrefs, ref);
}

I8X_EXPORT i8x_err_e
i8x_ctx_register_func (struct i8x_ctx *ctx, struct i8x_func *func)
{
  i8x_assert (i8x_func_get_ctx(func) == ctx);

  dbg (ctx, "registering %s\n", i8x_func_get_fullname (func));

  i8x_func_list_add (&ctx->first_func, func);

  // XXX lots TODO here

  return I8X_OK;
}

I8X_EXPORT i8x_err_e
i8x_ctx_unregister_func (struct i8x_ctx *ctx, struct i8x_func *func)
{
  i8x_assert (i8x_func_get_ctx(func) == ctx);

  dbg (ctx, "unregistering %s\n", i8x_func_get_fullname (func));

  i8x_func_list_remove (&ctx->first_func, func);

  // XXX lots TODO here

  return I8X_OK;
}
