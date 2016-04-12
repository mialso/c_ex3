#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


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
	//ts.it_value.tv_nsec = 0;

	if (-1 == (ret = timer_create(CLOCK_REALTIME, &evp, &timer))) {
		perror("server_timer: start_timer() create failed");
		return 1;
	}

	if (-1 == (ret = timer_settime(timer, 0, &ts, NULL))) {
		perror("server_timer: start_timer() settime failed");
		return 1;
	}
	return 0;
}
int stop_timer()
{
	if (-1 == timer_delete(timer)) {
		perror("server_timer: stop_timer() delete failed");
		return 1;
	}
	return 0;
}
