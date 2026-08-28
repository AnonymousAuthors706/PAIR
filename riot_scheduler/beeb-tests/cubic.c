/* BEEBS cubic benchmark

   Contributor: James Pallister <james.pallister@bristol.ac.uk>
   From http://www.snippets.org/.

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

/* +++Date last modified: 05-Jul-1997 */

/*
 **  CUBIC.C - Solve a cubic polynomial
 **  public domain by Ross Cottrell
 */

#include "hardware.h"

// #include <math.h>
//////// need to emulate because math.h cannot fit into the pmem
static inline float fabs(float x) {
    return (x < 0.0f) ? -x : x;
}

float sqrt(float x) {
    if (x <= 0.0f) return 0.0f;
    float guess = x * 0.5f;
    for (int i = 0; i < 5; i++) {
        guess = 0.5f * (guess + x / guess);
    }
    return guess;
}

float cos(float x) {
    while (x > 3.14159f)
      x -= 6.28318f;
    while (x < -3.14159f)
      x += 6.28318f;

    float x2 = x * x;
    return 1.0f - x2 * (0.5f - x2 * (1.0f/24.0f - x2 * (1.0f/720.0f)));
}

float acos(float x) {
    float negate = (x < 0);
    x = fabs(x);
    float ret = -0.0187293f;
    ret = ret * x + 0.0742610f;
    ret = ret * x - 0.2121144f;
    ret = ret * x + 1.5707288f;
    ret = ret * sqrt(1.0f - x);
    return negate ? (3.14159265f - ret) : ret;
}

float pow(float base, int exp) {
    float result = 1.0f;
    int e = (exp < 0) ? -exp : exp;
    while (e) {
        if (e & 1) result *= base;
        base *= base;
        e >>= 1;
    }
    return (exp < 0) ? (1.0f / result) : result;
}
//////////

//

// #include "snipmath.h"
// #include "pi.h"
#define PI         3.14f

// #include "sniptype.h"
typedef unsigned char  BYTE;
typedef unsigned long  DWORD;
typedef unsigned short WORD;

void SolveCubic(double a, double b, double c,      /* Cubic.C        */
      double d, int *solutions,
      double *x);

struct int_sqrt {
   unsigned sqrt,
            frac;
};

void SolveCubic(double  a,
      double  b,
      double  c,
      double  d,
      int    *solutions,
      double *x)
{
   long double    a1 = b/a, a2 = c/a, a3 = d/a;
   long double    Q = (a1*a1 - 3.0*a2)/9.0;
   long double R = (2.0*a1*a1*a1 - 9.0*a1*a2 + 27.0*a3)/54.0;
   double    R2_Q3 = R*R - Q*Q*Q;

   double    theta;

   if (R2_Q3 <= 0)
   {
      *solutions = 3;
      theta = acos(R/sqrt(Q*Q*Q));
      x[0] = -2.0*sqrt(Q)*cos(theta/3.0) - a1/3.0;
      x[1] = -2.0*sqrt(Q)*cos((theta+2.0*PI)/3.0) - a1/3.0;
      x[2] = -2.0*sqrt(Q)*cos((theta+4.0*PI)/3.0) - a1/3.0;
   }
   else
   {
      *solutions = 1;
      x[0] = pow(sqrt(R2_Q3)+fabs(R), 1/3.0);
      x[0] += Q/x[0];
      x[0] *= (R < 0.0) ? 1 : -1;
      x[0] -= a1/3.0;
   }
}

void* benchmark(void)
{
   double  a1 = 1.0, b1 = -10.5, c1 = 32.0, d1 = -30.0;
   double  a2 = 1.0, b2 = -4.5, c2 = 17.0, d2 = -30.0;
   double  a3 = 1.0, b3 = -3.5, c3 = 22.0, d3 = -31.0;
   double  a4 = 1.0, b4 = -13.7, c4 = 1.0, d4 = -35.0;
   int     solutions;

   double output[48] = {0};
   double *output_pos = &(output[0]);

   /* solve some cubic functions */
   /* should get 3 solutions: 2, 6 & 2.5   */
   SolveCubic(a1, b1, c1, d1, &solutions, output);
   /* should get 1 solution: 2.5           */
   SolveCubic(a2, b2, c2, d2, &solutions, output);
   SolveCubic(a3, b3, c3, d3, &solutions, output);
   SolveCubic(a4, b4, c4, d4, &solutions, output);
   /* Now solve some random equations */
   for(a1=1;a1<3;a1++) {
      for(b1=10;b1>8;b1--) {
         for(c1=5;c1<6;c1+=0.5) {
            for(d1=-1;d1>-3;d1--) {
               SolveCubic(a1, b1, c1, d1, &solutions, output_pos);
            }
         }
      }
   }

   // return 0;
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

/* vim: set ts=3 sw=3 et: */