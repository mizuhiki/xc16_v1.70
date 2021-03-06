/*
 * pic30-dis.c -- Disassembler for the PIC30 architecture.
 * Copyright (C) 2000 Free Software Foundation, Inc.
 * Contributed by Microchip Corporation.
 * Written by Tracy A. Kuhrt
 *
 * This file is part of GDB, GAS, and the GNU binutils.
 *
 * GDB, GAS, and the GNU binutils are free software; you can redistribute
 * them and/or modify them under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either version
 * 2, or (at your option) any later version.
 *
 * GDB, GAS, and the GNU binutils are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file COPYING.  If not, write to the Free
 * Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <stdio.h>
#include "sysdep.h"
#include <dis-asm.h>
#include <opcode/pic30.h>
#include "pic30-opc.h"
#include "../bfd/coff-pic30.h"

/* prototypes */
extern int pic30_is_ecore_machine(const bfd_arch_info_type *);
extern int pic30_is_isav4_machine(const bfd_arch_info_type *);


void pic30_debug_semantics(const struct pic30_opcode *, char *, int);
static int build_expr_operands(struct disassemble_info *, 
                               struct pic30_private_data *, 
                               int , 
                               struct pic30_semantic_expr *);
struct pic30_semantic_expr * pic30_analyse_semantics(struct disassemble_info *);
/* end of prototypes */


bfd_vma pic30_disassembly_address = 0; /* used by some extract functions
                                      in pic30-opc.c */

static unsigned char global_symbolic_disassembly = FALSE;

void pic30_parse_disassembler_options PARAMS (( char * option));

void pic30_disassemble_do_insn PARAMS
(( const struct pic30_opcode * opcode,
   unsigned long insn_word1, unsigned long insn_word2,
   struct disassemble_info * info));

void pic30_disassemble_call_goto_insn PARAMS
(( const struct pic30_opcode * opcode, unsigned long insn_word1,
   unsigned long insn_word2, struct disassemble_info *info));

void pic30_disassemble_bf_insn PARAMS
(( const struct pic30_opcode * opcode, unsigned long insn_word1,
   unsigned long insn_word2, struct disassemble_info *info));

char pic30_disassemble_2word_insn PARAMS
(( const struct pic30_opcode * opcode, unsigned long insn_word1,
   bfd_vma memaddr, struct disassemble_info *info));

unsigned char pic30_disassemble_1word_insn PARAMS
(( const struct pic30_opcode * opcode, unsigned long insn,
   struct disassemble_info *info));

#define PRINT_SYMBOL (print_symbol && global_symbolic_disassembly)

/* Disassemble PIC30 instructions.  */

/******************************************************************************/

void
pic30_parse_disassembler_options (option)
   char * option;

{
   if (option != NULL)
   {
      char * space = NULL;

      do {
         space = strchr (option, ' ');

         if (space)
            space = '\0';
         
         if (strcmp (option, "symbolic") == 0)
            global_symbolic_disassembly = TRUE;
         else
            fprintf (stderr, "Unrecognized disassembler option:  %s\n", option);

         if (space)
            option = space + 1;
      } while (space != NULL);
   } /* if (option != NULL) */
}

/******************************************************************************/

void
pic30_print_disassembler_options (stream)
   FILE * stream;
{
   fprintf (stream, "\nMicrochip specific disassembler options (supported with "
                    "-M):\n"
                    "  symbolic -- Include symbolic information during "
                    "disassembly.\n\n");
}

/******************************************************************************/

void
pic30_disassemble_do_insn (opcode, insn_word1, insn_word2, info)
   const struct pic30_opcode * opcode;
   unsigned long insn_word1;
   unsigned long insn_word2;
   struct disassemble_info * info;
{
   int i;

   (*info->fprintf_func) (info->stream, "%-10s", opcode->name);

