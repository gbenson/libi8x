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

#include <libi8x.h>

#include <thread_db.h>
#include "proc_service.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <libelf.h>
#include <gelf.h>
#include <gnu/lib-names.h>
#include <link.h>

#ifndef NT_GNU_INFINITY
# define NT_GNU_INFINITY 5
#endif

static td_err_e td_ta_init (td_thragent_t *ta);
static td_err_e td_ta_init_libi8x (td_thragent_t *ta);
static i8x_err_e td_ta_thr_iter_cb (struct i8x_xctx *xctx,
				    struct i8x_inf *inf,
				    union i8x_value *args,
				    union i8x_value *rets);

/* Handle for a process, created by td_ta_new and passed as
   the first argument to all other libthread_db functions.  */

struct td_thragent
{
  /* Handle for a process supplied by the debugger to be
     passed to the as the first argument to the various
     callbacks defined in proc_service.h.  */
  struct ps_prochandle *ph;

  /* Process ID of the process we're accessing.  */
  pid_t pid;

  /* Main executable filename.  */
  char exec_filename[32];

  /* Main executable ELF header.  */
  GElf_Ehdr exec_ehdr_mem;
  GElf_Ehdr *exec_ehdr;

  /* libi8x context.  */
  struct i8x_ctx *ctx;

  /* The process we are accessing.  */
  struct i8x_inf *inf;

  /* Execution context.  */
  struct i8x_xctx *xctx;

  /* References to functions we use.  */
  struct i8x_funcref *map_lwp2thr;
  struct i8x_funcref *thr_iter;
  struct i8x_funcref *thr_get_lwpid;
  struct i8x_funcref *thr_get_state;

  /* Callback for td_ta_thr_iter.  */
  struct i8x_funcref *thr_iter_cb;
  td_thr_iter_f *thr_iter_cb_impl;
};

/* Callback for td_ta_self_test.  */

static int
td_ta_selftest_cb (const td_thrhandle_t *th, void *arg)
{
  union i8x_value args[1], rets[2];
  i8x_err_e err;

  fputs ("   ", stderr);

  if (th == NULL)
    goto fail;
  fputc ('*', stderr);

  if (th->th_ta_p != arg)
    goto fail;
  fprintf (stderr, " %p", th->th_unique);

  args[0].p = th->th_unique;
  err = i8x_xctx_call (th->th_ta_p->xctx,
		       th->th_ta_p->thr_get_lwpid,
		       th->th_ta_p->inf, args, rets);
  if (err != I8X_OK)
    goto fail;
  fputs (" =>", stderr);

  if (rets[1].i != TD_OK)
    goto fail;
  lwpid_t lwpid = rets[0].i;
  fprintf (stderr, " %d", lwpid);

  args[0].i = lwpid;
  err = i8x_xctx_call (th->th_ta_p->xctx,
		       th->th_ta_p->map_lwp2thr,
		       th->th_ta_p->inf, args, rets);

  if (err != I8X_OK)
    goto fail;
  fputs (" =>", stderr);

  if (rets[1].i != TD_OK)
    goto fail;
  psaddr_t th_unique = rets[0].p;
  fprintf (stderr, " %p", th_unique);

  if (th_unique != th->th_unique)
    goto fail;

  fputc ('\n', stderr);
  return 0;

 fail:
  fputs (" FAIL!\n", stderr);
  return -1;
}

/* Check that libpthread::thr_get_lwpid and libpthread::map_lwp2thr
   are the inverse of each other for each thread.  This gives some
   assurance that the platform-specific libpthread::__lookup_th_unique
   is working.  */

