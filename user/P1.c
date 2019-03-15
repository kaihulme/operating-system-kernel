/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "P1.h"

void main_P1() {
  while( 1 ) {
    write( STDOUT_FILENO, "P1", 2 ); yield();
  }

  exit( EXIT_SUCCESS );
}
