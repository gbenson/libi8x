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

#include <thread_db.h>

/* Initialize the thread debug support library.  */

td_err_e
td_init (void)
{
  return TD_NOCAPAB;
}

/* Generate new thread debug library handle for process PS.  */

td_err_e
td_ta_new (struct ps_prochandle *ps, td_thragent_t **ta)
{
  return TD_NOCAPAB;
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
  return TD_NOCAPAB;
}

/* Call for each thread in a process associated with TA the callback
   function CALLBACK.  */

td_err_e
td_ta_thr_iter (const td_thragent_t *ta, td_thr_iter_f *callback,
		void *cbdata_p, td_thr_state_e state, int ti_pri,
		sigset_t *ti_sigmask_p, unsigned int ti_user_flags)
{
  return TD_NOCAPAB;
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
  return TD_NOCAPAB;
}
