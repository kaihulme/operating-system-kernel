#include "child.h"


pfd_t pipe_er     = 0;
int  pipe_receive = 2;

void main_child( pfd_t pfd ) {

  pid_t     pid     =                    get_pid();
  pfd_t     pipe_er =  pipe_reader_end( pfd, pid );
  program_t type_e  = set_type( pid, PHILOSOPHER );

  sysPrintString( "\nChild PID "); sysPrintInt(pid);
  sysPrintString( " has pipe: " ); sysPrintInt( pfd );
  sysPrintString( "\n" );

  if ( pipe_er != pfd )         sysPrintString( "\nError: pipe read-end error\n" );
  if ( type_e  != PHILOSOPHER )          sysPrintString( "\nError: type error\n" );

  while (1) {

    while ( !pipe_readable( pfd ) ) { continue; }

    pipe_receive = pipe_read( pfd, pipe_receive );

    if ( pipe_receive > THINK ) {

      set_philo_status( pid, EAT );

      // sysPrintString( "\nChild PID "); sysPrintInt(pid);
      // sysPrintString( " reads: EAT\n" );

    }

    else {

      set_philo_status( pid, THINK );

      // sysPrintString( "\nChild PID "); sysPrintInt(pid);
      // sysPrintString( " reads: THINK\n" );

    }

    if ( pipe_receive < 0 ) sysPrintString( "\nError: pipe read error\n" );

  }

  exit( EXIT_SUCCESS );
}
