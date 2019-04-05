/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

// mechanisms for storing processes and pipes between them
pcb_t pcb[ PCB_LENGTH ]; pipe_t pipes[ PFDS_LENGTH ];
pcb_t* current = NULL;

// prints a string to UART (string: string to print to UART)
void printStr( char* string ) {

  for ( int i=0; i<strlen(string); i++ ) {  // for each char in string
    PL011_putc( UART0, string[ i ], true ); // prints char to UART
  }

  return;
}

// prints an int to UART (n: int to print to UART)
void printInt( int n ) {

  char* string;       // creates empty
  itoa( string, n );  // converts int to string
  printStr( string ); // prints string to UART

  return;
}

// given a PID, returns the index of it in the PCB (PID: PID to find)
index_t findByPID( pid_t pid ) {

  for ( index_t i = 0; i<PCB_LENGTH; i++ ) { // for each process
    if ( pcb[i].pid == pid ) {              // if PID is found
      return i;                             // returns position of PID in PCB
    }
  }

  return -1;                                // returns -1 for error: not found

}

// returns the index of the first free position in PCB
index_t findFreePCB() {

  for ( index_t i=0; i<PCB_LENGTH; i++ ) {                            // for each in PCB
    if ( pcb[ i ].status == STATUS_TERMINATED || pcb[ i ].pid < 0 ) { // if PCB is free
      return i;                                                       // returns position of free PCB
    }
  }

  return -1;                                                          // returns -1 for error: PCB full
}

// generate unique PID for process
pid_t generatePID() {

  for ( pid_t pid=1; pid<=PCB_LENGTH; pid++ ) { // for PID = 1 to PCB_LENGTH

    int instances = 0;                          // stores number of instances of that PID

    for ( index_t i=0; i<PCB_LENGTH; i++ ) {    // for each in PCB
      if ( pcb [ i ].pid == pid ) {             // if PID found
        instances++;                            // incriments instances
      }
    }

    if ( instances == 0 ) {                   // if PID is unique
      return pid;                               // returns unique PID
    }

  }

  return -1;                                    // returns -1 for error: PID couldn't be generated
}

// performs a context switch (ctx: current context, prev: current process, next: next process)
void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {

  char p = '?';
  char n = '?';

  if( NULL != prev ) {                                // if current ctx is not null
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) );       // preserves current ctx
    p =                        '0' + prev->pid;       // gets current PID as char
  }
  if( NULL != next ) {                                // if next ctx is not null
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) );       // context switches to next ctx
    n =                        '0' + next->pid;       // gets next PID as char
  }

  printStr( "\nContext Switch: [" );                  // prints context switch to UART
  PL011_putc( UART0, p, true ); printStr( "] -> [" ); // prints current PID to UART
  PL011_putc( UART0, n, true ); printStr( "]   \n" ); // prints next PID to UART

  current = next;                                     // sets current PCB to next PCB

  return;
}

// prints state of a process to UART (n: process to print)
void printProgram( index_t n ) {

  // prints console
  if ( n == 0 ) {
    printStr( "Console PCB[" );           printInt( n );
    printStr( "] - PID: " );     printInt( pcb[n].pid );
    printStr( ", Priority: " ); printInt( pcb[n].prio );
    printStr("\n");
  }

  // prints single user program
  else {
    printStr( "User PCB[" );              printInt( n );
    printStr( "] - PID: " );     printInt( pcb[n].pid );
    printStr( ", Priority: " ); printInt( pcb[n].prio );
    printStr("\n");
  }

  return;
}

// prints the PCB as a table to UART
void printProcessTable() {

  // prints PCB table
  printStr("            PROCESS TABLE: active pid = "); printInt( current->pid );
  printStr("\n _________________________________________________\n");
  printStr("| Program Type | PCB index | PID | Priority | Age |\n");
  printStr("|--------------|-----------|-----|----------|-----|\n");

  for ( int i=0; i<PCB_LENGTH; i++ ) {
    if ( pcb[i].status != STATUS_TERMINATED && pcb[i].pid > 0 ) {

      if ( i == 0 ) { printStr( "|   console    |     " ); }
      else          { printStr( "|     user     |     " ); }

      printInt( i ); printStr( "     |  " );  printInt( pcb[i].pid );
      printStr( "  |    " );                 printInt( pcb[i].prio );

      if (pcb[i].prio < 10 ) { printStr("     |  "); }
      else                   { printStr("    |  "); }

      printInt( pcb[i].prio - pcb[i].basePrio ); printStr( "  |\n" );

    }
  }

  printStr("|______________|___________|_____|__________|_____|\n");

  return;
}

