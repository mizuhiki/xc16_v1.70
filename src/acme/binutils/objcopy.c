/* objcopy.c -- copy object file from input to output, optionally massaging it.
   Copyright 1991, 1992, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001, 2002, 2003
   Free Software Foundation, Inc.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "bfd.h"
#include "progress.h"
#include "bucomm.h"
#include "getopt.h"
#include "libiberty.h"
#include "budbg.h"
#include "filenames.h"
#include <sys/stat.h>

#ifdef PIC30
#include "coff-pic30.h"
#endif

/* A list of symbols to explicitly strip out, or to keep.  A linked
   list is good enough for a small number from the command line, but
   this will slow things down a lot if many symbols are being
   deleted.  */

struct symlist
{
  const char *name;
  struct symlist *next;
};

/* A list to support redefine_sym.  */
struct redefine_node
{
  char *source;
  char *target;
  struct redefine_node *next;
};

typedef struct section_rename
{
  const char *            old_name;
  const char *            new_name;
  flagword                flags;
  struct section_rename * next;
}
section_rename;

/* List of sections to be renamed.  */
static section_rename * section_rename_list;

static void copy_usage
  PARAMS ((FILE *, int));
static void strip_usage
  PARAMS ((FILE *, int));
static flagword parse_flags
  PARAMS ((const char *));
static struct section_list *find_section_list
  PARAMS ((const char *, bfd_boolean));
static void setup_section
  PARAMS ((bfd *, asection *, PTR));
static void copy_section
  PARAMS ((bfd *, asection *, PTR));
static void get_sections
  PARAMS ((bfd *, asection *, PTR));
static int compare_section_lma
  PARAMS ((const PTR, const PTR));
static void add_specific_symbol
  PARAMS ((const char *, struct symlist **));
static void add_specific_symbols
  PARAMS ((const char *, struct symlist **));
static bfd_boolean is_specified_symbol
  PARAMS ((const char *, struct symlist *));
static bfd_boolean is_strip_section
  PARAMS ((bfd *, asection *));
static unsigned int filter_symbols
  PARAMS ((bfd *, bfd *, asymbol **, asymbol **, long));
static void mark_symbols_used_in_relocations
  PARAMS ((bfd *, asection *, PTR));
static void filter_bytes
  PARAMS ((char *, bfd_size_type *));
static bfd_boolean write_debugging_info
  PARAMS ((bfd *, PTR, long *, asymbol ***));
static void copy_object
  PARAMS ((bfd *, bfd *));
static void copy_archive
  PARAMS ((bfd *, bfd *, const char *));
static void copy_file
  PARAMS ((const char *, const char *, const char *, const char *));
static int strip_main
  PARAMS ((int, char **));
static int copy_main
  PARAMS ((int, char **));
static const char *lookup_sym_redefinition
  PARAMS((const char *));
static void redefine_list_append
  PARAMS ((const char *, const char *));
static const char * find_section_rename
  PARAMS ((bfd *, sec_ptr, flagword *));
static void add_section_rename
  PARAMS ((const char *, const char *, flagword));

#define RETURN_NONFATAL(s) {bfd_nonfatal (s); status = 1; return;}

static asymbol **isympp = NULL;	/* Input symbols */
static asymbol **osympp = NULL;	/* Output symbols that survive stripping */

/* If `copy_byte' >= 0, copy only that byte of every `interleave' bytes.  */
static int copy_byte = -1;
static int interleave = 4;

static bfd_boolean verbose;		/* Print file and target names.  */
static bfd_boolean preserve_dates;	/* Preserve input file timestamp.  */
static int status = 0;		/* Exit status.  */

enum strip_action
  {
    STRIP_UNDEF,
    STRIP_NONE,			/* don't strip */
    STRIP_DEBUG,		/* strip all debugger symbols */
    STRIP_UNNEEDED,		/* strip unnecessary symbols */
#ifdef PIC30
    STRIP_LOCAL_ABSOLUTE,       /* strip local absolute symbols */
#endif
    STRIP_ALL			/* strip all symbols */
  };

/* Which symbols to remove.  */
static enum strip_action strip_symbols;
#ifdef PIC30
static char *keep_named=0;
#endif

enum locals_action
  {
    LOCALS_UNDEF,
    LOCALS_START_L,		/* discard locals starting with L */
    LOCALS_ALL			/* discard all locals */
  };

/* Which local symbols to remove.  Overrides STRIP_ALL.  */
static enum locals_action discard_locals;

/* What kind of change to perform.  */
enum change_action
{
  CHANGE_IGNORE,
  CHANGE_MODIFY,
  CHANGE_SET
};

/* Structure used to hold lists of sections and actions to take.  */
struct section_list
{
  struct section_list * next;	   /* Next section to change.  */
  const char *		name;	   /* Section name.  */
  bfd_boolean		used;	   /* Whether this entry was used.  */
  bfd_boolean		remove;	   /* Whether to remove this section.  */
  bfd_boolean		copy;	   /* Whether to copy this section.  */
  enum change_action	change_vma;/* Whether to change or set VMA.  */
  bfd_vma		vma_val;   /* Amount to change by or set to.  */
  enum change_action	change_lma;/* Whether to change or set LMA.  */
  bfd_vma		lma_val;   /* Amount to change by or set to.  */
  bfd_boolean		set_flags; /* Whether to set the section flags.	 */
  flagword		flags;	   /* What to set the section flags to.	 */
};

static struct section_list *change_sections;

/* TRUE if some sections are to be removed.  */
static bfd_boolean sections_removed;

/* TRUE if only some sections are to be copied.  */
static bfd_boolean sections_copied;

/* Changes to the start address.  */
static bfd_vma change_start = 0;
static bfd_boolean set_start_set = FALSE;
static bfd_vma set_start;

/* Changes to section addresses.  */
static bfd_vma change_section_address = 0;

/* Filling gaps between sections.  */
static bfd_boolean gap_fill_set = FALSE;
static bfd_byte gap_fill = 0;

/* Pad to a given address.  */
static bfd_boolean pad_to_set = FALSE;
static bfd_vma pad_to;

/* Use alternate machine code?  */
static int use_alt_mach_code = 0;

/* List of sections to add.  */
struct section_add
{
  /* Next section to add.  */
  struct section_add *next;
  /* Name of section to add.  */
  const char *name;
  /* Name of file holding section contents.  */
  const char *filename;
  /* Size of file.  */
  size_t size;
  /* Contents of file.  */
  bfd_byte *contents;
  /* BFD section, after it has been added.  */
  asection *section;
};

/* List of sections to add to the output BFD.  */
static struct section_add *add_sections;

/* Whether to convert debugging information.  */
static bfd_boolean convert_debugging = FALSE;

/* Whether to change the leading character in symbol names.  */
static bfd_boolean change_leading_char = FALSE;

/* Whether to remove the leading character from global symbol names.  */
static bfd_boolean remove_leading_char = FALSE;

/* List of symbols to strip, keep, localize, keep-global, weaken,
   or redefine.  */
static struct symlist *strip_specific_list = NULL;
static struct symlist *keep_specific_list = NULL;
static struct symlist *localize_specific_list = NULL;
static struct symlist *keepglobal_specific_list = NULL;
static struct symlist *weaken_specific_list = NULL;
static struct redefine_node *redefine_sym_list = NULL;

/* If this is TRUE, we weaken global symbols (set BSF_WEAK).  */
static bfd_boolean weaken = FALSE;

/* Prefix symbols/sections.  */
static char *prefix_symbols_string = 0;
static char *prefix_sections_string = 0;
static char *prefix_alloc_sections_string = 0;

/* 150 isn't special; it's just an arbitrary non-ASCII char value.  */

#define OPTION_ADD_SECTION 150
#define OPTION_CHANGE_ADDRESSES (OPTION_ADD_SECTION + 1)
#define OPTION_CHANGE_LEADING_CHAR (OPTION_CHANGE_ADDRESSES + 1)
#define OPTION_CHANGE_START (OPTION_CHANGE_LEADING_CHAR + 1)
#define OPTION_CHANGE_SECTION_ADDRESS (OPTION_CHANGE_START + 1)
#define OPTION_CHANGE_SECTION_LMA (OPTION_CHANGE_SECTION_ADDRESS + 1)
#define OPTION_CHANGE_SECTION_VMA (OPTION_CHANGE_SECTION_LMA + 1)
#define OPTION_CHANGE_WARNINGS (OPTION_CHANGE_SECTION_VMA + 1)
#define OPTION_DEBUGGING (OPTION_CHANGE_WARNINGS + 1)
#define OPTION_GAP_FILL (OPTION_DEBUGGING + 1)
#define OPTION_NO_CHANGE_WARNINGS (OPTION_GAP_FILL + 1)
#define OPTION_PAD_TO (OPTION_NO_CHANGE_WARNINGS + 1)
#define OPTION_REMOVE_LEADING_CHAR (OPTION_PAD_TO + 1)
#define OPTION_SET_SECTION_FLAGS (OPTION_REMOVE_LEADING_CHAR + 1)
#define OPTION_SET_START (OPTION_SET_SECTION_FLAGS + 1)
#define OPTION_STRIP_UNNEEDED (OPTION_SET_START + 1)
#define OPTION_WEAKEN (OPTION_STRIP_UNNEEDED + 1)
#define OPTION_REDEFINE_SYM (OPTION_WEAKEN + 1)
#define OPTION_SREC_LEN (OPTION_REDEFINE_SYM + 1)
#define OPTION_SREC_FORCES3 (OPTION_SREC_LEN + 1)
#define OPTION_STRIP_SYMBOLS (OPTION_SREC_FORCES3 + 1)
#define OPTION_KEEP_SYMBOLS (OPTION_STRIP_SYMBOLS + 1)
#define OPTION_LOCALIZE_SYMBOLS (OPTION_KEEP_SYMBOLS + 1)
#define OPTION_KEEPGLOBAL_SYMBOLS (OPTION_LOCALIZE_SYMBOLS + 1)
#define OPTION_WEAKEN_SYMBOLS (OPTION_KEEPGLOBAL_SYMBOLS + 1)
#define OPTION_RENAME_SECTION (OPTION_WEAKEN_SYMBOLS + 1)
#define OPTION_ALT_MACH_CODE (OPTION_RENAME_SECTION + 1)
#define OPTION_PREFIX_SYMBOLS (OPTION_ALT_MACH_CODE + 1)
#define OPTION_PREFIX_SECTIONS (OPTION_PREFIX_SYMBOLS + 1)
#define OPTION_PREFIX_ALLOC_SECTIONS (OPTION_PREFIX_SECTIONS + 1)
#define OPTION_FORMATS_INFO (OPTION_PREFIX_ALLOC_SECTIONS + 1)
#ifdef PIC30
#define OPTION_STRIP_LOCAL_ABSOLUTE (OPTION_FORMATS_INFO + 1)
#define OPTION_KEEP_NAMED (OPTION_STRIP_LOCAL_ABSOLUTE + 1)
#endif

/* Options to handle if running as "strip".  */

