/*
    libi8x - something with i8x

    Copyright (C) 2011 Someone <someone@example.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include <i8x/libi8x.h>

int main(int argc, char *argv[])
{
        struct i8x_ctx *ctx;
        struct i8x_thing *thing = NULL;
        int err;

        err = i8x_new(&ctx);
        if (err < 0)
                exit(EXIT_FAILURE);

        printf("version %s\n", VERSION);

        err = i8x_thing_new_from_string(ctx, "foo", &thing);
        if (err >= 0)
                i8x_thing_unref(thing);

        i8x_unref(ctx);
        return EXIT_SUCCESS;
}
