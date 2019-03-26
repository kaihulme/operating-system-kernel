/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ PCB_LENGTH ]; pcb_t* current = NULL;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {

  char prev_pid = '?', next_pid = '?';

  // preserve ctx of previous process
  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) );
    prev_pid = '0' + prev->pid;
  }

  // load ctx of of next process
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) );
    next_pid = '0' + next->pid;
  }

  PL011_putc( UART0, '[',      true );
  PL011_putc( UART0, prev_pid, true );
  PL011_putc( UART0, '-',      true );
  PL011_putc( UART0, '>',      true );
  PL011_putc( UART0, next_pid, true );
  PL011_putc( UART0, ']',      true );

  current = next;

  return;
}

void printPriorities() {
  char pP3 = pcb[0].priority + 48;
  char pP4 = pcb[1].priority + 48;
  char pP5 = pcb[2].priority + 48;

  PL011_putc( UART0, '(',      true );
  PL011_putc( UART0, pP3, true );
  PL011_putc( UART0, ',',      true );
  PL011_putc( UART0, pP4, true );
  PL011_putc( UART0, ',',      true );
  PL011_putc( UART0, pP5, true );
  PL011_putc( UART0, ')',      true );
}

int findHighestPriority() {

  // assume console has the highest priority
  int highestP = pcb[ 0 ].priority, highestP_index = 0;

  // for each user program
  for ( int i = 1; i < PCB_LENGTH; i++ ) {

    // PL011_putc( UART0, '\n', true );
    // char ci = i + 48;
    // char pidi = pcb[i].pid + 48;
    // PL011_putc( UART0, 'P', true );gnome
    // PL011_putc( UART0, 'I', true );
    // PL011_putc( UART0, 'D', true );
    // PL011_putc( UART0, ' ', true );
    // PL011_putc( UART0, ci, true );
    // PL011_putc( UART0, ' ', true );
    // PL011_putc( UART0, pidi, true );
    // PL011_putc( UART0, '\n', true );

    // if program is ready (pid >= 1) & priority is highest
    if ( pcb[ i ].pid >= 1 && pcb[ i ].priority > highestP ) {
      highestP       = pcb[ i ].priority;
      highestP_index = i;
    } else {
    }
  }

  return highestP_index;
}

void updatePriorities( int current_i ) {

  for ( int i=0; i<PCB_LENGTH; i++ ) {
    if ( pcb[ i ].pid >= 1 && i != current_i ) pcb[ i ].priority++;
    else                                       pcb[ i ].priority = pcb[ i ].basePriority;
  }

  return;
}

void schedule_priorityBased( ctx_t* ctx ) {

  int current_i = ((current->pid) % PCB_LENGTH) - 1;
  int next_i    = findHighestPriority();

  // PL011_putc( UART0, '\n', true );
  // char ci = current_i + 48;
  // char ni = next_i + 48;
  // PL011_putc( UART0, ni, true );
  // PL011_putc( UART0, ' ', true );
  // PL011_putc( UART0, ci, true );
  // PL011_putc( UART0, '\n', true );

  if ( !(pcb[ next_i ].pid == pcb[ current_i ].pid) && (pcb[ next_i ].priority > pcb[ current_i ].priority)) {

    // PL011_putc( UART0, 'D', true );

    dispatch(ctx, &pcb[ current_i ], &pcb[ next_i ]);

    pcb[ current_i ].status = STATUS_READY;
    pcb[ next_i ].status    = STATUS_EXECUTING;

    current_i = next_i;

  }

  updatePriorities( current_i );

  return;
}

void schedule_roundRobin( ctx_t* ctx ) {

  int length    = sizeof(pcb)/sizeof(pcb[0]);
  int current_i = (current->pid)     % length;
  int next_i    = (current->pid + 1) % length;

  dispatch( ctx, &pcb[current_i], &pcb[next_i] );

  pcb[ current_i ].status = STATUS_READY;
  pcb[ next_i ].status    = STATUS_EXECUTING;

  return;
}

void schedule_twoTasksOnly( ctx_t* ctx ) {

  //PL011_putc( UART0, 'C',      true );

  if        ( current->pid == pcb[ 0 ].pid ) {

    dispatch( ctx, &pcb[ 0 ], &pcb[ 1 ] );      // context switch P_3 -> P_4

    pcb[ 0 ].status = STATUS_READY;             // update   execution status  of P_3
    pcb[ 1 ].status = STATUS_EXECUTING;         // update   execution status  of P_4

  } else if ( current->pid == pcb[ 1 ].pid ) {

    dispatch( ctx, &pcb[ 1 ], &pcb[ 0 ] );      // context switch P_2 -> P_1

    pcb[ 1 ].status = STATUS_READY;             // update   execution status  of P_4
    pcb[ 0 ].status = STATUS_EXECUTING;         // update   execution status  of P_3

  }

  return;
}

