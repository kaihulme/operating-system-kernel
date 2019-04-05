#include "parent.h"

extern void main_child();

pfd_t pipe_e    = 0;
int   eater     = 0;
int   pipe_send = 0;

pid_t children[ PFDS_LENGTH ];
pfd_t         pfds_c[ PFDS_LENGTH ];

void main_parent() {

  pid_t     pid    =               get_pid();                      // gets pid of parent
  program_t type_e = set_type( pid, WAITER );

  if ( pid < 0 )          sysPrintString( "\nError: PID fetch error\n" );
  if ( type_e != WAITER ) sysPrintString( "\nError: type error\n" );

  for ( int i=0; i<PFDS_LENGTH; i++ ) {

    pfds_c[ i ]         = pipe_open();                       // creates new pipe
    children[ i ]       =      fork();                       // forks new child process

    // sysPrintString( "\nChild PID "); sysPrintInt(children[i]);
    // sysPrintString( " reads from pipe: " ); sysPrintInt( pfds_c[i] );
    // sysPrintString( "\n" );

    if ( children[ i ] == 0 ) {                                    // if process is child
      exec_child( &main_child, pfds_c[ i ] );               // executes child with pipe
    }

    // sysPrintString( "\nParent PID "); sysPrintInt(pid);
    // sysPrintString( " writes to pipe: " ); sysPrintInt( pfds_c[i] );
    // sysPrintString( "\n" );

    pipe_e = pipe_writer_end( pfds_c[ i ], pid );

    if ( pipe_e != pfds_c[ i ] ) sysPrintString( "\nError: pipe read-end error\n" );

  }

  while( 1 ) {

    for ( int i=0; i<PFDS_LENGTH; i++ ) {

      while ( !pipe_writable( pfds_c[ i ] ) ) { continue; }

      if ( i == eater ) {

        pipe_e = pipe_write( pfds_c[ i ], EAT );

        // sysPrintString( "\nParent says EAT to child: " );
        // sysPrintInt( children[ i ] ); sysPrintString( "\n" );

      }

      else {

        pipe_e = pipe_write( pfds_c[ i ], THINK );

        // sysPrintString( "\nParent says THINK to child: " );
        // sysPrintInt( children[ i ] ); sysPrintString( "\n" );

      }

      if ( pipe_e != pfds_c[ i ] ) sysPrintString( "\nError: pipe write error\n" );

    }

    eater++;
    if ( eater == PFDS_LENGTH ) eater = 0;

  }

  exit( EXIT_SUCCESS );
}
