#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>

// resources
static timer_t timer;

int start_timer(int m_secs)
{
	struct sigevent evp;
	struct itimerspec ts;
	int ret;

	evp.sigev_value.sival_ptr = &timer;
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = SIGUSR1;

	ts.it_interval.tv_sec = 0;
	ts.it_interval.tv_nsec = m_secs*1000000;
	ts.it_value.tv_sec = 0;
	ts.it_value.tv_nsec = m_secs*1000000;
	
	if (-1 == (ret = timer_create(CLOCK_REALTIME, &evp, &timer))) {
		return errno;
	}

	if (-1 == (ret = timer_settime(timer, 0, &ts, NULL))) {
		return errno;
	}
	
	return 0;
}
void stop_timer()
{
	timer_delete(timer);
}