   for (i = 0; i < opcode->number_of_operands; i++)
   {
      const struct pic30_operand * opnd =
         &(pic30_operands[opcode->operands[i]]);

      char operand_string[BUFSIZ] = "";
      unsigned long insn_word = (i == 0) ? insn_word1 : insn_word2;
      long operand_value = 0;
      unsigned char print_symbol = FALSE;

      if (opnd->extract)
      {
         char * extracted_string;
         unsigned char error_found = FALSE;

         extracted_string =
            (*opnd->extract) (insn_word, info,
                              opcode->flags, opnd, &error_found);
         strcpy (operand_string, extracted_string);
         free (extracted_string);
      } /* if (opnd->extract) */
      else
      {
         operand_value = ((insn_word) >> opnd->shift) & ((1 << opnd->bits) - 1);

         switch (opnd->type)
         {
            case OPND_REGISTER_DIRECT:
               strcpy (operand_string, pic30_registers[operand_value]);
               break;

            case OPND_SYMBOL:
               if ((*info->symbol_at_address_func) (operand_value, info))
                  print_symbol = TRUE;
               /* drop through */
            case OPND_VALUE:
            {
               if (opnd->immediate)
                  strcpy (operand_string, "#0x");
               else
                  strcpy (operand_string, "0x");

               if (!PRINT_SYMBOL)
                  sprintf (operand_string, "%s%lx", operand_string,
                           operand_value);
            }
         } /* switch */
      } /* else */

      (*info->fprintf_func) (info->stream, "%s", operand_string);

      if ((!opnd->extract) && (PRINT_SYMBOL))
         (*info->print_address_func) (operand_value, info);

      if ((i != (opcode->number_of_operands - 1)) &&
          (strcmp (operand_string, "") != 0))
         (*info->fprintf_func) (info->stream, ", ");
   }
}

/******************************************************************************/

void
pic30_disassemble_call_goto_insn (opcode, insn_word1, insn_word2, info)
   const struct pic30_opcode * opcode;
   unsigned long insn_word1;
   unsigned long insn_word2;
   struct disassemble_info *info;
{
  /* LC GSM */
/*    unsigned long LSB = ((insn_word1 & 0xFFFF) << 1); */
/*    unsigned long MSB = ((insn_word2 & 0xFFFF) << 17); */
   unsigned long LSB = ((insn_word1 & 0xFFFF));
   unsigned long MSB = ((insn_word2 & 0xFFFF) << 16);
   unsigned long addr = (MSB | LSB);
   struct pic30_private_data *private_data;

   private_data = info->private_data;
   info->flags_to_match = SEC_CODE;
   (*info->fprintf_func) (info->stream, "%-10s0x", opcode->name);
   (*info->print_address_func) (addr, info);
   info->flags_to_match = 0;
   info->target = addr;
   if (private_data) {
     private_data->reg[0] = addr;
   }
}

/******************************************************************************/

char
pic30_disassemble_2word_insn (opcode, insn_word1, memaddr, info)
   const struct pic30_opcode * opcode;
   unsigned long insn_word1;
   bfd_vma memaddr;
   struct disassemble_info *info;
{
   unsigned char rc = TRUE;
   int status;
   bfd_byte bytes_read[PIC30_SIZE_OF_PROGRAM_WORD + 1];
   int i;
   unsigned long insn_word2 = 0;
   struct pic30_private_data *private_data;

   private_data = info->private_data;

   status = (*info->read_memory_func) (memaddr + 2,
                                       bytes_read, PIC30_SIZE_OF_PROGRAM_WORD,
                                       info);

   if (status != 0)
      return -1;

   for (i = 0; i < PIC30_SIZE_OF_PROGRAM_WORD; i++)
      insn_word2 = insn_word2 | ((unsigned long)bytes_read[i] << (8 * i));

   if (private_data) private_data->opcode = opcode;

   switch (insn_word1 & opcode->mask)
   {
      case DO_INSN:
      case DOW_INSN:
         pic30_disassemble_do_insn (opcode, insn_word1, insn_word2, info);
         break;

      case CALL_INSN:
      case GOTO_INSN:
         pic30_disassemble_call_goto_insn (opcode, insn_word1,
                                           insn_word2, info);
         break;

      case BFEXTF_INSN:
      case BFEXT_INSN:

      case BFINSF_INSN:
      case BFINSL_INSN:
      case BFINS_INSN:
         pic30_disassemble_bf_insn (opcode, insn_word1, insn_word2, info);
         break;
      default:
         rc = FALSE;
         break;
   } /* switch (insn_word1) */

