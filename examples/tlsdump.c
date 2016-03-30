#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <libelf.h>
#include <gelf.h>

#include <i8x/libi8x.h>

#ifndef NT_GNU_INFINITY
#  define NT_GNU_INFINITY 5
#endif

static void __attribute__ ((__noreturn__,  format (printf, 1, 2)))
error (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "error: ");
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);

  exit (EXIT_FAILURE);
}

static void __attribute__ ((__noreturn__))
error_i8x (struct i8x_ctx *ctx, i8x_err_e code)
{
  static char buf[BUFSIZ];

  fprintf (stderr, "%s\n",
	   i8x_ctx_strerror_r (ctx, code, buf, sizeof (buf)));

  exit (EXIT_FAILURE);
}

static void *
xcalloc (size_t nmemb, size_t size)
{
  void *ptr = calloc (nmemb, size);

  if (ptr == NULL)
    error ("out of memory");

  return ptr;
}

static char *
xstrdup (const char *s)
{
  char *t = strdup (s);

  if (t == NULL)
    error ("out of memory");

  return t;
}

struct list_entry
{
  struct list_entry *next;
  void *item;
};

struct elffile
{
  const char *filename;
};

struct userdata
{
  pid_t pid; /* The PID of the process we are investigating.  */
  struct list_entry *elfs; /* List of ELFs loaded from this process.  */
};

static void
process_notes (struct i8x_ctx *ctx,
	       struct elffile *ef, size_t scn_offset,
	       Elf_Data *data)
{
  size_t offset = 0;
  GElf_Nhdr nhdr;
  size_t name_offset;
  size_t desc_offset;

  if (data == NULL)
    error ("%s: can't access note section at 0x%x", ef->filename,
	   scn_offset);

  while (offset < data->d_size
	 && (offset = gelf_getnote (data, offset,
				    &nhdr, &name_offset,
				    &desc_offset)) > 0)
    {
      const char *name = (const char *) data->d_buf + name_offset;
      const char *desc = (const char *) data->d_buf + desc_offset;
      struct i8x_note *note;
      i8x_err_e err;

      if (strncmp (name, "GNU", 4) || nhdr.n_type != NT_GNU_INFINITY)
	continue;

      /* Create the i8x_note.  */
      err = i8x_note_new_from_mem (ctx, desc, nhdr.n_descsz,
				   ef->filename,
				   scn_offset + desc_offset, &note);
      if (err != I8X_OK)
	error_i8x (ctx, err);

      /* XXX do stuff here.  */

      i8x_note_unref (note);
    }
}

static void
process_elffile (struct i8x_ctx *ctx, const char *filename, Elf *elf)
{
  struct userdata *ud = (struct userdata *) i8x_ctx_get_userdata (ctx);
  struct list_entry *le;
  struct elffile *ef;
  Elf_Scn *scn = NULL;

  /* Skip this file if we already processed it.  */
  for (le = ud->elfs; le != NULL; le = le->next)
    {
      ef = (struct elffile *) le->item;
      if (!strcmp (ef->filename, filename))
	return;
    }

  ef = xcalloc (1, sizeof (struct elffile));
  ef->filename = xstrdup (filename);

  /* Extract what we need.  */
  while ((scn = elf_nextscn (elf, scn)) != NULL)
    {
      GElf_Shdr shdr_mem;
      GElf_Shdr *shdr = gelf_getshdr (scn, &shdr_mem);

      if (shdr == NULL)
	continue;

      switch (shdr->sh_type)
	{
	case SHT_NOTE:
	  process_notes (ctx, ef, shdr->sh_offset,
			 elf_getdata (scn, NULL));
	  break;
	}
    }

  /* Hook the processed file into our list.  */
  le = xcalloc (1, sizeof (struct list_entry));
  le->item = ef;
  le->next = ud->elfs;
  ud->elfs = le;
}

static void
process_mapping (struct i8x_ctx *ctx, const char *filename)
{
  int fd;
  Elf *elf;

  fd = open (filename, O_RDONLY);
  if (fd == -1)
    error ("%s: %s", filename, strerror (errno));

  elf = elf_begin (fd, ELF_C_READ_MMAP, NULL);
  if (elf_kind (elf) == ELF_K_ELF)
    process_elffile (ctx, filename, elf);

  elf_end (elf);
  close (fd);
}

static void
read_mappings (struct i8x_ctx *ctx)
{
  struct userdata *ud = (struct userdata *) i8x_ctx_get_userdata (ctx);
  char mapfile[32]; /* Enough for the longest 64-bit PID.  */
  unsigned int len;
  FILE *fp;
  char buf[PATH_MAX]; /* XXX lazy.  */

  len = snprintf (mapfile, sizeof (mapfile), "/proc/%d/maps", ud->pid);
  if (len >= sizeof (mapfile))
    error ("internal error constructing maps filename");

  fp = fopen (mapfile, "r");
  if (fp == NULL)
    error ("%s: fopen failed", mapfile);

  while (fgets (buf, sizeof (buf), fp) != NULL)
    {
      char *p = buf;
      int i;

      /* Strip the terminal newline.  */
      if (strlen (buf) == 0)
	error ("%s: empty line?", mapfile);
      if (buf[strlen (buf) - 1] != '\n')
	error ("%s: buffer overflow?", mapfile);
      buf[strlen (buf) - 1] = '\0';

      /* Skip over the initial fields.  */
      for (i = 0; i < 5; i++)
	{
	  while (*p != '\0' && !isspace (*p))
	    p++;

	  while (*p != '\0' && isspace (*p))
	    p++;
	}

      /* Process this mapping if we have a filename.  */
      if (*p == '/')
	process_mapping (ctx, p);
    }

  fclose (fp);
}

static void
tlsdump_process (pid_t pid)
{
  struct i8x_ctx *ctx;
  struct userdata ud;
  i8x_err_e err;

  err = i8x_ctx_new (&ctx);
  if (err != I8X_OK)
    error_i8x (NULL, err);

  ud.pid = pid;
  ud.elfs = NULL;
  i8x_ctx_set_userdata (ctx, &ud, NULL);

  read_mappings (ctx);

  /* XXX free stuff in userdata e.g. the list of ELF files  */
  i8x_ctx_unref (ctx);
}

int
main (int argc, char *argv[])
{
  int i;

  if (argc < 2)
    error ("usage: %s PID...", argv[0]);

  /* Initialize libelf.  */
  elf_version (EV_CURRENT);

  for (i = 1; i < argc; i++)
    {
      char *endptr;
      pid_t pid;

      pid = strtol (argv[i], &endptr, 10);
      if (endptr[0] != '\0')
	error ("%s: invalid PID", argv[i]);

      tlsdump_process (pid);
    }

  return EXIT_SUCCESS;
}
