/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

SECTIONS {

   /* assign load address (per QEMU) */
  .       =     0x70010000;

  .text : { kernel/lolevel.o(.text) *(.text .rodata) } /* place text segment(s) */
  .data : {                         *(.data        ) } /* place data segment(s) */
  .bss  : {                         *(.bss         ) } /* place bss  segment(s) */

  .heap : {
                     end = .;
             _heap_start = .;
             .           = . + 0x00001000;
             _heap_end   = .; 
  }

  /* align address (per AAPCS) */
  .           = ALIGN( 8 );


  .           = . + 0x00001000; /* allocate stack for IRQ mode */
  tos_irq     = .;              /* assign TOS pointer for IRQ mode */

  .           = . + 0x00001000; /* allocate stack for SVC mode */
  tos_svc     = .;              /* assign TOS pointer for SVC mode */

  .           = . + 0x00001000; /* allocate stack for console */
  tos_console = .;              /* assign TOS pointer for console */

}
