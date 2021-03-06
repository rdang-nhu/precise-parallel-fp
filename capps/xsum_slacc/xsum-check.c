/* AUTOMATIC CORRECTNESS CHECKS FOR FUNCTIONS FOR EXACT SUMMATION. */

/* Written by Radford M. Neal, 2015 */

/* Program to check correctness of the small and large superaccumulator
   methods.  May be passed a -e argument to echo details of tests.  
   If -d is given instead, it both echos and prints debug information.
   If -dL# is given, it prints debug information only for test # in
   section L. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xsum.h"
#include "pbinary.h"

#define pow2_16 (1.0 / (1 << 16))
#define pow2_32 (pow2_16 * pow2_16)
#define pow2_64 (pow2_32 * pow2_32)
#define pow2_128 (pow2_64 * pow2_64)
#define pow2_256 (pow2_128 * pow2_128)
#define pow2_512 (pow2_256 * pow2_256)
#define pow2_1024 (pow2_512 * pow2_512)

#define pow2_52 (1.0 / (1 << 22) / (1 << 30))

#define Lnormal (2*((.5/pow2_1024)-(.25/pow2_1024)*pow2_52))
#define Snormal (4*pow2_1024)
#define Ldenorm (4*pow2_1024 - 4*pow2_1024*pow2_52)
#define Sdenorm (4*pow2_1024*pow2_52)

/* Tests with one term.  Answer should be the same as the term. */

xsum_flt one_term[] = {

  1.0,             /* Some unexceptional examples of normal numbers */
  -1.0,
  0.1,
  -0.1,
  3.1,
  -3.1,
  2.3e10,
  -2.3e10,
  3.2e-10,
  -3.2e-10,
  123e123,
  -123e123,
  54.11e-150,
  -54.11e-150,
  2*((.5/pow2_128)-(.25/pow2_128)*pow2_52),  /* Mantissa all 1s */
  -2*((.5/pow2_128)+(.25/pow2_128)*pow2_52),
  Lnormal,                                /* Largest normal number */
  -Lnormal,
  Snormal,                                /* Smallest normal number */
  -Snormal,
  Ldenorm,                                /* Largest denormalized number */
  -Ldenorm,
  Sdenorm,                                /* Smallest denormalized number > 0 */
  -Sdenorm,
  1.23e-309,                              /* Other denormalized numbers */
  -1.23e-309,
  4.57e-314,
  -4.57e-314,
  9.7e-322,
  -9.7e-322,
  Sdenorm/pow2_64/2,
  -Sdenorm/pow2_64/2,

0 };

/* Tests with two terms.  Answer should match ordinary floating point add. */