   return rc;
}

/******************************************************************************/

unsigned char
pic30_disassemble_1word_insn (opcode, insn, info)
   const struct pic30_opcode * opcode;
   unsigned long insn;
   struct disassemble_info *info;
{
   unsigned char rc = TRUE;
   int j;
   struct pic30_private_data *private_data;

   private_data = info->private_data;

   for (j = 0; j < opcode->number_of_operands; j++)
   {
      const struct pic30_operand * opnd =
         &(pic30_operands[opcode->operands[j]]);

      if (opnd->extract)
      {
         char * extracted_string;
         unsigned char error_found = FALSE;

         extracted_string =
            (*opnd->extract) (insn, info, opcode->flags, opnd, &error_found);

         free (extracted_string);

         if (error_found)
         {
            rc = FALSE;
            break;
         } /* if (error_found) */
      } /* if (opnd->extract) */
   } /* for j */

   if (private_data) private_data->opcode = opcode;

   if (rc)
   {
      (*info->fprintf_func) (info->stream, "%-10s", opcode->name);

      for (j = 0; j < opcode->number_of_operands; j++)
      {
         unsigned char print_symbol = FALSE;
         long operand_value = 0;
         char operand_string[BUFSIZ] = "";
         const struct pic30_operand * opnd =
            &(pic30_operands[opcode->operands[j]]);

         if (opnd->extract)
         {
            char * extracted_string;
            unsigned char error_found = FALSE;

            extracted_string =
               (*opnd->extract) (insn, info,
                                 opcode->flags, opnd, &error_found);
            strcpy (operand_string, extracted_string);
            free (extracted_string);
         } /* if (opnd->extract) */
         else
         {
            operand_value = ((insn) >> opnd->shift) & ((1 << opnd->bits) - 1);

            switch (opnd->type)
            {
               case OPND_REGISTER_DIRECT:
                  record_private_data(info, operand_value, -1*opnd->type);
                  strcpy (operand_string, pic30_registers[operand_value]);
                  break;

               case OPND_SYMBOL:
                  if ((*info->symbol_at_address_func) (operand_value, info))
                     print_symbol = TRUE;
                  /* drop through */
               case OPND_VALUE:
               {
                  record_private_data(info, operand_value, -1*opnd->type);
                  if (opnd->immediate)
                     strcpy (operand_string, "#0x");
                  else
                     strcpy (operand_string, "0x");

                  if (!PRINT_SYMBOL)
                     sprintf (operand_string, "%s%lx", operand_string,
                                                       operand_value);

                  break;
               }
            } /* switch */
         } /* else */

         (*info->fprintf_func) (info->stream, "%s", operand_string);

         if (PRINT_SYMBOL)
            (*info->print_address_func) (operand_value, info);

         if ((j != (opcode->number_of_operands - 1)) &&
             (strcmp (operand_string, "") != 0))
            (*info->fprintf_func) (info->stream, ", ");
      } /* for j */
   } /* if (rc) */

   return rc;
}

/******************************************************************************/

int
pic30_print_insn (memaddr, info)
   bfd_vma memaddr;
   struct disassemble_info *info;
{
   int rc = 0;
   bfd_byte bytes_read[PIC30_SIZE_OF_PROGRAM_WORD + 1];
   unsigned long insn = 0;
   int opb = info->octets_per_byte;
   int status;
   int i,j;
   int is_ecore= pic30_is_ecore_machine(bfd_lookup_arch(info->arch,info->mach));
   int is_isav4= pic30_is_isav4_machine(bfd_lookup_arch(info->arch,info->mach));
   struct pic30_private_data *private_data;

   private_data = info->private_data;
   if (private_data) {
      private_data->opcode = 0;
      private_data->opnd_no = 0;
   }
   pic30_disassembly_address = memaddr; /* make insn address visible */

   if (info->disassembler_options)
   {
      pic30_parse_disassembler_options (info->disassembler_options);
      
      /* To avoid repeated parsing of these options, we remove them here.  */
      info->disassembler_options = NULL;
   } /* if (info->disassembler_options) */