static struct option strip_options[] =
{
  {"discard-all", no_argument, 0, 'x'},
  {"discard-locals", no_argument, 0, 'X'},
  {"format", required_argument, 0, 'F'}, /* Obsolete */
  {"help", no_argument, 0, 'h'},
  {"info", no_argument, 0, OPTION_FORMATS_INFO},
  {"input-format", required_argument, 0, 'I'}, /* Obsolete */
  {"input-target", required_argument, 0, 'I'},
  {"keep-symbol", required_argument, 0, 'K'},
  {"output-format", required_argument, 0, 'O'},	/* Obsolete */
  {"output-target", required_argument, 0, 'O'},
  {"output-file", required_argument, 0, 'o'},
  {"preserve-dates", no_argument, 0, 'p'},
  {"remove-section", required_argument, 0, 'R'},
  {"strip-all", no_argument, 0, 's'},
  {"strip-debug", no_argument, 0, 'S'},
#ifdef PIC30
  {"strip-local-absolute", no_argument, 0, OPTION_STRIP_LOCAL_ABSOLUTE},
  {"keep-named", required_argument, 0, OPTION_KEEP_NAMED},
#endif
  {"strip-unneeded", no_argument, 0, OPTION_STRIP_UNNEEDED},
  {"strip-symbol", required_argument, 0, 'N'},
  {"target", required_argument, 0, 'F'},
  {"verbose", no_argument, 0, 'v'},
  {"version", no_argument, 0, 'V'},
  {0, no_argument, 0, 0}
};

/* Options to handle if running as "objcopy".  */

static struct option copy_options[] =
{
  {"add-section", required_argument, 0, OPTION_ADD_SECTION},
  {"adjust-start", required_argument, 0, OPTION_CHANGE_START},
  {"adjust-vma", required_argument, 0, OPTION_CHANGE_ADDRESSES},
  {"adjust-section-vma", required_argument, 0, OPTION_CHANGE_SECTION_ADDRESS},
  {"adjust-warnings", no_argument, 0, OPTION_CHANGE_WARNINGS},
  {"alt-machine-code", required_argument, 0, OPTION_ALT_MACH_CODE},
  {"binary-architecture", required_argument, 0, 'B'},
  {"byte", required_argument, 0, 'b'},
  {"change-addresses", required_argument, 0, OPTION_CHANGE_ADDRESSES},
  {"change-leading-char", no_argument, 0, OPTION_CHANGE_LEADING_CHAR},
  {"change-section-address", required_argument, 0, OPTION_CHANGE_SECTION_ADDRESS},
  {"change-section-lma", required_argument, 0, OPTION_CHANGE_SECTION_LMA},
  {"change-section-vma", required_argument, 0, OPTION_CHANGE_SECTION_VMA},
  {"change-start", required_argument, 0, OPTION_CHANGE_START},
  {"change-warnings", no_argument, 0, OPTION_CHANGE_WARNINGS},
  {"debugging", no_argument, 0, OPTION_DEBUGGING},
  {"discard-all", no_argument, 0, 'x'},
  {"discard-locals", no_argument, 0, 'X'},
  {"format", required_argument, 0, 'F'}, /* Obsolete */
  {"gap-fill", required_argument, 0, OPTION_GAP_FILL},
  {"help", no_argument, 0, 'h'},
  {"info", no_argument, 0, OPTION_FORMATS_INFO},
  {"input-format", required_argument, 0, 'I'}, /* Obsolete */
  {"input-target", required_argument, 0, 'I'},
  {"interleave", required_argument, 0, 'i'},
  {"keep-global-symbol", required_argument, 0, 'G'},
  {"keep-global-symbols", required_argument, 0, OPTION_KEEPGLOBAL_SYMBOLS},
  {"keep-symbol", required_argument, 0, 'K'},
  {"keep-symbols", required_argument, 0, OPTION_KEEP_SYMBOLS},
  {"localize-symbol", required_argument, 0, 'L'},
  {"localize-symbols", required_argument, 0, OPTION_LOCALIZE_SYMBOLS},
  {"no-adjust-warnings", no_argument, 0, OPTION_NO_CHANGE_WARNINGS},
  {"no-change-warnings", no_argument, 0, OPTION_NO_CHANGE_WARNINGS},
  {"only-section", required_argument, 0, 'j'},
  {"output-format", required_argument, 0, 'O'},	/* Obsolete */
  {"output-target", required_argument, 0, 'O'},
  {"pad-to", required_argument, 0, OPTION_PAD_TO},
  {"prefix-symbols", required_argument, 0, OPTION_PREFIX_SYMBOLS},
  {"prefix-sections", required_argument, 0, OPTION_PREFIX_SECTIONS},
  {"prefix-alloc-sections", required_argument, 0, OPTION_PREFIX_ALLOC_SECTIONS},
  {"preserve-dates", no_argument, 0, 'p'},
  {"redefine-sym", required_argument, 0, OPTION_REDEFINE_SYM},
  {"remove-leading-char", no_argument, 0, OPTION_REMOVE_LEADING_CHAR},
  {"remove-section", required_argument, 0, 'R'},
  {"rename-section", required_argument, 0, OPTION_RENAME_SECTION},
  {"set-section-flags", required_argument, 0, OPTION_SET_SECTION_FLAGS},
  {"set-start", required_argument, 0, OPTION_SET_START},
  {"srec-len", required_argument, 0, OPTION_SREC_LEN},
  {"srec-forceS3", no_argument, 0, OPTION_SREC_FORCES3},
  {"strip-all", no_argument, 0, 'S'},
  {"strip-debug", no_argument, 0, 'g'},
  {"strip-unneeded", no_argument, 0, OPTION_STRIP_UNNEEDED},
  {"strip-symbol", required_argument, 0, 'N'},
  {"strip-symbols", required_argument, 0, OPTION_STRIP_SYMBOLS},
  {"target", required_argument, 0, 'F'},
  {"verbose", no_argument, 0, 'v'},
  {"version", no_argument, 0, 'V'},
  {"weaken", no_argument, 0, OPTION_WEAKEN},
  {"weaken-symbol", required_argument, 0, 'W'},
  {"weaken-symbols", required_argument, 0, OPTION_WEAKEN_SYMBOLS},
  {0, no_argument, 0, 0}
};

/* IMPORTS */
extern char *program_name;

/* This flag distinguishes between strip and objcopy:
   1 means this is 'strip'; 0 means this is 'objcopy'.
   -1 means if we should use argv[0] to decide.  */
extern int is_strip;

/* The maximum length of an S record.  This variable is declared in srec.c
   and can be modified by the --srec-len parameter.  */
extern unsigned int Chunk;

/* Restrict the generation of Srecords to type S3 only.
   This variable is declare in bfd/srec.c and can be toggled
   on by the --srec-forceS3 command line switch.  */
extern bfd_boolean S3Forced;

/* Defined in bfd/binary.c.  Used to set architecture of input binary files.  */
extern enum bfd_architecture bfd_external_binary_architecture;


