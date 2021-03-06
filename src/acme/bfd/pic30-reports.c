/*
** pic30-reports.c
**
** Copyright (c) 2008 Microchip Technology, Inc.
**
** This file contains pic30-specifc reports
** processing for the linker.
**
** It is included by the following target-specific
** emulation files:
**
**   ld/emultmpl/pic30_coff.em
**   ld/emultmpl/pic30_elf32.em
**
*/

/*****************************************************************************/

#define REPORT_AS_PROGRAM(s) \
  (PIC30_IS_CODE_ATTR(s) || PIC30_IS_PSV_ATTR(s) || PIC30_IS_PACKEDFLASH_ATTR(s))

#define REPORT_AS_DATA(s) \
  (PIC30_SECTION_IN_DATA_MEMORY(s))

#define REPORT_AS_EEDATA(s) \
  (PIC30_IS_EEDATA_ATTR(s))

#define REPORT_AS_AUXFLASH(s) \
  (PIC30_IS_AUXFLASH_ATTR(s) || PIC30_IS_AUXPSV_ATTR(s) || PIC30_IS_PACKEDFLASH_ATTR(s))

/*****************************************************************************/
/* prototypes */
int indent_fprintf(FILE *, char *, ...);
extern char *pic30_resource_version;
extern struct pic30_mem_info_ pic30_mem_info;

int indent_fprintf(FILE *stream, char *format, ...) {
  int indent_level=0;
  static char buffer[] = "%12345s";
  static unsigned int nesting = 0;
  int indent_to = 0;
  va_list ap;
  char *c = format;
  char last_c;
  int result = 0;

  va_start(ap,format);
  last_c = 0;
  for (c = format; *c; c++) {
    switch (*c) {
      case '<': indent_to = 2 * nesting;
                nesting++;
                break;
      case '/': if (last_c == '<') {
                  nesting -= 2;
                  indent_to = 2 * nesting;
                  //indent_level -= 2;
                }
                else if (c[1] == '>') indent_to -= 2;
                break;
      default:  break;
    }
    last_c  = *c;
  }
  sprintf(buffer,"%%%ds", indent_to);
  fprintf(stream, buffer, "");
  result = vfprintf(stream, format, ap);
  va_end(ap);
  //indent_level += indent_to;
  if (indent_level < 0) indent_level = 0;
  if (indent_level > 40) indent_level = 40;
  return result;
}

/*
** Utility routine: bfd_pic30_report_program_sections()
*/ 
static void
bfd_pic30_report_program_sections (asection *sect, FILE *fp) {
  file_ptr start = sect->lma;
  bfd_size_type total = sect->_raw_size;
  bfd_size_type PCunits = total / 2;
  bfd_size_type actual = total * .75;


  if (REPORT_AS_PROGRAM(sect)) {
      char *name, *c;
      if (strstr(sect->name, "fill$"))
       {
        name = xmalloc(7);
        if (PIC30_IS_ABSOLUTE_ATTR(sect))
         strcpy(name, "FILLED");
        else 
         strcpy(name, "UNUSED");
       }
      else { 
      /* make a local copy of the name */
      name = xmalloc(strlen(sect->name) + 1);
      strcpy(name, sect->name);
      }
      /* clean the name of %n */
      c = strchr(name, '%');
      if (c) *c = '\x0';

      if (fp) {
        fprintf( fp, "%-24s%#10lx%#20lx%#16lx  (%ld)\n", name,
             start, PCunits, actual, actual);
      }
      actual_prog_memory_used += (total * .75);

      free(name);
    }
} /* static void bfd_pic30_report_program_sections (...)*/

