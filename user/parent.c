/*
 * TODO: write get_pid sys call
 * TODO: make parent and child generate PIDs
 * TODO: solve dining philosophers problem (16 child processes)
 * TODO: write set_type sys call
 * TODO: update print table to show IPC activity + philosopher states
 * TODO: change code around (generatePID, pipes)
 * TODO: change funciton and variable names (mkfifo?)
 * TODO: add comments
 * TODO: test on lab machine
 */

pid_t get_pid() {
  return 2;
}

#include "parent.h"

extern void main_child();

int pipe_signal = 5;

void main_parent() {

  pid_t pid   =   get_pid();                      // gets pid of parent
  pfd_t pfd   = pipe_open();                      // creates new pipe
  pid_t child =      fork();                      // forks new child process

  if ( child == 0 ) {                             // if process is child
    exec_child( &main_child, pfd );               // executes child with pipe
  }

  pfd_t pipe_back = pipe_writer_end( pfd, pid );
  //set_type( pid, WAITER );

  while( 1 ) {

    wait_for_write( pfd );

    sysPrintString("\nPFD parent: "); sysPrintInt(pfd); sysPrintString("\n");
    sysPrintString( "\nParent says: " ); sysPrintInt( pipe_signal );
    sysPrintString( "\n" );

    pipe_back = pipe_write( pfd, pipe_signal );

    if ( pipe_signal = 100 ) pipe_signal = 5;
    else                     pipe_signal++;

    wait_for_write( pfd );

  }

  exit( EXIT_SUCCESS );
}
