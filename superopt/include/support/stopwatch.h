#ifndef STOPWATCH_H
#define STOPWATCH_H

#define SW_WALLCLOCK	0x1
//#define SW_PROCESSOR	(0x1<<1)
//#define SW_CLOCKTICKS	(0x1<<2)
//#define SW_USER		(0x1<<3)
//#define SW_SYSTEM	(0x1<<4)


/* All time is reported in microseconds, except where specified 
   The fields listed first are calculated before their successors. If you
   want a more accurate reading you should only set the flags that you are
   interested in.
*/
struct time {
  long long num_starts;
  int		running ;
//  int		flags ;
  
  long long	wallClock_start ;
//  long long	processor_start ;
//  long long	clockticks_start ;	/* CLOCK TICKS, not USECS */
//  long long	user_start ;
//  long long	system_start ;
  
  long long	wallClock_elapsed ;
//  long long	processor_elapsed ;
//  long long	clockticks_elapsed ;	/* CLOCK TICKS, not USECS */
//  long long	user_elapsed ;
//  long long	system_elapsed ;
};

/* Reset the TIMER. TIMER should not be running */
void
stopwatch_reset(struct time *timer) ;

/* Set the timer running. The FLAGS field of TIMER should be set to
   reflect the quantities being measured. TIMER should not already
   be running
*/
int
stopwatch_run(struct time *timer) ;

/* Tells the current number of microseconds elapsed since start of stopwatch
   in ELAPSED fields of TIMER. Returns -1 on error. 0 otherwise
*/
int
stopwatch_tell(struct time *timer) ;

/* Tell the stopwatch to stop keeping time */
void
stopwatch_stop(struct time *timer) ;

/* Print the stopwatch time */
void
stopwatch_print(struct time *timer);



#endif