static void
copy_usage (stream, exit_status)
     FILE *stream;
     int exit_status;
{
  fprintf (stream, _("Usage: %s [option(s)] in-file [out-file]\n"), program_name);
  fprintf (stream, _(" Copies a binary file, possibly transforming it in the process\n"));
  fprintf (stream, _(" The options are:\n"));
  fprintf (stream, _("\
  -I --input-target <bfdname>      Assume input file is in format <bfdname>\n\
  -O --output-target <bfdname>     Create an output file in format <bfdname>\n\
  -B --binary-architecture <arch>  Set arch of output file, when input is binary\n\
  -F --target <bfdname>            Set both input and output format to <bfdname>\n\
     --debugging                   Convert debugging information, if possible\n\
  -p --preserve-dates              Copy modified/access timestamps to the output\n\
  -j --only-section <name>         Only copy section <name> into the output\n\
  -R --remove-section <name>       Remove section <name> from the output\n\
  -S --strip-all                   Remove all symbol and relocation information\n\
  -g --strip-debug                 Remove all debugging symbols\n\
     --strip-unneeded              Remove all symbols not needed by relocations\n\
  -N --strip-symbol <name>         Do not copy symbol <name>\n\
  -K --keep-symbol <name>          Only copy symbol <name>\n\
  -L --localize-symbol <name>      Force symbol <name> to be marked as a local\n\
  -G --keep-global-symbol <name>   Localize all symbols except <name>\n\
  -W --weaken-symbol <name>        Force symbol <name> to be marked as a weak\n\
     --weaken                      Force all global symbols to be marked as weak\n\
  -x --discard-all                 Remove all non-global symbols\n\
  -X --discard-locals              Remove any compiler-generated symbols\n\
  -i --interleave <number>         Only copy one out of every <number> bytes\n\
  -b --byte <num>                  Select byte <num> in every interleaved block\n\
     --gap-fill <val>              Fill gaps between sections with <val>\n\
     --pad-to <addr>               Pad the last section up to address <addr>\n\
     --set-start <addr>            Set the start address to <addr>\n\
    {--change-start|--adjust-start} <incr>\n\
                                   Add <incr> to the start address\n\
    {--change-addresses|--adjust-vma} <incr>\n\
                                   Add <incr> to LMA, VMA and start addresses\n\
    {--change-section-address|--adjust-section-vma} <name>{=|+|-}<val>\n\
                                   Change LMA and VMA of section <name> by <val>\n\
     --change-section-lma <name>{=|+|-}<val>\n\
                                   Change the LMA of section <name> by <val>\n\
     --change-section-vma <name>{=|+|-}<val>\n\
                                   Change the VMA of section <name> by <val>\n\
    {--[no-]change-warnings|--[no-]adjust-warnings}\n\
                                   Warn if a named section does not exist\n\
     --set-section-flags <name>=<flags>\n\
                                   Set section <name>'s properties to <flags>\n\
     --add-section <name>=<file>   Add section <name> found in <file> to output\n\
     --rename-section <old>=<new>[,<flags>] Rename section <old> to <new>\n\
     --change-leading-char         Force output format's leading character style\n\
     --remove-leading-char         Remove leading character from global symbols\n\
     --redefine-sym <old>=<new>    Redefine symbol name <old> to <new>\n\
     --srec-len <number>           Restrict the length of generated Srecords\n\
     --srec-forceS3                Restrict the type of generated Srecords to S3\n\
     --strip-symbols <file>        -N for all symbols listed in <file>\n\
     --keep-symbols <file>         -K for all symbols listed in <file>\n\
     --localize-symbols <file>     -L for all symbols listed in <file>\n\
     --keep-global-symbols <file>  -G for all symbols listed in <file>\n\
     --weaken-symbols <file>       -W for all symbols listed in <file>\n\
     --alt-machine-code <index>    Use alternate machine code for output\n\
     --prefix-symbols <prefix>     Add <prefix> to start of every symbol name\n\
     --prefix-sections <prefix>    Add <prefix> to start of every section name\n\
     --prefix-alloc-sections <prefix>\n\
                                   Add <prefix> to start of every allocatable\n\
                                     section name\n\
  -v --verbose                     List all object files modified\n\
  -V --version                     Display this program's version number\n\
  -h --help                        Display this output\n\
     --info                        List object formats & architectures supported\n\
"));
  list_supported_targets (program_name, stream);
  if (exit_status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (exit_status);
}

static void
strip_usage (stream, exit_status)
     FILE *stream;
     int exit_status;
{
  fprintf (stream, _("Usage: %s <option(s)> in-file(s)\n"), program_name);
  fprintf (stream, _(" Removes symbols and sections from files\n"));
  fprintf (stream, _(" The options are:\n"));
  fprintf (stream, _("\
  -I --input-target=<bfdname>      Assume input file is in format <bfdname>\n\
  -O --output-target=<bfdname>     Create an output file in format <bfdname>\n\
  -F --target=<bfdname>            Set both input and output format to <bfdname>\n\
  -p --preserve-dates              Copy modified/access timestamps to the output\n\
  -R --remove-section=<name>       Remove section <name> from the output\n\
  -s --strip-all                   Remove all symbol and relocation information\n\
  -g -S -d --strip-debug           Remove all debugging symbols\n\
     --strip-local-absolute        Remove all local absolute symbols\n\
     --strip-unneeded              Remove all symbols not needed by relocations\n\
  -N --strip-symbol=<name>         Do not copy symbol <name>\n\
  -K --keep-symbol=<name>          Only copy symbol <name>\n\
  -x --discard-all                 Remove all non-global symbols\n\
  -X --discard-locals              Remove any compiler-generated symbols\n\
  -v --verbose                     List all object files modified\n\
  -V --version                     Display this program's version number\n\
  -h --help                        Display this output\n\
     --info                        List object formats & architectures supported\n\
  -o <file>                        Place stripped output into <file>\n\
"));

  list_supported_targets (program_name, stream);
  if (exit_status == 0)
    fprintf (stream, _("Report bugs to %s\n"), REPORT_BUGS_TO);
  exit (exit_status);
}

/* Parse section flags into a flagword, with a fatal error if the
   string can't be parsed.  */

static flagword
parse_flags (s)
     const char *s;
{
  flagword ret;
  const char *snext;
  int len;

  ret = SEC_NO_FLAGS;

  do
    {
      snext = strchr (s, ',');
      if (snext == NULL)
	len = strlen (s);
      else
	{
	  len = snext - s;
	  ++snext;
	}

      if (0) ;
#define PARSE_FLAG(fname,fval) \
  else if (strncasecmp (fname, s, len) == 0) ret |= fval
      PARSE_FLAG ("alloc", SEC_ALLOC);
      PARSE_FLAG ("load", SEC_LOAD);
      PARSE_FLAG ("noload", SEC_NEVER_LOAD);
      PARSE_FLAG ("readonly", SEC_READONLY);
      PARSE_FLAG ("debug", SEC_DEBUGGING);
      PARSE_FLAG ("code", SEC_CODE);
      PARSE_FLAG ("data", SEC_DATA);
      PARSE_FLAG ("rom", SEC_ROM);
      PARSE_FLAG ("share", SEC_SHARED);
      PARSE_FLAG ("contents", SEC_HAS_CONTENTS);
      PARSE_FLAG ("exclude", SEC_EXCLUDE);
#undef PARSE_FLAG
      else
	{
	  char *copy;

	  copy = xmalloc (len + 1);
	  strncpy (copy, s, len);
	  copy[len] = '\0';
	  non_fatal (_("unrecognized section flag `%s'"), copy);
	  fatal (_("supported flags: %s"),
		 "alloc, load, noload, readonly, debug, code, data, rom, share, contents, exclude");
	}

      s = snext;
    }
  while (s != NULL);

  return ret;
}

/* Find and optionally add an entry in the change_sections list.  */

static struct section_list *
find_section_list (name, add)
     const char *name;
     bfd_boolean add;
{
  register struct section_list *p;

  for (p = change_sections; p != NULL; p = p->next)
    if (strcmp (p->name, name) == 0)
      return p;

  if (! add)
    return NULL;

  p = (struct section_list *) xmalloc (sizeof (struct section_list));
  p->name = name;
  p->used = FALSE;
  p->remove = FALSE;
  p->copy = FALSE;
  p->change_vma = CHANGE_IGNORE;
  p->change_lma = CHANGE_IGNORE;
  p->vma_val = 0;
  p->lma_val = 0;
  p->set_flags = FALSE;
  p->flags = 0;

  p->next = change_sections;
  change_sections = p;

  return p;
}

/* Add a symbol to strip_specific_list.  */

static void
add_specific_symbol (name, list)
     const char *name;
     struct symlist **list;
{
  struct symlist *tmp_list;

  tmp_list = (struct symlist *) xmalloc (sizeof (struct symlist));
  tmp_list->name = name;
  tmp_list->next = *list;
  *list = tmp_list;
}

/* Add symbols listed in `filename' to strip_specific_list.  */

#define IS_WHITESPACE(c)      ((c) == ' ' || (c) == '\t')
#define IS_LINE_TERMINATOR(c) ((c) == '\n' || (c) == '\r' || (c) == '\0')

static void
add_specific_symbols (filename, list)
     const char *filename;
     struct symlist **list;
{
  struct stat st;
  FILE * f;
  char * line;
  char * buffer;
  unsigned int line_count;

  if (stat (filename, & st) < 0)
    fatal (_("cannot stat: %s: %s"), filename, strerror (errno));
  if (st.st_size == 0)
    return;

  buffer = (char *) xmalloc (st.st_size + 2);
  f = fopen (filename, FOPEN_RT);
  if (f == NULL)
    fatal (_("cannot open: %s: %s"), filename, strerror (errno));

  if (fread (buffer, 1, st.st_size, f) == 0 || ferror (f))
    fatal (_("%s: fread failed"), filename);

  fclose (f);
  buffer [st.st_size] = '\n';
  buffer [st.st_size + 1] = '\0';

  line_count = 1;

  for (line = buffer; * line != '\0'; line ++)
    {
      char * eol;
      char * name;
      char * name_end;
      int finished = FALSE;

      for (eol = line;; eol ++)
	{
	  switch (* eol)
	    {
	    case '\n':
	      * eol = '\0';
	      /* Cope with \n\r.  */
	      if (eol[1] == '\r')
		++ eol;
	      finished = TRUE;
	      break;

	    case '\r':
	      * eol = '\0';
	      /* Cope with \r\n.  */
	      if (eol[1] == '\n')
		++ eol;
	      finished = TRUE;
	      break;

	    case 0:
	      finished = TRUE;
	      break;

	    case '#':
	      /* Line comment, Terminate the line here, in case a
		 name is present and then allow the rest of the
		 loop to find the real end of the line.  */
	      * eol = '\0';
	      break;

	    default:
	      break;
	    }

	  if (finished)
	    break;
	}

      /* A name may now exist somewhere between 'line' and 'eol'.
	 Strip off leading whitespace and trailing whitespace,
	 then add it to the list.  */
      for (name = line; IS_WHITESPACE (* name); name ++)
	;
      for (name_end = name;
	   (! IS_WHITESPACE (* name_end))
	   && (! IS_LINE_TERMINATOR (* name_end));
	   name_end ++)
	;

      if (! IS_LINE_TERMINATOR (* name_end))
	{
	  char * extra;

	  for (extra = name_end + 1; IS_WHITESPACE (* extra); extra ++)
	    ;

	  if (! IS_LINE_TERMINATOR (* extra))
	    non_fatal (_("Ignoring rubbish found on line %d of %s"),
		       line_count, filename);
	}

      * name_end = '\0';

      if (name_end > name)
	add_specific_symbol (name, list);

      /* Advance line pointer to end of line.  The 'eol ++' in the for
	 loop above will then advance us to the start of the next line.  */
      line = eol;
      line_count ++;
    }
}

/* See whether a symbol should be stripped or kept based on
   strip_specific_list and keep_symbols.  */

static bfd_boolean
is_specified_symbol (name, list)
     const char *name;
     struct symlist *list;
{
  struct symlist *tmp_list;

  for (tmp_list = list; tmp_list; tmp_list = tmp_list->next)
    if (strcmp (name, tmp_list->name) == 0)
      return TRUE;

  return FALSE;
}

/* See if a section is being removed.  */

static bfd_boolean
is_strip_section (abfd, sec)
     bfd *abfd ATTRIBUTE_UNUSED;
     asection *sec;
{
  struct section_list *p;

  if ((bfd_get_section_flags (abfd, sec) & SEC_DEBUGGING) != 0
      && (strip_symbols == STRIP_DEBUG
	  || strip_symbols == STRIP_UNNEEDED
	  || strip_symbols == STRIP_ALL
	  || discard_locals == LOCALS_ALL
	  || convert_debugging))
    return TRUE;

  if (! sections_removed && ! sections_copied)
    return FALSE;

  p = find_section_list (bfd_get_section_name (abfd, sec), FALSE);
  if (sections_removed && p != NULL && p->remove)
    return TRUE;
  if (sections_copied && (p == NULL || ! p->copy))
    return TRUE;
  return FALSE;
}

/* Choose which symbol entries to copy; put the result in OSYMS.
   We don't copy in place, because that confuses the relocs.
   Return the number of symbols to print.  */

static unsigned int
filter_symbols (abfd, obfd, osyms, isyms, symcount)
     bfd *abfd;
     bfd *obfd;
     asymbol **osyms, **isyms;
     long symcount;
{
  register asymbol **from = isyms, **to = osyms;
  long src_count = 0, dst_count = 0;
  int relocatable = (abfd->flags & (HAS_RELOC | EXEC_P | DYNAMIC))
		    == HAS_RELOC;

  for (; src_count < symcount; src_count++)
    {
      asymbol *sym = from[src_count];
      flagword flags = sym->flags;
      char *name = (char *) bfd_asymbol_name (sym);
      int keep;
      bfd_boolean undefined;
#ifdef PIC30
      bfd_boolean local_absolute;
#endif
      bfd_boolean rem_leading_char;
      bfd_boolean add_leading_char;

      undefined = bfd_is_und_section (bfd_get_section (sym));
#ifdef PIC30
      /*
      ** Support --strip-local-absolute option, but don't strip
      **  info symbols created by the compiler
      */
      local_absolute = bfd_is_abs_section (bfd_get_section (sym)) &&
         ((flags & (BSF_GLOBAL | BSF_WEAK)) == 0) &&
         (strstr(sym->name, "__C30") == 0);
#endif

      if (redefine_sym_list)
	{
	  char *old_name, *new_name;

	  old_name = (char *) bfd_asymbol_name (sym);
	  new_name = (char *) lookup_sym_redefinition (old_name);
	  bfd_asymbol_name (sym) = new_name;
	  name = new_name;
	}

      /* Check if we will remove the current leading character.  */
      rem_leading_char =
	(name[0] == bfd_get_symbol_leading_char (abfd))
	&& (change_leading_char
	    || (remove_leading_char
		&& ((flags & (BSF_GLOBAL | BSF_WEAK)) != 0
		    || undefined
		    || bfd_is_com_section (bfd_get_section (sym)))));

      /* Check if we will add a new leading character.  */
      add_leading_char =
	change_leading_char
	&& (bfd_get_symbol_leading_char (obfd) != '\0')
	&& (bfd_get_symbol_leading_char (abfd) == '\0'
	    || (name[0] == bfd_get_symbol_leading_char (abfd)));

      /* Short circuit for change_leading_char if we can do it in-place.  */
      if (rem_leading_char && add_leading_char && !prefix_symbols_string)
        {
	  name[0] = bfd_get_symbol_leading_char (obfd);
	  bfd_asymbol_name (sym) = name;
	  rem_leading_char = FALSE;
	  add_leading_char = FALSE;
        }

      /* Remove leading char.  */
      if (rem_leading_char)
	bfd_asymbol_name (sym) = ++name;

      /* Add new leading char and/or prefix.  */
      if (add_leading_char || prefix_symbols_string)
        {
          char *n, *ptr;

          ptr = n = xmalloc (1 + strlen (prefix_symbols_string) + strlen (name) + 1);
          if (add_leading_char)
	    *ptr++ = bfd_get_symbol_leading_char (obfd);

          if (prefix_symbols_string)
            {
              strcpy (ptr, prefix_symbols_string);
              ptr += strlen (prefix_symbols_string);
           }

          strcpy (ptr, name);
          bfd_asymbol_name (sym) = n;
          name = n;
	}

      if (strip_symbols == STRIP_ALL)
	keep = 0;
      else if ((flags & BSF_KEEP) != 0		/* Used in relocation.  */
	       || ((flags & BSF_SECTION_SYM) != 0
		   && ((*bfd_get_section (sym)->symbol_ptr_ptr)->flags
		       & BSF_KEEP) != 0))
	keep = 1;
      else if (relocatable			/* Relocatable file.  */
	       && (flags & (BSF_GLOBAL | BSF_WEAK)) != 0)
	keep = 1;
      else if (bfd_decode_symclass (sym) == 'I')
	/* Global symbols in $idata sections need to be retained
	   even if relocatable is FALSE.  External users of the
	   library containing the $idata section may reference these
	   symbols.  */
	keep = 1;
      else if ((flags & BSF_GLOBAL) != 0	/* Global symbol.  */
	       || (flags & BSF_WEAK) != 0
	       || undefined
	       || bfd_is_com_section (bfd_get_section (sym)))
	keep = strip_symbols != STRIP_UNNEEDED;
      else if ((flags & BSF_DEBUGGING) != 0)	/* Debugging symbol.  */
	keep = (strip_symbols != STRIP_DEBUG
		&& strip_symbols != STRIP_UNNEEDED
		&& ! convert_debugging);
      else if (bfd_get_section (sym)->comdat)
	/* COMDAT sections store special information in local
	   symbols, so we cannot risk stripping any of them.  */
	keep = 1;
      else			/* Local symbol.  */
	keep = (strip_symbols != STRIP_UNNEEDED
		&& (discard_locals != LOCALS_ALL
		    && (discard_locals != LOCALS_START_L
			|| ! bfd_is_local_label (abfd, sym))));

      if (keep && is_specified_symbol (name, strip_specific_list))
	keep = 0;
      if (!keep && is_specified_symbol (name, keep_specific_list))
	keep = 1;
      if (keep && is_strip_section (abfd, bfd_get_section (sym)))
	keep = 0;
#ifdef PIC30
      if (keep && local_absolute && (strip_symbols == STRIP_LOCAL_ABSOLUTE))
        keep = 0;
#endif

#ifdef PIC30
      if (keep_named && 
           (strstr(sym->name, keep_named) ||
            strstr(sym->name, EXT_ATTR_PREFIX) ||
            strstr(sym->name, LINKED_PREFIX))) keep = 1;
#endif

      if (keep && (flags & BSF_GLOBAL) != 0
	  && (weaken || is_specified_symbol (name, weaken_specific_list)))
	{
	  sym->flags &=~ BSF_GLOBAL;
	  sym->flags |= BSF_WEAK;
	}
      if (keep && !undefined && (flags & (BSF_GLOBAL | BSF_WEAK))
	  && (is_specified_symbol (name, localize_specific_list)
	      || (keepglobal_specific_list != NULL
		  && ! is_specified_symbol (name, keepglobal_specific_list))))
	{
	  sym->flags &= ~(BSF_GLOBAL | BSF_WEAK);
	  sym->flags |= BSF_LOCAL;
	}

      if (keep)
	to[dst_count++] = sym;
    }

  to[dst_count] = NULL;

  return dst_count;
}

/* Find the redefined name of symbol SOURCE.  */

static const char *
lookup_sym_redefinition (source)
     const char *source;
{
  struct redefine_node *list;

  for (list = redefine_sym_list; list != NULL; list = list->next)
    if (strcmp (source, list->source) == 0)
      return list->target;

  return source;
}

/* Add a node to a symbol redefine list.  */

static void
redefine_list_append (source, target)
     const char *source;
     const char *target;
{
  struct redefine_node **p;
  struct redefine_node *list;
  struct redefine_node *new_node;

  for (p = &redefine_sym_list; (list = *p) != NULL; p = &list->next)
    {
      if (strcmp (source, list->source) == 0)
	fatal (_("%s: Multiple redefinition of symbol \"%s\""),
	       "--redefine-sym",
	       source);

      if (strcmp (target, list->target) == 0)
	fatal (_("%s: Symbol \"%s\" is target of more than one redefinition"),
	       "--redefine-sym",
	       target);
    }

  new_node = (struct redefine_node *) xmalloc (sizeof (struct redefine_node));

  new_node->source = strdup (source);
  new_node->target = strdup (target);
  new_node->next = NULL;

  *p = new_node;
}

/* Keep only every `copy_byte'th byte in MEMHUNK, which is *SIZE bytes long.
   Adjust *SIZE.  */

static void
filter_bytes (memhunk, size)
     char *memhunk;
     bfd_size_type *size;
{
  char *from = memhunk + copy_byte, *to = memhunk, *end = memhunk + *size;

  for (; from < end; from += interleave)
    *to++ = *from;

  if (*size % interleave > (bfd_size_type) copy_byte)
    *size = (*size / interleave) + 1;
  else
    *size /= interleave;
}

/* Copy object file IBFD onto OBFD.  */

static void
copy_object (ibfd, obfd)
     bfd *ibfd;
     bfd *obfd;
{
  bfd_vma start;
  long symcount;
  asection **osections = NULL;
  bfd_size_type *gaps = NULL;
  bfd_size_type max_gap = 0;
  long symsize;
  PTR dhandle;
  enum bfd_architecture iarch;
  unsigned int imach;

  if (ibfd->xvec->byteorder != obfd->xvec->byteorder
      && ibfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN
      && obfd->xvec->byteorder != BFD_ENDIAN_UNKNOWN)
    {
      fatal (_("Unable to change endianness of input file(s)"));
      return;
    }

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    RETURN_NONFATAL (bfd_get_filename (obfd));

  if (verbose)
    printf (_("copy from %s(%s) to %s(%s)\n"),
	    bfd_get_filename (ibfd), bfd_get_target (ibfd),
	    bfd_get_filename (obfd), bfd_get_target (obfd));

  if (set_start_set)
    start = set_start;
  else
    start = bfd_get_start_address (ibfd);
  start += change_start;

  /* Neither the start address nor the flags
     need to be set for a core file.  */
  if (bfd_get_format (obfd) != bfd_core)
    {
      if (!bfd_set_start_address (obfd, start)
	  || !bfd_set_file_flags (obfd,
				  (bfd_get_file_flags (ibfd)
				   & bfd_applicable_file_flags (obfd))))
	RETURN_NONFATAL (bfd_get_filename (ibfd));
    }

  /* Copy architecture of input file to output file.  */
  iarch = bfd_get_arch (ibfd);
  imach = bfd_get_mach (ibfd);
  if (!bfd_set_arch_mach (obfd, iarch, imach)
      && (ibfd->target_defaulted
	  || bfd_get_arch (ibfd) != bfd_get_arch (obfd)))
    non_fatal (_("Warning: Output file cannot represent architecture %s"),
	       bfd_printable_arch_mach (bfd_get_arch (ibfd),
					bfd_get_mach (ibfd)));

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    RETURN_NONFATAL (bfd_get_filename (ibfd));

  if (isympp)
    free (isympp);

  if (osympp != isympp)
    free (osympp);

  /* BFD mandates that all output sections be created and sizes set before
     any output is done.  Thus, we traverse all sections multiple times.  */
  bfd_map_over_sections (ibfd, setup_section, (void *) obfd);

  if (add_sections != NULL)
    {
      struct section_add *padd;
      struct section_list *pset;

      for (padd = add_sections; padd != NULL; padd = padd->next)
	{
	  padd->section = bfd_make_section (obfd, padd->name);
	  if (padd->section == NULL)
	    {
	      non_fatal (_("can't create section `%s': %s"),
		       padd->name, bfd_errmsg (bfd_get_error ()));
	      status = 1;
	      return;
	    }
	  else
	    {
	      flagword flags;

	      if (! bfd_set_section_size (obfd, padd->section, padd->size))
		RETURN_NONFATAL (bfd_get_filename (obfd));

	      pset = find_section_list (padd->name, FALSE);
	      if (pset != NULL)
		pset->used = TRUE;

	      if (pset != NULL && pset->set_flags)
		flags = pset->flags | SEC_HAS_CONTENTS;
	      else
		flags = SEC_HAS_CONTENTS | SEC_READONLY | SEC_DATA;

	      if (! bfd_set_section_flags (obfd, padd->section, flags))
		RETURN_NONFATAL (bfd_get_filename (obfd));

	      if (pset != NULL)
		{
		  if (pset->change_vma != CHANGE_IGNORE)
		    if (! bfd_set_section_vma (obfd, padd->section, pset->vma_val))
		      RETURN_NONFATAL (bfd_get_filename (obfd));

		  if (pset->change_lma != CHANGE_IGNORE)
		    {
		      padd->section->lma = pset->lma_val;

		      if (! bfd_set_section_alignment
			  (obfd, padd->section,
			   bfd_section_alignment (obfd, padd->section)))
			RETURN_NONFATAL (bfd_get_filename (obfd));
		    }
		}
	    }
	}
    }

  if (gap_fill_set || pad_to_set)
    {
      asection **set;
      unsigned int c, i;

      /* We must fill in gaps between the sections and/or we must pad
	 the last section to a specified address.  We do this by
	 grabbing a list of the sections, sorting them by VMA, and
	 increasing the section sizes as required to fill the gaps.
	 We write out the gap contents below.  */

      c = bfd_count_sections (obfd);
      osections = (asection **) xmalloc (c * sizeof (asection *));
      set = osections;
      bfd_map_over_sections (obfd, get_sections, (void *) &set);

      qsort (osections, c, sizeof (asection *), compare_section_lma);

      gaps = (bfd_size_type *) xmalloc (c * sizeof (bfd_size_type));
      memset (gaps, 0, c * sizeof (bfd_size_type));

      if (gap_fill_set)
	{
	  for (i = 0; i < c - 1; i++)
	    {
	      flagword flags;
	      bfd_size_type size;
	      bfd_vma gap_start, gap_stop;

	      flags = bfd_get_section_flags (obfd, osections[i]);
	      if ((flags & SEC_HAS_CONTENTS) == 0
		  || (flags & SEC_LOAD) == 0)
		continue;

	      size = bfd_section_size (obfd, osections[i]);
	      gap_start = bfd_section_lma (obfd, osections[i]) + size;
	      gap_stop = bfd_section_lma (obfd, osections[i + 1]);
	      if (gap_start < gap_stop)
		{
		  if (! bfd_set_section_size (obfd, osections[i],
					      size + (gap_stop - gap_start)))
		    {
		      non_fatal (_("Can't fill gap after %s: %s"),
				 bfd_get_section_name (obfd, osections[i]),
				 bfd_errmsg (bfd_get_error ()));
		      status = 1;
		      break;
		    }
		  gaps[i] = gap_stop - gap_start;
		  if (max_gap < gap_stop - gap_start)
		    max_gap = gap_stop - gap_start;
		}
	    }
	}

      if (pad_to_set)
	{
	  bfd_vma lma;
	  bfd_size_type size;

	  lma = bfd_section_lma (obfd, osections[c - 1]);
	  size = bfd_section_size (obfd, osections[c - 1]);
	  if (lma + size < pad_to)
	    {
	      if (! bfd_set_section_size (obfd, osections[c - 1],
					  pad_to - lma))
		{
		  non_fatal (_("Can't add padding to %s: %s"),
			     bfd_get_section_name (obfd, osections[c - 1]),
			     bfd_errmsg (bfd_get_error ()));
		  status = 1;
		}
	      else
		{
		  gaps[c - 1] = pad_to - (lma + size);
		  if (max_gap < pad_to - (lma + size))
		    max_gap = pad_to - (lma + size);
		}
	    }
	}
    }

  /* Symbol filtering must happen after the output sections
     have been created, but before their contents are set.  */
  dhandle = NULL;
  symsize = bfd_get_symtab_upper_bound (ibfd);
  if (symsize < 0)
    RETURN_NONFATAL (bfd_get_filename (ibfd));

  osympp = isympp = (asymbol **) xmalloc (symsize);
  symcount = bfd_canonicalize_symtab (ibfd, isympp);
  if (symcount < 0)
    RETURN_NONFATAL (bfd_get_filename (ibfd));

  if (convert_debugging)
    dhandle = read_debugging_info (ibfd, isympp, symcount);

  if (strip_symbols == STRIP_DEBUG
      || strip_symbols == STRIP_ALL
      || strip_symbols == STRIP_UNNEEDED
#ifdef PIC30
      || strip_symbols == STRIP_LOCAL_ABSOLUTE
#endif
      || discard_locals != LOCALS_UNDEF
      || strip_specific_list != NULL
      || keep_specific_list != NULL
      || localize_specific_list != NULL
      || keepglobal_specific_list != NULL
      || weaken_specific_list != NULL
      || prefix_symbols_string
      || sections_removed
      || sections_copied
      || convert_debugging
      || change_leading_char
      || remove_leading_char
      || redefine_sym_list
      || weaken)
    {
      /* Mark symbols used in output relocations so that they
	 are kept, even if they are local labels or static symbols.

	 Note we iterate over the input sections examining their
	 relocations since the relocations for the output sections
	 haven't been set yet.  mark_symbols_used_in_relocations will
	 ignore input sections which have no corresponding output
	 section.  */
      if (strip_symbols != STRIP_ALL)
	bfd_map_over_sections (ibfd,
			       mark_symbols_used_in_relocations,
			       (PTR)isympp);
      osympp = (asymbol **) xmalloc ((symcount + 1) * sizeof (asymbol *));
      symcount = filter_symbols (ibfd, obfd, osympp, isympp, symcount);
    }

  if (convert_debugging && dhandle != NULL)
    {
      if (! write_debugging_info (obfd, dhandle, &symcount, &osympp))
	{
	  status = 1;
	  return;
	}
    }

  bfd_set_symtab (obfd, osympp, symcount);

  /* This has to happen after the symbol table has been set.  */
  bfd_map_over_sections (ibfd, copy_section, (void *) obfd);

  if (add_sections != NULL)
    {
      struct section_add *padd;

      for (padd = add_sections; padd != NULL; padd = padd->next)
	{
	  if (! bfd_set_section_contents (obfd, padd->section,
					  (PTR) padd->contents,
					  (file_ptr) 0,
					  (bfd_size_type) padd->size))
	    RETURN_NONFATAL (bfd_get_filename (obfd));
	}
    }

  if (gap_fill_set || pad_to_set)
    {
      bfd_byte *buf;
      int c, i;

      /* Fill in the gaps.  */
      if (max_gap > 8192)
	max_gap = 8192;
      buf = (bfd_byte *) xmalloc (max_gap);
      memset (buf, gap_fill, (size_t) max_gap);

      c = bfd_count_sections (obfd);
      for (i = 0; i < c; i++)
	{
	  if (gaps[i] != 0)
	    {
	      bfd_size_type left;
	      file_ptr off;

	      left = gaps[i];
	      off = bfd_section_size (obfd, osections[i]) - left;

	      while (left > 0)
		{
		  bfd_size_type now;

		  if (left > 8192)
		    now = 8192;
		  else
		    now = left;

		  if (! bfd_set_section_contents (obfd, osections[i], buf,
						  off, now))
		    RETURN_NONFATAL (bfd_get_filename (obfd));

		  left -= now;
		  off += now;
		}
	    }
	}
    }

  /* Allow the BFD backend to copy any private data it understands
     from the input BFD to the output BFD.  This is done last to
     permit the routine to look at the filtered symbol table, which is
     important for the ECOFF code at least.  */
  if (! bfd_copy_private_bfd_data (ibfd, obfd))
    {
      non_fatal (_("%s: error copying private BFD data: %s"),
		 bfd_get_filename (obfd),
		 bfd_errmsg (bfd_get_error ()));
      status = 1;
      return;
    }

  /* Switch to the alternate machine code.  We have to do this at the
     very end, because we only initialize the header when we create
     the first section.  */
  if (use_alt_mach_code != 0)
    {
      if (!bfd_alt_mach_code (obfd, use_alt_mach_code))
	non_fatal (_("unknown alternate machine code, ignored"));
    }
}

#undef MKDIR
#if defined (_WIN32) && !defined (__CYGWIN32__)
#define MKDIR(DIR, MODE) mkdir (DIR)
#else
#define MKDIR(DIR, MODE) mkdir (DIR, MODE)
#endif

/* Read each archive element in turn from IBFD, copy the
   contents to temp file, and keep the temp file handle.  */

static void
copy_archive (ibfd, obfd, output_target)
     bfd *ibfd;
     bfd *obfd;
     const char *output_target;
{
  struct name_list
    {
      struct name_list *next;
      const char *name;
      bfd *obfd;
    } *list, *l;
  bfd **ptr = &obfd->archive_head;
  bfd *this_element;
  char *dir = make_tempname (bfd_get_filename (obfd));

  /* Make a temp directory to hold the contents.  */
  if (MKDIR (dir, 0700) != 0)
    {
      fatal (_("cannot mkdir %s for archive copying (error: %s)"),
	     dir, strerror (errno));
    }
  obfd->has_armap = ibfd->has_armap;

  list = NULL;

  this_element = bfd_openr_next_archived_file (ibfd, NULL);

  if (!bfd_set_format (obfd, bfd_get_format (ibfd)))
    RETURN_NONFATAL (bfd_get_filename (obfd));

  while (!status && this_element != (bfd *) NULL)
    {
      char *output_name;
      bfd *output_bfd;
      bfd *last_element;
      struct stat buf;
      int stat_status = 0;

      /* Create an output file for this member.  */
      output_name = concat (dir, "/",
			    bfd_get_filename (this_element), (char *) 0);

      /* If the file already exists, make another temp dir.  */
      if (stat (output_name, &buf) >= 0)
	{
	  output_name = make_tempname (output_name);
	  if (MKDIR (output_name, 0700) != 0)
	    {
	      fatal (_("cannot mkdir %s for archive copying (error: %s)"),
		     output_name, strerror (errno));
	    }
	  l = (struct name_list *) xmalloc (sizeof (struct name_list));
	  l->name = output_name;
	  l->next = list;
	  l->obfd = NULL;
	  list = l;
	  output_name = concat (output_name, "/",
				bfd_get_filename (this_element), (char *) 0);
	}

      output_bfd = bfd_openw (output_name, output_target);
      if (preserve_dates)
	{
	  stat_status = bfd_stat_arch_elt (this_element, &buf);

	  if (stat_status != 0)
	    non_fatal (_("internal stat error on %s"),
		       bfd_get_filename (this_element));
	}

      l = (struct name_list *) xmalloc (sizeof (struct name_list));
      l->name = output_name;
      l->next = list;
      list = l;

      if (output_bfd == (bfd *) NULL)
	RETURN_NONFATAL (output_name);

      if (bfd_check_format (this_element, bfd_object))
	copy_object (this_element, output_bfd);

      if (!bfd_close (output_bfd))
	{
	  bfd_nonfatal (bfd_get_filename (output_bfd));
	  /* Error in new object file. Don't change archive.  */
	  status = 1;
	}

      if (preserve_dates && stat_status == 0)
	set_times (output_name, &buf);

      /* Open the newly output file and attach to our list.  */
      output_bfd = bfd_openr (output_name, output_target);

      l->obfd = output_bfd;

      *ptr = output_bfd;
      ptr = &output_bfd->next;

      last_element = this_element;

      this_element = bfd_openr_next_archived_file (ibfd, last_element);

      bfd_close (last_element);
    }
  *ptr = (bfd *) NULL;

  if (!bfd_close (obfd))
    RETURN_NONFATAL (bfd_get_filename (obfd));

  if (!bfd_close (ibfd))
    RETURN_NONFATAL (bfd_get_filename (ibfd));

  /* Delete all the files that we opened.  */
  for (l = list; l != NULL; l = l->next)
    {
      if (l->obfd == NULL)
	rmdir (l->name);
      else
	{
	  bfd_close (l->obfd);
	  unlink (l->name);
	}
    }
  rmdir (dir);
}

/* The top-level control.  */

static void
copy_file (input_filename, output_filename, input_target, output_target)
     const char *input_filename;
     const char *output_filename;
     const char *input_target;
     const char *output_target;
{
  bfd *ibfd;
  char **obj_matching;
  char **core_matching;

  /* To allow us to do "strip *" without dying on the first
     non-object file, failures are nonfatal.  */
  ibfd = bfd_openr (input_filename, input_target);
  if (ibfd == NULL)
    RETURN_NONFATAL (input_filename);

  if (bfd_check_format (ibfd, bfd_archive))
    {
      bfd *obfd;

      /* bfd_get_target does not return the correct value until
         bfd_check_format succeeds.  */
      if (output_target == NULL)
	output_target = bfd_get_target (ibfd);

      obfd = bfd_openw (output_filename, output_target);
      if (obfd == NULL)
	RETURN_NONFATAL (output_filename);

      copy_archive (ibfd, obfd, output_target);
    }
  else if (bfd_check_format_matches (ibfd, bfd_object, &obj_matching))
    {
      bfd *obfd;
    do_copy:
      /* bfd_get_target does not return the correct value until
         bfd_check_format succeeds.  */
      if (output_target == NULL)
	output_target = bfd_get_target (ibfd);

      obfd = bfd_openw (output_filename, output_target);
      if (obfd == NULL)
	RETURN_NONFATAL (output_filename);

      copy_object (ibfd, obfd);

      if (!bfd_close (obfd))
	RETURN_NONFATAL (output_filename);

      if (!bfd_close (ibfd))
	RETURN_NONFATAL (input_filename);
    }
  else
    {
      bfd_error_type obj_error = bfd_get_error ();
      bfd_error_type core_error;

      if (bfd_check_format_matches (ibfd, bfd_core, &core_matching))
	{
	  /* This probably can't happen..  */
	  if (obj_error == bfd_error_file_ambiguously_recognized)
	    free (obj_matching);
	  goto do_copy;
	}

      core_error = bfd_get_error ();
      /* Report the object error in preference to the core error.  */
      if (obj_error != core_error)
	bfd_set_error (obj_error);

      bfd_nonfatal (input_filename);

      if (obj_error == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (obj_matching);
	  free (obj_matching);
	}
      if (core_error == bfd_error_file_ambiguously_recognized)
	{
	  list_matching_formats (core_matching);
	  free (core_matching);
	}

      status = 1;
    }
}

/* Add a name to the section renaming list.  */

static void
add_section_rename (old_name, new_name, flags)
     const char * old_name;
     const char * new_name;
     flagword flags;
{
  section_rename * rename;

  /* Check for conflicts first.  */
  for (rename = section_rename_list; rename != NULL; rename = rename->next)
    if (strcmp (rename->old_name, old_name) == 0)
      {
	/* Silently ignore duplicate definitions.  */
	if (strcmp (rename->new_name, new_name) == 0
	    && rename->flags == flags)
	  return;

	fatal (_("Multiple renames of section %s"), old_name);
      }

  rename = (section_rename *) xmalloc (sizeof (* rename));

  rename->old_name = old_name;
  rename->new_name = new_name;
  rename->flags    = flags;
  rename->next     = section_rename_list;

  section_rename_list = rename;
}

/* Check the section rename list for a new name of the input section
   ISECTION.  Return the new name if one is found.
   Also set RETURNED_FLAGS to the flags to be used for this section.  */

static const char *
find_section_rename (ibfd, isection, returned_flags)
     bfd * ibfd ATTRIBUTE_UNUSED;
     sec_ptr isection;
     flagword * returned_flags;
{
  const char * old_name = bfd_section_name (ibfd, isection);
  section_rename * rename;

  /* Default to using the flags of the input section.  */
  * returned_flags = bfd_get_section_flags (ibfd, isection);

  for (rename = section_rename_list; rename != NULL; rename = rename->next)
    if (strcmp (rename->old_name, old_name) == 0)
      {
	if (rename->flags != (flagword) -1)
	  * returned_flags = rename->flags;

	return rename->new_name;
      }

  return old_name;
}

/* Create a section in OBFD with the same
   name and attributes as ISECTION in IBFD.  */

static void
setup_section (ibfd, isection, obfdarg)
     bfd *ibfd;
     sec_ptr isection;
     PTR obfdarg;
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  sec_ptr osection;
  bfd_size_type size;
  bfd_vma vma;
  bfd_vma lma;
  flagword flags;
  const char *err;
  const char * name;
  char *prefix = NULL;

  if ((bfd_get_section_flags (ibfd, isection) & SEC_DEBUGGING) != 0
      && (strip_symbols == STRIP_DEBUG
	  || strip_symbols == STRIP_UNNEEDED
	  || strip_symbols == STRIP_ALL
	  || discard_locals == LOCALS_ALL
	  || convert_debugging))
    return;

  p = find_section_list (bfd_section_name (ibfd, isection), FALSE);
  if (p != NULL)
    p->used = TRUE;

  if (sections_removed && p != NULL && p->remove)
    return;
  if (sections_copied && (p == NULL || ! p->copy))
    return;

  /* Get the, possibly new, name of the output section.  */
  name = find_section_rename (ibfd, isection, & flags);

  /* Prefix sections.  */
  if ((prefix_alloc_sections_string) && (bfd_get_section_flags (ibfd, isection) & SEC_ALLOC))
    prefix = prefix_alloc_sections_string;
  else if (prefix_sections_string)
    prefix = prefix_sections_string;

  if (prefix)
    {
      char *n;

      n = xmalloc (strlen (prefix) + strlen (name) + 1);
      strcpy (n, prefix);
      strcat (n, name);
      name = n;
    }

  osection = bfd_make_section_anyway (obfd, name);

  if (osection == NULL)
    {
      err = _("making");
      goto loser;
    }

#ifdef PIC30
  /* copy PIC30 specific flags */
#define COPY_FLAG(flag) osection->flag = isection->flag
  COPY_FLAG(near);
  COPY_FLAG(persistent);
  COPY_FLAG(xmemory);
  COPY_FLAG(ymemory);
  COPY_FLAG(psv);
  COPY_FLAG(eedata);
  COPY_FLAG(memory);
  COPY_FLAG(absolute);
  COPY_FLAG(reverse);
  COPY_FLAG(dma);
  COPY_FLAG(boot);
  COPY_FLAG(secure);
  COPY_FLAG(heap);
  COPY_FLAG(stack);
  COPY_FLAG(eds);
  COPY_FLAG(page);
  COPY_FLAG(auxflash);
  COPY_FLAG(packedflash);
  COPY_FLAG(linked);
  COPY_FLAG(shared);
  COPY_FLAG(preserved);
  COPY_FLAG(auxpsv);
  COPY_FLAG(linker_generated);
  COPY_FLAG(update);
  /* linker_generated flag - not required to copy */
#undef COPY_FLAG
#endif

  size = bfd_section_size (ibfd, isection);
  if (copy_byte >= 0)
    size = (size + interleave - 1) / interleave;
  if (! bfd_set_section_size (obfd, osection, size))
    {
      err = _("size");
      goto loser;
    }

  vma = bfd_section_vma (ibfd, isection);
  if (p != NULL && p->change_vma == CHANGE_MODIFY)
    vma += p->vma_val;
  else if (p != NULL && p->change_vma == CHANGE_SET)
    vma = p->vma_val;
  else
    vma += change_section_address;

  if (! bfd_set_section_vma (obfd, osection, vma))
    {
      err = _("vma");
      goto loser;
    }

  lma = isection->lma;
  if ((p != NULL) && p->change_lma != CHANGE_IGNORE)
    {
      if (p->change_lma == CHANGE_MODIFY)
	lma += p->lma_val;
      else if (p->change_lma == CHANGE_SET)
	lma = p->lma_val;
      else
	abort ();
    }
  else
    lma += change_section_address;

  osection->lma = lma;

  /* FIXME: This is probably not enough.  If we change the LMA we
     may have to recompute the header for the file as well.  */
  if (!bfd_set_section_alignment (obfd,
				  osection,
				  bfd_section_alignment (ibfd, isection)))
    {
      err = _("alignment");
      goto loser;
    }

  if (p != NULL && p->set_flags)
    flags = p->flags | (flags & (SEC_HAS_CONTENTS | SEC_RELOC));
  if (!bfd_set_section_flags (obfd, osection, flags))
    {
      err = _("flags");
      goto loser;
    }

  /* Copy merge entity size.  */
  osection->entsize = isection->entsize;

  /* This used to be mangle_section; we do here to avoid using
     bfd_get_section_by_name since some formats allow multiple
     sections with the same name.  */
  isection->output_section = osection;
  isection->output_offset = 0;

  /* Allow the BFD backend to copy any private data it understands
     from the input section to the output section.  */
  if (!bfd_copy_private_section_data (ibfd, isection, obfd, osection))
    {
      err = _("private data");
      goto loser;
    }

  /* All went well.  */
  return;

loser:
  non_fatal (_("%s: section `%s': error in %s: %s"),
	     bfd_get_filename (ibfd),
	     bfd_section_name (ibfd, isection),
	     err, bfd_errmsg (bfd_get_error ()));
  status = 1;
}

