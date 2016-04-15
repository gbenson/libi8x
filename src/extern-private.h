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

#ifndef _EXTERN_PRIVATE_H_
#define _EXTERN_PRIVATE_H_

/* i8x_funcref and i8x_symref are exposed so the bytecode interpreter
   can access what it needs without making function calls.  All other
   access to these structures should be made via accessors.  */

#include "libi8x-private.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function references.  */

struct i8x_funcref
{
  I8X_OBJECT_FIELDS;

  char *fullname;	/* Fully qualified name.  */
  bool is_private;	/* Is this function API-private?  */
  struct i8x_type *type;	/* The function's type.  */

  int regcount;		/* Number of functions registered in this
			   context with this signature.  */

  /* Pointer to the exactly one function registered in the
     context with this signature, or NULL if there is not
     exactly one function registered with this signature.  */
  struct i8x_func *unique;

  /* Pointer to the function this reference resolves to, or
     NULL if this reference is not resolved.  For a reference
     to be resolved there needs to be exactly one function
     registered in the context with this signature, and every
     function that function references in its externals table
     must also be resolved.  */
  struct i8x_func *resolved;

  /* Interpreted and native implementations of the function
     this reference resolves to, or NULL if this reference
     is not resolved.  */
  struct i8x_code *interp_impl;
  i8x_nat_fn_t *native_impl;
};

/* Symbol references.  */

struct i8x_symref
{
  I8X_OBJECT_FIELDS;

  char *name;	/* The symbol's name.  */
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _EXTERN_PRIVATE_H_ */
