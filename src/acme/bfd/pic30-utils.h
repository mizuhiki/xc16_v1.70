/*
** pic30-utils.h
**
** Copyright (c) 2017 Microchip Technology, Inc.
**
** This file contains utility macros and structures
** that are common to mulitple OMF formats.
*/

#ifndef _PIC30_UTILS_H
#define _PIC30_UTILS_H

/*****************************************************************************/

extern bfd_boolean pic30_debug;

/*
** Macro to clear all section attributes
**
** We also clear parameters to address()
** and merge(), but not align(), reverse().
*/
#define PIC30_CLEAR_ALL_ATTR(sec) \
  { (sec)->flags = 0;             \
  (sec)->near = 0;                \
  (sec)->persistent = 0;          \
  (sec)->xmemory = 0;             \
  (sec)->ymemory = 0;             \
  (sec)->psv = 0;                 \
  (sec)->eedata = 0;              \
  (sec)->absolute = 0;            \
  (sec)->reverse = 0;             \
  (sec)->dma = 0;                 \
  (sec)->boot = 0;                \
  (sec)->secure = 0;              \
  (sec)->memory = 0;              \
  (sec)->heap = 0;                \
  (sec)->eds = 0;                 \
  (sec)->page = 0;                \
  (sec)->stack = 0;               \
  (sec)->vma = 0;                 \
  (sec)->lma = 0;                 \
  (sec)->entsize = 0;             \
  (sec)->auxflash = 0;            \
  (sec)->packedflash = 0;         \
  (sec)->shared = 0;              \
  (sec)->preserved = 0;           \
  (sec)->auxpsv = 0;              \
  (sec)->linker_generated = 0;    \
  (sec)->update = 0;              \
  (sec)->priority = 0; }

#define PIC30_SAVE_FLAGS(sec,to)                         \
  { to = malloc(28*sizeof(int));  /* modify as needed */ \
    to[0] = (sec)->flags;                                \
    to[1] = (sec)->near;                                 \
    to[2] = (sec)->persistent;                           \
    to[3] = (sec)->xmemory;                              \
    to[4] = (sec)->ymemory;                              \
    to[5] = (sec)->psv;                                  \
    to[6] = (sec)->eedata;                               \
    to[7] = (sec)->absolute;                             \
    to[8] = (sec)->reverse;                              \
    to[9] = (sec)->dma;                                  \
    to[10] = (sec)->boot;                                \
    to[11] = (sec)->secure;                              \
    to[12] = (sec)->memory;                              \
    to[13] = (sec)->heap;                                \
    to[14] = (sec)->eds;                                 \
    to[15] = (sec)->page;                                \
    to[16] = (sec)->stack;                               \
    to[17] = (sec)->vma;                                 \
    to[18] = (sec)->lma;                                 \
    to[19] = (sec)->entsize;                             \
    to[20] = (sec)->auxflash;                            \
    to[21] = (sec)->packedflash;                         \
    to[22] = (sec)->shared;                              \
    to[23] = (sec)->preserved;                           \
    to[24] = (sec)->auxpsv;                              \
    to[25] = (sec)->linker_generated;                    \
    to[26] = (sec)->update;                              \
    to[27] = (sec)->priority;                            \
  }
   
#define PIC30_RESTORE_FLAGS(sec,to)                      \
  {                                                      \
    (sec)->flags = to[0];                                \
    (sec)->near = to[1];                                 \
    (sec)->persistent = to[2];                           \
    (sec)->xmemory = to[3];                              \
    (sec)->ymemory = to[4];                              \
    (sec)->psv = to[5];                                  \
    (sec)->eedata = to[6];                               \
    (sec)->absolute = to[7];                             \
    (sec)->reverse = to[8];                              \
    (sec)->dma = to[9];                                  \
    (sec)->boot = to[10];                                \
    (sec)->secure = to[11];                              \
    (sec)->memory = to[12];                              \
    (sec)->heap = to[13];                                \
    (sec)->eds = to[14];                                 \
    (sec)->page = to[15];                                \
    (sec)->stack = to[16];                               \
    (sec)->vma = to[17];                                 \
    (sec)->lma = to[18];                                 \
    (sec)->entsize = to[19];                             \
    (sec)->auxflash = to[20];                            \
    (sec)->packedflash = to[21];                         \
    (sec)->shared = to[22];                              \
    (sec)->preserved = to[23];                           \
    (sec)->auxpsv = to[24];                              \
    (sec)->linker_generated = to[25];                    \
    (sec)->update = to[26];                              \
    (sec)->priority = to[27];                            \
  }

