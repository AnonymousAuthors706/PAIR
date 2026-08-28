/* BEEBS recursion benchmark

   Copyright (C) 2014 Embecosm Limited and University of Bristol

   Contributor James Pallister <james.pallister@bristol.ac.uk>

   This file is part of the Bristol/Embecosm Embedded Benchmark Suite.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>. */

// #include "support.h"

/* This scale factor will be changed to equalise the runtime of the
   benchmarks. */
#define SCALE_FACTOR    (REPEAT_FACTOR >> 0)

/* $Id: recursion.c,v 1.2 2005/04/04 11:34:58 csg Exp $ */



/* Generate an example of recursive code, to see  *

 * how it can be modeled in the scope graph.      */

#include "hardware.h"

/* self-recursion  */

int fib(int i)

{

  if(i==0)

    return 1;

  if(i==1)

    return 1;

  return fib(i-1) + fib(i-2);

}



/* mutual recursion */

int anka(int);



int kalle(int i)

{

  if(i<=0)

    return 0;

  else

    return anka(i-1);

}



int anka(int i)

{

  if(i<=0)

    return 1;

  else

    return kalle(i-1);

}



volatile int In;

static int n;
void initialise_benchmark() {
  n = 10;
}

int verify_benchmark(int r)
{
  int expected = 89;
  if (r != expected)
    return 0;
  return 1;
}

void * benchmark(void)
{
  initialise_benchmark();
  In = fib(n);
  P1OUT = verify_benchmark(In);
}


#include "thread.h"

// Watchdog timer
#define WDTCTL_ 0x0120 /* Watchdog Timer Control */
#define WDTHOLD (0x0080)
#define WDTPW (0x5A00)

char thread_stack[THREAD_STACKSIZE_MEDIUM];

int main(void)
{
   uint32_t* wdt = (uint32_t*)(WDTCTL_);
   *wdt = WDTPW | WDTHOLD;

   P3OUT = 0x50;

   thread_create(thread_stack, sizeof(thread_stack), THREAD_PRIORITY_MAIN - 2,
                THREAD_CREATE_WOUT_YIELD, benchmark, NULL, "beebs");

   thread_yield();

   return 0;
}