
#include "child.h"

pid_t get_pid() {
  return 3;
}

int pipe_signal = 2;

void main_child( pfd_t pfd ) {

  pid_t child     =                     get_pid();
  pfd_t pipe_back = pipe_reader_end( pfd, child );
  //set_type( pid, PHILOSOPHER );

  while (1) {

    wait_for_read( pfd );

    pipe_signal = pipe_read( pfd, pipe_signal );

    sysPrintString( "\nChild reads: " ); sysPrintInt( pipe_signal );
    sysPrintString( "\n" );

    wait_for_read( pfd );

  }

  exit( EXIT_SUCCESS );
}