/*
** Macros used to set section attributes
*/
#define PIC30_SET_CODE_ATTR(sec) \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_LOAD | SEC_CODE | SEC_ALLOC);
#define PIC30_SET_DATA_ATTR(sec) \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_LOAD | SEC_DATA | SEC_ALLOC);
#define PIC30_SET_BSS_ATTR(sec)   \
  { (sec)->flags |= SEC_ALLOC;      \
  (sec)->flags &= ~(SEC_LOAD | SEC_DATA | SEC_HAS_CONTENTS); }
#define PIC30_SET_PERSIST_ATTR(sec)  \
  { (sec)->persistent = 1;           \
  (sec)->flags |= SEC_ALLOC;         \
  (sec)->flags &= ~(SEC_LOAD | SEC_DATA); }
#define PIC30_SET_PSV_ATTR(sec) \
  { (sec)->psv = 1;             \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY); }
#define PIC30_SET_EEDATA_ATTR(sec) \
  { (sec)->eedata = 1;             \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD); }
#define PIC30_SET_MEMORY_ATTR(sec) \
  { (sec)->memory = 1;             \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC); }
#define PIC30_SET_HEAP_ATTR(sec) \
  { (sec)->heap = 1;             \
  (sec)->flags |= SEC_ALLOC;     \
  (sec)->flags &= ~(SEC_LOAD | SEC_DATA | SEC_HAS_CONTENTS); }
#define PIC30_SET_STACK_ATTR(sec) \
  { (sec)->stack = 1;            \
  (sec)->flags |= SEC_ALLOC;     \
  (sec)->flags &= ~(SEC_LOAD | SEC_DATA | SEC_HAS_CONTENTS); }
#define PIC30_SET_AUXFLASH_ATTR(sec) \
  {(sec)->auxflash = 1;              \
   (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD);\
   (sec)->flags &= ~(SEC_CODE);}
#define PIC30_SET_PACKEDFLASH_ATTR(sec) \
  {(sec)->packedflash = 1;              \
   (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD);}

#define PIC30_SET_ABSOLUTE_ATTR(sec) \
  (sec)->absolute = 1;
#define PIC30_SET_NEAR_ATTR(sec) \
  (sec)->near = 1;
#define PIC30_SET_XMEMORY_ATTR(sec) \
  (sec)->xmemory = 1;
#define PIC30_SET_YMEMORY_ATTR(sec) \
  (sec)->ymemory = 1;
#define PIC30_SET_REVERSE_ATTR(sec) \
  (sec)->reverse = 1;
#define PIC30_SET_DMA_ATTR(sec) \
  (sec)->dma = 1;
#define PIC30_SET_BOOT_ATTR(sec) \
  (sec)->boot = 1;
#define PIC30_SET_SECURE_ATTR(sec) \
  (sec)->secure = 1;
#define PIC30_SET_NOLOAD_ATTR(sec)  \
  { (sec)->flags |= SEC_NEVER_LOAD; \
    (sec)->flags &= ~ SEC_LOAD; }
#define PIC30_SET_MERGE_ATTR(sec) \
  (sec)->flags |= SEC_MERGE;
#define PIC30_SET_INFO_ATTR(sec) \
  { (sec)->flags |= SEC_DEBUGGING; \
    (sec)->flags &= ~SEC_ALLOC; }
#define PIC30_SET_EDS_ATTR(sec) \
  (sec)->eds = 1;
#define PIC30_SET_PAGE_ATTR(sec) \
  (sec)->page = 1;
#define PIC30_SET_KEEP_ATTR(sec) \
  (sec)->flags |= SEC_KEEP;
#define PIC30_SET_SHARED_ATTR(sec) \
  (sec)->shared = 1;