/*
** Utility routine: bfd_pic30_report_auxflash_sections()
*/
static void
bfd_pic30_report_auxflash_sections (asection *sect, FILE *fp) {
  file_ptr start = sect->lma;
  bfd_size_type total = sect->_raw_size;
  bfd_size_type PCunits = total / 2;
  bfd_size_type actual = total * .75;


  if (REPORT_AS_AUXFLASH(sect)) {
      char *name, *c;
      if (strstr(sect->name, "fill$"))
       {
        name = xmalloc(7);
        if (PIC30_IS_ABSOLUTE_ATTR(sect))
         strcpy(name, "FILLED");
        else
         strcpy(name, "UNUSED");
       }
      else {
      /* make a local copy of the name */
      name = xmalloc(strlen(sect->name) + 1);
      strcpy(name, sect->name);
      }

      /* clean the name of %n */
      c = strchr(name, '%');
      if (c) *c = '\x0';

      fprintf( fp, "%-24s%#10lx%#20lx%#16lx  (%ld)\n", name,
             start, PCunits, actual, actual);
      actual_auxflash_memory_used += (total * .75);

      free(name);
    }
} /* static void bfd_pic30_report_auxflash_sections (...)*/


/*
** Utility routine: bfd_pic30_report_eedata_sections()
*/ 
static void
bfd_pic30_report_eedata_sections (asection *sect, FILE *fp) {
  file_ptr start = sect->lma;
  bfd_size_type total = sect->_raw_size;
  bfd_size_type PCunits = total / 2;
  bfd_size_type actual = PCunits;

  if (REPORT_AS_EEDATA(sect)) {
      char *name, *c;

      /* make a local copy of the name */
      name = xmalloc(strlen(sect->name) + 1);
      strcpy(name, sect->name);

      /* clean the name of %n */
      c = strchr(name, '%');
      if (c) *c = '\x0';

      fprintf( fp, "%-24s%#10lx%#20lx%#16lx  (%ld)\n", name,
             start, PCunits, actual, actual);
      actual_eedata_memory_used += actual;

      free(name);
    }
} /* static void bfd_pic30_report_eedata_sections (...)*/


/*
** Utility routine: bfd_pic30_report_data_sections()
*/ 
static void
bfd_pic30_report_data_sections (asection *sect, FILE *fp) {
  file_ptr start = sect->vma;
  unsigned int opb = bfd_octets_per_byte (output_bfd);
  bfd_size_type total = sect->_raw_size / opb; /* ignore phantom bytes */

  if (REPORT_AS_DATA(sect))
    {
      unsigned int gaps;
      char *name, *c;
      struct bfd_link_hash_entry *h = (struct bfd_link_hash_entry *) NULL;

      /* make a local copy of the name */
      name = xmalloc(strlen(sect->name) + strlen(GAP_ID) + 1);
      strcpy(name, sect->name);

      strcat(name, GAP_ID);
      h = bfd_link_hash_lookup (link_info.hash, name, FALSE, FALSE, TRUE);

      if (h == (struct bfd_link_hash_entry *) NULL)
        gaps = 0;
      else
        gaps = h->u.def.value / opb; /* ignore phantom bytes */

      /* clean the name of GAP_ID or %n.GAPID */
      c = strchr(name, '%');
      if (c) *c = '\x0';
      if (fp) {
        fprintf( fp, "%-24s%#10lx%#20x%#16lx  (%ld)\n", name,
             start, gaps, total, total);
      }
      data_memory_used += total;

      free(name);
    }

  return;
} /* static void bfd_pic30_report_data_sections (...)*/


