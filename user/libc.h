/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __LIBC_H
#define __LIBC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Define a type that that captures a Process IDentifier (PID).

typedef int  pid_t;
typedef int  pfd_t;

/* The definitions below capture symbolic constants within these classes:
 *
 * 1. system call identifiers (i.e., the constant used by a system call
 *    to specify which action the kernel should take),
 * 2. signal identifiers (as used by the kill system call),
 * 3. status codes for exit,
 * 4. standard file descriptors (e.g., for read and write system calls),
 * 5. platform-specific constants, which may need calibration (wrt. the
 *    underlying hardware QEMU is executed on).
 *
 * They don't *precisely* match the standard C library, but are intended
 * to act as a limited model of similar concepts.
 */

#define SYS_YIELD           ( 0x00 )
#define SYS_WRITE           ( 0x01 )
#define SYS_READ            ( 0x02 )
#define SYS_FORK            ( 0x03 )
#define SYS_EXIT            ( 0x04 )
#define SYS_EXEC            ( 0x05 )
#define SYS_KILL            ( 0x06 )
#define SYS_NICE            ( 0x07 )
#define SYS_EXEC_CHILD      ( 0x08 )
#define SYS_PIPE_OPEN       ( 0x09 )
#define SYS_PIPE_WRITE      ( 0x10 )
#define SYS_PIPE_READ       ( 0x11 )
#define SYS_PIPE_WRITABLE   ( 0x12 )
#define SYS_PIPE_READABLE   ( 0x13 )
#define SYS_PIPE_WRITER_END ( 0x14 )
#define SYS_PIPE_READER_END ( 0x15 )

#define SIG_TERM      ( 0x00 )
#define SIG_QUIT      ( 0x01 )

#define EXIT_SUCCESS  ( 0 )
#define EXIT_FAILURE  ( 1 )

#define  STDIN_FILENO ( 0 )
#define STDOUT_FILENO ( 1 )
#define STDERR_FILENO ( 2 )

//////////////////////////////// EXECUTION ////////////////////////////////

// cooperatively yield control of processor, i.e., invoke the scheduler
extern void yield();
// perform fork, returning 0 iff. child or > 0 iff. parent process
extern int  fork();
// perform exec, i.e., start executing program at address x
extern void exec( const void* x );

// for process identified by pid, set  priority to x
extern void nice( pid_t pid, int x );

 //________________________________________________________________________
/////////////////////////////// TERMINATION ///////////////////////////////

// for process identified by pid, send signal of x
extern int  kill( pid_t pid, int x );
// perform exit, i.e., terminate process with status x
extern void exit(       int   x );

 //________________________________________________________________________
////////////////////////////////// PIPES //////////////////////////////////

// executes child process with pipe
extern void exec_child( const void* x, pfd_t pfd );
// opens new pipe
extern pfd_t pipe_open();

// sets pfd as pipe end point
extern pfd_t pipe_writer_end( pfd_t pfd, pid_t pid );
// writes to pipe
extern pfd_t pipe_write( pfd_t pfd, uint32_t pipe_signal );
// gets if pipe is waiting for writes
extern bool pipe_writable( pfd_t pfd );
// waits for pipe to need write
extern void wait_for_write( pfd_t pfd );

// sets pfd as pipe end point
extern pfd_t pipe_reader_end( pfd_t pfd, pid_t pid );
// reads from pipe
extern int pipe_read( pfd_t pfd, uint32_t pipe_signal );
// gets if pipe is waiting for read
extern bool pipe_readable( pfd_t pfd );
// waits for pipe to need read
extern void wait_for_read( pfd_t pfd );

 //________________________________________________________________________
/////////////////////////////////// I/O ///////////////////////////////////

// convert ASCII string x into integer r
extern int  atoi( char* x        );
// convert integer x into ASCII string r
extern void itoa( char* r, int x );

// print string to UART
extern void sysPrintString( char* string );
// print int to UART
extern void sysPrintInt( int n );

// write n bytes from x to   the file descriptor fd; return bytes written
extern int write( int fd, const void* x, size_t n );
// read  n bytes into x from the file descriptor fd; return bytes read
extern int  read( int fd,       void* x, size_t n );

 //________________________________________________________________________
///////////////////////////////////////////////////////////////////////////

#endif
