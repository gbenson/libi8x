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

/* A very simple multithreaded program.  */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

static __thread int tlsvar = 23;

void *
thread_routine (void *arg)
{
  tlsvar -= 2;
  tlsvar *= 2;

  printf ("I am PID %d, press Return and I'll exit.\n", getpid ());
  getchar ();

  return NULL;
}

int
main (int argc, char *argv)
{
  pthread_t the_thread;

  if (pthread_create (&the_thread, NULL, thread_routine, NULL))
    {
      printf ("Error creating thread\n");
      exit (EXIT_FAILURE);
    }

  if (pthread_join (the_thread, NULL))
    {
      printf("Error joining thread\n");
      exit (EXIT_FAILURE);
    }

  exit (EXIT_SUCCESS);
}
