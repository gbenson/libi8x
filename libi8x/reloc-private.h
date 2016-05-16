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

#ifndef _RELOC_PRIVATE_H_
#define _RELOC_PRIVATE_H_

#include <libi8x.h>

/* Symbol references.  */

struct i8x_reloc
{
  I8X_OBJECT_FIELDS;

  uintptr_t unrelocated;	/* Unrelocated value.  */

  /* A very primitive cache, keyed by the inferior that looked up
     the value.  It's liable to frequently be invalid if you have
     more than one inferior, and it prevents i8x_xctx_call from
     being callable simultaneously from multiple threads.  */
  uintptr_t cached_value;
  struct i8x_inferior *cached_from;
};

#endif /* _RELOC_PRIVATE_H_ */