#define PIC30_SET_PRESERVED_ATTR(sec) \
  (sec)->preserved = 1;
#define PIC30_SET_AUXPSV_ATTR(sec) \
  { (sec)->auxpsv = 1;             \
  (sec)->flags |= (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY); }

/* UNORDERED is used internally by the assembler
   and is not encoded in the object file */
#define PIC30_SET_UNORDERED_ATTR(sec) \
  (sec)->unordered = 1;

#define PIC30_SET_UPDATE_ATTR(sec) (sec)->update = 1

/*
** Macros used to query section attributes
*/
#define PIC30_IS_CODE_ATTR(sec) \
  (((sec)->flags & (SEC_CODE | SEC_ALLOC)) == (SEC_CODE | SEC_ALLOC))
#define PIC30_IS_DATA_ATTR(sec) \
  ((((sec)->flags & (SEC_DATA | SEC_ALLOC)) == (SEC_DATA | SEC_ALLOC)) && \
   ((sec)->memory !=1))
#define PIC30_IS_BSS_ATTR(sec) \
  ((((sec)->flags & (SEC_ALLOC|SEC_LOAD|SEC_CODE|SEC_DATA|SEC_HAS_CONTENTS)) == SEC_ALLOC) && \
   ((sec)->persistent != 1) && ((sec)->memory !=1) && \
   ((sec)->heap != 1) && ((sec)->stack !=1))
#define PIC30_IS_PERSIST_ATTR(sec) \
  ((((sec)->flags & (SEC_ALLOC|SEC_LOAD|SEC_CODE|SEC_DATA|SEC_HAS_CONTENTS)) == SEC_ALLOC) && \
   ((sec)->persistent == 1))
#define PIC30_IS_PSV_ATTR(sec) \
  (((sec)->psv == 1) || ((((sec)->flags & SEC_READONLY) == SEC_READONLY)   \
                         && sec->auxpsv != 1))
#define PIC30_IS_EEDATA_ATTR(sec) \
  ((sec)->eedata == 1)
#define PIC30_IS_MEMORY_ATTR(sec) \
  ((((sec)->flags & (SEC_ALLOC|SEC_LOAD|SEC_CODE|SEC_DATA)) == SEC_ALLOC) && \
  ((sec)->memory == 1))
#define PIC30_IS_HEAP_ATTR(sec) \
  ((((sec)->flags & (SEC_ALLOC|SEC_LOAD|SEC_CODE|SEC_DATA|SEC_HAS_CONTENTS)) == SEC_ALLOC) && \
  ((sec)->heap == 1))
#define PIC30_IS_STACK_ATTR(sec) \
  ((((sec)->flags & (SEC_ALLOC|SEC_LOAD|SEC_CODE|SEC_DATA|SEC_HAS_CONTENTS)) == SEC_ALLOC) && \
  ((sec)->stack == 1))
#define PIC30_IS_AUXFLASH_ATTR(sec) \
  (((sec)->auxflash == 1) && (((sec)->flags & SEC_ALLOC) == SEC_ALLOC))
#define PIC30_IS_PACKEDFLASH_ATTR(sec) \
  (((sec)->packedflash == 1) && (((sec)->flags & SEC_ALLOC) == SEC_ALLOC))

#define PIC30_IS_ABSOLUTE_ATTR(sec) \
  ((sec)->absolute == 1)
#define PIC30_IS_NEAR_ATTR(sec) \
  ((sec)->near == 1)
#define PIC30_IS_XMEMORY_ATTR(sec) \
  ((sec)->xmemory == 1)
#define PIC30_IS_YMEMORY_ATTR(sec) \
  ((sec)->ymemory == 1)
#define PIC30_IS_REVERSE_ATTR(sec) \
  ((sec)->reverse == 1)
#define PIC30_IS_DMA_ATTR(sec) \
  ((sec)->dma == 1)
#define PIC30_IS_BOOT_ATTR(sec) \
  ((sec)->boot == 1)
#define PIC30_IS_SECURE_ATTR(sec) \
  ((sec)->secure == 1)
#define PIC30_IS_NOLOAD_ATTR(sec) \
  (((sec)->flags & SEC_NEVER_LOAD) == SEC_NEVER_LOAD)