   if (info->section == NULL) 
     return rc;

   if (PIC30_DISPLAY_AS_DATA_MEMORY(info->section))
   {
      status = (*info->read_memory_func) (memaddr, bytes_read,
                                          PIC30_SIZE_OF_DATA_WORD * opb, info);

      if (status != 0)
      {
         (*info->memory_error_func) (status, memaddr, info);
         return -1;
      } /* if (status != 0) */
   
      for (i = 0, j = 0; i < PIC30_SIZE_OF_DATA_WORD * opb; i++)
        if (PIC30_CAN_PRINT_IN_DATA_MEMORY(i))
          {
            insn = insn | ((unsigned long)bytes_read[i] << (8 * j));
            j++;
          }

      (*info->fprintf_func) (info->stream, ".word %#04x", insn);

      rc = PIC30_SIZE_OF_DATA_WORD * opb;
   } /* if (PIC30_DISPLAY_AS_DATA_MEMORY(info->section)) */

   else if (PIC30_DISPLAY_AS_PROGRAM_MEMORY(info->section))
   {
      unsigned char found_insn = FALSE;

      status = (*info->read_memory_func) (memaddr, bytes_read,
                                          PIC30_SIZE_OF_PROGRAM_WORD, info);

      if (status != 0)
      {
         (*info->memory_error_func) (status, memaddr, info);
         return -1;
      } /* if (status != 0) */

      for (i = 0; i < PIC30_SIZE_OF_PROGRAM_WORD; i++)
         insn = insn | ((unsigned long)bytes_read[i] << (8 * i));

      for (i = 0; i < pic30_num_opcodes; i++)
      {
#if 1
         int valid_insn = 1;
  
         if (is_isav4) {
           /* must be marked for F_ISVA4 or not marked at all */
           if (((pic30_opcodes[i].flags & F_ISAV4) == 0) &&
               (pic30_opcodes[i].flags & (F_ECORE | F_FCORE))) valid_insn = 0;
         } else if (is_ecore) {   
           /* must be marked for F_ECORE or not marked at all */
           if (((pic30_opcodes[i].flags & F_ECORE) == 0) &&
               (pic30_opcodes[i].flags & (F_ISAV4 | F_FCORE))) valid_insn = 0;
         } else {
           if (pic30_opcodes[i].flags & (F_ISAV4 | F_ECORE)) valid_insn = 0;
         }
         if (valid_insn == 0) continue;
#else
         if ((pic30_opcodes[i].flags & F_ISAV4) && !is_isav4) continue;
         if ((pic30_opcodes[i].flags & F_ECORE) && !is_ecore) continue;
         if ((pic30_opcodes[i].flags & F_FCORE) && is_ecore) continue;
#endif
         if ((pic30_opcodes[i].number_of_words != 0) &&
             ((pic30_opcodes[i].mask & insn) == pic30_opcodes[i].opcode[0]))
         {
            if (pic30_opcodes[i].number_of_words == 2)
            {
               char rc = pic30_disassemble_2word_insn(&pic30_opcodes[i],
                                                      insn, memaddr, info);

               if (rc < 0)  
                  found_insn = FALSE;
               else
                  found_insn = (unsigned char)rc;
            } /* if (pic30_opcodes[i].number_of_words == 2) */
            else if (pic30_opcodes[i].number_of_words == 1)
               found_insn = pic30_disassemble_1word_insn(&pic30_opcodes[i],
                                                         insn, info);
            else
               found_insn = FALSE;

            break;
         } /* if ((pic30_opcodes[i].mask & insn) == pic30_opcodes[i].opcode) */
      } /* for i */

      rc = PIC30_SIZE_OF_PROGRAM_WORD;

      if (found_insn)
         rc *= pic30_opcodes[i].number_of_words;
      else
         (*info->fprintf_func) (info->stream, ".pword %#08x", insn);
   } /* else if (PIC30_DISPLAY_AS_PROGRAM_MEMORY(info->section))*/