/* Copy the data of input section ISECTION of IBFD
   to an output section with the same name in OBFD.
   If stripping then don't copy any relocation info.  */

static void
copy_section (ibfd, isection, obfdarg)
     bfd *ibfd;
     sec_ptr isection;
     PTR obfdarg;
{
  bfd *obfd = (bfd *) obfdarg;
  struct section_list *p;
  arelent **relpp;
  long relcount;
  sec_ptr osection;
  bfd_size_type size;
  long relsize;
  flagword flags;

  /* If we have already failed earlier on,
     do not keep on generating complaints now.  */
  if (status != 0)
    return;

  flags = bfd_get_section_flags (ibfd, isection);
  if ((flags & SEC_DEBUGGING) != 0
      && (strip_symbols == STRIP_DEBUG
	  || strip_symbols == STRIP_UNNEEDED
	  || strip_symbols == STRIP_ALL
	  || discard_locals == LOCALS_ALL
	  || convert_debugging))
    return;

  if ((flags & SEC_GROUP) != 0)
    return;

  p = find_section_list (bfd_section_name (ibfd, isection), FALSE);

  if (sections_removed && p != NULL && p->remove)
    return;
  if (sections_copied && (p == NULL || ! p->copy))
    return;

  osection = isection->output_section;
  size = bfd_get_section_size_before_reloc (isection);

  if (size == 0 || osection == 0)
    return;

  /* Core files do not need to be relocated.  */
  if (bfd_get_format (obfd) == bfd_core)
    relsize = 0;
  else
    relsize = bfd_get_reloc_upper_bound (ibfd, isection);

  if (relsize < 0)
    RETURN_NONFATAL (bfd_get_filename (ibfd));

  if (relsize == 0)
    bfd_set_reloc (obfd, osection, (arelent **) NULL, 0);
  else
    {
      relpp = (arelent **) xmalloc (relsize);
      relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, isympp);
      if (relcount < 0)
	RETURN_NONFATAL (bfd_get_filename (ibfd));

      if (strip_symbols == STRIP_ALL)
	{
	  /* Remove relocations which are not in
	     keep_strip_specific_list.  */
	  arelent **temp_relpp;
	  long temp_relcount = 0;
	  long i;

	  temp_relpp = (arelent **) xmalloc (relsize);
	  for (i = 0; i < relcount; i++)
	    if (is_specified_symbol
		(bfd_asymbol_name (*relpp [i]->sym_ptr_ptr),
		 keep_specific_list))
	      temp_relpp [temp_relcount++] = relpp [i];
	  relcount = temp_relcount;
	  free (relpp);
	  relpp = temp_relpp;
	}

      bfd_set_reloc (obfd, osection,
		     (relcount == 0 ? (arelent **) NULL : relpp), relcount);
    }

  isection->_cooked_size = isection->_raw_size;
  isection->reloc_done = TRUE;

  if (bfd_get_section_flags (ibfd, isection) & SEC_HAS_CONTENTS
      && bfd_get_section_flags (obfd, osection) & SEC_HAS_CONTENTS)
    {
      PTR memhunk = (PTR) xmalloc ((unsigned) size);

      if (!bfd_get_section_contents (ibfd, isection, memhunk, (file_ptr) 0,
				     size))
	RETURN_NONFATAL (bfd_get_filename (ibfd));

      if (copy_byte >= 0)
	filter_bytes (memhunk, &size);

      if (!bfd_set_section_contents (obfd, osection, memhunk, (file_ptr) 0,
				     size))
	RETURN_NONFATAL (bfd_get_filename (obfd));

      free (memhunk);
    }
  else if (p != NULL && p->set_flags && (p->flags & SEC_HAS_CONTENTS) != 0)
    {
      PTR memhunk = (PTR) xmalloc ((unsigned) size);

      /* We don't permit the user to turn off the SEC_HAS_CONTENTS
	 flag--they can just remove the section entirely and add it
	 back again.  However, we do permit them to turn on the
	 SEC_HAS_CONTENTS flag, and take it to mean that the section
	 contents should be zeroed out.  */

      memset (memhunk, 0, size);
      if (! bfd_set_section_contents (obfd, osection, memhunk, (file_ptr) 0,
				      size))
	RETURN_NONFATAL (bfd_get_filename (obfd));
      free (memhunk);
    }
}

