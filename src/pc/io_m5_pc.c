#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#include "io_m5.h"
#include "macrodef.h"
#include "dbg.h"


#define WAIT_MILLI 500L


FILE *io_m5;

static struct
{
	pthread_t thread;
	int (*handler)();
	bool handler_started;
	pthread_cond_t data_avail;
} trans = {.handler_started = false, .data_avail = PTHREAD_COND_INITIALIZER};


void io_m5_init(char const *path)
{
	io_m5 = fopen(path, "r+");
	// TODO: correctly handle termios
	if (!io_m5)
	{
		DEBUG("Failed to open %s", path);
	}
	return;
}

void io_m5_clean()
{
	fclose(io_m5);
	return;
}

static void *m5_trans_thread(void *arg)
{
	(void)arg;
	for (;;)
	{
		if (trans.handler() == EOF && feof(io_m5))
		{
			return NULL;
		}
	}
}

int io_m5_trans_set(int (*handler)())
{
	trans.handler = handler;
	return 0;
}

void io_m5_trans_stop()
{
	pthread_cancel(trans.thread);
	return;
}

static struct
{
	unsigned char write : 2;
	unsigned char clean : 2;
	unsigned char read  : 2;
	unsigned char new   : 1;
	pthread_mutex_t lock;
} tripbuf = {0, 1, 2, false, PTHREAD_MUTEX_INITIALIZER};

bool io_m5_tripbuf_update()
{
	pthread_mutex_lock(&tripbuf.lock);

	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	bool updated;
	if (tripbuf.new)
	{
		tripbuf.new = false;
		unsigned char tmp = tripbuf.read;
		tripbuf.read = tripbuf.clean;
		tripbuf.clean = tmp;
		updated = true;
	}
	else
	{
		updated = false;
	}
	pthread_mutex_unlock(&tripbuf.lock);
	return updated;
}

void io_m5_tripbuf_offer_resume()
{
	pthread_mutex_lock(&tripbuf.lock);

	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	unsigned char tmp = tripbuf.write;
	tripbuf.write = tripbuf.clean;
	tripbuf.clean = tmp;
	tripbuf.new = true;

	// Have a waiting trans.handler start transmitting new data immediately
	pthread_cond_broadcast(&trans.data_avail);
	pthread_mutex_unlock(&tripbuf.lock);

	// Transmission handler gets started on first call
	if (!trans.handler_started)
	{
		trans.handler_started = true;
		pthread_create(&trans.thread, NULL, m5_trans_thread, NULL);
	}
}

unsigned char io_m5_tripbuf_write()
{
	return tripbuf.write;
}

unsigned char io_m5_tripbuf_read()
{
	return tripbuf.read;
}

void io_m5_trans_trywait()
{
	pthread_mutex_lock(&tripbuf.lock);

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	ts.tv_sec += WAIT_MILLI / 1000;
	ts.tv_nsec += WAIT_MILLI * 1000000L;
	// normalize timespec
	if (ts.tv_nsec > 1000000000L)
	{
		ts.tv_nsec -= 1000000000L;
		ts.tv_sec += 1;
	}
	// ignore spurious wakeups
	for (int rc = 0; !tripbuf.new && rc == 0;)
	{
		rc = pthread_cond_timedwait(&trans.data_avail, &tripbuf.lock, &ts);
	}

	pthread_mutex_unlock(&tripbuf.lock);
}