xsum_flt two_term[] = {

1.0, 2.0,         /* Unexceptional adds of normal numbers */
-1.0, -2.0,
0.1, 12.2,
-0.1, -12.2,
12.1, -11.3,
-12.1, 11.3,
11.3, -12.1,
-11.3, 12.1,
1.234567e14, 9.87654321,
-1.234567e14, -9.87654321,
1.234567e14, -9.87654321,
-1.234567e14, 9.87654321,
3.1e200, 1.7e-100,  /* Smaller term should disappear */
3.1e200, -1.7e-100,
-3.1e200, 1.7e-100,
-3.1e200, -1.7e-100,
1.7e-100, 3.1e200,
1.7e-100, -3.1e200,
-1.7e-100, 3.1e200,
-1.7e-100, -3.1e200,
1, pow2_52,       /* Test rounding */
-1, -pow2_52,
1, pow2_52/2,
-1, -pow2_52/2,
1, pow2_52/2+pow2_52/4096,
-1, -pow2_52/2-pow2_52/4096,
1, pow2_52/2+pow2_52/(1<<30)/(1<<10),
-1, -pow2_52/2-pow2_52/(1<<30)/(1<<10),
1, pow2_52/2-pow2_52/4096,
-1, -pow2_52/2+pow2_52/4096,
1 + pow2_52, pow2_52/2,
1 + pow2_52, pow2_52/2 - pow2_52*pow2_52,
-(1 + pow2_52), -pow2_52/2,
-(1 + pow2_52), -(pow2_52/2 - pow2_52*pow2_52),
Sdenorm, 7.1,              /* Adds with denormalized numbers */
Sdenorm, -7.1,
-Sdenorm, -7.1,
-Sdenorm, 7.1,
7.1, Sdenorm,
-7.1, Sdenorm,
-7.1, -Sdenorm,
7.1, -Sdenorm,
Ldenorm, Sdenorm,
Ldenorm, -Sdenorm,
-Ldenorm, Sdenorm,
-Ldenorm, -Sdenorm,
Sdenorm, Sdenorm,
Sdenorm, -Sdenorm,
-Sdenorm, Sdenorm,
-Sdenorm, -Sdenorm,
Ldenorm, Snormal,
Snormal, Ldenorm,
-Ldenorm, -Snormal,
-Snormal, -Ldenorm,
4.57e-314, 9.7e-322,
-4.57e-314, 9.7e-322,
4.57e-314, -9.7e-322,
-4.57e-314, -9.7e-322,
4.57e-321, 9.7e-322,
-4.57e-321, 9.7e-322,
4.57e-321, -9.7e-322,
-4.57e-321, -9.7e-322,
2.0, -2.0*(1+pow2_52),
Lnormal, Lnormal,              /* Overflow */
-Lnormal, -Lnormal,
Lnormal, Lnormal*pow2_52/2,
-Lnormal, -Lnormal*pow2_52/2,
1.0/0.0, 123,                  /* Infinity / NaN */
-1.0/0.0, 123,
1.0/0.0, -1.0/0.0,
0.0/0.0, 123,
123, 0.0/0.0,
0 };

/* Tests with three terms.  Answers are given here as a fourth number,
   some computed/verified using Rmpfr in check.r. */

xsum_flt three_term[] = {
Lnormal, Sdenorm, -Lnormal, Sdenorm,
-Lnormal, Sdenorm, Lnormal, Sdenorm,
Lnormal, -Sdenorm, -Lnormal, -Sdenorm,
-Lnormal, -Sdenorm, Lnormal, -Sdenorm,
Sdenorm, Snormal, -Sdenorm, Snormal,
-Sdenorm, -Snormal, Sdenorm, -Snormal,
12345.6, Snormal, -12345.6, Snormal,
12345.6, -Snormal, -12345.6, -Snormal,
12345.6, Ldenorm, -12345.6, Ldenorm,
12345.6, -Ldenorm, -12345.6, -Ldenorm,
2.0, -2.0*(1+pow2_52), pow2_52/8, -2*pow2_52+pow2_52/8,
1.0, 2.0, 3.0, 6.0,
12.0, 3.5, 2.0, 17.5,
3423.34e12, -93.431, -3432.1e11, 3080129999999906.5,
432457232.34, 0.3432445, -3433452433, -3000995200.3167553,
0 };

/* Tests with ten terms.  Answers are given here as an eleventh number,
   some computed/verified using Rmpfr in check.r. */

xsum_flt ten_term[] = {
Lnormal, Lnormal, Lnormal, Lnormal, Lnormal, Lnormal, -Lnormal, -Lnormal, -Lnormal, -Lnormal, 1.0/0.0,
-Lnormal, -Lnormal, -Lnormal, -Lnormal, -Lnormal, -Lnormal, Lnormal, Lnormal, Lnormal, Lnormal, -1.0/0.0,
Lnormal, Lnormal, Lnormal, Lnormal, 0.125, 0.125, -Lnormal, -Lnormal, -Lnormal, -Lnormal, 0.25,
2.0*(1+pow2_52), -2.0, -pow2_52, -pow2_52, 0, 0, 0, 0, 0, 0, 0,
1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1111111111e0,
-1e0, -1e1, -1e2, -1e3, -1e4, -1e5, -1e6, -1e7, -1e8, -1e9, -1111111111e0,
1.234e88, -93.3e-23, 994.33, 1334.3, 457.34, -1.234e88, 93.3e-23, -994.33, -1334.3, -457.34, 0,
1., -23., 456., -78910., 1112131415., -161718192021., 22232425262728., -2930313233343536., 373839404142434445., -46474849505152535455., -46103918342424313856.,
2342423.3423, 34234.450, 945543.4, 34345.34343, 1232.343, 0.00004343, 43423.0, -342344.8343, -89544.3435, -34334.3, 2934978.4009734304,
0.9101534, 0.9048397, 0.4036596, 0.1460245, 0.2931254, 0.9647649, 0.1125303, 0.1574193, 0.6522300, 0.7378597, 5.2826068,
428.366070546, 707.3261930632, 103.29267289, 9040.03475821, 36.2121638, 19.307901408, 1.4810709160, 8.077159101, 1218.907244150, 778.068267017, 12341.0735011012,
1.1e-322, 5.3443e-321, -9.343e-320, 3.33e-314, 4.41e-322, -8.8e-318, 3.1e-310, 4.1e-300, -4e-300, 7e-307, 1.0000070031003328e-301,
0 };

