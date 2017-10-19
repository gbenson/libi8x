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

#include <libi8x-test.h>

#include <thread_db.h>
#include <proc_service.h>

#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>

static const char *CORPUS = "corpus/glibc/0.00-4b";

/* Return the PID of the process.  */

pid_t
ps_getpid (struct ps_prochandle *ph)
{
  return 12345;
}

/* Read process memory at the given address.  */

ps_err_e
ps_pdread (struct ps_prochandle *ph, psaddr_t src, void *dst, size_t len)
{
  memset (dst, 0, len);

  return PS_OK;
}

/* Relocate an address in an Infinity note.  */

static ps_err_e
reloc_cb (void *rf_arg, psaddr_t unrelocated, psaddr_t *result_p)
{
  *result_p = unrelocated;

  return PS_OK;
}

/* Look up the named symbol in the named DSO in the symbol tables
   associated with the process being debugged, filling in *SYM_ADDR
   with the corresponding run-time address.  */

ps_err_e
ps_pglobal_lookup (struct ps_prochandle *ph, const char *object_name,
		   const char *sym_name, psaddr_t *sym_addr)
{
  SHOULD_NOT_CALL ();
}

/* Get the given LWP's general register set.  */

ps_err_e
ps_lgetregs (struct ps_prochandle *ph, lwpid_t lwpid, prgregset_t regs)
{
  SHOULD_NOT_CALL ();
}

struct ps_prochandle
{
  /* Architecture whose corpus notes we are using.  */
  const char *arch;
};

/* Call the callback CB for each Infinity note in the process.  The
   callback should return PS_OK to indicate that iteration should
   continue, or any other value to indicate that iteration should stop
   and that ps_foreach_infinity_note should return the non-PS_OK value
   that the callback returned.  Return PS_OK if the callback returned
   PS_OK for all Infinity notes, or if there are no Infinity notes in
   the process.  */

ps_err_e
ps_foreach_infinity_note (struct ps_prochandle *ph,
			  ps_visit_infinity_note_f *cb, void *cb_arg)
{
  CHECK (ph->arch != NULL);
  char *dirname;
  int error = asprintf (&dirname, "%s/%s", i8x_test_srcfile (CORPUS),
			ph->arch);
  CHECK (error > 0);

  DIR *dirp = opendir (i8x_test_srcfile (dirname));
  CHECK (dirp != NULL);

  ps_err_e result = PS_OK;
  while (result == PS_OK)
    {
      CHECK (errno == 0);
      struct dirent *de = readdir (dirp);
      CHECK (errno == 0);
      if (de == NULL)
	break;

      if (de->d_name[0] == '.')
	continue;

      char *filename;
      error = asprintf (&filename, "%s/%s", dirname, de->d_name);
      CHECK (error > 0);

      struct i8x_sized_buf buf;
      i8x_test_mmap (filename, &buf);

      result = cb (cb_arg,
		   buf.ptr, buf.size, filename, 0,
		   reloc_cb, NULL);

      i8x_test_munmap (&buf);

      free (filename);
    }

  closedir (dirp);
  free (dirname);

  return result;
}

static void
i8x_libthread_db_test (const char *arch)
{
  fprintf (stderr, "Testing %s:\n", arch);

  struct ps_prochandle _ph, *ph = &_ph;

  ph->arch = strdup (arch);
  CHECK (ph->arch != NULL);

  td_thragent_t *ta;
  CHECK (td_ta_new (ph, &ta) == TD_OK);

  CHECK (td_ta_delete (ta) == TD_OK);
}

int
main (int argc, char *argv[])
{
  CHECK (td_init () == TD_OK);

  DIR *dirp = opendir (i8x_test_srcfile (CORPUS));
  CHECK (dirp != NULL);

  while (true)
    {
      CHECK (errno == 0);
      struct dirent *de = readdir (dirp);
      CHECK (errno == 0);
      if (de == NULL)
	break;

      if (de->d_name[0] == '.' || strcmp (de->d_name, "README") == 0)
	continue;

      i8x_libthread_db_test (de->d_name);
    }

  closedir (dirp);

  exit (EXIT_SUCCESS);
}