   else if (PIC30_DISPLAY_AS_READONLY_MEMORY(info->section))
     {
       status = (*info->read_memory_func) (memaddr, bytes_read,
                                           PIC30_SIZE_OF_PROGRAM_WORD, info);

       if (status != 0)
         {
           (*info->memory_error_func) (status, memaddr, info);
           return -1;
         } /* if (status != 0) */

       for (i = 0; i < PIC30_SIZE_OF_PROGRAM_WORD; i++)
         insn = insn | ((unsigned long)bytes_read[i] << (8 * i));

       rc = PIC30_SIZE_OF_PROGRAM_WORD;

       (*info->fprintf_func) (info->stream, ".word %#0x", insn & 0xFFFF);
     } /* else if (PIC30_DISPLAY_AS_READONLY_MEMORY(info->section)) */

   return rc;
}

/******************************************************************************/

void
pic30_disassemble_bf_insn (opcode, insn_word1, insn_word2, info)
   const struct pic30_opcode * opcode;
   unsigned long insn_word1;
   unsigned long insn_word2;
   struct disassemble_info * info;
{
   int i;
   int start_offset = -1;

   (*info->fprintf_func) (info->stream, "%-10s", opcode->name);

   for (i = 0; i < opcode->number_of_operands; i++)
   {
      const struct pic30_operand * opnd =
         &(pic30_operands[opcode->operands[i]]);

      char operand_string[BUFSIZ] = "";
      unsigned long insn_word = insn_word1;
      long operand_value = 0;
      unsigned char print_symbol = FALSE;

      if ((insn_word1 & opcode->mask) == BFINSL_INSN) {
        if (i >= 2) insn_word = insn_word2;
      } else if (((insn_word1 & opcode->mask) == BFEXTF_INSN) ||
                 ((insn_word1 & opcode->mask) == BFEXT_INSN)) {
        if (i == 2) insn_word = insn_word2;
      } else if (i == 3) insn_word = insn_word2;

      if (opnd->extract)
      {
         char * extracted_string;
         unsigned char error_found = FALSE;

         extracted_string =
            (*opnd->extract) (insn_word, info,
                              opcode->flags, opnd, &error_found);
         strcpy (operand_string, extracted_string);
         free (extracted_string);
      } /* if (opnd->extract) */
      else
      {
         operand_value = ((insn_word) >> opnd->shift) & ((1 << opnd->bits) - 1);

         switch (opnd->type)
         {
            case OPND_REGISTER_DIRECT:
               strcpy (operand_string, pic30_registers[operand_value]);
               break;

            case OPND_SYMBOL:
               if ((*info->symbol_at_address_func) (operand_value, info))
                  print_symbol = TRUE;
               /* drop through */
            case OPND_VALUE:
            {  int value;

               value = operand_value;
               if (i == 0) start_offset = operand_value;
               if (i == 1) value = value - start_offset + 1;
               if (opnd->immediate)
                  strcpy (operand_string, "#0x");
               else
                  strcpy (operand_string, "0x");

               if (!PRINT_SYMBOL)
                  sprintf (operand_string, "%s%lx", operand_string, value);
            }
         } /* switch */
      } /* else */

      (*info->fprintf_func) (info->stream, "%s", operand_string);

      if ((!opnd->extract) && (PRINT_SYMBOL))
         (*info->print_address_func) (operand_value, info);

      if ((i != (opcode->number_of_operands - 1)) &&
          (strcmp (operand_string, "") != 0))
         (*info->fprintf_func) (info->stream, ", ");
   }
}

/*** instruction semantics ***/

void
pic30_debug_semantics(opcode, buffer, n)
  const struct pic30_opcode *opcode;
  char *buffer;
  int n;
{
  /* debug and store description in buffer - might want to do some pretty 
        printing, but not now */
  *buffer = 0;
  if (opcode->semantics) strncpy(buffer, opcode->semantics, n);
}

struct expr_info {
  const char *op;
  int number_of_operands;
};