static td_err_e
td_ta_self_test (td_thragent_t *ta)
{
  td_err_e result;

  fputs (
"==================================================================\n"
"\n"
"  WELCOME to INFINITY!\n"
"\n"
"  >>> This libthread_db is a DEMONSTRATION,\n"
"  >>> DO NOT USE IN PRODUCTION IT WILL EAT YOUR BABIES!!!\n"
"\n"
"  Note that td_thr_get_info does not fill in all the fields that\n"
"  glibc's libthread_db fills in -- only ti_ta_p, ti_tid, ti_lid\n"
"  and ti_state are valid.  Note also that many functions are not\n"
"  present, and some that are present always return TD_NOCAPAB.\n"
"\n"
"  SELF-TEST:\n", stderr);

  result = td_ta_thr_iter (ta, td_ta_selftest_cb, ta,
			   TD_THR_ANY_STATE,
			   TD_THR_LOWEST_PRIORITY,
			   TD_SIGNO_MASK,
			   TD_THR_ANY_USER_FLAGS);
  if (result == TD_OK)
    fputs ("\n  OK HAVE FUN!\n", stderr);
  else
    result = TD_DBERR;

  fputs (
"\n"
"==================================================================\n",
	 stderr);

  return result;
}

/* Map an i8x error code to a libthread_db one.  */

static td_err_e
td_err_from_i8x_err (i8x_err_e err)
{
  switch (err)
    {
    case I8X_OK:
      return TD_OK;

    case I8X_ENOMEM:
      return TD_MALLOC;

    default:
      return TD_ERR;
    }
}

/* Read a NUL-terminated string from the inferior.  */

static ps_err_e
td_pdreadstr (struct ps_prochandle *ph, psaddr_t src,
	      char *dst, size_t len)
{
  for (char *limit = dst + len; dst < limit; dst++, src++)
    {
      ps_err_e err = ps_pdread (ph, src, dst, sizeof (char));

      if (err != PS_OK || *dst == '\0')
	return err;
    }

  return PS_ERR;
}

/* Load and register Infinity notes from an ELF.  */

static td_err_e
td_import_notes_from_elf (td_thragent_t *ta, const char *filename,
			  psaddr_t base_address, Elf *elf)
{
  Elf_Scn *scn = NULL;

  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr == NULL || shdr->sh_type != SHT_NOTE)
	continue;

      Elf_Data *data = elf_getdata (scn, NULL);
      if (data == NULL)
	continue;

      size_t offset = 0;
      GElf_Nhdr nhdr;
      size_t name_offset;
      size_t desc_offset;

      while (offset < data->d_size
	     && (offset = gelf_getnote (data, offset,
					&nhdr, &name_offset,
					&desc_offset)) > 0)
	{
	  if (nhdr.n_type != NT_GNU_INFINITY)
	    continue;

	  const char *name = (const char *) data->d_buf + name_offset;
	  if (strncmp (name, "GNU", 4) != 0)
	    continue;

	  /* We have an Infinity note!  */

	  const char *desc = (const char *) data->d_buf + desc_offset;

	  i8x_err_e err;
	  if (ta->ctx == NULL)
	    {
	      err = i8x_ctx_new (0, NULL, &ta->ctx);

	      if (err != I8X_OK)
		return td_err_from_i8x_err (err);
	    }

	  struct i8x_func *func;
	  err = i8x_ctx_import_bytecode (ta->ctx,
					 desc, nhdr.n_descsz,
					 filename,
					 shdr->sh_offset + desc_offset,
					 &func);
	  if (err != I8X_OK)
	    {
	      if (err == I8X_NOTE_CORRUPT
		  || err == I8X_NOTE_UNHANDLED
		  || err == I8X_NOTE_INVALID)
		continue;

	      return td_err_from_i8x_err (err);
	    }

	  i8x_note_set_userdata (i8x_func_get_note (func),
				 base_address, NULL);

	  func = i8x_func_unref (func);
	}
    }

  return TD_OK;
}

/* Load and register Infinity notes from a local file.  */

