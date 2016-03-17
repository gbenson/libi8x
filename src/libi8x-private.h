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

#include <stdbool.h>
#include <syslog.h>

#include <i8x/libi8x.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline void __attribute__ ((always_inline, format (printf, 2, 3)))
i8x_log_null (struct i8x_ctx *ctx, const char *format, ...) {}

#define i8x_log_cond(ctx, prio, arg...) \
  do { \
    if (i8x_get_log_priority (ctx) >= prio) \
      i8x_log (ctx, prio, __FILE__, __LINE__, __FUNCTION__, ## arg); \
  } while (0)

#ifdef ENABLE_LOGGING
#  ifdef ENABLE_DEBUG
#    define dbg(ctx, arg...) i8x_log_cond (ctx, LOG_DEBUG, ## arg)
#  else
#    define dbg(ctx, arg...) i8x_log_null (ctx, ## arg)
#  endif
#  define info(ctx, arg...) i8x_log_cond (ctx, LOG_INFO, ## arg)
#  define err(ctx, arg...) i8x_log_cond (ctx, LOG_ERR, ## arg)
#else
#  define dbg(ctx, arg...) i8x_log_null (ctx, ## arg)
#  define info(ctx, arg...) i8x_log_null (ctx, ## arg)
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

void i8x_log (struct i8x_ctx *ctx,
	      int priority, const char *file, int line, const char *fn,
	      const char *format, ...)
  __attribute__ ((format (printf, 6, 7)));

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_PRIVATE_H_ */
