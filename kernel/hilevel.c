/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

pcb_t pcb[ PCB_LENGTH ]; pcb_t* current = NULL;

void printStr( char* string ) {

  for ( int i=0; i<strlen(string); i++ ) {
    PL011_putc( UART0, string[ i ], true );
  }

  return;
}

void printInt( int n ) {

  char* string;
  itoa( string, n );
  printStr( string );

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
    if ( pcb[ i ].status == STATUS_TERMINATED || pcb[ i ].pid < 0) {
      return i;
    }
  }

  return -1;
}

// generate unique PID for process
pid_t generatePID() {

  for ( pid_t pid=1; pid<=PCB_LENGTH; pid++ ) { // 1 <= PID <= PCB_LENGTH
    int instances = 0;                          // number of instances of that PID
    for ( index_t i=0; i<PCB_LENGTH; i++ ) {    // for each in PCB
      if ( pcb [ i ].pid == pid ) {
        instances++;
      }
    } if ( instances == 0 ) {
      return pid;                               // return unique PID
    }
  }

  return PCB_LENGTH; // returns final position (shouldn't happen)
}

// performs a context switch (ctx: current context, prev: current process, next: next process)
void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {

  char p = '?';
  char n = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) );
    p = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) );
    n = '0' + next->pid;
  }

  printStr( "\nContext Switch: [" );
  PL011_putc( UART0, p, true ); printStr( "] -> [" );
  PL011_putc( UART0, n, true ); printStr( "]   \n" );

  current = next;

  return;
}

// prints priorities of tasks - for debugging
void printProgram( index_t n ) {

  // print console
  if ( n == 0 ) {
    printStr( "Console PCB[" );           printInt( n );
    printStr( "] - PID: " );     printInt( pcb[n].pid );
    printStr( ", Priority: " ); printInt( pcb[n].prio );
    printStr("\n");
  }

  // print single user program
  else {
    printStr( "User PCB[" );              printInt( n );
    printStr( "] - PID: " );     printInt( pcb[n].pid );
    printStr( ", Priority: " ); printInt( pcb[n].prio );
    printStr("\n");
  }

  return;
}