/*
** Utility routine: bfd_pic30_report_memory_sections()
**
** - show memory usage of sections in user-defined regions
*/ 
static void
bfd_pic30_report_memory_sections (const char *region, asection *sect,
                                  FILE *fp) {
  file_ptr start = sect->vma;
  unsigned int opb = bfd_octets_per_byte (output_bfd);
  bfd_size_type total = sect->_raw_size / opb; /* ignore phantom bytes */

  if (PIC30_IS_MEMORY_ATTR(sect)) {
    const char *c1, *c2;
    struct pic30_section *s;
    unsigned int n;

    /* loop through the list of memory-type sections */
    for (s = user_memory_sections; s != NULL; s = s->next) {
      if (!s->file || !s->sec) continue;
      if (strcmp((char *)s->file, region) != 0) continue;
      
      c1 = (const char *) s->sec;  /* name is stored here! */
      c2 = strchr(c1, '%');        /* avoid the %n terminator */
      n = c2 ? c2 - c1 : strlen(c1);
      if (strncmp(c1, sect->name, n) == 0) {
        char *c, *name = xmalloc(strlen(sect->name) + 1);
        strcpy(name, sect->name);  /* make a local copy of the name */
        c = strchr(name, '%');
        if (c) *c = '\x0';         /* remove the %n terminator */
        
        fprintf(fp, "%-24s%#10lx%#36lx  (%ld)\n", name, start, total, total);
        external_memory_used += total;
        free(name);
        break;
      }
    }
  }
  return;
} /* static void bfd_pic30_report_memory_sections (...)*/


static void
report_percent_used (bfd_vma used, bfd_vma max, FILE *fp) {
  int percent = 100 * ((float) used/max);

  if (percent)
    fprintf( fp, "%d%%", percent);
  else
    fprintf( fp, "<1%%");
}

static int
in_bounds (asection *sec, lang_memory_region_type *region) {
  bfd_vma start;
  bfd_size_type len;
  int result = 0;
  
  if (REPORT_AS_DATA(sec)) {
    start = sec->vma;
    len  = sec->_raw_size / 2;
  }
  else {
    start = sec->lma;
    len  = sec->_raw_size / 2;
  }

  if (start >= region->origin)
    result = 1;

  if ((start + len) > (region->origin + region->length))
    result = 0;

  return result;
}