static td_err_e
td_import_notes_from_file (td_thragent_t *ta, const char *filename,
			   psaddr_t base_address)
{
  int fd = open (filename, O_RDONLY);
  if (fd == -1)
    return TD_OK;  /* Silently skip files we can't read.  */

  Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  td_err_e err;

  switch (elf_kind (elf))
    {
    case ELF_K_ELF:
      err = td_import_notes_from_elf (ta, filename, base_address, elf);
      break;

    default:
      err = TD_ERR;
    }

  elf_end (elf);
  close (fd);

  return err;
}

/* Load and register Infinity notes from a local process.  */

static td_err_e
td_import_notes_from_process (td_thragent_t *ta)
{
  struct r_debug r_debug;
  struct link_map lm;
  psaddr_t addr;
  td_err_e err;

  /* GDB tries to load libthread_db every time a new object file is
     added to the inferior until td_ta_new completes successfully.
     It expects these early calls to fail with TD_NOLIBTHREAD or
     TD_VERSION, two errors which it silently ignores, which is why
     this function returns those codes for the first few failures.  */

  if (ps_pglobal_lookup (ta->ph, LD_SO, "_r_debug", &addr) != PS_OK)
    return TD_NOLIBTHREAD;

  if (ps_pdread (ta->ph, addr, &r_debug, sizeof (r_debug)) != PS_OK)
    return TD_NOLIBTHREAD;

  if (r_debug.r_version != 1)
    return TD_VERSION;

  /* Scan each ELF in the link map for notes.  */
  for (addr = r_debug.r_map; addr != NULL; addr = lm.l_next)
    {
      if (ps_pdread (ta->ph, addr, &lm, sizeof (lm)) != PS_OK)
	return TD_ERR;

      /* No idea if this might happen.  */
      if (lm.l_name == NULL)
	continue;

      char filename[PATH_MAX];
      if (td_pdreadstr (ta->ph, lm.l_name, filename,
			sizeof (filename)) != PS_OK)
	return TD_ERR;

      /* Skip both empty filenames and linux-vdso.so.1.  */
      if (filename[0] != '/')
	continue;

      err = td_import_notes_from_file (ta, filename, (void *) lm.l_addr);
      if (err != TD_OK)
	return err;
    }

  /* If we have no context then no notes were found.  It's possible
     this is a static executable so we try the main executable to
     cover that case.  */
  if (ta->ctx == NULL)
    {
      err = td_import_notes_from_file (ta, ta->exec_filename,
				       (void *) r_debug.r_ldbase);
      if (err != TD_OK)
	return err;
    }

  return TD_OK;
}

/* Address relocation function.  */

static i8x_err_e
td_relocate_address (struct i8x_inf *inf, struct i8x_note *note,
		     uintptr_t unresolved, uintptr_t *result)
{
  *result = (uintptr_t) i8x_note_get_userdata (note) + unresolved;

  return I8X_OK;
}

/* Memory reader function.  */

static i8x_err_e
td_read_memory (struct i8x_inf *inf, uintptr_t addr, size_t len,
		void *result)
{
  td_thragent_t *ta = (td_thragent_t *) i8x_inf_get_userdata (inf);

  if (ps_pdread (ta->ph, (psaddr_t) addr, result, len) != PS_OK)
    return I8X_READ_MEM_FAILED;

  return I8X_OK;
}

/* Infinity native function wrapper for ps_getpid.  */

static i8x_err_e
td_ps_getpid (struct i8x_xctx *xctx, struct i8x_inf *inf,
	      union i8x_value *args, union i8x_value *rets)
{
  td_thragent_t *ta = (td_thragent_t *) i8x_inf_get_userdata (inf);

  rets[0].i = ta->pid;

  return I8X_OK;
}

/* Infinity native function wrapper for ps_get_register.  */

static i8x_err_e
td_ps_get_register (struct i8x_xctx *xctx, struct i8x_inf *inf,
		    union i8x_value *args, union i8x_value *rets)
{
  td_thragent_t *ta = (td_thragent_t *) i8x_inf_get_userdata (inf);

