/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ 2 ]; pcb_t* current = NULL;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );

    current = next;                             // update   executing index   to P_{next}

  return;
}

void schedule( ctx_t* ctx ) {
  if     ( current->pid == pcb[ 0 ].pid ) {
    dispatch( ctx, &pcb[ 0 ], &pcb[ 1 ] );      // context switch P_1 -> P_2

    pcb[ 0 ].status = STATUS_READY;             // update   execution status  of P_1
    pcb[ 1 ].status = STATUS_EXECUTING;         // update   execution status  of P_2
  }
  else if( current->pid == pcb[ 1 ].pid ) {
    dispatch( ctx, &pcb[ 1 ], &pcb[ 0 ] );      // context switch P_2 -> P_1

    pcb[ 1 ].status = STATUS_READY;             // update   execution status  of P_2
    pcb[ 0 ].status = STATUS_EXECUTING;         // update   execution status  of P_1
  }

  return;
}

extern void     main_P1();
extern uint32_t tos_P1;
extern void     main_P2();
extern uint32_t tos_P2;

void hilevel_handler_rst( ctx_t* ctx              ) {

  /* Initialise two PCBs, representing user processes stemming from execution
   * of two user programs.  Note in each case that
   *
   * - the CPSR value of 0x50 means the processor is switched into USR mode,
   *   with IRQ interrupts enabled, and
   * - the PC and SP values matche the entry point and top of stack.
   */

  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_1
  pcb[ 0 ].pid      = 1;
  pcb[ 0 ].status   = STATUS_CREATED;
  pcb[ 0 ].ctx.cpsr = 0x50;
  pcb[ 0 ].ctx.pc   = ( uint32_t )( &main_P1 );
  pcb[ 0 ].ctx.sp   = ( uint32_t )( &tos_P1  );

  memset( &pcb[ 1 ], 0, sizeof( pcb_t ) );     // initialise 1-st PCB = P_2
  pcb[ 1 ].pid      = 2;
  pcb[ 1 ].status   = STATUS_CREATED;
  pcb[ 1 ].ctx.cpsr = 0x50;
  pcb[ 1 ].ctx.pc   = ( uint32_t )( &main_P2 );
  pcb[ 1 ].ctx.sp   = ( uint32_t )( &tos_P2  );

  // for(i=0; i<(sizeof(pcb))/sizeof(pcb[0]); i++) {
  //
  //   memset( &pcb[ i ], i, sizeof( pcb_t ) );     // initialise i-th PCB = P_i
  //   pcb[ i ].pid      = i+1;
  //   pcb[ i ].status   = STATUS_CREATED;
  //   pcb[ i ].ctx.cpsr = 0x50;
  //   pcb[ i ].ctx.pc   = ( uint32_t )( &main_P1 );
  //   pcb[ i ].ctx.sp   = ( uint32_t )( &tos_P1  );
  //
  // }


  /* Once the PCBs are initialised, we arbitrarily select the one in the 0-th
   * PCB to be executed: there is no need to preserve the execution context,
   * since it is is invalid on reset (i.e., no process will previously have
   * been executing).
   */

  dispatch( ctx, NULL, &pcb[ 0 ] );

  return;
}

void hilevel_handler_irq() {
  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction,
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // 0x00 => yield()
      schedule( ctx );

      break;
    }

    case 0x01 : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;

      break;
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