/*
** Utility routine: bfd_pic30_report_memory_usage
**
** - print a section chart to file *fp
**/
static void
bfd_pic30_report_memory_usage (FILE *fp) {
  bfd_size_type max_heap, max_stack;
  lang_memory_region_type *region;
  struct pic30_section *s;
  int has_eedata = 0;
  char *program_regions[] = { "program", "ivt", "aivt", 0 };
  char *region_name;
  int header_printed=0;

  /* clear the counters */
  actual_eedata_memory_used = 0;
  actual_auxflash_memory_used = 0;

  /* build an ordered list of output sections */
  pic30_init_section_list(&pic30_section_list);
  bfd_map_over_sections(output_bfd, &pic30_build_section_list, NULL);

  fprintf(fp,"\n\nxc16-ld %s", pic30_resource_version); 
  if ((pic30_mem_info.ram[0] + pic30_mem_info.sfr[0]) > 8192) {
    fprintf(fp,
            "\n\nDefault Code Model: Small\nDefault Data Model: Large\nDefault Scalar Model: Small"); 
  } else {
    fprintf(fp,
            "\n\nDefault Code Model: Small\nDefault Data Model: Small\nDefault Scalar Model: Small"); 
  }

  for (region = pic30_all_regions(); region; region=region=region->next) {
  
    actual_prog_memory_used = 0;
    header_printed=0;
    region_name = strdup(region->name);

    if ((region) && (region->length < 0xFFFFFFFFUL)) {
      bfd_boolean hidden;
     
      if ((region->flags & (SEC_CODE | SEC_READONLY)) == 0) continue;
      program_origin = region->origin;
      program_length = region->length;

      /* report code sections */
      for (s = pic30_section_list; s != NULL; s = s->next)
        if ((s->sec) && in_bounds(s->sec, region)) {

          hidden = FALSE;
          /* CAW movable vector tables are in program space ... */
          {
            /* for ivt or aivt sections, check to see if this has been
                 filled in with the defualt handler */
            ivt_record_type *l;
            for (l = ivt_records_list; l; l = l->next) {
              if (l->sec_name == 0) {
                /* this can happen on devices with movable aivt's and none
                   defind... there is a record, but we haven't filled one in */
              } else if (strcmp(l->sec_name, s->sec->name) == 0) {
                if (l->flags & ivt_default) hidden = TRUE;
                break;
              }
            }
          }

          if (!hidden) {
            if (header_printed == 0) {
              /* print code header */
              fprintf(fp,
                      "\n\n\"%s\" Memory  [Origin = 0x%lx, Length = 0x%lx]\n\n",
                      region_name, region->origin, region->length);
              fprintf(fp, 
                      "section                    address   length (PC units)"
                      "   length (bytes) (dec)\n");
              fprintf(fp, 
                      "-------                    -------   -----------------"
                      "   --------------------\n");
              header_printed++;
            }
            bfd_pic30_report_program_sections (s->sec, fp);
          }
        }
    }
    if (header_printed) {
      /* print code summary */
      fprintf(fp, "\n                 Total \"%s\" memory used (bytes):"
              "     %#10lx  (%ld) ",
              region->name,actual_prog_memory_used, actual_prog_memory_used);
      report_percent_used((actual_prog_memory_used * 2)/3,
                          region->length, fp);
      fprintf( fp, "\n");
    }
  }
 
  /* auxflash section */
  if (global_PROCESSOR && pic30_is_auxflash_machine(global_PROCESSOR)) {
    region = region_lookup("auxflash");
    if ((region) && (region->length < 0xFFFFFFFFUL)) {
      auxflash_origin = region->origin;
      auxflash_length = region->length;
      /* print auxflash header */
      fprintf( fp, "\n\nAuxflash Memory  [Origin = 0x%lx, Length = 0x%lx]\n\n",
               region->origin, region->length);
      fprintf( fp, "section                    address   length (PC units)"
               "   length (bytes) (dec)\n");
      fprintf( fp, "-------                    -------   -----------------"
               "   --------------------\n");

      /* report auxflash sections */
      for (s = pic30_section_list; s != NULL; s = s->next)
        if ((s->sec) && in_bounds(s->sec, region))
          bfd_pic30_report_auxflash_sections (s->sec, fp);
  
      /* print auxflash summary */
      fprintf( fp, "\n                     Total auxflash memory used (bytes):"
               "     %#10lx  (%ld) ",
             actual_auxflash_memory_used, actual_auxflash_memory_used);
      report_percent_used((actual_auxflash_memory_used * 2)/3,
                          region->length, fp);
      fprintf( fp, "\n");
    } else {
      fprintf(fp, "\nMEMORY region 'auxflash' not defined in linker script.\n");
    }
  }
 
  /* the eedata report is optional */
  for (s = pic30_section_list; s != NULL; s = s->next)
    if ((s->sec) && PIC30_IS_EEDATA_ATTR(s->sec))
      has_eedata = 1;

  if (has_eedata) {
    region = region_lookup("eedata");
    /* print eedata header */
    fprintf( fp, "\n\nData EEPROM Memory  [Origin = 0x%lx, Length = 0x%lx]\n\n",
             region->origin, region->length);
    fprintf( fp, "section                    address   length (PC units)"
             "   length (bytes) (dec)\n");
    fprintf( fp, "-------                    -------   -----------------"
             "   --------------------\n");

    /* report eedata sections */
    for (s = pic30_section_list; s != NULL; s = s->next)
      if ((s->sec) && in_bounds(s->sec, region))
        bfd_pic30_report_eedata_sections (s->sec, fp);

    /* print eedata summary */
    fprintf( fp, "\n                 Total data EEPROM used (bytes):"
             "     %#10lx  (%ld) ",
             actual_eedata_memory_used, actual_eedata_memory_used);
  report_percent_used(actual_eedata_memory_used, region->length, fp);
  fprintf( fp, "\n");
  }

  /* print data header */
  for (region = pic30_all_regions(); region; region=region=region->next) {

    if ((region->flags & (SEC_CODE | SEC_READONLY)) != 0) continue;
    if ((region->flags & (SEC_ALLOC)) == 0) continue;
    data_memory_used = 0;
    fprintf( fp, "\n\n\"%s\" Memory  [Origin = 0x%lx, Length = 0x%lx]\n\n",
             region->name, region->origin, region->length);
    fprintf( fp, "section                    address      alignment gaps"
             "    total length  (dec)\n");
    fprintf( fp, "-------                    -------      --------------"
             "    -------------------\n");

    /* report data sections */
    for (s = pic30_section_list; s != NULL; s = s->next)
      if ((s->sec) && in_bounds(s->sec, region))
        bfd_pic30_report_data_sections (s->sec, fp);

    /* print data summary */
    fprintf(fp, "\n                 Total \"%s\" memory used (bytes):"
            "     %#10lx  (%ld) ",
            region->name,data_memory_used, data_memory_used);
    if (data_memory_used > 0)
      report_percent_used(data_memory_used, region->length, fp);
    fprintf( fp, "\n");
  }

  /* print dynamic header */
  fprintf( fp, "\n\nDynamic Memory Usage\n\n");
  fprintf( fp, "region                     address                      "
           "maximum length  (dec)\n");
  fprintf( fp, "------                     -------                      "
           "---------------------\n");

  /* report dynamic regions */
  max_heap  = heap_limit - heap_base;
  fprintf( fp, "%-24s%#10x%#36lx  (%ld)\n", "heap",
           heap_base, max_heap, max_heap);

  if (user_defined_stack) {
    max_stack = 0;
    fprintf( fp, "(user-defined stack at 0x%x, length = 0x%x)\n",
             stack_base, (unsigned int) (stack_limit - stack_base));
  }
  else {
    max_stack = stack_limit - stack_base + pic30_stackguard_size;
    fprintf( fp, "%-24s%#10x%#36lx  (%ld)\n", "stack",
             stack_base, max_stack, max_stack);
  }

  /* print dynamic summary */
  fprintf( fp, "\n                 Maximum dynamic memory (bytes):"
           " %#14lx  (%ld)\n\n",
         (max_heap + max_stack), (max_heap + max_stack));

  /* report user-defined memory sections...
     they require some extra effort to organize by
     external memory region */
  if (has_user_defined_memory) {
    struct pic30_section *r, *rnext, *s;
    const char *region_name;
    /* loop through any user-defined regions */
    for (r = memory_region_list; r != NULL; r = rnext) {
  
      rnext = r->next;
      if (r->sec == 0) continue;
    
      region_name = r->sec->name + strlen(memory_region_prefix);
      fprintf( fp, "\nExternal Memory %s"
               "  [Origin = 0x%lx, Length = 0x%lx]\n\n",
                region_name, r->sec->vma, r->sec->lma);

      fprintf( fp, "section                    address                       "
               " total length  (dec)\n");
      fprintf( fp, "-------                    -------                       "
               " -------------------\n");

      external_memory_used = 0;
      for (s = pic30_section_list; s != NULL; s = s->next)
        if (s->sec)
          bfd_pic30_report_memory_sections (region_name, s->sec, fp);

      /* print summary for this region */
      fprintf( fp, "\n                 Total external memory used (bytes):"
               "     %#10lx  (%ld) ",
              external_memory_used, external_memory_used);
      if (external_memory_used > 0)
        report_percent_used(external_memory_used, r->sec->lma, fp);
      fprintf( fp, "\n\n");
    }
  }
 
  /* Check the data_memory_used and see if small-data model is sufficient
   */
  if ((data_memory_used < (8192 - pic30_mem_info.sfr[0])) 
      && (large_data_scalar == 1)
      && (!pic30_mno_info_linker)) {
    fprintf(fp,
            "Info: Project is using a large data memory model when small data memory model is sufficient.\n\n"); 
  }
   
  /* free the output section list */
  pic30_free_section_list(&pic30_section_list);

} /* static void bfd_pic30_report_memory_usage (...)*/


