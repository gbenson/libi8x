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
  int refcount;
  i8x_log_fn_t *log_fn;
  void *userdata;
  int log_priority;
  struct i8x_note *error_note;	/* Note that caused the last error.  */
  const char *error_ptr;	/* Pointer into error_note.  */
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

/**
 * i8x_ctx_get_userdata:
 * @ctx: i8x library context
 *
 * Retrieve stored data pointer from library context. This might be
 * useful to access from callbacks like a custom logging function.
 *
 * Returns: stored userdata
 **/
I8X_EXPORT void *
i8x_ctx_get_userdata (struct i8x_ctx *ctx)
{
  return ctx->userdata;
}

/**
 * i8x_ctx_set_userdata:
 * @ctx: i8x library context
 * @userdata: data pointer
 *
 * Store custom @userdata in the library context.
 **/
I8X_EXPORT void
i8x_ctx_set_userdata (struct i8x_ctx *ctx, void *userdata)
{
  ctx->userdata = userdata;
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

  c = calloc (1, sizeof (struct i8x_ctx));
  if (c == NULL)
    return i8x_out_of_memory (NULL);

  c->refcount = 1;
  c->log_fn = log_stderr;
  c->log_priority = LOG_ERR;

  /* environment overwrites config */
  env = secure_getenv ("I8X_LOG");
  if (env != NULL)
    i8x_ctx_set_log_priority (c, log_priority (env));

  info (c, "ctx %p created\n", c);
  dbg (c, "log_priority=%d\n", c->log_priority);
  *ctx = c;

  return I8X_OK;
}

/**
 * i8x_ctx_ref:
 * @ctx: i8x library context
 *
 * Take a reference of the i8x library context.
 *
 * Returns: the passed i8x library context
 **/
I8X_EXPORT struct i8x_ctx *
i8x_ctx_ref (struct i8x_ctx *ctx)
{
  if (ctx == NULL)
    return NULL;

  ctx->refcount++;

  return ctx;
}

/**
 * i8x_ctx_unref:
 * @ctx: i8x library context
 *
 * Drop a reference of the i8x library context.
 *
 **/
I8X_EXPORT struct i8x_ctx *
i8x_ctx_unref (struct i8x_ctx *ctx)
{
  if (ctx == NULL)
    return NULL;

  ctx->refcount--;
  if (ctx->refcount > 0)
    return NULL;

  info (ctx, "context %p released\n", ctx);

  i8x_note_unref (ctx->error_note);
  free (ctx);

  return NULL;
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
      i8x_note_unref (ctx->error_note);
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
