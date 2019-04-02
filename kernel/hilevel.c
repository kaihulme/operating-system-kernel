/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ PCB_LENGTH ]; pcb_t* current = NULL;

void writeStr( char* string ) {

  for ( int i=0; i<strlen(string); i++ ) {
    PL011_putc( UART0, string[ i ], true );
  }

  return;
}

void writeInt( int n ) {

  char* string;
  itoa( string, n );
  writeStr( string );

  return;
}

index_t findByPID( pid_t pid ) {

  for ( index_t i = 0; i<PCB_LENGTH; i++) {
    if ( pcb[i].pid == pid ) {
      return i;
    }
  }

  return -1;

}

index_t findFreePCB() {

  for ( index_t i=0; i<PCB_LENGTH; i++ ) {

    // writeStr("\nPCB["); writeInt( i );

    if ( pcb[ i ].status == STATUS_TERMINATED ) {
      // writeStr("] FREE");
      return i;
    }
    else {
      // writeStr("] USED");
    }

  }

  return 0;
}

// generate unique PID for process
pid_t generatePID() {

  for ( pid_t pid=1; pid<=PCB_LENGTH; pid++ ) { // 1 <= PID <= PCB_LENGTH
    int instances = 0;                      // number of instances of that PID
    for ( index_t i=0; i<PCB_LENGTH; i++ ) {    // for each in PCB
      if ( pcb [ i ].pid == pid ) {
        instances++;
      }
    } if ( instances == 0 ) {
      return pid;                           // return unique PID
    }
  }

  return PCB_LENGTH;                  // returns final position (shouldn't happen)
}

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

  writeStr( "\n[" );  PL011_putc( UART0, prev_pid, true );
  writeStr( "->" ); PL011_putc( UART0, next_pid, true );
  writeStr( "]\n" );

  current = next;

  return;
}

// prints priorities of tasks - for debugging
void printProgram( index_t n ) {

  writeStr("\n");

  // print all user programs
  if ( n == -1 ) {
    for ( int i=1; i<PCB_LENGTH; i++ ) {
      if ( pcb[i].status != STATUS_TERMINATED ) {
        writeStr( "USER PCB[" );           writeInt( i );
        writeStr( "] - PID: " );    writeInt( pcb[i].pid );
        writeStr( ", PRIO: " );    writeInt( pcb[i].prio );
        writeStr("\n");
      }
    }
  }

  // print console
  else if ( n == 0 ) {
    writeStr("CNSL PCB[0] - PID: 1, PRIO: ");
    writeInt( pcb[n].prio );
    writeStr("\n");
  }

  // print single user program
  else {
    writeStr( "USER PCB[" );           writeInt( n );
    writeStr( "] - PID: " );  writeInt( pcb[n].pid );
    writeStr( ", PRIO: " );  writeInt( pcb[n].prio );
    writeStr("\n");
  }

  return;
}

// finds highest priority of active tasks in PCB
int findHighestPriority() {

  prio_t highestP = pcb[ 0 ].prio;
  index_t highestP_i = 0;

  for ( index_t i = 1; i < PCB_LENGTH; i++ ) {
    if ( pcb[ i ].status != STATUS_TERMINATED && highestP < pcb[ i ].prio) {
      highestP       = pcb[ i ].prio;
      highestP_i = i;
    }

  }

  return highestP_i;
}

// ages priorities of non-executed tasks (current_i: position of current process in PCB)
void agePriorities( index_t current_i ) {

  for ( index_t i=0; i<PCB_LENGTH; i++ ) {
    if ( pcb[ i ].status != STATUS_TERMINATED) {
      if ( i != current_i ) pcb[ i ].prio++;
      else                  pcb[ i ].prio = pcb[ i ].basePrio;
    }
  }

  return;
}