/*
** build_data_symbol_list
**
** - build a list of external data symbols
** - called via bfd_link_hash_traverse()
*/
#define LINK_HASH_VALUE(H)  ((H)->u.def.value \
                    + (H)->u.def.section->output_offset \
                    + (H)->u.def.section->output_section->lma)

static bfd_boolean
pic30_build_data_symbol_list(struct bfd_link_hash_entry *h, PTR p) {
  struct pic30_symbol **list = ( struct pic30_symbol **) p;

  if (( h->type == bfd_link_hash_defined) &&
      (!(h->u.def.section->flags & SEC_EXCLUDE) &&                                            PIC30_SECTION_IN_DATA_MEMORY(h->u.def.section))) {
    bfd_vma value = LINK_HASH_VALUE(h);
    pic30_add_to_symbol_list( list, h->root.string, value);
  }
  return TRUE;  /* continue */
}


/*
** build_eedata_symbol_list
**
** - build a list of external eedata symbols
** - called via bfd_link_hash_traverse()
*/
static bfd_boolean
pic30_build_eedata_symbol_list(struct bfd_link_hash_entry *h, PTR p) {
  struct pic30_symbol **list = ( struct pic30_symbol **) p;

  if (( h->type == bfd_link_hash_defined) &&
       PIC30_IS_EEDATA_ATTR(h->u.def.section)) {

    bfd_vma value = LINK_HASH_VALUE(h);
    pic30_add_to_symbol_list( list, h->root.string, value);
  }
  return TRUE;  /* continue */
}