void printPipe( pfd_t pfd ) {

  printStr( "\nPipe[" );                    printInt( pfd );
  printStr( "]: Data: " );    printInt( pipes[ pfd ].data );
  printStr( " Writer-end: " ); printInt( pipes[ pfd ].writer_end );
  printStr( " Reader-end: " );  printInt( pipes[ pfd ].reader_end );

  printStr( ", Status: " );

  if      ( pipes[ pfd ].status == STATUS_OPEN )      printStr( "OPEN\n" );
  else if ( pipes[ pfd ].status == STATUS_READ )      printStr( "READ\n" );
  else if ( pipes[ pfd ].status == STATUS_WRITE )    printStr( "WRITE\n" );
  else if ( pipes[ pfd ].status == STATUS_CLOSED )  printStr( "CLOSED\n" );
  else                                             printStr( "UNKNOWN\n" );

  return;
}

// finds highest priority of active tasks in PCB
int findHighestPriority() {

  prio_t highestP    = pcb[ 0 ].prio;                                        // sets highest priority to console
  index_t highestP_i =             0;                                        // sets position of highest priority to 0

  for ( index_t i = 1; i < PCB_LENGTH; i++ ) {                               // for each user program in PCB
    if ( pcb[ i ].status != STATUS_TERMINATED && highestP < pcb[ i ].prio) { // if active with higher priority
      highestP   = pcb[ i ].prio;                                            // updates highest priority
      highestP_i =             i;                                            // updates position of highest priority
    }

  }

  return highestP_i;                                                         // returns position of highest priority in PCB
}

// ages priorities of non-executed tasks (current_i: position of current process in PCB)
void agePriorities( index_t current_i ) {

  for ( index_t i=0; i<PCB_LENGTH; i++ ) {                             // for each process in PCB
    if ( pcb[ i ].status != STATUS_TERMINATED && pcb[ i ].pid > -1 ) { // if active
      if ( i != current_i ) pcb[ i ].prio++;                           // ages priority if not being executed
      else                  pcb[ i ].prio = pcb[ i ].basePrio;         // sets priority to base prio if being executes
    }
  }

  return;
}

// priority-based schedulling (ctx: current context)
void schedule_priorityBased( ctx_t* ctx ) {

  index_t current_i = (current->pid)-1;
  index_t next_i    = findHighestPriority();

  printProcessTable();

  printPipe( 0 );

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

  if ( current->pid == pcb[ 0 ].pid ) {         // if current is PCB[0]
    dispatch( ctx, &pcb[ 0 ], &pcb[ 1 ] );      // context switches PCB[0] -> PCB[1]
    pcb[ 0 ].status = STATUS_READY;             // updates execution status PCB[0]
    pcb[ 1 ].status = STATUS_EXECUTING;         // updates execution status PCB[1]
  }
  else if ( current->pid == pcb[ 1 ].pid ) {    // if current is PCB[1]
    dispatch( ctx, &pcb[ 1 ], &pcb[ 0 ] );      // context switches PCB[1] -> PCB[0]
    pcb[ 1 ].status = STATUS_READY;             // updates execution status PCB[1]
    pcb[ 0 ].status = STATUS_EXECUTING;         // updates execution status PCB[0]
  }

  return;
}

// marks non-used PCB items as empty (n: first empty item in PCB)
void setRemainingEmpty( index_t i ) {

  for ( i; i<PCB_LENGTH; i++ ) {         // for each remaining item in PCB
    pcb[ i ].pid    = -1;                // sets PID to -1 (empty)
    pcb[ i ].status = STATUS_TERMINATED; // sets status to terminated
  }

  return;
}

// ensures there are no gaps in PCB
void defrag_pcb() {

  for ( index_t i=1; i<PCB_LENGTH-1; i++ ) {                             // for each user program in PCB
    if ( pcb[ i ].status == STATUS_TERMINATED && pcb[ i+1 ].pid >= 0 ) { // if current PCB is empty and next is active
      memcpy( &pcb[ i ], &pcb[ i+1 ], sizeof( pcb_t ) );                 // moves next PCB to empty PCB
      pcb[ i+1 ].pid    =                            -1;                 // sets next PID to -1 (empty)
      pcb[ i+1 ].status =             STATUS_TERMINATED;                 // sets next status to terminated
      pcb[ i+1 ].prio   =                             0;                 // sets next priority to 0
    }
  }

  return;
}