  /* We cannot use prgregset_t here because we may be accessing a
     process on an architecture with a larger prgregset_t than our
     own.  I will probably eat my words later, but here goes: 1024
     registers should be suitably vast.  */
  elf_greg_t regs[1024];
  rets[1].i = ps_lgetregs (ta->ph, args[0].i, regs);
  if (rets[1].i != PS_OK)
    return I8X_OK;

  switch (i8x_xctx_get_wordsize (xctx))
    {
#define CASE(SIZE)							\
    case SIZE:								\
      rets[0].u =							\
	*(uint ## SIZE ## _t *) ((uint8_t *) regs + args[1].i);		\
      break

    CASE(32);

#if __WORDSIZE >= 64
    CASE(64);
#endif /* __WORDSIZE >= 64 */

    default:
      rets[1].i = PS_ERR;
      return I8X_OK;
    }

  return I8X_OK;
}

/* Infinity native function wrapper for ps_get_thread_area.  */

#pragma weak ps_get_thread_area

static i8x_err_e
td_ps_get_thread_area (struct i8x_xctx *xctx, struct i8x_inf *inf,
		       union i8x_value *args, union i8x_value *rets)
{
  td_thragent_t *ta = (td_thragent_t *) i8x_inf_get_userdata (inf);

  rets[1].i = ps_get_thread_area (ta->ph, args[0].i, args[1].i,
				  &rets[0].p);