// priority-based schedulling (ctx: current context)
void schedule_priorityBased( ctx_t* ctx ) {

  index_t current_i = (current->pid)-1;
  index_t next_i    = findHighestPriority();

  printProgram( 0 );
  printProgram( -1 );

  if ( current_i != next_i ) {
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
void setRemainingEmpty( index_t i ) {

  for ( i; i<PCB_LENGTH; i++ ) { // for each remaining item in PCB
    pcb[ i ].pid    = -1;                // set PID to -1 (empty)
    pcb[ i ].status = STATUS_TERMINATED;
  }

  return;
}

void terminate( index_t i ) {

  writeStr( "\nTERMINATING PCB" ); writeInt( i );

  pcb[ i ].pid    =                -1;
  pcb[ i ].status = STATUS_TERMINATED;

  return;
}

extern void     main_console(); // pointer to console main()
extern uint32_t tos_console;    // pointer to console top-of-stack
uint32_t        tos;            // pointer to current top-of-stack

// high-level reset-interrupt handler (ctx: current context)
void hilevel_handler_rst( ctx_t* ctx ) {

  writeStr( "\nRESET\n" );

  setRemainingEmpty( 0 );

  pid_t console_pid = generatePID();
  index_t console_i   = findFreePCB();

  pcb_t console = {                                // creates console process
    .pid          =                   console_pid, // sets console PID to 1
    .status       =                STATUS_CREATED, // sets console status to created
    .ctx.cpsr     =                          0x50, // sets console to SVC mode with IRQ & FIQ enabled
    .ctx.pc       = ( uint32_t )( &main_console ), // sets console PC to main()
    .ctx.sp       = ( uint32_t )( &tos_console  ), // sets console stack pointer to console top-of-stack pointer
    .basePrio =                            10, // sets console base priority
    .prio     =                            10  // sets console priority with age
  }; memcpy( &pcb[ console_i ], &console, sizeof(pcb_t) ); // sets console in PCB

  writeStr( "\nCONSOLE CREATED: \n" ); printProgram( console_i );

  tos = tos_console;                        // updates top of stack pointer
  setRemainingEmpty( 1 );                    // marks remaining in pcb as empty
  dispatch( ctx, NULL, &pcb[ console_i ] ); // dispatches console

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

  uint32_t id = GICC0->IAR;            // get interrupt identifier

  if( id == GIC_SOURCE_TIMER0 ) {      // if timer interrupt
    // writeStr( "\nTIMER\n"); // print timer interrupt to qemu
    schedule_priorityBased( ctx );     // call scheduller
    TIMER0->Timer1IntClr = 0x01;       // reset timer interrupt
  }

  GICC0->EOIR = id;                    // write interrupt identifier signal

  return;
}

// high-level SVC-interrupt handler
void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

  // based on id execute supervisor call
  switch( id ) {

    // SVC fork()
    case SYS_FORK: {

      writeStr( "\nSYS_FORK" );

      pid_t   child_pid = generatePID();               // generates unique PID
      index_t child_i   = findFreePCB();               // calculates position in PCB

      pcb_t child = {                                  // creates new child
        .pid          = child_pid,                     // sets childs PID
        .status       = STATUS_CREATED,                // sets childs status to created
        .basePrio = 5,                             // sets childs base priority
        .prio     = 5                              // sets childs priority with age
      };

      memcpy( &child.ctx, ctx, sizeof(ctx_t) );        // copies current context into child
      child.ctx.sp = ctx->sp + P_STACKSIZE;            // sets stack pointer in child
      memcpy( &pcb[ child_i], &child, sizeof(pcb_t) ); // sets child in PCB

      pcb[ child_i ].ctx.gpr[ 0 ] = 0;                 // return from fork in child -> 0
      ctx->gpr[ 0 ]               = child_pid;         // return from fork in parent -> child_pid
      tos                        += P_STACKSIZE;       // updates top-of-stack pointer

      writeStr( "\nNEW USER PROGRAM CREATED: \n" ); printProgram( child_i );

      break;
    }

    // SVC exec()
    case SYS_EXEC: {

      writeStr( "SYS_EXEC\n" );

      ctx->pc = ctx->gpr[ 0 ]; // updates childs PC
      ctx->sp = tos;           // updates stack pointer

      break;
    }

    // SVC kill()
    case SYS_KILL: {

      writeStr( "SYS_KILL\n\n\n" );

      pid_t pid           =        ctx->gpr[ 0 ];
      index_t       index =     findByPID( pid );
      index_t   console_i =                    0;
      pid_t   console_pid = pcb[ console_i ].pid;

      if ( index >= 0 ) {
        if ( ctx->gpr[ 1 ] == SIG_TERM ) {
          if ( pid != console_pid ) {
            terminate( findByPID( pid ) );
          }
        }
      } else if ( pid == 0 ) {
        if ( ctx->gpr[ 1 ] == SIG_TERM ) {
          for ( index_t i=console_i+1; i<PCB_LENGTH; i++ ) {
            terminate( i );
          }
        }
      }

      break;
    }

    // SVC exit()
    case SYS_EXIT: {

      writeStr( "SYS_EXIT\n" );

      if ( ctx->gpr[ 0 ] == EXIT_SUCCESS ) {

        index_t i = findByPID( current->pid );

        if ( i >= 0 ) {
          terminate( i );
        }

      }

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