void setRemainingEmpty( int n ) {
  for ( int i=n; i<PCB_LENGTH; i++ ) {
    pcb [ i ].pid = -1;
  }
  return;
}

extern void     main_console();
extern uint32_t tos_console;
uint32_t tos;

void hilevel_handler_rst( ctx_t* ctx              ) {

  PL011_putc( UART0, 'R', true );

  // initialise console

  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );     // initialise 0-th PCB = P_3
  pcb[ 0 ].pid          = 1;
  pcb[ 0 ].status       = STATUS_CREATED;
  pcb[ 0 ].ctx.cpsr     = 0x50;
  pcb[ 0 ].ctx.pc       = ( uint32_t )( &main_console );
  pcb[ 0 ].ctx.sp       = ( uint32_t )( &tos_console  );
  pcb[ 0 ].basePriority = 1;
  pcb[ 0 ].priority     = pcb[ 0 ].basePriority;

  tos = tos_console;

  setRemainingEmpty( 1 );

  dispatch( ctx, NULL, &pcb[ 0 ] );

  // configure timer for interrupt handling

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable          timer

  GICC0->PMR          = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer          interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  return;

}

void doNothing() {
  // do nothing (for breakpoints)
  return;
}

void hilevel_handler_irq( ctx_t* ctx ) {

  // get interrupt identifier
  uint32_t id = GICC0->IAR;

  // hadle and reset source
  if( id == GIC_SOURCE_TIMER0 ) {
    PL011_putc( UART0, 'T', true );
    schedule_priorityBased( ctx );
    TIMER0->Timer1IntClr = 0x01;
  }

  // write interrupt identifier signal
  GICC0->EOIR = id;

  return;
}

int getPCBPosition() {
  return 1;
}

pid_t generatePID( pid_t pid ) {
  for ( int i=0; i<PCB_LENGTH; i++ ) {
    if ( pid == pcb[ i ].pid) pid = generatePID( pid++ );
  }
  return pid;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

  // based on id execute supervisor call
  switch( id ) {

    // SVC yield()
    case 0x00: {
      schedule_twoTasksOnly( ctx );
      break;
    }

    // SCV write()
    case 0x01: {
      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );
      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }
      ctx->gpr[ 0 ] = n;
      break;
    }

    // SVC fork()
    case 0x03: {

      PL011_putc( UART0, 'F',      true );

      // get PCB position and create unique PID
      int   child_i   = getPCBPosition();
      pid_t child_pid = generatePID( 0 );

      // get tos and stack pointers
      uint32_t child_tos = tos + 0x00001000;
      uint32_t child_sp  = ctx->sp + 0x00001000;

      // copy state of parent into child
      ctx_t child_ctx;
      memcpy( &child_ctx, &ctx, sizeof(ctx_t) );

      // initialise child in PCB with gpr[0] = 0
      memset( &pcb[ child_i ], 0, sizeof( pcb_t ) );
      pcb[ child_i ].pid          = child_pid;
      pcb[ child_i ].status       = STATUS_CREATED;
      pcb[ child_i ].ctx          = child_ctx;
      pcb[ child_i ].ctx.gpr[ 0 ] = 0;
      pcb[ child_i ].ctx.sp       = child_sp;
      pcb[ child_i ].tos          = child_tos;
      pcb[ child_i ].basePriority = 1;
      pcb[ child_i ].priority     = pcb[ child_i ].basePriority;

      // copy stack of parent into child
      uint32_t stack_size = tos - ctx->sp;
      memcpy((void *) child_sp, (const void*)ctx->sp, stack_size);

      // update tos
      tos = child_tos;

      // return from fork with parent-> child_pid
      ctx->gpr[ 0 ] = child_pid;

      doNothing();

      PL011_putc( UART0, 'G',      true );

      break;

    }

    // SVC exit()
    case 0x04: {



      break;

    }

    // SVC exec()
    case 0x05: {

      PL011_putc( UART0, 'E',      true );

      ctx->pc = ctx->gpr[ 0 ];
      ctx->sp = tos;

      break;
    }

    // SVC unknown / unsupported
    default: { break; }

  }

  return;
}
