/**
 * See the ahrs io_ahrs_avr.c file for more verbose commenting on dealing with
 * the avr usarts and interrupts
 */

#include <assert.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __STDC_NO_ATOMICS__
#error "stdatomic.h unsupported. If necessary, use of stdatomic can be removed and it can be hacked together with volatile instead."
#endif

#include <stdatomic.h>

#include "io_m5.h"
#include "dbg.h"
#include "macrodef.h"


#define NUSART 2 // usart number
#define DP_PORT A // Port of line drive enable GPIO
#define NDP 0 // Pin number of line drive enable GPIO

// baud: 115,200 per m5 manual
#define BAUD 115200UL // used by util/setbaud.h


// static int (*handler_m5_recv)();

static int (*handler_m5_trans)();


/**
 * Sets the pin to indicate to TTL<->RS-485 converter to stop driving the lines
 * (allowing someone else to transmit).
 *
 * When writing to the USART, the pin should be set, and then unsetting it once
 * there is nothing left to transmit can be left to this interrupt.
 *
 * Unlike the Data Register Empty Interrupt, which triggers when UBRn is ready
 * for a new byte to be written, this interrupt triggers when the last
 * available bit has been pushed out of the USART, ie UBRn and the serial shift
 * register are empty. In fact, the atmega2560 datasheet specifically mentions
 * using this for RS-485.
 */
ISR(CC_XXX(USART, NUSART, _TX_vect)) // Transmit Complete Interrupt
{
	CC_XXX(PORT, DP_PORT, ) &= ~(1U << CC_XXX(P, DP_PORT, NDP));
}

static int uart_m5_putchar(char c, FILE *stream)
{
	(void)stream;
	while (!(CC_XXX(UCSR, NUSART, A) & (1U << CC_XXX(UDRE, NUSART, ))))
	{
		// wait for transmit buffer to be ready
	}

	// Disable the Transmit Complete Interrupt because if it triggered between
	// setting the line drive pin and writing UBRn, the line drive pin would be
	// unset, and we would be trying to transmit but not driving the lines.
	CC_XXX(UCSR, NUSART, B) &= ~(1U << CC_XXX(TXCIE, NUSART, ));

	// Set pin to indicate we're driving the lines. The Transmit Complete
	// Interrupt will unset if transmissions completes.
	CC_XXX(PORT, DP_PORT, ) |= (1U << CC_XXX(P, DP_PORT, NDP));
	CC_XXX(UDR, NUSART, ) = c;

	CC_XXX(UCSR, NUSART, B) |= (1U << CC_XXX(TXCIE, NUSART, )); // Reenable the Transmit Complete Interrupt
	return 0;
}

static int uart_m5_getchar(FILE *stream)
{
	(void)stream;
	while (!(CC_XXX(UCSR, NUSART, A) & (1U << CC_XXX(RXC, NUSART, ))))
	{
		// wait for data to be received
	}
	// UCSRnA needs to be read before UDRn, because reading UDRn changes the
	// read buffer location.
	unsigned char status = CC_XXX(UCSR, NUSART, A);
	// read buffer asap, so it's free to accept new data
	unsigned char data = CC_XXX(UDR, NUSART, );

	assert(!(status & (1U << CC_XXX(UPE, NUSART, ))) /* Parity Error. This
	should never happen, since parity is supposed to be disabled?! */);

	if (status & (1U << CC_XXX(FE, NUSART, ))) // was stop bit incorrect (zero)?
	{
		DEBUG("Frame Error on usart " STRINGIFY_X(NUSART) ". Out-of-sync or break condition may have occured.");
		// Return rather than waiting for the next byte because we don't want
		// to block if reading from a Receive Complete Interrupt.
		return _FDEV_EOF;
	}

	if (status & (1U << CC_XXX(DOR, NUSART, ))) // was there a data overrun?
	{
		// At least one frame was lost due to data being received while the
		// receive buffer was full.
		DEBUG("Receive buffer overrun on usart " STRINGIFY_X(NUSART) ". At least one uart frame lost.");
		// No indication of this is made. The caller is expected to be handling
		// synchronization and error checking above this layer.
	}
	return data;
}


FILE *io_m5 = &(FILE)FDEV_SETUP_STREAM(uart_m5_putchar, uart_m5_getchar,
		_FDEV_SETUP_RW);


/**
 * Assumes uart NUSART and pin NDP on port DP_PORT will be connected to a
 * ttl<->RS-485 converter connected to the m5's RS-485 interface.
 *
 *     Start Bits: 1
 *     Number of Data Bits: 8
 *     Stop Bits: 1
 *     Parity: none
 *     Baud: BAUD
 */
void io_m5_init(char const *path)
{
	(void)path;
	sei(); // enable global interrupts (they may be already enabled anyway)

	// set line drive enable pin to output
	CC_XXX(DDR, DP_PORT, ) |= (1U << CC_XXX(DD, DP_PORT, NDP));
#include <util/setbaud.h> // uses BAUD macro
	CC_XXX(UBRR, NUSART, ) = UBRR_VALUE; // set baud rate register
	// USE_2X is 1 only if necessary to be within BAUD_TOL
	CC_XXX(UCSR, NUSART, A) |= ((uint8_t)USE_2X << CC_XXX(U2X, NUSART, ));

	// register defaults are already 8 data, no parity, 1 stop bit

	// Enable transmitter, receiver and Transmit Complete Interrupt (To stop
	// driving the lines when finished transmitting).
	CC_XXX(UCSR, NUSART, B) = (1U << CC_XXX(TXEN, NUSART, )) |
		(1U << CC_XXX(RXEN, NUSART, )) | (1U << CC_XXX(TXCIE, NUSART, ));
	return;
}

