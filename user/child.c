
#include "child.h"

void main_child( pipe_t pipe ) {

  pid_t pid = 3;

  int pipe_signal;

  while (1) {

    pipe_signal = pipe_read( pipe );

  }

  exit( EXIT_SUCCESS );
}
