#ifdef GIT_WIN32

double clar_timer(void)
{
	/* We need the initial tick count to detect if the tick
	 * count has rolled over. */
	static DWORD initial_tick_count = 0;

	/* GetTickCount returns the number of milliseconds that have
	 * elapsed since the system was started. */
	DWORD count = GetTickCount();

	if(initial_tick_count == 0) {
		initial_tick_count = count;
	} else if (count < initial_tick_count) {
		/* The tick count has rolled over - adjust for it. */
		count = (0xFFFFFFFF - initial_tick_count) + count;
	}

	return (double count / (double 1000));
}

#elif __APPLE__

#include <mach/mach_time.h>

double clar_timer(void)
{
   uint64_t time = mach_absolute_time();
   static double scaling_factor = 0;

   if (scaling_factor == 0) {
       mach_timebase_info_data_t info;
       (void)mach_timebase_info(&info);
       scaling_factor = (double)info.numer / (double)info.denom;
   }

   return (double)time * scaling_factor / 1.0E9;
}

#elif defined(AMIGA)

#include <proto/timer.h>

double clar_timer(void)
{
	struct TimeVal tv;
	ITimer->GetUpTime(&tv);
	return (doubletv.Seconds + (doubletv.Microseconds / 1.0E6));
}

#else

#include <sys/time.h>

double clar_timer(void)
{
	struct timespec tp;

	if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
		return (double tp.tv_sec + (double tp.tv_nsec / 1.0E9));
	} else {
		/* Fall back to using gettimeofday */
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		return (doubletv.tv_sec + (doubletv.tv_usec / 1.0E6));
	}
}

#endif