  return I8X_OK;
}

/* Initialize the thread debug support library.  */

td_err_e
td_init (void)
{
  /* Initialize libelf.  */
  elf_version (EV_CURRENT);

  return TD_OK;
}

/* Generate new thread debug library handle for process PS.  */

td_err_e
td_ta_new (struct ps_prochandle *ps, td_thragent_t **__ta)
{
  td_thragent_t *ta;
  td_err_e err;

  ta = calloc (1, sizeof (td_thragent_t));
  if (ta == NULL)
    return TD_MALLOC;

  ta->ph = ps;

  err = td_ta_init (ta);
  if (err == TD_OK)
    *__ta = ta;
  else
    td_ta_delete (ta);

  return err;
}

/* Helper for td_ta_new.  */

static td_err_e
td_ta_init (td_thragent_t *ta)
{
  td_err_e err;

  /* Store the main process PID.  */
  ta->pid = ps_getpid (ta->ph);

  /* Build the main executable filename.  */
  int len = snprintf (ta->exec_filename,
		      sizeof (ta->exec_filename),
		      "/proc/%d/exe", ta->pid);
  if (len > sizeof (ta->exec_filename))
    return TD_DBERR;  /* Should be enough for longest PID.  */

  /* Read the main executable's ELF header.  */
  int fd = open (ta->exec_filename, O_RDONLY);
  if (fd != -1)
    {
      Elf *elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
      if (elf_kind (elf) == ELF_K_ELF)
	ta->exec_ehdr = gelf_getehdr (elf, &ta->exec_ehdr_mem);

      elf_end (elf);
      close (fd);
    }
  if (ta->exec_ehdr == NULL)
    return TD_VERSION;

  /* Load and register any Infinity notes from the process we've been
     created on.  This will initialize ta->ctx if notes are found.  */
  err = td_import_notes_from_process (ta);
  if (err != TD_OK)
    return err;
  if (ta->ctx == NULL)
    return TD_VERSION;  /* The process contained no notes.  */

  /* Initialize all libi8x bits except ta->ctx which is done already.  */
  err = td_ta_init_libi8x (ta);
  if (err != TD_OK)
    return err;

  /* Run a quick self-test and print the warning banner.  */
  err = td_ta_self_test (ta);
  if (err != TD_OK)
    return err;

  return TD_OK;
}

/* Helper for td_ta_init.  */

static td_err_e
td_ta_init_libi8x (td_thragent_t *ta)
{
  i8x_err_e err;

  /* Register the native functions we provide.  */
#define REGISTER(name, args, rets, impl)			    \
  do {								    \
    err = i8x_ctx_import_native (ta->ctx,			    \
				 "procservice", #name,		    \
				 args, rets,			    \
				 impl, NULL);			    \
    if (err != I8X_OK)						    \
      return td_err_from_i8x_err (err);				    \
  } while (0)

  REGISTER (getpid,       "",   "i",  td_ps_getpid);
  REGISTER (get_register, "ii", "ii", td_ps_get_register);

  if (&ps_get_thread_area != NULL)
    REGISTER (get_thread_area, "ii", "ip", td_ps_get_thread_area);

#undef REGISTER

  /* Store references to the functions we use.  */
#define GET_FUNCREF(name, args, rets)				    \
  do {								    \
    err = i8x_ctx_get_funcref (ta->ctx,				    \
			       "libpthread", #name, args, rets,     \
			       &ta->name);			    \
    if (err != I8X_OK)						    \
      return td_err_from_i8x_err (err);				    \
  } while (0)

  GET_FUNCREF (map_lwp2thr,	"i",		"ip");
  GET_FUNCREF (thr_iter,	"Fi(po)oi",	"i");
  GET_FUNCREF (thr_get_lwpid,	"p",		"ii");
  GET_FUNCREF (thr_get_state,	"p",		"ii");

#undef GET_FUNCREF

  /* Check we have at least one resolved function.  */
  if (!i8x_funcref_is_resolved (ta->map_lwp2thr)
      && !i8x_funcref_is_resolved (ta->thr_iter)
      && !(i8x_funcref_is_resolved (ta->thr_get_lwpid)
	   && i8x_funcref_is_resolved (ta->thr_get_state)))
    return TD_VERSION;

  /* Register the callback wrapper for td_ta_thr_iter.  */
  struct i8x_func *func;

  err = i8x_ctx_import_native (ta->ctx,
			       "libthread_db", "thr_iter_cb",
			       "po", "i",
			       td_ta_thr_iter_cb, &func);
  if (err != I8X_OK)
    return td_err_from_i8x_err (err);

  ta->thr_iter_cb = i8x_funcref_ref (i8x_func_get_funcref (func));
  func = i8x_func_unref (func);

  /* Create the inferior.  */
  err = i8x_inf_new (ta->ctx, &ta->inf);
  if (err != I8X_OK)
    return td_err_from_i8x_err (err);

  i8x_inf_set_userdata (ta->inf, ta, NULL);
  i8x_inf_set_relocate_fn (ta->inf, td_relocate_address);
  i8x_inf_set_read_mem_fn (ta->inf, td_read_memory);

  /* Create an execution context.  */
  err = i8x_xctx_new (ta->ctx, 512, &ta->xctx);
  if (err != I8X_OK)
    return td_err_from_i8x_err (err);

  return TD_OK;
}

/* Free resources allocated for TA.  */

td_err_e
td_ta_delete (td_thragent_t *ta)
{
  i8x_funcref_unref (ta->map_lwp2thr);
  i8x_funcref_unref (ta->thr_iter);
  i8x_funcref_unref (ta->thr_get_lwpid);
  i8x_funcref_unref (ta->thr_get_state);

  i8x_funcref_unref (ta->thr_iter_cb);

  i8x_inf_unref (ta->inf);
  i8x_xctx_unref (ta->xctx);

  i8x_ctx_unref (ta->ctx);

  free (ta);

  return TD_OK;
}

/* Map thread library handle PT to thread debug library handle for
   process associated with TA and store result in *TH.  */

td_err_e
td_ta_map_id2thr (const td_thragent_t *ta, pthread_t pt,
		  td_thrhandle_t *th)
{
  return TD_NOCAPAB;
}

/* Map process ID LWPID to thread debug library handle for process
   associated with TA and store result in *TH.  */

td_err_e
td_ta_map_lwp2thr (const td_thragent_t *ta, lwpid_t lwpid,
		   td_thrhandle_t *th)
{
  union i8x_value args[1], rets[2];
  i8x_err_e err;

  args[0].i = lwpid;

  err = i8x_xctx_call (ta->xctx, ta->map_lwp2thr, ta->inf, args, rets);
  if (err != I8X_OK)
    return td_err_from_i8x_err (err);

  if (rets[1].i != TD_OK)
    return rets[1].i;

  th->th_ta_p = (td_thragent_t *) ta;
  th->th_unique = rets[0].p;

  return TD_OK;
}

/* Call for each thread in a process associated with TA the callback
   function CALLBACK.  */

td_err_e
td_ta_thr_iter (const td_thragent_t *ta, td_thr_iter_f *callback,
		void *cbdata_p, td_thr_state_e state, int ti_pri,
		sigset_t *ti_sigmask_p, unsigned int ti_user_flags)
{
  /* glibc's libthread_db completely ignores these two, but that
     seems wrong.  We're going to bail with a meaningful error.  */
  if (ti_sigmask_p != TD_SIGNO_MASK
      || ti_user_flags != TD_THR_ANY_USER_FLAGS)
    return TD_NOCAPAB;

  /* glibc's libthread_db doesn't ignore this one but maybe it
     should; what it actually does is return TD_OK without
     iterating the thread list.  That's totally misleading so
     again we're going to bail with a meaningful error.  */
  if (state != TD_THR_ANY_STATE)
    return TD_NOCAPAB;

  /* We have something glibc's libpthread::thr_iter can handle.  */
  if (ta->thr_iter == NULL || !i8x_funcref_is_resolved (ta->thr_iter))
    return TD_NOCAPAB;

  union i8x_value args[3], rets[1];
  i8x_err_e err;

  args[0].f = ta->thr_iter_cb;
  args[1].p = cbdata_p;
  args[2].i = ti_pri;

  ((td_thragent_t *) ta)->thr_iter_cb_impl = callback;

  err = i8x_xctx_call (ta->xctx, ta->thr_iter, ta->inf, args, rets);
  if (err != I8X_OK)
    return td_err_from_i8x_err (err);

  return rets[0].i;
}

/* Helper for td_ta_thr_iter.  */

static i8x_err_e
td_ta_thr_iter_cb (struct i8x_xctx *xctx, struct i8x_inf *inf,
		   union i8x_value *args, union i8x_value *rets)
{
  td_thragent_t *ta = (td_thragent_t *) i8x_inf_get_userdata (inf);
  struct td_thrhandle th = {ta, args[0].p};

