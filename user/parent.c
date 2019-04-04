
#include "parent.h"

extern void main_child();

void main_parent() {

  pid_t parent_pid = 2;
  pid_t  child_pid = 3;

  int pipe_signal = 1;

  pipe_t child_pipe = pipe_open();

  child_pid = fork();

  if ( child_pid == 0 ) {
    exec_child( &main_child, child_pipe );
  }

  while( 1 ) {

    pipe_t pipe_back = pipe_write( child_pid, pipe_signal );

    pipe_signal++;

  }

  exit( EXIT_SUCCESS );
}
