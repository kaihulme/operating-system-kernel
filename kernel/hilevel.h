/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#ifndef __HILEVEL_H
#define __HILEVEL_H

// Include functionality relating to newlib (the standard C library).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <string.h>

#include "libc.h"

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the   kernel.

#include "lolevel.h"

#define PCB_LENGTH    ( 10 )         // max no. of processes
#define PFDS_LENGTH   ( 1 )          // max no. of pipes
#define P_STACKSIZE   ( 0x00001000 ) // stack size for user processes

// direction
#define OPEN   ( 0 )
#define WRITE  ( 1 )
#define READ  ( -1 )

// type
#define CONSOLE     ( 10 )
#define USER        ( 11 )
#define WAITER      ( 12 )
#define PHILOSOPHER ( 13 )

/* The kernel source code is made simpler and more consistent by using
 * some human-readable type definitions:
 *
 * - a type that captures a Process IDentifier (PID), which is really
 *   just an integer,
 * - an enumerated type that captures the status of a process, e.g.,
 *   whether it is currently executing,
 * - a type that captures each component of an execution context (i.e.,
 *   processor state) in a compatible order wrt. the low-level handler
 *   preservation and restoration prologue and epilogue, and
 * - a type that captures a process PCB.
 */

typedef int     index_t;
typedef int       pid_t;
typedef int      prio_t;
typedef int direction_t;
typedef int   program_t;

typedef enum {
  STATUS_CREATED,
  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING,
  STATUS_TERMINATED
} pcb_status_t;

typedef enum {
  STATUS_OPEN,
  STATUS_WAIT,
  STATUS_WRITE,
  STATUS_READ,
  STATUS_CLOSED
} pipe_status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
       program_t     type;
           pid_t      pid;
    pcb_status_t   status;
           ctx_t      ctx;
        uint32_t      tos;
          prio_t basePrio;
          prio_t     prio;
} pcb_t;

typedef struct {
             int         data;
           pid_t   writer_end;
           pid_t   reader_end;
     direction_t    direction;
   pipe_status_t       status;
} pipe_t;

#endif