/* Get all the sections.  This is used when --gap-fill or --pad-to is
   used.  */

static void
get_sections (obfd, osection, secppparg)
     bfd *obfd ATTRIBUTE_UNUSED;
     asection *osection;
     PTR secppparg;
{
  asection ***secppp = (asection ***) secppparg;

  **secppp = osection;
  ++(*secppp);
}

/* Sort sections by VMA.  This is called via qsort, and is used when
   --gap-fill or --pad-to is used.  We force non loadable or empty
   sections to the front, where they are easier to ignore.  */

static int
compare_section_lma (arg1, arg2)
     const PTR arg1;
     const PTR arg2;
{
  const asection **sec1 = (const asection **) arg1;
  const asection **sec2 = (const asection **) arg2;
  flagword flags1, flags2;

  /* Sort non loadable sections to the front.  */
  flags1 = (*sec1)->flags;
  flags2 = (*sec2)->flags;
  if ((flags1 & SEC_HAS_CONTENTS) == 0
      || (flags1 & SEC_LOAD) == 0)
    {
      if ((flags2 & SEC_HAS_CONTENTS) != 0
	  && (flags2 & SEC_LOAD) != 0)
	return -1;
    }
  else
    {
      if ((flags2 & SEC_HAS_CONTENTS) == 0
	  || (flags2 & SEC_LOAD) == 0)
	return 1;
    }

  /* Sort sections by LMA.  */
  if ((*sec1)->lma > (*sec2)->lma)
    return 1;
  else if ((*sec1)->lma < (*sec2)->lma)
    return -1;

  /* Sort sections with the same LMA by size.  */
  if ((*sec1)->_raw_size > (*sec2)->_raw_size)
    return 1;
  else if ((*sec1)->_raw_size < (*sec2)->_raw_size)
    return -1;

  return 0;
}

