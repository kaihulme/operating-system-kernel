/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ PCB_LENGTH ]; pcb_t* current = NULL;

process_tree_t* p_tree;

/***************** PROCESS TREE FUNCTIONS **********************/

process_tree_t * new_process( pcb_t process ) {

  process_tree_t *new_process = malloc( sizeof( process_tree_t ) );

  if ( new_process ) {
    new_process->process = process;
    new_process->child   = NULL;
    new_process->next    = NULL;
  }

  return new_process;
}

process_tree_t * next_process( process_tree_t * p, pcb_t process) {

  if  ( p == NULL ) { return NULL; }

  while ( p->next ) { p = p->next; }

  return (p->next = new_process( process ));

}

process_tree_t * child_process( process_tree_t * p, pcb_t process) {

  if ( p == NULL) { return NULL; }

  if ( p->child ) { return next_process( p->child, process ); }
  else { return (p->child = new_process( process )); }

}

/***************** PRIORITY LIST FUNCTIONS **********************/

///////////////////////////////////////////////////////////////////////////

// performs a context switch (ctx: current context, prev: current process, next: next process)
void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {

  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) );
    prev_pid = '0' + prev->pid;
  } if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) );
    next_pid = '0' + next->pid;
  }

  PL011_putc( UART0, '[',      true ); PL011_putc( UART0, prev_pid, true ); PL011_putc( UART0, '-',      true );
  PL011_putc( UART0, '>',      true ); PL011_putc( UART0, next_pid, true ); PL011_putc( UART0, ']',      true );

  current = next;

  return;
}

// prints priorities of tasks - for debugging
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

// finds highest priority of active tasks in PCB
int findHighestPriority() {

  priority_t highestP = pcb[ 0 ].priority;
  int highestP_index = 0;

  for ( int i = 1; i < PCB_LENGTH; i++ ) {
    if (highestP < pcb[ i ].priority) {
      highestP       = pcb[ i ].priority;
      highestP_index = i;
    }

  }

  return highestP_index;
}

// ages priorities of non-executed tasks (current_i: position of current process in PCB)
void agePriorities( int current_i ) {

  for ( int i=0; i<PCB_LENGTH; i++ ) {
    if ( pcb[ i ].pid >= 1 && i != current_i ) pcb[ i ].priority++;
    else                                       pcb[ i ].priority = pcb[ i ].basePriority;
  }

  return;
}

// process_tree_t * ageTree( process_tree_t * p, pid_t current_pid ) {
//
//
//
//   if ( p->child == NULL && (p->process->pid != current_pid)) {
//     return p;
//   }
//
//
// }

// priority-based schedulling (ctx: current context)
void schedule_priorityBased( ctx_t* ctx ) {

  int current_i = (current->pid)-1;
  int next_i    = findHighestPriority();

  if ( !(current_i == next_i) && (pcb[ next_i ].priority > pcb[ current_i ].priority)) {
    dispatch(ctx, &pcb[ current_i ], &pcb[ next_i ]);
    pcb[ current_i ].status = STATUS_READY;
    pcb[ next_i ].status    = STATUS_EXECUTING;
    current_i = next_i;
  }

  agePriorities( current_i );

  return;
}

// round-robin schedulling for n tasks (ctx: current context)
void schedule_roundRobin( ctx_t* ctx ) {

  int current_i = (current->pid)-1;
  int next_i    = (current->pid);

  dispatch( ctx, &pcb[current_i], &pcb[next_i] );
  pcb[ current_i ].status = STATUS_READY;
  pcb[ next_i ].status    = STATUS_EXECUTING;

  return;
}

// handles shedulling for two tasks (ctx: current context)
void schedule_twoTasksOnly( ctx_t* ctx ) {

  if      ( current->pid == pcb[ 0 ].pid ) {
    dispatch( ctx, &pcb[ 0 ], &pcb[ 1 ] );      // context switch P_3 -> P_4
    pcb[ 0 ].status = STATUS_READY;             // update   execution status  of P_3
    pcb[ 1 ].status = STATUS_EXECUTING;         // update   execution status  of P_4
  }
  else if ( current->pid == pcb[ 1 ].pid ) {
    dispatch( ctx, &pcb[ 1 ], &pcb[ 0 ] );      // context switch P_2 -> P_1
    pcb[ 1 ].status = STATUS_READY;             // update   execution status  of P_4
    pcb[ 0 ].status = STATUS_EXECUTING;         // update   execution status  of P_3
  }

  return;
}

// marks non-used PCB items as empty (n: first empty item in PCB)
void setRemainingEmpty( int n ) {

  for ( int i=n; i<PCB_LENGTH; i++ ) { // for each remaining item in PCB
    pcb [ i ].pid = -1;                // set PID to -1 (empty)
  }

  return;
}