#define PIC30_IS_MERGE_ATTR(sec) \
  (((sec)->flags & SEC_MERGE) == SEC_MERGE)
#define PIC30_IS_INFO_ATTR(sec) \
  (((sec)->flags & SEC_DEBUGGING) == SEC_DEBUGGING)
#define PIC30_IS_EDS_ATTR(sec) \
  ((sec)->eds == 1)
#define PIC30_IS_PAGE_ATTR(sec) \
  ((sec)->page == 1)
#define PIC30_IS_KEEP_ATTR(sec) \
  (((sec)->flags & SEC_KEEP) == SEC_KEEP)
#define PIC30_IS_SHARED_ATTR(sec) \
  ((sec)->shared == 1)
#define PIC30_IS_PRESERVED_ATTR(sec) \
  ((sec)->preserved == 1)
#define PIC30_IS_AUXPSV_ATTR(sec) \
  (((sec)->auxpsv == 1) || ((((sec)->flags & SEC_READONLY) == SEC_READONLY) \
                        &&  ((sec)->psv != 1)))
#define PIC30_IS_UPDATE_ATTR(sec) ((sec)->update)

/* UNORDERED is used internally by the assembler
   and is not encoded in the object file */
#define PIC30_IS_UNORDERED_ATTR(sec) \
  ((sec)->unordered == 1)
#define PIC30_IS_EXTERNAL_ATTR(sec) \
  (0)
#define PIC30_IS_PRIORITY_ATTR(sec) ((sec)->priority != 0)

#define PIC30_IS_LOCAL_DATA(sec)       \
  ((!PIC30_IS_EDS_ATTR(sec) &&         \
      (PIC30_IS_BSS_ATTR(sec) ||       \
       PIC30_IS_DATA_ATTR(sec) ||      \
       PIC30_IS_PERSIST_ATTR(sec))))
#define REPORT_AS_PROGRAM(s) \
  (PIC30_IS_CODE_ATTR(s) || PIC30_IS_AUXPSV_ATTR(s) || PIC30_IS_PSV_ATTR(s) || \
   PIC30_IS_PACKEDFLASH_ATTR(s))
#define REPORT_AS_AUXFLASH(s) \
  (PIC30_IS_AUXFLASH_ATTR(s) || PIC30_IS_AUXPSV_ATTR(s) || \
   PIC30_IS_PSV_ATTR(s) || PIC30_IS_PACKEDFLASH_ATTR(s))
#define REPORT_AS_DATA(s) \
  (PIC30_SECTION_IN_DATA_MEMORY(s) || PIC30_IS_MEMORY_ATTR(s))

/*
** Macros used for access CodeGuard data structures
*/
#define NUM_BOOT_ACCESS_SLOTS 32
#define NUM_SECURE_ACCESS_SLOTS 32

/*
** enums for array indexing
**
** wrong:    array[BOOT][RAM]
** correct:  array[BOOTx][RAMx]
*/
enum { BOOTx = 0, SECUREx, GENERALx, SEGMENTS };
enum { RAMx  = 0, FLASHx,  EEPROMx, MEMORIES };

#define BOOT_IS_ACTIVE(mem_index)  \
  (( end_address[BOOTx][mem_index] != (bfd_vma) ~1) && \
   (base_address[BOOTx][mem_index] <= \
     end_address[BOOTx][mem_index]))

#define SECURE_IS_ACTIVE(mem_index)  \
  (( end_address[SECUREx][mem_index] != (bfd_vma) ~1) && \
   (base_address[SECUREx][mem_index] <= \
     end_address[SECUREx][mem_index]))

#define CRT0_KEY "__resetPRI"
#define CRT1_KEY "__resetALT"
#define CRTMODE_KEY "__crt_start_mode"
#define CRTMODEOFF_KEY "__crt_start_mode_normal"

/*****************************************************************************/

/*
** Define some structures for the Handle hash table.
**
** This is used by the linker to collect and process
** handle relocations.
*/
struct handle_hash_entry
{
  struct bfd_hash_entry root;
  int index;     /* jump table index    */
  char *sym;     /* name of jump target */
  flagword flags;
};