// terminates process at given position in PCB (index: position of process in PCB)
void terminate( index_t index ) {

  printStr( "\nTerminating PCB[" ); printInt( index ); printStr("]\n");

  pcb[ index ].pid    =                -1;
  pcb[ index ].status = STATUS_TERMINATED;
  pcb[ index ].prio   =                 0;

  defrag_pcb();

  return;
}

extern void     main_console(); // pointer to console main()
extern uint32_t    tos_console; // pointer to console top-of-stack

uint32_t tos; // pointer to current top-of-stack

// high-level reset-interrupt handler (ctx: current context)
void hilevel_handler_rst( ctx_t* ctx ) {

  printStr( "\nRESET...\n" );                                             // prints reset call to UART

  setRemainingEmpty( 0 );                                                 // marks all PCBs as empty

  pid_t console_pid = generatePID();                                      // generates unique PID
  index_t console_i = findFreePCB();                                      // gets next free position in PCB

  pcb_t console = {                                                       // creates console process
    .type         =                       CONSOLE,                        // sets type to console
    .pid          =                   console_pid,                        // sets console PID to 1
    .status       =                STATUS_CREATED,                        // sets console status to created
    .ctx.cpsr     =                          0x50,                        // sets console to SVC mode with IRQ & FIQ enabled
    .ctx.pc       = ( uint32_t )( &main_console ),                        // sets console PC to main()
    .ctx.sp       = ( uint32_t )( &tos_console  ),                        // sets console stack pointer to console top-of-stack pointer
    .basePrio     =                            10,                        // sets console base priority
    .prio         =                            10                         // sets console priority with age
  }; memcpy( &pcb[ console_i ], &console, sizeof(pcb_t) );                // sets console in PCB

  printStr( "\nConsole process created: \n" ); printProgram( console_i ); // prints console to UART

  tos = tos_console;                                                      // updates top of stack pointer
  setRemainingEmpty( 1 );                                                 // marks remaining in pcb as empty
  dispatch( ctx, NULL, &pcb[ console_i ] );                               // dispatches console

  TIMER0->Timer1Load  = 0x00100000;                                       // selects period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl  = 0x00000002;                                       // selects 32-bit timer
  TIMER0->Timer1Ctrl |= 0x00000040;                                       // selects periodic timer
  TIMER0->Timer1Ctrl |= 0x00000020;                                       // enables timer interrupt
  TIMER0->Timer1Ctrl |= 0x00000080;                                       // enables timer
  GICC0->PMR          = 0x000000F0;                                       // unmasks all interrupts
  GICD0->ISENABLER1  |= 0x00000010;                                       // enables timer interrupt
  GICC0->CTLR         = 0x00000001;                                       // enables GIC interface
  GICD0->CTLR         = 0x00000001;                                       // enables GIC distributor

  for ( pfd_t pfd=0; pfd<PFDS_LENGTH; pfd++ ) {
    pipes[ pfd ].data       =             0;
    pipes[ pfd ].writer_end =            -1;
    pipes[ pfd ].reader_end =            -1;
    pipes[ pfd ].direction  =             0;
    pipes[ pfd ].status     = STATUS_CLOSED;
  }

  return;
}

// high-level IRQ-interrupt handler (ctx: current context)
void hilevel_handler_irq( ctx_t* ctx ) {

  uint32_t id = GICC0->IAR;        // get interrupt identifier signal

  if( id == GIC_SOURCE_TIMER0 ) {  // if timer interrupt
    printStr( "\ntimer ~~>\n");    // print timer interrupt to UART
    schedule_priorityBased( ctx ); // calls scheduller
    TIMER0->Timer1IntClr = 0x01;   // resets timer interrupt
  }

  GICC0->EOIR = id;                // writes interrupt identifier signal

  return;
}

// bool checkStatus( pfd_t pfd, pipe_status_t check_status ) {
//   return pipes[ pfd ].status == check_status;
// }

