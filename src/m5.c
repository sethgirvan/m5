/**
 * This file lacks powerful or generic packet creation (as well as any packet
 * parsing)
 *
 * The VRCSR doc makes recommendations about data being little endian, and the
 * m5 example Python code serializes it's data in native byte order, indicating
 * that all data should be little endian byte order, including, eg the
 * synchronization bytes and floating point numbers. Indeed based on the Python
 * code, we can assume that the m5's want floating point numbers in little
 * endian IEEE754 format.
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "m5.h"
#include "io_m5.h"
#include "macrodef.h"
#include "crc32.h"


#define SYNC_REQUEST_0 0xF5U // Per VRCSR doc
#define SYNC_REQUEST_1 0x5FU //
#define RESPONSE_NONE 0x00U /* Per the CSR doc (Per the m5 manual, 0x02 results
							   in the standard thruster response.) */
#define ADDR_CUSTOM_COMMAND 0xF0U // Per CSR doc and m5 manual


// Default per the m5 manual. Our thrusters actually came with this set to
// 0xFF, but we should have set them all to 0x81.
#define THRUSTER_GROUP_ID 0x81U
#define PROPULSION_COMMAND 0xAAU // Per m5 manual

#define PROPULSION_COMMAND_R_ID 0x00U // we don't want a response

// 0xAA + R_ID + NUM_THRUSTERS * (4 byte float)
#define PROPULSION_COMMAND_PACKET_LEN (2 + 4 * NUM_THRUSTERS)

// Manually calculated assuming NUM_THRUSTERS == 8. Update if any part of the
// header changes! (32 bit CRC: ANSI X3.66 per VRCSR doc)
#define PROPULSION_COMMAND_HEADER_CRC ((uint_fast32_t)0xDAE70014)

// Manually calculated assuming R_ID == 0 (without final xor mask)
#define CRC_POST_R_ID ((uint_fast32_t)0xEACEB963)


// triple buffer coordinated with io_m5_tripbuf... functions
static struct
{
	float power[NUM_THRUSTERS]; // Powers range [-1, 1]
} m5[3];


static enum
{
	INIT,
	HEADER,
	R_ID,
	POWER,
	CRC
} state = INIT;

/**
 * Sets *c to the next byte in the current Propulsion Command packet or the
 * first byte of a Propulsion Command packet if there is no current one.
 *
 * returns true when it has set *c to the last byte of the packet, otherwise
 * false.
 */
static bool pack_power(unsigned char *c)
{
	// The VRCSR doc currently says "The packet consists of a 7 byte header, a
	// header checksum, arbitrary length payload data, and a final checksum.",
	// but based on the packet structure diagrams and m5 example Python code,
	// it is clearly in fact a 6 byte header (followed by its 4 byte crc)
	static unsigned char const header[] =
	{
		// Sync bytes order is regardless of endianness. In thruster.py,
		// SYNC_REQUEST is defined in reverse byte order but serialized in
		// native byte order, resulting in big endian.
		SYNC_REQUEST_0,
		SYNC_REQUEST_1,
		THRUSTER_GROUP_ID,
		RESPONSE_NONE,
		ADDR_CUSTOM_COMMAND,
		PROPULSION_COMMAND_PACKET_LEN,
		// Begin header CRC (little endian)
		PROPULSION_COMMAND_HEADER_CRC & 0xFFU,
		(PROPULSION_COMMAND_HEADER_CRC >> 8) & 0xFFU,
		(PROPULSION_COMMAND_HEADER_CRC >> 16) & 0xFFU,
		PROPULSION_COMMAND_HEADER_CRC >> 24
	};

	switch (state)
	{
		case INIT:
		{
			static uint_fast8_t header_idx;
			for (header_idx = 0; header_idx < COUNTOF(header); ++header_idx)
			{
				*c = header[header_idx];

				state = HEADER;
				return false;
		case HEADER:;
			}
		}

		*c = PROPULSION_COMMAND;
		state = R_ID;
		return false;

		case R_ID:
		{
			*c = PROPULSION_COMMAND_R_ID;
		}

		static uint32_t crc = CRC_POST_R_ID;

		{
			// We delay committing to power values as long as possible, so we
			// can transmit the freshest data. However, once we begin
			// transmitting the thrust powers, we won't update again (even
			// between individual motor powers), to avoid using disparate data.
			io_m5_tripbuf_update();

#ifndef IEEE754
#error "The native float must be stored in little endian single precision IEEE754 format. If this is the case, define IEEE754 when compiling"
#endif

#ifndef AVR // avr-libc doesn't seem to support static_assert
			static_assert(sizeof(float) == 4, "float isn't four bytes, so it can't be IEEE754.");
#endif

			// The PROPULSION_COMMAND is structured such that thrust value
			// floats are given for each consecutive motor id, starting from 0.
			static enum thruster t;
			for (t = 0; t < NUM_THRUSTERS; ++t)
			{
				static uint_fast8_t i;
				for (i = 0; i < 4U; ++i)
				{
					// the m5's expect little endian floats
					state = POWER;
					return false;
		case POWER:;

					assert(IN_RANGE(-1.f, m5[io_m5_tripbuf_read()].power[t],
								1.f));
					*c = ((char *)&m5[io_m5_tripbuf_read()].power[t])[i];
					crc = crc32_update(crc, *c);
				}
			}
		}

		{
			crc = crc32_final_mask(crc);
			static uint_fast8_t b;
			for (b = 0; b < 32; b += 8)
			{
				state = CRC;
				return false;
		case CRC:;

				// little endian
				*c = 0xFFU & (crc >> b);
			}
			state = INIT;
			return true;
		}
	}
	assert(0 /* Should never be reached */);
}

void m5_pack_power_reset()
{
	state = INIT;
	return;
}

int m5_power_trans()
{
	assert(io_m5);
	unsigned char c;
	bool finished_pack = pack_power(&c);
	int ret = putc(c, io_m5) == EOF ? EOF : finished_pack;
	fflush(io_m5);
	return ret;
}

void m5_power(enum thruster t, float power)
{
	assert(IN_RANGE(-1.f, power, 1.f));
	m5[io_m5_tripbuf_write()].power[t] = power;
}

void m5_power_offer()
{
	io_m5_tripbuf_offer();
}