struct handle_hash_table
{
  struct bfd_hash_table table;
  int num;  /* # of entries */
};

/* Memory sizes for current device; */
struct pic30_mem_info_ {
  int flash[2];      /* [0] -> main flash,    [1] -> aux flash */
  int ram[2];        /* [0] -> main ram,      [1] -> aux ram */
  int eeprom[2];     /* [0] -> eeprom size,   [1] -> unused */
  int dataflash[2];  /* [0] -> size in bytes, [1] -> page number */
  int sfr[2];        /* [0] -> end of SFR space, [1] -> unused */
};

#define handle_hash_lookup(t, string, create, copy) \
  ((struct handle_hash_entry *) \
   bfd_hash_lookup (&(t)->table, (string), (create), (copy)))

/*
** Define some structures for the Undefined Symbol hash table.
**
** This is used by the linker to collect object signatures
** for undefined symbols. Signatures are used to help
** choose between multiple definitions of a symbol
** in archive files (aka libraries).
**
** As multiple objects refer to the same symbol,
** a composite signature is created. The most recent file
** to reference is saved, to facilitate error reporting.
*/
#define PIC30_UNDEFSYM_INIT 20
struct pic30_undefsym_entry {
  struct bfd_hash_entry root;
  bfd *most_recent_reference;
  int external_options_mask;
  int options_set;
};

struct pic30_undefsym_table {
  struct bfd_hash_table table;
  int num;  /* # of entries */
};

#define pic30_undefsym_lookup(t, string, create, copy) \
  ((struct pic30_undefsym_entry *) \
   bfd_hash_lookup (&(t)->table, (string), (create), (copy)))

extern struct pic30_undefsym_table *undefsyms;

extern struct bfd_hash_entry *pic30_undefsym_newfunc
  PARAMS ((struct bfd_hash_entry *, struct bfd_hash_table *, const char *));

extern void pic30_undefsym_traverse
  PARAMS ((struct pic30_undefsym_table *,
           bfd_boolean (*func) PARAMS ((struct bfd_hash_entry *, PTR)), PTR));

extern struct pic30_undefsym_table *pic30_undefsym_init
  PARAMS ((void));


/* Section Lists */
struct pic30_section
{
  struct pic30_section *next;
  unsigned int attributes;
  PTR *file;
  asection *sec;
};

typedef struct pic30_codeguard_setting {
  char *name;
  unsigned int flags;
  unsigned int mask;
  unsigned int value;
  bfd_vma address;
  struct pic30_codeguard_setting *next;
} codeguard_setting_type;

typedef struct fuse_setting {
  char *name;
  bfd_vma address;
  struct fuse_setting *next;
} fuse_setting_type;

enum pic30_ivt_record_flags {
  ivt_seen =    1 << 0,                 /* this entry has been defined in the
                                           linked application */
  ivt_default = 1 << 1,                 /* this slot has been filled with the
                                           default handler */
};

typedef struct pic30_ivt_record {
  char *name;
  char *sec_name;
  bfd_vma offset;
  bfd_vma value;
  bfd_boolean is_alternate_vector;
  enum pic30_ivt_record_flags flags;
  asection *ivt_sec;
  struct pic30_ivt_record *next;
  struct pic30_ivt_record *matching_vector; /* for is_alternatte_vectors */
} ivt_record_type;