// high-level SVC-interrupt handler (ctx: current context, id: system call id)
void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) {

  // based on id execute relevant supervisor call
  switch( id ) {

    //////////////////////////////// EXECUTION ////////////////////////////////

    // SVC yield() - gives control to next process
    case SYS_YIELD: {

      schedule_twoTasksOnly( ctx ); // context switch to next process

      break;
    }

    // SVC fork() - create new child process -> return child PID in parent and 0 in child
    case SYS_FORK: {

      printStr( "\nSYS_FORK" );                           // prints fork to UART

      pid_t   child_pid = generatePID();                  // generates unique PID
      index_t child_i   = findFreePCB();                  // calculates position in PCB

      if ( child_i < 0 ) {
        printStr( "\nERROR: PCB is full \n" );            // break if PCB is full
        break;
      }

      pcb_t child = {                                     // creates new child
        .typedef  = USER,                                 // sets type to user program
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

      printStr( "\nSUCCESS: user program created \n" );   // prints successful fork to UART
      printProgram( child_i );                            // prints child to UART

      break;
    }

    // SVC exec() - execute child process
    case SYS_EXEC: {

      ctx->pc = ctx->gpr[ 0 ]; // sets PC to that of the child program
      ctx->sp =           tos; // updates stack pointer

      break;
    }

     //________________________________________________________________________
    /////////////////////////////// TERMINATION ///////////////////////////////

    // SVC kill() - terminates all or given process by PID
    case SYS_KILL: {

      printStr( "SYS_KILL\n" );                              // prints kill system call to UART

      pid_t           pid =        ctx->gpr[ 0 ];            // get PID to be terminated
      index_t       index =     findByPID( pid );            // finds position of PID in PCB
      index_t   console_i =                    0;            // gets index of console
      pid_t   console_pid = pcb[ console_i ].pid;            // gets PID of console

      if ( index >= 0  && index < PCB_LENGTH ) {             // if position is in PCB
        if ( ctx->gpr[ 1 ] == SIG_TERM ) {                   // if signal is termiante
          if ( pid != console_pid ) {                        // if PID is not console PID
            terminate( index );                              // terminates process as PCB[]
            ctx->gpr[ 0 ] = EXIT_SUCCESS;                    // returns success
            break;
          }
        }
      } else if ( pid == 0 ) {                               // if PID = 0 (terminate all)
        if ( ctx->gpr[ 1 ] == SIG_TERM ) {                   // if signal is terminate
          for ( index_t i=console_i+1; i<PCB_LENGTH; i++ ) { // for each user process
            terminate( i );                                  // terminates process at PCB[i]
          }
          ctx->gpr[ 0 ] = EXIT_SUCCESS;                      // returns success
          break;
        }
      } else {
        printStr( "\nERROR: process not found \n" );         // else prints error not found
        break;
      }

      ctx->gpr[ 0 ] = EXIT_FAILURE;                          // returns failure

      break;
    }

    // SVC exit() - terminates process which calls exit()
    case SYS_EXIT: {

      printStr( "SYS_EXIT\n" );                // prints exit to UART

      if ( ctx->gpr[ 0 ] == EXIT_SUCCESS ) {   // if exit is successful

        index_t i = findByPID( current->pid ); // gets position of current process in PCB

        if ( i >= 0  && i < PCB_LENGTH ) {     // if position is in PCB
          terminate( i );                      // terminates process at PCB[i]
        }

      }

      break;
    }

     //________________________________________________________________________
    ////////////////////////////////// PIPES //////////////////////////////////

    // SVC exec_child() - executes child process with pipe
    case SYS_EXEC_CHILD: {

      uint32_t   x = (uint32_t) ( ctx->gpr[ 0 ] );
      pfd_t    pfd = (pfd_t)    ( ctx->gpr[ 1 ] );

      ctx->pc       = (uint32_t) x; // updates childs PC
      ctx->sp       =          tos; // updates stack pointer

      ctx->gpr[ 0 ] = (pfd_t) pfd ; // gives pipe to child

      break;
    }

    // SVC pipe_open() - opens pipe
    case SYS_PIPE_OPEN: {

      pfd_t pfd = 0;

      pipe_t* pipe = &pipes[ pfd ];

      pipe->data       =           0;
      pipe->writer_end =          -1;
      pipe->reader_end =          -1;
      pipe->direction  =        OPEN;
      pipe->status     = STATUS_OPEN;

      ctx->gpr[ 0 ]    =         pfd; // returns pipe position of pipe in pipes

      printStr( "\nPipe created:" ); printPipe( pfd );

      break;
    }

    // SVC pipe_writer_end() - sets pipe writer end point
    case SYS_PIPE_WRITER_END: {

      pfd_t pfd = ( pfd_t ) ( ctx->gpr[ 0 ] );
      pid_t pid = ( pid_t ) ( ctx->gpr[ 1 ] );

      pipe_t*     pipe = &pipes[ pfd ];
      pipe->writer_end = pid;

      if ( pipe->reader_end > 0 ) {
        pipe->status    = STATUS_WRITE;
        pipe->direction =        WRITE;
      }

      ctx->gpr[ 1 ] = pfd;

      break;
    }

    // SVC pipe_write() - writes to pipe
    case SYS_PIPE_WRITE: {

      pfd_t            pfd = ( pfd_t )    ( ctx->gpr[ 0 ] ); // gets position of pipe in pipes
      uint32_t pipe_signal = ( uint32_t ) ( ctx->gpr[ 1 ] ); // gets signal to write to pipe
      pipe_t* pipe         =                  &pipes[ pfd ]; // gets pointer to pipe in pipes

      pipe->data      = pipe_signal;
      pipe->status    = STATUS_READ;
      pipe->direction =        READ;

      ctx->gpr[ 0 ]   =         pfd; // returns position of pipe in pipes

      break;
    }

    // SVC pipe_writable() - gets if pipe is waiting for write
    case SYS_PIPE_WRITABLE: {

      pfd_t pfd      = ( pfd_t )( ctx->gpr[ 0 ] );                              // gets pipe location in pipe
      pipe_t*   pipe =              &pipes[ pfd ];

      if ( pipe->status == STATUS_WRITE ) {
        ctx->gpr[ 0 ] =  ( bool ) ( true );
      }
      else if ( pipe->direction >= WRITE ) {
        ctx->gpr[ 0 ] =  ( bool ) ( true );
      }
      else {
        ctx->gpr[ 0 ] = ( bool ) ( false );
      }

      break;
    }

    // SVC pipe_reader_end() - sets pipe reader end point
    case SYS_PIPE_READER_END: {

      pfd_t pfd = ( pfd_t ) ( ctx->gpr[ 0 ] );
      pid_t pid = ( pid_t ) ( ctx->gpr[ 1 ] );

      pipe_t* pipe     = &pipes[ pfd ];
      pipe->reader_end =           pid;

      if ( pipe->writer_end > 0 ) {
        pipe->status    = STATUS_WRITE;
        pipe->direction =        WRITE;
      };

      ctx->gpr[ 1 ] = pfd;

      break;
    }

    // SVC pipe_read() - reads from pipe
    case SYS_PIPE_READ: {

      pfd_t            pfd = ( pfd_t )    ( ctx->gpr[ 0 ] ); // gets position of pipe in pipes
      uint32_t pipe_signal = ( uint32_t ) ( ctx->gpr[ 1 ] ); // gets signal to write to pipe
      pipe_t* pipe         =                  &pipes[ pfd ]; // gets pointer to pipe in pipes

      pipe_signal     =   pipe->data;
      pipe->status    = STATUS_WRITE;
      pipe->direction =        WRITE;

      ctx->gpr[ 0 ]   =  pipe_signal; // returns signal from pipe

      break;
    }

    // SVC pipe_readable - gets if pipe is waiting for read
    case SYS_PIPE_READABLE: {

      pfd_t pfd      = ( pfd_t )( ctx->gpr[ 0 ] );                              // gets pipe location in pipe
      pipe_t*   pipe =              &pipes[ pfd ];

      if ( pipe->status == STATUS_READ ) {
        ctx->gpr[ 0 ] =  ( bool ) ( true );
      }
      else if ( pipe->direction <= READ ) {
        ctx->gpr[ 0 ] =  ( bool ) ( true );
      }
      else {
        ctx->gpr[ 0 ] = ( bool ) ( false );
      }

      break;
    }

     //________________________________________________________________________
    /////////////////////////////////// I/O ///////////////////////////////////

    // SCV write() - writes string to UART
    case SYS_WRITE: {

      int   fd = ( int   )( ctx->gpr[ 0 ] ); // gets pipe
      char*  x = ( char* )( ctx->gpr[ 1 ] ); // gets string x
      int    n = ( int   )( ctx->gpr[ 2 ] ); // gets length of string

      for( int i = 0; i < n; i++ ) {         // for each char in x
        PL011_putc( UART0, *x++, true );     // prints char to UART
      }

      ctx->gpr[ 0 ] = n;                     // returns length of string

      break;
    }

     //________________________________________________________________________
    ///////////////////////////////// UNKNOWN /////////////////////////////////

    // SVC unknown / unsupported
    default: break;

     //________________________________________________________________________
    ///////////////////////////////////////////////////////////////////////////

  }

  return;
}
