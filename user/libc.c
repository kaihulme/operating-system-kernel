/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "libc.h"

//////////////////////////////// EXECUTION ////////////////////////////////

void yield() {
  asm volatile( "svc %0     \n" // make system call SYS_YIELD
              :
              : "I" (SYS_YIELD)
              : );

  return;
}

int fork() {
  int r;

  asm volatile( "svc %1     \n" // make system call SYS_FORK
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_FORK)
              : "r0" );

  return r;
}

void exec( const void* x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC), "r" (x)
              : "r0" );

  return;
}

void nice( int pid, int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  pid
                "mov r1, %2 \n" // assign r1 =    x
                "svc %0     \n" // make system call SYS_NICE
              :
              : "I" (SYS_NICE), "r" (pid), "r" (x)
              : "r0", "r1" );

  return;
}

//________________________________________________________________________
/////////////////////////////// TERMINATION ///////////////////////////////

int kill( int pid, int x ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 =  pid
                "mov r1, %3 \n" // assign r1 =    x
                "svc %1     \n" // make system call SYS_KILL
                "mov %0, r0 \n" // assign r0 =    r
              : "=r" (r)
              : "I" (SYS_KILL), "r" (pid), "r" (x)
              : "r0", "r1" );

  return r;
}

void exit( int x ) {
  asm volatile( "mov r0, %1 \n" // assign r0 =  x
                "svc %0     \n" // make system call SYS_EXIT
              :
              : "I" (SYS_EXIT), "r" (x)
              : "r0" );

  return;
}

//________________________________________________________________________
////////////////////////////////// PIPES //////////////////////////////////

void exec_child( const void* x, pfd_t pfd ) {
  asm volatile( "mov r0, %1 \n" // assign r0 = x
                "mov r1, %2 \n" // assign r1 = pfd
                "svc %0     \n" // make system call SYS_EXEC
              :
              : "I" (SYS_EXEC_CHILD), "r" (x), "r" (pfd)
              : "r0", "r1" );
  return;
}

pfd_t pipe_open() {
  pfd_t r;
  asm volatile( "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r= r0
              : "=r" (r)
              : "I" (SYS_PIPE_OPEN)
              : "r0" );
  return r;
}

pfd_t pipe_writer_end( pfd_t pfd, pid_t pid ) {
  pfd_t r;
  asm volatile( "mov r0, %2 \n"
                "mov r1, %3 \n"
                "svc %1     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "I" (SYS_PIPE_WRITER_END), "r" (pfd), "r" (pid)
              : "r0", "r1" );
  return r;
}

pfd_t pipe_write( pfd_t pfd, uint32_t pipe_signal ) {
  pfd_t r;
  asm volatile( "mov r0, %2 \n" // assign r0 = pfd
                "mov r1, %3 \n" // assign r1 = pipe_signal
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n"
              : "=r" (r)
              : "I" (SYS_PIPE_WRITE), "r" (pfd), "r" (pipe_signal)
              : "r0", "r1" );
  return r;
}

bool pipe_writable( pfd_t pfd ) {
  bool r;
  asm volatile( "mov r0, %2 \n" // assign r0 = pfd
                "svc %1     \n" // make system call SYS_PIPE_READABLE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_PIPE_WRITABLE), "r" (pfd)
              : "r0" );
  return r;
}

void wait_for_write( pfd_t pfd ) {
  while ( !pipe_writable( pfd ) ) { continue; }
}

pfd_t pipe_reader_end( pfd_t pfd, pid_t pid ) {
  pfd_t r;
  asm volatile( "mov r0, %2 \n"
                "mov r1, %3 \n"
                "svc %1     \n"
                "mov %0, r0 \n"
              : "=r" (r)
              : "I" (SYS_PIPE_READER_END), "r" (pfd), "r" (pid)
              : "r0", "r1" );
  return r;
}

int pipe_read( pfd_t pfd, uint32_t pipe_signal ) {
  int r;
  asm volatile( "mov r0, %2 \n" // assign r0 = pfd
                "mov r1, %3 \n"
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_PIPE_READ), "r" (pfd), "r" (pipe_signal)
              : "r0", "r1" );
  return r;
}

bool pipe_readable( pfd_t pfd ) {
  bool r;
  asm volatile( "mov r0, %2 \n" // assign r0 = pfd
                "svc %1     \n" // make system call SYS_PIPE_READABLE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_PIPE_READABLE),  "r" (pfd)
              : "r0" );
  return r;
}

void wait_for_read( pfd_t pfd ) {
  while ( !pipe_readable( pfd ) ) { continue; }
}

//________________________________________________________________________
/////////////////////////////////// I/O ///////////////////////////////////

int  atoi( char* x ) {
  char* p = x; bool s = false; int r = 0;

  if     ( *p == '-' ) {
    s =  true; p++;
  }
  else if( *p == '+' ) {
    s = false; p++;
  }

  for( int i = 0; *p != '\x00'; i++, p++ ) {
    r = s ? ( r * 10 ) - ( *p - '0' ) :
            ( r * 10 ) + ( *p - '0' ) ;
  }

  return r;
}

void itoa( char* r, int x ) {
  char* p = r; int t, n;

  if( x < 0 ) {
     p++; t = -x; n = t;
  }
  else {
          t = +x; n = t;
  }

  do {
     p++;                    n /= 10;
  } while( n );

    *p-- = '\x00';

  do {
    *p-- = '0' + ( t % 10 ); t /= 10;
  } while( t );

  if( x < 0 ) {
    *p-- = '-';
  }

  return;
}

void sysPrintString( char* string ) {

  write ( STDOUT_FILENO, string, strlen(string) );

  return;
}

void sysPrintInt( int n ) {

  // write ( STDOUT_FILENO, "\npint\n", 6 )
  char* string;
  itoa( string, n );
  sysPrintString( string );

  return;
}

int write( int fd, const void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_WRITE
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_WRITE), "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

int read( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %2 \n" // assign r0 = fd
                "mov r1, %3 \n" // assign r1 =  x
                "mov r2, %4 \n" // assign r2 =  n
                "svc %1     \n" // make system call SYS_READ
                "mov %0, r0 \n" // assign r  = r0
              : "=r" (r)
              : "I" (SYS_READ),  "r" (fd), "r" (x), "r" (n)
              : "r0", "r1", "r2" );

  return r;
}

//________________________________________________________________________
///////////////////////////////////////////////////////////////////////////