/* Mark all the symbols which will be used in output relocations with
   the BSF_KEEP flag so that those symbols will not be stripped.

   Ignore relocations which will not appear in the output file.  */

static void
mark_symbols_used_in_relocations (ibfd, isection, symbolsarg)
     bfd *ibfd;
     sec_ptr isection;
     PTR symbolsarg;
{
  asymbol **symbols = (asymbol **) symbolsarg;
  long relsize;
  arelent **relpp;
  long relcount, i;

  /* Ignore an input section with no corresponding output section.  */
  if (isection->output_section == NULL)
    return;

  relsize = bfd_get_reloc_upper_bound (ibfd, isection);
  if (relsize < 0)
    bfd_fatal (bfd_get_filename (ibfd));

  if (relsize == 0)
    return;

  relpp = (arelent **) xmalloc (relsize);
  relcount = bfd_canonicalize_reloc (ibfd, isection, relpp, symbols);
  if (relcount < 0)
    bfd_fatal (bfd_get_filename (ibfd));

  /* Examine each symbol used in a relocation.  If it's not one of the
     special bfd section symbols, then mark it with BSF_KEEP.  */
  for (i = 0; i < relcount; i++)
    {
      if (*relpp[i]->sym_ptr_ptr != bfd_com_section_ptr->symbol
	  && *relpp[i]->sym_ptr_ptr != bfd_abs_section_ptr->symbol
	  && *relpp[i]->sym_ptr_ptr != bfd_und_section_ptr->symbol)
	(*relpp[i]->sym_ptr_ptr)->flags |= BSF_KEEP;
    }

  if (relpp != NULL)
    free (relpp);
}

