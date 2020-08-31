#include "stopwatch.h"
#include <stdio.h>
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <dirent.h>
//#include "support/utils.h"
#include "support/debug.h"
//#define _POSIX_C_SOURCE 199309L

#define TRUE	1
#define FALSE	0

//static struct time start_epoch ;
//static int cur_flags = -1 ;

static long
clock_monotonic_curtime_usecs(void)
{
  /*struct timespec tp;
  int rc;

  rc = clock_gettime(CLOCK_MONOTONIC, &tp);
  ASSERT(rc == 0);
  
  return tp.tv_sec * 1e+6 + tp.tv_nsec/1e+3;*/

  struct timeval tval;

  gettimeofday(&tval, NULL);
  return tval.tv_sec * 1e+6 + tval.tv_usec;
}

int
stopwatch_run (struct time *timer)
{
  //int flags = timer->flags ;

  ASSERT(timer->running == FALSE) ;

  //if (flags & SW_WALLCLOCK)
    //{
      timer->wallClock_start = clock_monotonic_curtime_usecs();

      /*struct timeval tv ;
      if (gettimeofday (&tv, NULL) < 0)
	{
	  timer->flags = -1 ;
	  return -1 ;
	}
      timer->wallClock_start = tv.tv_usec + tv.tv_sec*1e6;*/

/*
      DBG (STOPWATCH, ("%s() %d: after call to gettimeofday,"
		       " timer->wallClock_start = %" PRId64 "\n", __func__, __LINE__,
		       timer->wallClock_start)) ;
*/
//    }
//  else
//    {
//      timer->wallClock_start = -1 ;
//    }
//
//  if (flags & SW_PROCESSOR)
//    {
//      clock_t clockticks = clock() ;
//      timer->processor_start = ceil ((double)clockticks*1e6/CLOCKS_PER_SEC) ;
//    }
//  else
//    {
//      timer->processor_start = -1 ;
//    }
//
//  if (flags & SW_CLOCKTICKS || flags & SW_USER || flags & SW_SYSTEM)
//    {
//      struct tms tms ;
//      clock_t clockticks = times (&tms) ;
//      if (clockticks==(clock_t)-1)
//	{
//	  timer->flags = -1 ;
//	  return -1 ;
//	}
//      if (flags & SW_CLOCKTICKS)
//	{
//	  timer->clockticks_start = clockticks ;
//	}
//      else
//	{
//	  timer->clockticks_start = -1 ;
//	}
//
//      if (flags & SW_USER)
//	{
//	  timer->user_start = tms.tms_utime;
//	}
//      else
//	{
//	  timer->user_start = -1 ;
//	}
//
//      if (flags & SW_SYSTEM)
//	{
//	  timer->system_start = tms.tms_stime;
//	}
//      else
//	{
//	  timer->system_start = -1 ;
//	}
//    }
  timer->running = TRUE;
  timer->num_starts++;
  return 0 ;
}

void
stopwatch_reset(struct time *timer)
{
  timer->num_starts = 0;
  timer->running = FALSE ;

  timer->wallClock_elapsed = 0;
//  timer->processor_elapsed = 0;
//  timer->clockticks_elapsed = 0;
//  timer->user_elapsed = 0;
//  timer->system_elapsed = 0;
}

int stopwatch_tell (struct time *timer)
{

  if (timer->running == FALSE)
    return 0 ;

//  int flags = timer->flags;
//
//  if (flags == (clock_t)-1)
//    {
//      return -1 ;
//    }
 
//  if (flags & SW_WALLCLOCK)
//    {
      timer->wallClock_elapsed += clock_monotonic_curtime_usecs() - timer->wallClock_start ;
      //printf("%s(): wallClock_elapsed = %lld\n", __func__, (long long)timer->wallClock_elapsed);
      //struct timeval tv ;

/*
      if (STOPWATCH)
	{
	  tv.tv_sec = tv.tv_usec = 0 ;
	}
*/

      /*if (gettimeofday (&tv, NULL) < 0)
	{
	  ASSERT(0) ;
	  return -1 ;
	}

      timer->wallClock_elapsed += (tv.tv_usec + tv.tv_sec*1e6)
						 - timer->wallClock_start ;*/
//    }
//
//  if (flags & SW_PROCESSOR)
//    {
//      clock_t clockticks = clock() ;
//
//      timer->processor_elapsed += ceil ((double)clockticks*1e6/CLOCKS_PER_SEC)
//					  	 - timer->processor_start ;
//
//      /* assuming clock_t is unsigned and we are running on a 32 bit
//	 machine */
//      if (timer->processor_elapsed < 0)
//	timer->processor_elapsed += (unsigned int)0xffffffff ;  
//    }
//
//  if (flags & SW_CLOCKTICKS || flags & SW_USER || flags & SW_SYSTEM)
//    {
//      struct tms tms ;
//      clock_t clockticks = times (&tms) ;
//      if (clockticks==(clock_t)-1)
//	{
//	  assert (0) ;
//	  return -1 ;
//	}
//
//      if (flags & SW_CLOCKTICKS)
//	{
//	  timer->clockticks_elapsed += clockticks - timer->clockticks_start ;
//
//	  /* assumes clock_t is unsigned and we are running on a 32 bit
//	     machine */
//	  if (timer->clockticks_elapsed < 0)
//	    timer->clockticks_elapsed += (unsigned int)0xffffffff ;  
//	}
//
//      if (flags & SW_USER)
//	{
//	  timer->user_elapsed += tms.tms_utime - timer->user_start ;
//
//	  /* assuming clock_t is unsigned and we are running on a 32 bit
//	     machine */
//	  if (timer->user_elapsed < 0)
//	    timer->user_elapsed += (unsigned int)0xffffffff ;  
//	}
//
//      if (flags & SW_SYSTEM)
//	{
//	  timer->system_elapsed += tms.tms_stime - timer->system_start ;
//
//	  /* assumes clock_t is unsigned and we are running on a 32 bit
//	     machine */
//	  if (timer->user_elapsed < 0)
//	    timer->user_elapsed += (unsigned int)0xffffffff ;  
//	}
//    }
  return 0 ;
}

void
stopwatch_stop (struct time *timer)
{
  ASSERT(timer->running == TRUE);
  stopwatch_tell (timer);
  timer->running = FALSE;
}

void
stopwatch_print (struct time *timer)
{
  stopwatch_tell (timer);

  double secs;
  secs = (double)timer->wallClock_elapsed/1e6 ;
  printf ("Wall-clock [%lld entries]: %d:%d:%d\n", timer->num_starts,
      (int)(secs/3.6e3),
      ((int)((secs/60))) % 60,
      ((int)secs)%60);
}

/*
int test (int argc, char **argv) {
    int i, num_iter ;
    struct time timer ;
    timer.flags = SW_WALLCLOCK ;

    if (argc!=2) {
	printf ("Usage: %s <numlines>\n",argv[0]) ;
	exit (1) ;
    }

    num_iter = atoi (argv[1]) ;

    stopwatch_reset (&timer) ;
    stopwatch_run (&timer) ;

    for (i=0;i<num_iter ;i++) {
	printf("hello\n") ;
    }

    stopwatch_tell(&timer) ;
    printf ("time taken = %lld. per line = %f\n", timer.wallClock_elapsed,
	    (double)timer.wallClock_elapsed/num_iter) ;
}*/
