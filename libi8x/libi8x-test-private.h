/* Copyright (C) 2017 Red Hat, Inc.
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

#ifndef _LIBI8X_TEST_PRIVATE_H_
#define _LIBI8X_TEST_PRIVATE_H_

#include <libi8x.h>

/* Functions used by testcases, but not part of the public API.
   These functions will likely be exported when direct Infinity
   support is added to GDB, but are not useful while support is
   via glibc's libthread_db.  They should all be LIBI8X_PRIVATE
   in libi8x.sym.  */

#define i8x_ctx_strerror_r	__i8xtest_ctx_strerror_r
#define i8x_func_get_externals	__i8xtest_func_get_externals
#define i8x_func_new_bytecode	__i8xtest_func_new_bytecode
#define i8x_func_new_native	__i8xtest_func_new_native
#define i8x_func_register	__i8xtest_func_register
#define i8x_note_new		__i8xtest_note_new
#define i8x_type_get_encoded	__i8xtest_type_get_encoded

#ifdef __cplusplus
extern "C" {
#endif

const char *i8x_ctx_strerror_r (struct i8x_ctx *ctx, i8x_err_e code,
				char *buf, size_t bufsiz);
struct i8x_list *i8x_func_get_externals (struct i8x_func *func);
i8x_err_e i8x_func_new_bytecode (struct i8x_note *note,
				 struct i8x_func **func);
i8x_err_e i8x_func_new_native (struct i8x_ctx *ctx,
			       struct i8x_funcref *ref,
			       i8x_nat_fn_t *impl_fn,
			       struct i8x_func **func);
i8x_err_e i8x_func_register (struct i8x_func *func);
i8x_err_e i8x_note_new (struct i8x_ctx *ctx,
			const char *buf, size_t bufsiz,
			const char *srcname, ssize_t srcoffset,
			struct i8x_note **note);
const char *i8x_type_get_encoded (struct i8x_type *type);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _LIBI8X_PRIVATE_H_ */