struct expr_info expr_table[] = {
 { S_ADD,   2 },
 { S_ADDC,  2 },
 { S_AND,   2 },
 { S_ASR,   2 },
 { S_BCLR,  2 },
 { S_BRA,   1 },
 { S_BRAC,  2 },
 { S_BSET,  2 },
 { S_CALL,  1 },
 { S_CLR,   1 },
 { S_CLRAC, 1 },
 { S_CLBR,  1 },
 { S_CP0,   1 },
 { S_CPB,   1 },
 { S_DEREF, 1 },
 { S_DEC,   1 },
 { S_DEC2,  1 },
 { S_DIV,   2 },
 { S_DO,    2 },
 { S_ED,    5 },
 { S_EDAC,  5 },
 { S_EXCH,  2 },
 { S_EQ,    0 },
 { S_FBCL,  1 },
 { S_FF1L,  1 },
 { S_FF1R,  1 },
 { S_GT,    0 },
 { S_INC,   1 },
 { S_INC2,  1 },
 { S_IOR,   2 },
 { S_LNK,   1 },
 { S_LSR,   2 },
 { S_LT,    0 },
 { S_MAC,   2 },
 { S_MOD,   2 },
 { S_MOV,   2 },
 { S_MPY,   2 },
 { S_MPYN,  2 },
 { S_MSC,   2 },
 { S_MUL,   2 },
 { S_NE,    0 },
 { S_NEG,   1 },
 { S_NOP,   0 },
 { S_OTHER, 1 },
 { S_POP,   1 },
 { S_PFTCH, 2 },
 { S_PUSH,  1 },
 { S_RET,   0 },
 { S_RESET, 0 },
 { S_ROTLC, 1 },
 { S_ROTLN, 1 },
 { S_ROTRC, 1 },
 { S_ROTRN, 1 },
 { S_RPT,   1 },
 { S_SE,    1 },
 { S_SET,   1 },
 { S_SKPC,  0 },
 { S_SKPS,  0 },
 { S_SL,    2 },
 { S_SQR,   2 },
 { S_SUB,   2 },
 { S_SUBB,  2 },
 { S_SUBR,  2 },
 { S_SUBBR, 2 },
 { S_SWAP,  1 },
 { S_TGL,   2 },
 { S_TST,   2 },
 { S_ULNK,  0 },
 { S_XOR,   2 },
 { S_ZE,    1 },
 { S_UNK,   0 },
 { 0, 0 }
};

#define DEBUG 0

struct pic30_semantic_operand *
pic30_semantic_operand_n(e,n)
  struct pic30_semantic_expr *e;
  int n;
{
  struct pic30_semantic_operand *res;

  int j;
  for (j = 0; j < MAX_OPERANDS; j++) {
    if ((e->operands[j].kind == ok_operand_number) && 
        (e->operands[j].value.number == n)) return &e->operands[j];
    if (e->operands[j].kind == ok_subexpr) {
       res = pic30_semantic_operand_n(e->operands[j].value.e, n);
       if (res) return res;
    } 
  }
  return 0;
}

static struct pic30_semantic_expr *
build_expr(struct disassemble_info *, struct pic30_private_data *, int *);

static int build_expr_operands(info, private_data, index, e)
  struct disassemble_info *info;
  struct pic30_private_data *private_data;
  int index;
  struct pic30_semantic_expr *e;
{
  int opnd = 0;
  int subexp = 0;
  enum opflags flags;