extern void     main_console(); // pointer to console main()
extern uint32_t tos_console;    // pointer to console top-of-stack
uint32_t        tos;            // pointer to current top-of-stack

// high-level reset-interrupt handler (ctx: current context)
void hilevel_handler_rst( ctx_t* ctx ) {

  PL011_putc( UART0, 'R', true );

  pcb_t console = {                                // creates console process
    .pid          =                             1, // sets console PID to 1
    .status       =                STATUS_CREATED, // sets console status to created
    .ctx.cpsr     =                          0x50, // sets console to SVC mode with IRQ & FIQ enabled
    .ctx.pc       = ( uint32_t )( &main_console ), // sets console PC to main()
    .ctx.sp       = ( uint32_t )( &tos_console  ), // sets console stack pointer to console top-of-stack pointer
    .basePriority =                            10, // sets console base priority
    .priority     =                            10  // sets console priority with age
  };

  // memcpy( &pcb[ 0 ], &console, sizeof(pcb_t) ); // sets console in PCB

  p_tree = new_process( console );

  tos = tos_console;                // updates top of stack pointer

  //setRemainingEmpty( 1 );           // marks remaining in pcb as empty
  dispatch( ctx, NULL, &p_tree->process ); // dispatches console

  memcpy( ctx, &console.ctx, sizeof( ctx ) );

  TIMER0->Timer1Load  = 0x00100000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002; // select 32-bit timer
  TIMER0->Timer1Ctrl |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020; // enable timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080; // enable timer
  GICC0->PMR          = 0x000000F0; // unmask all interrupts
  GICD0->ISENABLER1  |= 0x00000010; // enable timer interrupt
  GICC0->CTLR         = 0x00000001; // enable GIC interface
  GICD0->CTLR         = 0x00000001; // enable GIC distributor

  return;
}

// high-level IRQ-interrupt handler (ctx: current context)
void hilevel_handler_irq( ctx_t* ctx ) {

  uint32_t id = GICC0->IAR;         // get interrupt identifier

  if( id == GIC_SOURCE_TIMER0 ) {   // if timer interrupt
    PL011_putc( UART0, 'T', true ); // print timer interrupt to qemu
    //schedule_priorityBased( ctx );  // call scheduller
    TIMER0->Timer1IntClr = 0x01;    // reset timer interrupt
  }

  GICC0->EOIR = id;                 // write interrupt identifier signal

  return;
}

// generate unique PID for process
pid_t generatePID() {

  for ( int i=1; i<PCB_LENGTH; i++ ) {  // for each in PCB
    if ( pcb[ i ].pid < 0 ) return i+1; // if PCB[i] is not in use return position
  }

  return PCB_LENGTH;                  // returns final position (shouldn't happen)
}

// high-level SVC-interrupt handler
void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

  // based on id execute supervisor call
  switch( id ) {

    // SVC fork()
    case SYS_FORK: {

      pid_t child_pid = generatePID();                 // generates unique PID
      int child_i     = child_pid - 1;                 // calculates position in PCB

      pcb_t child = {                                  // creates new child
        .pid          = child_pid,                     // sets childs PID
        .status       = STATUS_CREATED,                // sets childs status to created
        .basePriority = 5,                             // sets childs base priority
        .priority     = 5                              // sets childs priority with age
      };

      memcpy( &child.ctx, ctx, sizeof(ctx_t) );        // copies current context into child
      child.ctx.gpr[ 0 ] = 0;                 // return from fork in child -> 0
      child.ctx.sp = ctx->sp + P_STACKSIZE;            // sets stack pointer in child

      //memcpy( &pcb[ child_i], &child, sizeof(pcb_t) ); // sets child in PCB
      p_tree = child_process( p_tree, child);

      ctx->gpr[ 0 ]               = child_pid;         // return from fork in parent -> child_pid
      tos                        += P_STACKSIZE;       // updates top-of-stack pointer

      break;
    }

    // SVC exec()
    case SYS_EXEC: {

      ctx->pc = ctx->gpr[ 0 ]; // updates childs PC
      ctx->sp = tos;           // updates stack pointer

      break;
    }

    // SVC kill()
    case SYS_KILL: {


      break;
    }

    // SVC exit()
    case SYS_EXIT: {

      // if ( ctx.gpr[ 0 ] == EXIT_SUCCESS ) {
      //
      //
      //
      // }

      break;
    }

    // SVC yield()
    case SYS_YIELD: {

      schedule_twoTasksOnly( ctx );

      break;
    }

    // SCV write()
    case SYS_WRITE: {

      int   fd = ( int   )( ctx->gpr[ 0 ] );
      char*  x = ( char* )( ctx->gpr[ 1 ] );
      int    n = ( int   )( ctx->gpr[ 2 ] );

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }

      ctx->gpr[ 0 ] = n;

      break;
    }

    // SVC unknown / unsupported
    default: break;

  }

  return;
}