#if 1
#define REP1 (1 << 23) /* Repeat factor for second set of one term tests */
#define REP10 (1 << 13) /* Repeat factor for second set of ten term tests */
#else
#define REP1 2 /* Small repetition count maybe sometimes useful for debugging */
#define REP10 2
#endif

int echo, debug_all, debug_letter, debug_number;

int different (double a, double b)
{ 
  return isnan(a) != isnan(b) || !isnan(a) && !isnan(b) && a != b;
}

void small_result (xsum_small_accumulator *sacc, double s, int i)
{
  double r, r2;
  if (xsum_debug) xsum_small_display(sacc);
  r = xsum_small_round (sacc);
  r2 = xsum_small_round (sacc);
  if (xsum_debug) xsum_small_display(sacc);
  if (different(r,r2))
  { printf("%3d small: Different second time %.16le %.16le\n", i, r, r2);
  }
  if (different(r,s))
  { printf("%3d small: Result incorrect %.16le %.16le\n", i, r, s);
    printf("    "); pbinary_double(r); printf("\n");
    printf("    "); pbinary_double(s); printf("\n");
  }
}

void large_result (xsum_large_accumulator *lacc, double s, int i)
{
  double r, r2;
  if (xsum_debug) xsum_large_display(lacc);
  r = xsum_large_round (lacc);
  r2 = xsum_large_round (lacc);
  if (xsum_debug) xsum_large_display(lacc);
  if (different(r,r2))
  { printf("%3d large: Different second time %.16le %.16le\n", i, r, r2);
  }
  if (different(r,s))
  { printf("%3d large: Result incorrect %.16le %.16le\n", i, r, s);
    printf("    "); pbinary_double(r); printf("\n");
    printf("    "); pbinary_double(s); printf("\n");
  }
}

