/* Test the `vtbl3u8' ARM Neon intrinsic.  */
/* This file was autogenerated by neon-testgen.  */

/* { dg-do assemble } */
/* { dg-require-effective-target arm_neon_ok } */
/* { dg-options "-save-temps -O0" } */
/* { dg-add-options arm_neon } */

#include "arm_neon.h"

void test_vtbl3u8 (void)
{
  uint8x8_t out_uint8x8_t;
  uint8x8x3_t arg0_uint8x8x3_t;
  uint8x8_t arg1_uint8x8_t;

  out_uint8x8_t = vtbl3_u8 (arg0_uint8x8x3_t, arg1_uint8x8_t);
}

/* { dg-final { scan-assembler "vtbl\.8\[ 	\]+\[dD\]\[0-9\]+, \\\{((\[dD\]\[0-9\]+-\[dD\]\[0-9\]+)|(\[dD\]\[0-9\]+, \[dD\]\[0-9\]+, \[dD\]\[0-9\]+))\\\}, \[dD\]\[0-9\]+!?\(\[ 	\]+@.*\)?\n" } } */
/* { dg-final { cleanup-saved-temps } } */