void printProcessTable() {

  printStr("            PROCESS TABLE: active pid = "); printInt( current->pid );
  printStr("\n _________________________________________________\n");
  printStr("| Program Type | PCB index | PID | Priority | Age |\n");
  printStr("|--------------|-----------|-----|----------|-----|\n");
  for ( int i=0; i<PCB_LENGTH; i++ ) {
    if ( pcb[i].status != STATUS_TERMINATED && pcb[i].pid > 0 ) {
      if ( i == 0 ) { printStr( "|   console    |     " ); }
      else { printStr( "|     user     |     " ); }
      printInt( i ); printStr( "     |  " ); printInt( pcb[i].pid );
      printStr( "  |    " ); printInt( pcb[i].prio );
      if (pcb[i].prio < 10 ) { printStr("     |  "); }
      else { printStr("    |  "); }
      printInt( pcb[i].prio - pcb[i].basePrio ); printStr( "  |\n" );
    }
  }
  printStr("|______________|___________|_____|__________|_____|\n");

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
    if ( pcb[ i ].status != STATUS_TERMINATED && pcb[ i ].pid > -1 ) {
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

  printProcessTable();

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

void defrag_pcb() {

  // printStr("\nDEFRAG_PCB() CALL\n");

  for ( index_t i=1; i<PCB_LENGTH-1; i++ ) {

    // printStr("\nPCB[");  printInt(i);
    // printStr("] PID: "); printInt( pcb[i].pid );
    // printStr("\nPCB[");  printInt(i+1);
    // printStr("] PID: "); printInt( pcb[i+1].pid );
    // printStr("\n");

    if ( pcb[ i ].status == STATUS_TERMINATED && pcb[ i+1 ].pid >= 0 ) {

      // printStr("\nMOVE PCB["); printInt(i+1); printStr("] to PCB["); printInt(i); printStr("]\n");

      memcpy( &pcb[ i ], &pcb[ i+1 ], sizeof( pcb_t ) );
      pcb[ i+1 ].pid    =                -1;
      pcb[ i+1 ].status = STATUS_TERMINATED;
      pcb[ i+1 ].prio   =                 0;

    }

  }

  return;
}

void terminate( index_t index ) {

  printStr( "\nTerminating PCB[" ); printInt( index ); printStr("]\n");

  pcb[ index ].pid    =                -1;
  pcb[ index ].status = STATUS_TERMINATED;
  pcb[ index ].prio   =                 0;

  defrag_pcb();

  return;
}

extern void     main_console(); // pointer to console main()
extern uint32_t tos_console;    // pointer to console top-of-stack
uint32_t        tos;            // pointer to current top-of-stack

// high-level reset-interrupt handler (ctx: current context)
void hilevel_handler_rst( ctx_t* ctx ) {

  printStr( "\nRESET...\n" );

  setRemainingEmpty( 0 );

  pid_t console_pid = generatePID();
  index_t console_i = findFreePCB();

  pcb_t console = {                                // creates console process
    .pid          =                   console_pid, // sets console PID to 1
    .status       =                STATUS_CREATED, // sets console status to created
    .ctx.cpsr     =                          0x50, // sets console to SVC mode with IRQ & FIQ enabled
    .ctx.pc       = ( uint32_t )( &main_console ), // sets console PC to main()
    .ctx.sp       = ( uint32_t )( &tos_console  ), // sets console stack pointer to console top-of-stack pointer
    .basePrio =                            10, // sets console base priority
    .prio     =                            10  // sets console priority with age
  }; memcpy( &pcb[ console_i ], &console, sizeof(pcb_t) ); // sets console in PCB

  printStr( "\nConsole process created: \n" ); printProgram( console_i );

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
    printStr( "\ntimer ~~>\n"); // print timer interrupt to qemu
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

      printStr( "\nSYS_FORK" );

      pid_t   child_pid = generatePID();                  // generates unique PID
      index_t child_i   = findFreePCB();                  // calculates position in PCB

      if ( child_i < 0 ) {
        printStr( "\nERROR: PCB is full \n" );            // break if PCB is full
        break;
      }

      pcb_t child = {                                     // creates new child
        .pid      = child_pid,                            // sets childs PID
        .status   = STATUS_CREATED,                       // sets childs status to created
        .basePrio = 5,                                    // sets childs base priority
        .prio     = 5                                     // sets childs priority with age
      };

      memcpy( &child.ctx, ctx, sizeof(ctx_t) );           // copies current context into child
      child.ctx.sp = ctx->sp + P_STACKSIZE;               // sets stack pointer in child
      memcpy( &pcb[ child_i ], &child, sizeof( pcb_t ) ); // sets child in PCB

      pcb[ child_i ].ctx.gpr[ 0 ] = 0;                    // return from fork in child -> 0
      ctx->gpr[ 0 ]               = child_pid;            // return from fork in parent -> child_pid
      tos                        += P_STACKSIZE;          // updates top-of-stack pointer

      printStr( "\nSUCCESS: user program created \n" );
      printProgram( child_i );

      break;
    }

    // SVC exec()
    case SYS_EXEC: {

      printStr( "SYS_EXEC\n" );

      ctx->pc = ctx->gpr[ 0 ]; // updates childs PC
      ctx->sp = tos;           // updates stack pointer

      break;
    }

    // SVC kill()
    case SYS_KILL: {

      printStr( "SYS_KILL\n" );

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
      } else {
        printStr( "\nERROR: process not found \n" );    // break if PCB is full
        break;
      }

      break;
    }

    // SVC exit()
    case SYS_EXIT: {

      printStr( "SYS_EXIT\n" );

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
