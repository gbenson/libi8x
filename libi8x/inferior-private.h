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

#ifndef _INFERIOR_PRIVATE_H_
#define _INFERIOR_PRIVATE_H_

#include <libi8x.h>

/* Inferior processes.  */

struct i8x_inferior
{
  I8X_OBJECT_FIELDS;

  i8x_relocate_fn_t *relocate_fn;
};

#endif /* _INFERIOR_PRIVATE_H_ */