/*
** build_program_symbol_list
**
** - build a list of external program symbols
** - called via bfd_link_hash_traverse()
*/
static bfd_boolean
pic30_build_program_symbol_list(struct bfd_link_hash_entry *h, PTR p) {
  struct pic30_symbol **list = ( struct pic30_symbol **) p;

  if (( h->type == bfd_link_hash_defined) &&
      (!(h->u.def.section->flags & SEC_EXCLUDE) &&
      (PIC30_IS_CODE_ATTR(h->u.def.section) ||
       PIC30_IS_PSV_ATTR(h->u.def.section) ||
       PIC30_IS_AUXFLASH_ATTR(h->u.def.section) ||
       PIC30_IS_AUXPSV_ATTR(h->u.def.section)))) {

    bfd_vma value = LINK_HASH_VALUE(h);
    pic30_add_to_symbol_list( list, h->root.string, value);
  }
  return TRUE;  /* continue */
}


static int
compare_symbol_value (const void *p1, const void *p2) {
  struct pic30_symbol *sym1 = (struct pic30_symbol *) p1;
  struct pic30_symbol *sym2 = (struct pic30_symbol *) p2;

  if (sym1->value < sym2->value)
    return -1;
  else if (sym1->value == sym2->value)
    return 0;
  else
    return 1;
}


static int
compare_symbol_name (const void *p1, const void *p2) {
  struct pic30_symbol *sym1 = (struct pic30_symbol *) p1;
  struct pic30_symbol *sym2 = (struct pic30_symbol *) p2;

  return strcmp(sym1->name, sym2->name);
}



static void
pic30_report_symbol_table (FILE *fp,
                           struct pic30_symbol *list,
                           int count, char * name, int format) {
  char *s1 = "\nExternal Symbols in ";
  char *s2 = " (by address):\n\n";
  char *s3 = " (by name):\n\n";

  fprintf(fp, "%s%s%s", s1, name, s2);
  qsort(list, count, sizeof(struct pic30_symbol), compare_symbol_value);
  pic30_print_symbol_list(list, fp, format);

  fprintf(fp, "%s%s%s", s1, name, s3);
  qsort(list, count, sizeof(struct pic30_symbol), compare_symbol_name);
  pic30_print_symbol_list(list, fp, format);
}