/* Write out debugging information.  */

static bfd_boolean
write_debugging_info (obfd, dhandle, symcountp, symppp)
     bfd *obfd;
     PTR dhandle;
     long *symcountp ATTRIBUTE_UNUSED;
     asymbol ***symppp ATTRIBUTE_UNUSED;
{
  if (bfd_get_flavour (obfd) == bfd_target_ieee_flavour)
    return write_ieee_debugging_info (obfd, dhandle);

  if (bfd_get_flavour (obfd) == bfd_target_coff_flavour
      || bfd_get_flavour (obfd) == bfd_target_elf_flavour)
    {
      bfd_byte *syms, *strings;
      bfd_size_type symsize, stringsize;
      asection *stabsec, *stabstrsec;

      if (! write_stabs_in_sections_debugging_info (obfd, dhandle, &syms,
						    &symsize, &strings,
						    &stringsize))
	return FALSE;

      stabsec = bfd_make_section (obfd, ".stab");
      stabstrsec = bfd_make_section (obfd, ".stabstr");
      if (stabsec == NULL
	  || stabstrsec == NULL
	  || ! bfd_set_section_size (obfd, stabsec, symsize)
	  || ! bfd_set_section_size (obfd, stabstrsec, stringsize)
	  || ! bfd_set_section_alignment (obfd, stabsec, 2)
	  || ! bfd_set_section_alignment (obfd, stabstrsec, 0)
	  || ! bfd_set_section_flags (obfd, stabsec,
				   (SEC_HAS_CONTENTS
				    | SEC_READONLY
				    | SEC_DEBUGGING))
	  || ! bfd_set_section_flags (obfd, stabstrsec,
				      (SEC_HAS_CONTENTS
				       | SEC_READONLY
				       | SEC_DEBUGGING)))
	{
	  non_fatal (_("%s: can't create debugging section: %s"),
		     bfd_get_filename (obfd),
		     bfd_errmsg (bfd_get_error ()));
	  return FALSE;
	}

      /* We can get away with setting the section contents now because
         the next thing the caller is going to do is copy over the
         real sections.  We may someday have to split the contents
         setting out of this function.  */
      if (! bfd_set_section_contents (obfd, stabsec, syms, (file_ptr) 0,
				      symsize)
	  || ! bfd_set_section_contents (obfd, stabstrsec, strings,
					 (file_ptr) 0, stringsize))
	{
	  non_fatal (_("%s: can't set debugging section contents: %s"),
		     bfd_get_filename (obfd),
		     bfd_errmsg (bfd_get_error ()));
	  return FALSE;
	}

      return TRUE;
    }

  non_fatal (_("%s: don't know how to write debugging information for %s"),
	     bfd_get_filename (obfd), bfd_get_target (obfd));
  return FALSE;
}

