/* OBSOLETE /* Definitions to make GDB run on an ARM under RISCiX (4.3bsd). */
/* OBSOLETE    Copyright (C) 1986, 1987, 1989 Free Software Foundation, Inc. */
/* OBSOLETE  */
/* OBSOLETE    This file is part of GDB. */
/* OBSOLETE  */
/* OBSOLETE    This program is free software; you can redistribute it and/or modify */
/* OBSOLETE    it under the terms of the GNU General Public License as published by */
/* OBSOLETE    the Free Software Foundation; either version 2 of the License, or */
/* OBSOLETE    (at your option) any later version. */
/* OBSOLETE  */
/* OBSOLETE    This program is distributed in the hope that it will be useful, */
/* OBSOLETE    but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* OBSOLETE    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* OBSOLETE    GNU General Public License for more details. */
/* OBSOLETE  */
/* OBSOLETE    You should have received a copy of the GNU General Public License */
/* OBSOLETE    along with this program; if not, write to the Free Software */
/* OBSOLETE    Foundation, Inc., 59 Temple Place - Suite 330, */
/* OBSOLETE    Boston, MA 02111-1307, USA.  *x/ */
/* OBSOLETE  */
/* OBSOLETE /* This is the amount to subtract from u.u_ar0 */
/* OBSOLETE    to get the offset in the core file of the register values.  *x/ */
/* OBSOLETE  */
/* OBSOLETE #define KERNEL_U_ADDR (0x01000000 - (UPAGES * NBPG)) */
/* OBSOLETE  */
/* OBSOLETE /* Override copies of {fetch,store}_inferior_registers in infptrace.c.  *x/ */
/* OBSOLETE #define FETCH_INFERIOR_REGISTERS */
/* OBSOLETE #define HOST_BYTE_ORDER LITTLE_ENDIAN */
