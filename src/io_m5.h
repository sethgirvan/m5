#ifndef IO_M5_H
#define IO_M5_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

/**
 * io with this is blocking, use io_m5_..._set to handle async io.
 */
extern FILE *io_m5;

void io_m5_init(char const *path);

void io_m5_clean();

/**
 * Sets the function to asyncronously (concurrently on pc) write data to io_m5.
 * It will not be started until io_m5_offer_resume is called.
 */
int io_m5_trans_set(int (*handler)());

/**
 * To be called from the transmission handler (ie io_m5_trans_set argument).
 * Meant to be called after finishing transmission of a packet.
 *
 * If io_m5_tripbuf_offer_resume has not been called since the last call of
 * io_m5_tripbuf_update, then the transmission handler is suspended for ~.5
 * seconds or until io_m5_offer_resume is called, whichever is first. If there
 * is new triple buffer data available, returns immediately.
 *
 * By default, a thruster will automatically set itself to power 0 if it has
 * not received a command for over a second. While this can be disabled, it
 * provides useful safety in case something goes fatally wrong in our software
 * or there are connection problems. We could just transmit continuously (on
 * the avr, never disabling the Data Register Ready Interrupt, but at 115,200
 * baud, that is a lot of processing wasted on the interrupt). While could
 * probably just rely on io_m5_trans_offer_resume being called at least once a
 * second, this times out after .5 seconds (arbitrary margin of safety) just in
 * case.
 */
void io_m5_trans_trywait();

void io_m5_trans_stop();

/**
 * Causes io_m5_tripbuf_read to return index that was most recently
 * 'submitted' by io_m5_tripbuf_offer_resume.
 *
 * This should only be run between complete 'uses' of the data, to avoid using
 * disparate data.
 *
 * Unfortunately, since there is no way to have a lock-free/interruptible
 * triple buffer implementation on the avr, this has to be platform-specific.
 * A lock-free ring buffer would not handle cases when the producer is faster
 * than the consumer well.
 *
 * returns true only if io_m5_tripbuf_offer_resume has been called (ie there is
 * new data) since last call.
 */
bool io_m5_tripbuf_update();

/**
 * Makes the current write index available to io_m5_tripbuf_update, and
 * changes the value returned by io_m5_tripbuf_write.
 *
 * Every value in the write triple buffer (eg buf[io_m5_tripbuf_write()] )
 * should be written before calling this, as unwritten areas have unspecified
 * value.
 *
 * Also resumes the transmission handler if it is paused (so it will start
 * transmitting the new data asap). io_m5_trans_set must be called before this
 * is called.
 */
void io_m5_tripbuf_offer_resume();

/**
 * returns the index of the buffer the data consumer should read from. Only
 * changes if io_m5_tripbuf_update is called and returns true.
 */
unsigned char io_m5_tripbuf_read();

/**
 * returns the index of the buffer the data producer should write to. Only
 * changes when io_m5_tripbuf_offer_resume is called.
 */
unsigned char io_m5_tripbuf_write();

#ifdef __cplusplus
}
#endif

#endif
