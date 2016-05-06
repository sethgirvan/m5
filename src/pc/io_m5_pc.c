#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "io_m5.h"
#include "macrodef.h"
#include "dbg.h"


FILE *io_m5;

static pthread_t thread_trans;

static int (*handler_trans)();


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
		if (handler_trans() == EOF && feof(io_m5))
		{
			return NULL;
		}
	}
}

int io_m5_trans_start(int (*handler)())
{
	handler_trans = handler;
	return pthread_create(&thread_trans, NULL, m5_trans_thread, NULL);
}

void io_m5_trans_stop()
{
	pthread_cancel(thread_trans);
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

void io_m5_tripbuf_offer()
{
	pthread_mutex_lock(&tripbuf.lock);

	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	unsigned char tmp = tripbuf.write;
	tripbuf.write = tripbuf.clean;
	tripbuf.clean = tmp;
	tripbuf.new = true;

	pthread_mutex_unlock(&tripbuf.lock);
}

unsigned char io_m5_tripbuf_write()
{
	return tripbuf.write;
}

unsigned char io_m5_tripbuf_read()
{
	return tripbuf.read;
}
