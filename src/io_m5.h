#ifndef IO_M5_H
#define IO_M5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

/**
 * io with this is blocking, use io_m5_..._start to handle async io.
 */
extern FILE *io_m5;

void io_m5_init(char const *path);

void io_m5_clean();

/**
 * NOTE: Per the m5 manual, a thruster will automatically set itself to power 0
 * after not receiving a command for over a second. Right now, the handler is
 * called as often as new data can be written out, but depending on how quickly
 * desired motor powers are updated, it may be a worthwhile optimization to not
 * send data until either new data is ready or the timeout is nearing. With the
 * m5 baud of 115,200, the handler will be called relatively often.
 */
int io_m5_trans_start(int (*handler)());

void io_m5_trans_stop();

/**
 * Causes io_m5_tripbuf_read to return index that was most recently
 * 'submitted' by io_m5_tripbuf_offer.
 *
 * This should only be run between complete 'uses' of the data, to avoid using
 * disparate data.
 *
 * Unfortunately, since there is no way to have a lock-free/interruptable
 * triple buffer implementation on the avr, this has to be platform-specific.
 * A lock-free ring buffer would not handle cases when the producer is faster
 * than the consumer well.
 */
bool io_m5_tripbuf_update();

/**
 * Makes the current write index available to io_m5_tripbuf_update, and
 * changes the value returned by io_m5_tripbuf_write.
 */
void io_m5_tripbuf_offer();

/**
 * returns the index of the buffer the data consumer should read from. Only
 * changes if io_m5_tripbuf_update is called and returns true.
 */
unsigned char io_m5_tripbuf_read();

/**
 * returns the index of the buffer the data producer should write to. Only
 * changes when io_m5_tripbuf_offer is called.
 */
unsigned char io_m5_tripbuf_write();

#ifdef __cplusplus
}
#endif

#endif