static int
strip_main (argc, argv)
     int argc;
     char *argv[];
{
  char *input_target = NULL;
  char *output_target = NULL;
  bfd_boolean show_version = FALSE;
  bfd_boolean formats_info = FALSE;
  int c;
  int i;
  struct section_list *p;
  char *output_file = NULL;

  while ((c = getopt_long (argc, argv, "I:O:F:K:N:R:o:sSpdgxXHhVv",
			   strip_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'I':
	  input_target = optarg;
	  break;
	case 'O':
	  output_target = optarg;
	  break;
	case 'F':
	  input_target = output_target = optarg;
	  break;
	case 'R':
	  p = find_section_list (optarg, TRUE);
	  p->remove = TRUE;
	  sections_removed = TRUE;
	  break;
	case 's':
	  strip_symbols = STRIP_ALL;
	  break;
	case 'S':
	case 'g':
	case 'd':	/* Historic BSD alias for -g.  Used by early NetBSD.  */
	  strip_symbols = STRIP_DEBUG;
	  break;
	case OPTION_STRIP_UNNEEDED:
	  strip_symbols = STRIP_UNNEEDED;
	  break;
	case 'K':
	  add_specific_symbol (optarg, &keep_specific_list);
	  break;
	case 'N':
	  add_specific_symbol (optarg, &strip_specific_list);
	  break;
	case 'o':
	  output_file = optarg;
	  break;
	case 'p':
	  preserve_dates = TRUE;
	  break;
	case 'x':
	  discard_locals = LOCALS_ALL;
	  break;
	case 'X':
	  discard_locals = LOCALS_START_L;
	  break;
	case 'v':
	  verbose = TRUE;
	  break;
	case 'V':
	  show_version = TRUE;
	  break;
	case OPTION_FORMATS_INFO:
	  formats_info = TRUE;
	  break;
#ifdef PIC30
	case OPTION_STRIP_LOCAL_ABSOLUTE:
	  strip_symbols = STRIP_LOCAL_ABSOLUTE;
          break;
	case OPTION_KEEP_NAMED:
	  keep_named = optarg;
          break;
#endif
	case 0:
	  /* We've been given a long option.  */
	  break;
	case 'H':
	case 'h':
	  strip_usage (stdout, 0);
	default:
	  strip_usage (stderr, 1);
	}
    }

 if (formats_info)
   {
     display_info ();
     return 0;
   }
 
  if (show_version)
    print_version ("strip");

  /* Default is to strip all symbols.  */
  if (strip_symbols == STRIP_UNDEF
      && discard_locals == LOCALS_UNDEF
      && strip_specific_list == NULL)
    strip_symbols = STRIP_ALL;

  if (output_target == (char *) NULL)
    output_target = input_target;

  i = optind;
  if (i == argc
      || (output_file != NULL && (i + 1) < argc))
    strip_usage (stderr, 1);

  for (; i < argc; i++)
    {
      int hold_status = status;
      struct stat statbuf;
      char *tmpname;

      if (preserve_dates)
	{
	  if (stat (argv[i], &statbuf) < 0)
	    {
	      non_fatal (_("%s: cannot stat: %s"), argv[i], strerror (errno));
	      continue;
	    }
	}

      if (output_file != NULL)
	tmpname = output_file;
      else
	tmpname = make_tempname (argv[i]);
      status = 0;

      copy_file (argv[i], tmpname, input_target, output_target);
      if (status == 0)
	{
	  if (preserve_dates)
	    set_times (tmpname, &statbuf);
	  if (output_file == NULL)
	    smart_rename (tmpname, argv[i], preserve_dates);
	  status = hold_status;
	}
      else
	unlink (tmpname);
      if (output_file == NULL)
	free (tmpname);
    }

  return 0;
}

static int
copy_main (argc, argv)
     int argc;
     char *argv[];
{
  char * binary_architecture = NULL;
  char *input_filename = NULL;
  char *output_filename = NULL;
  char *input_target = NULL;
  char *output_target = NULL;
  bfd_boolean show_version = FALSE;
  bfd_boolean change_warn = TRUE;
  bfd_boolean formats_info = FALSE;
  int c;
  struct section_list *p;
  struct stat statbuf;

  while ((c = getopt_long (argc, argv, "b:B:i:I:j:K:N:s:O:d:F:L:G:R:SpgxXHhVvW:",
			   copy_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'b':
	  copy_byte = atoi (optarg);
	  if (copy_byte < 0)
	    fatal (_("byte number must be non-negative"));
	  break;

	case 'B':
	  binary_architecture = optarg;
	  break;

	case 'i':
	  interleave = atoi (optarg);
	  if (interleave < 1)
	    fatal (_("interleave must be positive"));
	  break;

	case 'I':
	case 's':		/* "source" - 'I' is preferred */
	  input_target = optarg;
	  break;

	case 'O':
	case 'd':		/* "destination" - 'O' is preferred */
	  output_target = optarg;
	  break;

	case 'F':
	  input_target = output_target = optarg;
	  break;

	case 'j':
	  p = find_section_list (optarg, TRUE);
	  if (p->remove)
	    fatal (_("%s both copied and removed"), optarg);
	  p->copy = TRUE;
	  sections_copied = TRUE;
	  break;

	case 'R':
	  p = find_section_list (optarg, TRUE);
	  if (p->copy)
	    fatal (_("%s both copied and removed"), optarg);
	  p->remove = TRUE;
	  sections_removed = TRUE;
	  break;

	case 'S':
	  strip_symbols = STRIP_ALL;
	  break;

	case 'g':
	  strip_symbols = STRIP_DEBUG;
	  break;

	case OPTION_STRIP_UNNEEDED:
	  strip_symbols = STRIP_UNNEEDED;
	  break;

	case 'K':
	  add_specific_symbol (optarg, &keep_specific_list);
	  break;

	case 'N':
	  add_specific_symbol (optarg, &strip_specific_list);
	  break;

	case 'L':
	  add_specific_symbol (optarg, &localize_specific_list);
	  break;

	case 'G':
	  add_specific_symbol (optarg, &keepglobal_specific_list);
	  break;

	case 'W':
	  add_specific_symbol (optarg, &weaken_specific_list);
	  break;

	case 'p':
	  preserve_dates = TRUE;
	  break;

	case 'x':
	  discard_locals = LOCALS_ALL;
	  break;

	case 'X':
	  discard_locals = LOCALS_START_L;
	  break;

	case 'v':
	  verbose = TRUE;
	  break;

	case 'V':
	  show_version = TRUE;
	  break;

	case OPTION_FORMATS_INFO:
	  formats_info = TRUE;
	  break;

	case OPTION_WEAKEN:
	  weaken = TRUE;
	  break;

	case OPTION_ADD_SECTION:
	  {
	    const char *s;
	    struct stat st;
	    struct section_add *pa;
	    int len;
	    char *name;
	    FILE *f;

	    s = strchr (optarg, '=');

	    if (s == NULL)
	      fatal (_("bad format for %s"), "--add-section");

	    if (stat (s + 1, & st) < 0)
	      fatal (_("cannot stat: %s: %s"), s + 1, strerror (errno));

	    pa = (struct section_add *) xmalloc (sizeof (struct section_add));

	    len = s - optarg;
	    name = (char *) xmalloc (len + 1);
	    strncpy (name, optarg, len);
	    name[len] = '\0';
	    pa->name = name;

	    pa->filename = s + 1;

	    pa->size = st.st_size;

	    pa->contents = (bfd_byte *) xmalloc (pa->size);
	    f = fopen (pa->filename, FOPEN_RB);

	    if (f == NULL)
	      fatal (_("cannot open: %s: %s"), pa->filename, strerror (errno));

	    if (fread (pa->contents, 1, pa->size, f) == 0
		|| ferror (f))
	      fatal (_("%s: fread failed"), pa->filename);

	    fclose (f);

	    pa->next = add_sections;
	    add_sections = pa;
	  }
	  break;

	case OPTION_CHANGE_START:
	  change_start = parse_vma (optarg, "--change-start");
	  break;

	case OPTION_CHANGE_SECTION_ADDRESS:
	case OPTION_CHANGE_SECTION_LMA:
	case OPTION_CHANGE_SECTION_VMA:
	  {
	    const char *s;
	    int len;
	    char *name;
	    char *option = NULL;
	    bfd_vma val;
	    enum change_action what = CHANGE_IGNORE;

	    switch (c)
	      {
	      case OPTION_CHANGE_SECTION_ADDRESS:
		option = "--change-section-address";
		break;
	      case OPTION_CHANGE_SECTION_LMA:
		option = "--change-section-lma";
		break;
	      case OPTION_CHANGE_SECTION_VMA:
		option = "--change-section-vma";
		break;
	      }

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      {
		s = strchr (optarg, '+');
		if (s == NULL)
		  {
		    s = strchr (optarg, '-');
		    if (s == NULL)
		      fatal (_("bad format for %s"), option);
		  }
	      }

	    len = s - optarg;
	    name = (char *) xmalloc (len + 1);
	    strncpy (name, optarg, len);
	    name[len] = '\0';

	    p = find_section_list (name, TRUE);

	    val = parse_vma (s + 1, option);

	    switch (*s)
	      {
	      case '=': what = CHANGE_SET; break;
	      case '-': val  = - val; /* Drop through.  */
	      case '+': what = CHANGE_MODIFY; break;
	      }

	    switch (c)
	      {
	      case OPTION_CHANGE_SECTION_ADDRESS:
		p->change_vma = what;
		p->vma_val    = val;
		/* Drop through.  */

	      case OPTION_CHANGE_SECTION_LMA:
		p->change_lma = what;
		p->lma_val    = val;
		break;

	      case OPTION_CHANGE_SECTION_VMA:
		p->change_vma = what;
		p->vma_val    = val;
		break;
	      }
	  }
	  break;

	case OPTION_CHANGE_ADDRESSES:
	  change_section_address = parse_vma (optarg, "--change-addresses");
	  change_start = change_section_address;
	  break;

	case OPTION_CHANGE_WARNINGS:
	  change_warn = TRUE;
	  break;

	case OPTION_CHANGE_LEADING_CHAR:
	  change_leading_char = TRUE;
	  break;

	case OPTION_DEBUGGING:
	  convert_debugging = TRUE;
	  break;

	case OPTION_GAP_FILL:
	  {
	    bfd_vma gap_fill_vma;

	    gap_fill_vma = parse_vma (optarg, "--gap-fill");
	    gap_fill = (bfd_byte) gap_fill_vma;
	    if ((bfd_vma) gap_fill != gap_fill_vma)
	      {
		char buff[20];

		sprintf_vma (buff, gap_fill_vma);

		non_fatal (_("Warning: truncating gap-fill from 0x%s to 0x%x"),
			   buff, gap_fill);
	      }
	    gap_fill_set = TRUE;
	  }
	  break;

	case OPTION_NO_CHANGE_WARNINGS:
	  change_warn = FALSE;
	  break;

	case OPTION_PAD_TO:
	  pad_to = parse_vma (optarg, "--pad-to");
	  pad_to_set = TRUE;
	  break;

	case OPTION_REMOVE_LEADING_CHAR:
	  remove_leading_char = TRUE;
	  break;

	case OPTION_REDEFINE_SYM:
	  {
	    /* Push this redefinition onto redefine_symbol_list.  */

	    int len;
	    const char *s;
	    const char *nextarg;
	    char *source, *target;

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      fatal (_("bad format for %s"), "--redefine-sym");

	    len = s - optarg;
	    source = (char *) xmalloc (len + 1);
	    strncpy (source, optarg, len);
	    source[len] = '\0';

	    nextarg = s + 1;
	    len = strlen (nextarg);
	    target = (char *) xmalloc (len + 1);
	    strcpy (target, nextarg);

	    redefine_list_append (source, target);

	    free (source);
	    free (target);
	  }
	  break;

	case OPTION_SET_SECTION_FLAGS:
	  {
	    const char *s;
	    int len;
	    char *name;

	    s = strchr (optarg, '=');
	    if (s == NULL)
	      fatal (_("bad format for %s"), "--set-section-flags");

	    len = s - optarg;
	    name = (char *) xmalloc (len + 1);
	    strncpy (name, optarg, len);
	    name[len] = '\0';

	    p = find_section_list (name, TRUE);

	    p->set_flags = TRUE;
	    p->flags = parse_flags (s + 1);
	  }
	  break;

	case OPTION_RENAME_SECTION:
	  {
	    flagword flags;
	    const char *eq, *fl;
	    char *old_name;
	    char *new_name;
	    unsigned int len;

	    eq = strchr (optarg, '=');
	    if (eq == NULL)
	      fatal (_("bad format for %s"), "--rename-section");

	    len = eq - optarg;
	    if (len == 0)
	      fatal (_("bad format for %s"), "--rename-section");

	    old_name = (char *) xmalloc (len + 1);
	    strncpy (old_name, optarg, len);
	    old_name[len] = 0;

	    eq++;
	    fl = strchr (eq, ',');
	    if (fl)
	      {
		flags = parse_flags (fl + 1);
		len = fl - eq;
	      }
	    else
	      {
		flags = -1;
		len = strlen (eq);
	      }

	    if (len == 0)
	      fatal (_("bad format for %s"), "--rename-section");

	    new_name = (char *) xmalloc (len + 1);
	    strncpy (new_name, eq, len);
	    new_name[len] = 0;

	    add_section_rename (old_name, new_name, flags);
	  }
	  break;

	case OPTION_SET_START:
	  set_start = parse_vma (optarg, "--set-start");
	  set_start_set = TRUE;
	  break;

	case OPTION_SREC_LEN:
	  Chunk = parse_vma (optarg, "--srec-len");
	  break;

	case OPTION_SREC_FORCES3:
	  S3Forced = TRUE;
	  break;

	case OPTION_STRIP_SYMBOLS:
	  add_specific_symbols (optarg, &strip_specific_list);
	  break;

	case OPTION_KEEP_SYMBOLS:
	  add_specific_symbols (optarg, &keep_specific_list);
	  break;

	case OPTION_LOCALIZE_SYMBOLS:
	  add_specific_symbols (optarg, &localize_specific_list);
	  break;

	case OPTION_KEEPGLOBAL_SYMBOLS:
	  add_specific_symbols (optarg, &keepglobal_specific_list);
	  break;

	case OPTION_WEAKEN_SYMBOLS:
	  add_specific_symbols (optarg, &weaken_specific_list);
	  break;

	case OPTION_ALT_MACH_CODE:
	  use_alt_mach_code = atoi (optarg);
	  if (use_alt_mach_code <= 0)
	    fatal (_("alternate machine code index must be positive"));
	  break;

	case OPTION_PREFIX_SYMBOLS:
	  prefix_symbols_string = optarg;
	  break;

	case OPTION_PREFIX_SECTIONS:
	  prefix_sections_string = optarg;
	  break;

	case OPTION_PREFIX_ALLOC_SECTIONS:
	  prefix_alloc_sections_string = optarg;
	  break;

	case 0:
	  break;		/* we've been given a long option */

	case 'H':
	case 'h':
	  copy_usage (stdout, 0);

	default:
	  copy_usage (stderr, 1);
	}
    }

  if (formats_info)
    {
      display_info ();
      return 0;
    }
 
  if (show_version)
    print_version ("objcopy");

  if (copy_byte >= interleave)
    fatal (_("byte number must be less than interleave"));

  if (optind == argc || optind + 2 < argc)
    copy_usage (stderr, 1);

  input_filename = argv[optind];
  if (optind + 1 < argc)
    output_filename = argv[optind + 1];

  /* Default is to strip no symbols.  */
  if (strip_symbols == STRIP_UNDEF && discard_locals == LOCALS_UNDEF)
    strip_symbols = STRIP_NONE;

  if (output_target == (char *) NULL)
    output_target = input_target;

  if (binary_architecture != (char *) NULL)
    {
      if (input_target && strcmp (input_target, "binary") == 0)
	{
	  const bfd_arch_info_type * temp_arch_info;

	  temp_arch_info = bfd_scan_arch (binary_architecture);

	  if (temp_arch_info != NULL)
	    bfd_external_binary_architecture = temp_arch_info->arch;
	  else
	    fatal (_("architecture %s unknown"), binary_architecture);
	}
      else
	{
	  non_fatal (_("Warning: input target 'binary' required for binary architecture parameter."));
	  non_fatal (_(" Argument %s ignored"), binary_architecture);
	}
    }

  if (preserve_dates)
    if (stat (input_filename, & statbuf) < 0)
      fatal (_("Cannot stat: %s: %s"), input_filename, strerror (errno));

  /* If there is no destination file, or the source and destination files
     are the same,  then create a temp and rename the result into the input.  */
  if ((output_filename == (char *) NULL) ||
      (strcmp (input_filename, output_filename) == 0))
    {
      char *tmpname = make_tempname (input_filename);

      copy_file (input_filename, tmpname, input_target, output_target);
      if (status == 0)
	{
	  if (preserve_dates)
	    set_times (tmpname, &statbuf);
	  smart_rename (tmpname, input_filename, preserve_dates);
	}
      else
	unlink (tmpname);
    }
  else
    {
      copy_file (input_filename, output_filename, input_target, output_target);

      if (status == 0 && preserve_dates)
	set_times (output_filename, &statbuf);
    }

  if (change_warn)
    {
      for (p = change_sections; p != NULL; p = p->next)
	{
	  if (! p->used)
	    {
	      if (p->change_vma != CHANGE_IGNORE)
		{
		  char buff [20];

		  sprintf_vma (buff, p->vma_val);

		  /* xgettext:c-format */
		  non_fatal (_("%s %s%c0x%s never used"),
			     "--change-section-vma",
			     p->name,
			     p->change_vma == CHANGE_SET ? '=' : '+',
			     buff);
		}

	      if (p->change_lma != CHANGE_IGNORE)
		{
		  char buff [20];

		  sprintf_vma (buff, p->lma_val);

		  /* xgettext:c-format */
		  non_fatal (_("%s %s%c0x%s never used"),
			     "--change-section-lma",
			     p->name,
			     p->change_lma == CHANGE_SET ? '=' : '+',
			     buff);
		}
	    }
	}
    }

  return 0;
}

int main PARAMS ((int, char **));

int
main (argc, argv)
     int argc;
     char *argv[];
{
#if defined (HAVE_SETLOCALE) && defined (HAVE_LC_MESSAGES)
  setlocale (LC_MESSAGES, "");
#endif
#if defined (HAVE_SETLOCALE)
  setlocale (LC_CTYPE, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  program_name = argv[0];
  xmalloc_set_program_name (program_name);

  START_PROGRESS (program_name, 0);

  strip_symbols = STRIP_UNDEF;
  discard_locals = LOCALS_UNDEF;

  bfd_init ();
  set_default_bfd_target ();

  if (is_strip < 0)
    {
      int i = strlen (program_name);
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
      /* Drop the .exe suffix, if any.  */
      if (i > 4 && FILENAME_CMP (program_name + i - 4, ".exe") == 0)
	{
	  i -= 4;
	  program_name[i] = '\0';
	}
#endif
      is_strip = (i >= 5 && FILENAME_CMP (program_name + i - 5, "strip") == 0);
    }

  if (is_strip)
    strip_main (argc, argv);
  else
    copy_main (argc, argv);

  END_PROGRESS (program_name);

  return status;
}