void io_m5_clean()
{
	// TODO: disable transmit and receive
	return;
}

// ISR(CC_XXX(USART, NUSART, _RX_vect)) // Receive Complete Interrupt
// {
// 	assert(handler_m5_recv /* m5 usart receive handler should not be NULL */);
// 
// 	// This function must read from the UDR (eg, with 'fgetc(io_m5)'),
// 	// clearing the RXC flag, otherwise this interrupt will keep triggering
// 	// until the flag is cleared.
// 	handler_m5_recv();
// }
// 
// int io_m5_recv_start(int (*handler)())
// {
// 	handler_m5_recv = handler;
// 
// 	// Try to ensure that memory ops (such as setting handler_m5_recv) are not
// 	// ordered after enabling the interrupt.
// 	atomic_signal_fence(memory_order_acq_rel);
// 
// 	CC_XXX(UCSR, NUSART, B) |= (1U << CC_XXX(RXCIE, NUSART, )); // enable Receive Complete Interrupt
// 	return 0;
// }
// 
// void io_m5_recv_stop()
// {
// 	CC_XXX(UCSR, NUSART, B) &= ~(1U << CC_XXX(RXCIE, NUSART, )); // disable Receive Complete Interrupt
// 	// we won't bother setting handler_m5_recv to NULL
// 	return;
// }


ISR(CC_XXX(USART, NUSART, _UDRE_vect)) // Data Register empty Interrupt
{
	assert(handler_m5_trans /* m5 usart data register empty handler should not
							   be NULL */);

		// This function must write to the UDR (eg, with 'putc(byte, io_m5)'),
		// clearing the UDRE flag, otherwise the interrupts will keep
		// triggering until the flag is cleared.
		handler_m5_trans();
}

int io_m5_trans_start(int (*handler)())
{
	handler_m5_trans = handler;

	// Try to prevent memory ops ordered after enabling interrupt
	atomic_signal_fence(memory_order_acq_rel);

	// enable USART Data Register Empty Interrupt
	CC_XXX(UCSR, NUSART, B) |= (1U << CC_XXX(UDRIE, NUSART, ));
	return 0;
}

void io_m5_trans_stop()
{
	// disable USART Data Register Empty Interrupt
	CC_XXX(UCSR, NUSART, B) &= ~(1U << CC_XXX(UDRIE, NUSART, ));
	// won't bother setting handler_m5_trans to NULL
	return;
}

// new is initialized to 0, so the reader/consumer can know initially when
// there has been any valid data.
//
// clean and new need to be volatile so changes from ISR are visible and so
// changes outside ISR are correctly ordered relative to disabling and
// enabling the ISR.
//
// write, clean, and read may only hold values in range [0, 2]
static struct
{
	unsigned char write          : 2; // Writer/producer writes to buf[write]
	volatile unsigned char clean : 2; // if (new), indexes the newest data
	unsigned char read           : 2; // Reader/consumer reads from buf[read]
	volatile unsigned char new   : 1; /* true if the writer/producer has
										 written a new complete set of data
										 since the last call of
										 io_m5_tripbuf_update */
} tripbuf = {0, 1, 2, false};

// Must be atomic relative to io_m5_tripbuf_update
static void tripbuf_offer_crit()
{
	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	// Try to ensure buffer writes are ordered correctly
	atomic_signal_fence(memory_order_release);
	unsigned char tmp = tripbuf.write;
	tripbuf.write = tripbuf.clean;
	tripbuf.clean = tmp;
	tripbuf.new = true;
	return;
}

/**
 * May be interrupted by io_m5_tripbuf_update, but must not interrupt it.
 */
void io_m5_tripbuf_offer()
{
	if (CC_XXX(UCSR, NUSART, B) & (1U << CC_XXX(UDRIE, NUSART, )))
	{
		// Disable Data Register Empty Interrupt, because we assume that if it
		// is enabled, io_m5_tripbuf_udpate may be called from the ISR.
		CC_XXX(UCSR, NUSART, B) &= ~(1U << CC_XXX(UDRIE, NUSART, ));
		tripbuf_offer_crit();
		CC_XXX(UCSR, NUSART, B) |= (1U << CC_XXX(UDRIE, NUSART, ));
		return;
	}
	tripbuf_offer_crit();
	return;
}

/**
 * returns whether there has been any new data since last call
 */
bool io_m5_tripbuf_update()
{
	assert(IN_RANGE(0, tripbuf.write, 2) && IN_RANGE(0, tripbuf.clean, 2) &&
			IN_RANGE(0, tripbuf.read, 2) && IN_RANGE (0, tripbuf.new, 1));

	if (tripbuf.new)
	{
		tripbuf.new = false;
		// tripbuf.clean must contain a new set of data, so make that the new
		// read, and make read available for clean.
		unsigned char tmp = tripbuf.read;
		tripbuf.read = tripbuf.clean;
		tripbuf.clean = tmp;
		// try to ensure buffer reads will have proper memory ordering
		atomic_signal_fence(memory_order_acquire);
		return true;
	}
	return false;
}

unsigned char io_m5_tripbuf_write()
{
	return tripbuf.write;
}

unsigned char io_m5_tripbuf_read()
{
	return tripbuf.read;
}