  rets[0].i = ta->thr_iter_cb_impl (&th, args[1].p);

  return I8X_OK;
}

/* Validate that TH is a thread handle.  */

td_err_e
td_thr_validate (const td_thrhandle_t *th)
{
  return TD_NOCAPAB;
}

/* Return information about thread TH.  */

td_err_e
td_thr_get_info (const td_thrhandle_t *th, td_thrinfo_t *infop)
{
  td_thragent_t *ta = th->th_ta_p;

  /* Clear first to provide reproducable results.  */
  memset (infop, 0, sizeof (td_thrinfo_t));

  /* Fill in information.  */
  infop->ti_tid = (thread_t) th->th_unique;
  infop->ti_ta_p = th->th_ta_p;

  /* Fill in the fields we handle.  */
  union i8x_value args[1], rets[2];
  i8x_err_e err;

  args[0].p = th->th_unique;

#define GET_FIELD(fn, field, itype)				    \
  do {								    \
    err = i8x_xctx_call (ta->xctx, ta->fn, ta->inf, args, rets);    \
    if (err != I8X_OK)						    \
      return td_err_from_i8x_err (err);				    \
								    \
    if (rets[1].i != TD_OK)					    \
      return rets[1].i;						    \
								    \
    infop->field = (typeof (infop->field)) rets[0].itype;	    \
  } while (0)

  GET_FIELD (thr_get_lwpid, ti_lid, i);
  GET_FIELD (thr_get_state, ti_state, i);

#undef GET_FIELD

  return TD_OK;
}
