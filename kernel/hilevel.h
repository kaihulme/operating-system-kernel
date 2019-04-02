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

#include <stdlib.h>

// Include functionality relating to the platform.

#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"

// Include functionality relating to the   kernel.

#include "lolevel.h"
#include     "int.h"

#define PCB_LENGTH  4          // console + user programs (P3,4,5)
#define P_STACKSIZE 0x00001000 // stack size for user processes

#define SYS_YIELD     ( 0x00 )
#define SYS_WRITE     ( 0x01 )
#define SYS_READ      ( 0x02 )
#define SYS_FORK      ( 0x03 )
#define SYS_EXIT      ( 0x04 )
#define SYS_EXEC      ( 0x05 )
#define SYS_KILL      ( 0x06 )
#define SYS_NICE      ( 0x07 )

typedef int pid_t;
typedef int priority_t;

typedef struct process_tree   process_tree_t;
typedef struct priority_list priority_list_t;

typedef enum {
  STATUS_CREATED,
  STATUS_READY,
  STATUS_EXECUTING,
  STATUS_WAITING,
  STATUS_TERMINATED
} status_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef struct {
           pid_t          pid;
        status_t       status;
           ctx_t          ctx;
        uint32_t          tos;
      priority_t basePriority;
      priority_t     priority;
} pcb_t;

struct process_tree {
                pcb_t process;
  struct process_tree  *child;
  struct process_tree   *next;
};

struct priority_list {
        process_tree_t *process;
  struct priority_list    *next;
}

#endif
