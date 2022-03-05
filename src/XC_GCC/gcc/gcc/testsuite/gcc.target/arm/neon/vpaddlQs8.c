/* Test the `vpaddlQs8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vpaddlQs8 (void)
{
  int16x8_t out_int16x8_t;
  int8x16_t arg0_int8x16_t;

  out_int16x8_t = vpaddlq_s8 (arg0_int8x16_t);
}

/* { dg-final { scan-assembler "vpaddl\.s8\[ 	\]+\[qQ\]\[0-9\]+, \[qQ\]\[0-9\]+!?\(\[ 	\]+@.*\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