  if (DEBUG) {
    fprintf(stdout,"*** trying to find opnd: %s\n",private_data->opcode->name);
    fprintf(stdout,"    %s\n",private_data->opcode->semantics);
    fprintf(stdout,"    %s^\n",&"                              "[30-index]);
  }
  /* an operand can be: simple integer (operand number)
   *                    W<int> (a fixed register)
   *                    sub-expression 
   */
  if (expr_table[e->operation].number_of_operands == 0) return index;
  for (opnd = 0; opnd < expr_table[e->operation].number_of_operands; opnd++) {
    if (DEBUG) {
      fprintf(stdout,"*** opnd %d: %s\n",opnd, private_data->opcode->name);
      fprintf(stdout,"    %s\n",private_data->opcode->semantics);
      fprintf(stdout,"    %s^\n",&"                              "[30-index]);
    }
    if ((opnd == 0) && (strcmp(expr_table[e->operation].op,S_MOV) == 0)) {
      flags = of_output;
    } else {
      flags = of_input;
    }
    if ((private_data->opcode->semantics[index] > '0') &&
        (private_data->opcode->semantics[index] <= '9')) {
      e->operands[opnd].kind = ok_operand_number;
      e->operands[opnd].value.number = 
        private_data->opcode->semantics[index++] - '0';
    } else if (private_data->opcode->semantics[index] == '#') {
      int j;

      for (j = index+1; 
           ((private_data->opcode->semantics[j]) &&
            (private_data->opcode->semantics[j] != '#')); j++);
      if ((j!=index) && (private_data->opcode->semantics[j] == '#')) {
        e->operands[opnd].kind = ok_literal;
        e->operands[opnd].value.number = 
          strtol(&private_data->opcode->semantics[index], 0, 0);
        index = j+1;
      } else {
        index++;
      }
    } else if (private_data->opcode->semantics[index] == 'W') {
      e->operands[opnd].kind = ok_register_number;
      index++;
      e->operands[opnd].value.number = 
        (private_data->opcode->semantics[index++] - '0')*10;
      e->operands[opnd].value.number += 
        private_data->opcode->semantics[index++] - '0';
    } else {
      /* subexpression ? */
      struct pic30_semantic_expr *sub_exp;

      sub_exp = build_expr(info, private_data, &index);
      if (sub_exp) {
        e->operands[opnd].kind = ok_subexpr;
        e->operands[opnd].value.e = sub_exp;
      } else {
        if (DEBUG) {
          fprintf(stdout,"*** cannot find opnd %d: %s\n",
                         opnd, private_data->opcode->name);
          fprintf(stdout,"    %s\n",private_data->opcode->semantics);
          fprintf(stdout,"    %s^\n", 
                         &"                              "[30-index]);
        }
        e->operands[opnd].kind = ok_error;
        index = strlen(private_data->opcode->semantics);
      }
    }
    e->operands[opnd].flags = flags;
  }
  return index;
}
  

static struct pic30_semantic_expr *
build_expr(info, private_data, p_index)
  struct disassemble_info *info;
  struct pic30_private_data *private_data;
  int *p_index;
{
  int op;
  struct pic30_semantic_expr *e;
  int index;
  int j;

  index = *p_index;
  if (DEBUG) {
    fprintf(stdout,"*** trying to find op: %s\n",private_data->opcode->name);
    fprintf(stdout,"    %s\n",private_data->opcode->semantics);
    fprintf(stdout,"    %s^\n", &"                              "[30-index]);
  }
  for (op = 0; expr_table[op].op; op++) {
    if (strncmp(expr_table[op].op,&private_data->opcode->semantics[index],
                strlen(expr_table[op].op)) == 0) {
      break;
    }
  }
  if (expr_table[op].op == 0) {
    if (DEBUG) {
      fprintf(stdout,"*** cannot find op: %s\n",private_data->opcode->name);
      fprintf(stdout,"    %s\n",private_data->opcode->semantics);
      fprintf(stdout,"    %s^\n", &"                              "[30-index]);
    }
    return 0;
  }
  e = malloc(sizeof(struct pic30_semantic_expr));
  e->operation = op;
  e->next = 0;
  for (j = 0; j < MAX_OPERANDS; j++) e->operands[j].kind = ok_unused;

  index = build_expr_operands(info, private_data, 
                              index + strlen(expr_table[op].op), e);
  if (private_data->opcode->semantics[index] != 0) {
    /* another expression... */
    struct pic30_semantic_expr *next;

    next = build_expr(info, private_data, &index);
    if (next) {
      e->next = next;
    }
  }
  return e;
}

struct pic30_semantic_expr *
pic30_analyse_semantics(info) 
  struct disassemble_info *info;
{
  struct pic30_private_data *private_data = 0;
  int index = 0;

  if (info == 0) return 0;
  private_data = info->private_data;
  if ((private_data == 0) || (private_data->opcode == 0)) return 0;

  return build_expr(info, private_data, &index);
}

   

/******************************************************************************/