int main (int argc, char **argv)
{
  xsum_small_accumulator sacc;
  xsum_large_accumulator lacc;
  double s;
  int i, j;

  if (argc>2 || argc==2 && strcmp(argv[1],"-e")!=0 
                        && (argv[1][0]!='-' || argv[1][1]!='d'))
  { fprintf(stderr,"Usage: xsum-check [ -e | -d[L#] ]\n");
    exit(1);
  }
 
  echo = argc > 1;
  debug_all = debug_letter = debug_number = 0;
  if (argc > 1 && argv[1][1]=='d')
  { debug_all = argv[1][2]==0;
    if (!debug_all)
    { debug_letter = argv[1][2];
      debug_number = atoi(argv[1]+3);
    }
  }

  /* On an Intel machine, set the 387 FPU to do double rounding, in order
     to get correct IEEE 64-bit floating point.  (Only relevant if SSE2
     instructions not used instead of FPU.)  This will disable higher
     precision for long double, however! */

# ifdef DOUBLE
  { unsigned int mode = 0x27f;
    __asm__ ("fldcw %0" : : "m" (*&mode));
  }
# endif

  printf("\nCORRECTNESS TESTS\n");

  printf("\nA: ZERO TERM TEST\n");

  if (echo) printf(" \n-- TEST 0: \n");
  if (echo) printf("   ANSWER: %.16le\n",0.0);

  xsum_debug = debug_all || debug_letter=='A';

  xsum_small_init (&sacc);
  small_result(&sacc,0,0);

  xsum_large_init (&lacc);
  large_result(&lacc,0,0);

  printf("\nB: ONE TERM TESTS\n");

  for (i = 0; one_term[i] != 0; i += 1)
  { 
    if (echo) printf(" \n-- TEST %d: %.16le\n",i,one_term[i]);
    s = one_term[i];
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='B' && debug_number==i;

    xsum_small_init (&sacc);
    xsum_small_add1 (&sacc, one_term[i]);
    small_result(&sacc,s,i);

    xsum_large_init (&lacc);
    xsum_large_addv (&lacc, one_term+i, 1);
    large_result(&lacc,s,i);
  }

  printf("\nC: ONE TERM TESTS TIMES %d\n",REP1);

  for (i = 0; one_term[i] != 0; i += 1)
  { 
    if (echo) printf(" \n-- TEST %d: %.16le\n",i,one_term[i]);
    s = one_term[i] * REP1;
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='C' && debug_number==i;

    xsum_small_init (&sacc);
    for (j = 0; j < REP1; j++) xsum_small_add1 (&sacc, one_term[i]);
    small_result(&sacc,s,i);

    xsum_large_init (&lacc);
    for (j = 0; j < REP1; j++) xsum_large_addv (&lacc, one_term+i, 1);
    large_result(&lacc,s,i);
  }

  printf("\nD: TWO TERM TESTS\n");

  for (i = 0; two_term[i] != 0; i += 2)
  { 
    if (echo) printf(" \n-- TEST %d: %.16le %.16le\n",
                      i/2,two_term[i],two_term[i+1]);
    s = two_term[i] + two_term[i+1];
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='D' && debug_number==i/2;

    xsum_small_init (&sacc);
    xsum_small_addv (&sacc, two_term+i, 2);
    small_result(&sacc,s,i/2);

    xsum_large_init (&lacc);
    xsum_large_addv (&lacc, two_term+i, 2);
    large_result(&lacc,s,i/2);
  }

  printf("\nE: THREE TERM TESTS\n");

  for (i = 0; three_term[i] != 0; i += 4)
  { 
    if (echo) printf(" \n-- TEST %d: %.16le %.16le %.16le\n",
                      i/4,three_term[i],three_term[i+1],three_term[i+2]);
    s = three_term[i+3];
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='E' && debug_number==i/4;

    xsum_small_init (&sacc);
    xsum_small_addv (&sacc, three_term+i, 3);
    small_result(&sacc,s,i/4);

    xsum_large_init (&lacc);
    xsum_large_addv (&lacc, three_term+i, 3);
    large_result(&lacc,s,i/4);
  }

  printf("\nF: TEN TERM TESTS\n");

  for (i = 0; ten_term[i] != 0; i += 11)
  { 
    if (echo) printf(" \n-- TEST %d\n",i/11);
    s = ten_term[i+10];
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='F' && debug_number==i/11;

    xsum_small_init (&sacc);
    xsum_small_addv (&sacc, ten_term+i, 10);
    small_result(&sacc,s,i/11);

    xsum_large_init (&lacc);
    xsum_large_addv (&lacc, ten_term+i, 10);
    large_result(&lacc,s,i/11);
  }

  printf("\nG: TEN TERM TESTS TIMES %d\n",REP10);

  for (i = 0; ten_term[i] != 0; i += 11)
  { 
    if (echo) printf(" \n-- TEST %d\n",i/11);
    s = ten_term[i+10] * REP10;
    if (echo) printf("   ANSWER: %.16le\n",s);

    xsum_debug = debug_all || debug_letter=='G' && debug_number==i/11;

    xsum_small_init (&sacc);
    for (j = 0; j < REP10; j++) xsum_small_addv (&sacc, ten_term+i, 10);
    small_result(&sacc,s,i/11);

    xsum_large_init (&lacc);
    for (j = 0; j < REP10; j++) xsum_large_addv (&lacc, ten_term+i, 10);
    large_result(&lacc,s,i/11);
  }
  
  printf("\nDONE\n\n");
  
  return 0;
}