/*
** Utility routine: bfd_pic30_report_external_symbols
**
** - print sorted symbol tables to file *fp
**/
static void
pic30_report_external_symbols (FILE *fp) {

#define DATA 1
#define CODE 2

  pic30_init_symbol_list (&pic30_symbol_list);

  /* data memory */
  bfd_link_hash_traverse (link_info.hash,
                          pic30_build_data_symbol_list, &pic30_symbol_list);
  if (pic30_symbol_count)
    pic30_report_symbol_table (fp, pic30_symbol_list, pic30_symbol_count,
                               "Data Memory", DATA);
  /* data EEPROM */
  pic30_symbol_count = 0;
  bfd_link_hash_traverse (link_info.hash,
                          pic30_build_eedata_symbol_list, &pic30_symbol_list);
    if (pic30_symbol_count)
      pic30_report_symbol_table (fp, pic30_symbol_list, pic30_symbol_count,
                                 "Data EEPROM", CODE);
  /* program memory */
  pic30_symbol_count = 0;
  bfd_link_hash_traverse (link_info.hash,
                          pic30_build_program_symbol_list, &pic30_symbol_list);
    if (pic30_symbol_count)
      pic30_report_symbol_table (fp, pic30_symbol_list, pic30_symbol_count,
                                 "Program Memory", CODE);

  free(pic30_symbol_list);

} /* static void bfd_pic30_report_external_symbols */

/**
**  bfd_pic30_memory_summary
**
**/
static void
bfd_pic30_memory_summary (char *arg) {
  
  FILE *memory_summary_File;
  memory_summary_File = fopen(arg, "w");

  if (memory_summary_File == NULL) {
    fprintf(stderr,"Warning: Could not open %s.\n", arg);
    return;
  }

  else {
    lang_memory_region_type *region;
    struct pic30_section *s;
    int i;
    const char *header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    const char *end_header = "\n";
    const char *units = "bytes";

    /* build an ordered list of output sections */
    pic30_init_section_list(&pic30_section_list);
    bfd_map_over_sections(output_bfd, &pic30_build_section_list, NULL);

    fprintf(memory_summary_File, "%s", header);
    fprintf(memory_summary_File, "%s", end_header);

    indent_fprintf(memory_summary_File, "<project>\n");
    indent_fprintf(memory_summary_File, "<executable name=\"%s\">\n", output_bfd->filename);

       bfd_size_type length = 0;
       bfd_size_type used = 0;
       bfd_size_type free = 0;

       indent_fprintf(memory_summary_File, "<memory name=\"data\">\n");
       region = region_lookup("data");

       length = region->length;
       for (s = pic30_section_list; s != NULL; s=s->next)
          if ((s->sec) && in_bounds(s->sec, region))
            if (REPORT_AS_DATA(s->sec))
              used += (s->sec->_raw_size / 2);

       free = length - used;

       indent_fprintf(memory_summary_File,"<units>%s</units>\n", units);
       indent_fprintf(memory_summary_File,"<length>%d</length>\n", length);
       indent_fprintf(memory_summary_File,"<used>%d</used>\n", used);
       indent_fprintf(memory_summary_File,"<free>%d</free>\n",free);

       indent_fprintf(memory_summary_File,"</memory>\n");

       length = 0;
       used = 0;
       free = 0;
       
       indent_fprintf(memory_summary_File, "<memory name=\"program\">\n");
       region = region_lookup("program");
            
       length = ((region->length / 2 ) * 3);
       for (s = pic30_section_list; s != NULL; s=s->next)
          if ((s->sec) && in_bounds(s->sec, region))
            if (REPORT_AS_PROGRAM(s->sec))
              used += (s->sec->_raw_size * 3 / 4);

       free = length - used;

       indent_fprintf(memory_summary_File,"<units>%s</units>\n", units);
       indent_fprintf(memory_summary_File,"<length>%d</length>\n", length);
       indent_fprintf(memory_summary_File,"<used>%d</used>\n", used);
       indent_fprintf(memory_summary_File,"<free>%d</free>\n",free);

       indent_fprintf(memory_summary_File,"</memory>\n");

    indent_fprintf(memory_summary_File, "</executable>\n");
    indent_fprintf(memory_summary_File, "</project>\n");

    fclose(memory_summary_File);
  }

}