extern char *pic30_add_data_flags;
extern char *pic30_add_code_flags;
extern char *pic30_add_const_flags;
extern char *pic30_dfp;
extern bfd_boolean pic30_preserve_all;
extern bfd_boolean pic30_has_fill_option;
extern bfd_boolean pic30_partition_flash;
extern bfd_boolean pic30_memory_usage;
extern bfd_boolean pic30_pad_flash_option;
extern bfd_vma pad_flash_arg;
extern bfd_vma program_origin;
extern bfd_vma auxflash_origin;
extern bfd_vma program_length;
extern bfd_vma auxflash_length;
extern fuse_setting_type *fuse_settings;
extern codeguard_setting_type *CG_settings;
extern bfd_boolean pic30_has_CG_settings;
extern bfd_vma aivt_base;
extern int ivt_elements;
extern bfd_vma ivt_base;
extern bfd_vma max_aivt_addr;
extern bfd_vma max_ivt_addr;
extern ivt_record_type *ivt_records_list;
extern const bfd_arch_info_type * global_PROCESSOR;
extern char *application_id;
extern bfd_vma ivt_base;
extern bfd_vma aivt_base;
extern struct pic30_section *inherited_sections;
extern struct pic30_section *preserved_sections;
extern bfd_boolean pic30_inherit_application_info;
extern bfd_boolean pic30_preserve_application_info;
extern char *inherited_application;
extern char *preserved_application;
extern bfd_boolean aivt_enabled;
extern bfd_boolean pic30_has_floating_aivt;
extern bfd_boolean pic30_has_fixed_aivt;
extern asymbol **bfd_pic30_load_symbol_table(bfd *abfd, long *num);

/* macros and functions to improve select_objects process 
 * 
 *  Here we attempt to pick the object that adds the least amount of
 *  extra requirements, unless there is an exact match we defer the choice
 *  until we have seen all.
 */

/*
 * ELF uses carsyms when looking at archives
 * COFF uses asymbols
 *
 * accept one for each function since ELF also uses the COFF model,
 *   at different times
 *
 */

struct pic30_deferred_archive_members {
  carsym *symdef;
  asymbol *sym;
  bfd *abfd;
  struct bfd_link_info *info;
  unsigned int new_mask_bits;
  unsigned int new_set_bits;
  symindex aye;                         /* aptly? named as 'i' in elflink.h */
  struct pic30_deferred_archive_members *next;
};

extern void pic30_defer_archive(carsym *symdef,
                                asymbol *sym,
                                bfd *abfd, 
                                struct bfd_link_info *info, 
                                unsigned int new_mask_bits, 
                                unsigned int new_set_bits,
                                symindex aye);
extern void pic30_remove_archive(carsym *symdef,asymbol *sym);
extern struct pic30_deferred_archive_members *pic30_pop_tail_archive(void);
extern void pic30_clear_deferred(void);
extern int pic30_count_ones(unsigned int a);

extern void build_aivt_free_block_list(bfd_vma start, bfd_vma length);
extern void pic30_add_section_attributes(asection *, char *, char *, char *);

extern char *pic30_dfp;
extern char *pic30_requested_processor;
extern char *version_string;

extern int pic30_is_dsp_machine(const bfd_arch_info_type *);
extern int pic30_is_eedata_machine(const bfd_arch_info_type *);
extern int pic30_is_auxflash_machine(const bfd_arch_info_type *);
extern int pic30_is_dma_machine(const bfd_arch_info_type *);
extern int pic30_is_codeguard_machine(const bfd_arch_info_type *);
extern int pic30_is_eds_machine(const bfd_arch_info_type *);
extern int pic30_is_pmp_machine(const bfd_arch_info_type *);
extern int pic30_is_epmp_machine(const bfd_arch_info_type *);
extern int pic30_is_ecore_machine(const bfd_arch_info_type *);
extern int pic30_is_dualpartition_machine(const bfd_arch_info_type *);
extern int pic30_is_5V_machine(const bfd_arch_info_type *);
extern int pic30_is_isaBB_machine(const bfd_arch_info_type *);
extern int pic30_is_isav32_machine(const bfd_arch_info_type *);
extern int pic30_is_isav4_machine(const bfd_arch_info_type *);
extern int pic30_is_contexts_machine(const bfd_arch_info_type *);
extern int pic30_is_generic(const bfd_arch_info_type *);

int pic30_is_valid_attributes
  PARAMS ((unsigned int, unsigned char));


int pic30_display_as_program_memory_p(asection *sec);
int pic30_display_as_data_memory_p(asection *sec);
int pic30_display_as_readonly_memory_p(asection *sec);

void pic30_load_codeguard_settings(const bfd_arch_info_type *, int);

void pic30_create_data_init_templates();
void pic30_create_rom_usage_template(void);
void pic30_create_ram_usage_template(void);
void pic30_create_specific_fill_sections(void);

/*****************************************************************************/

#endif
