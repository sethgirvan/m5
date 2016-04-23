/**
 * The CRC32 is reverse bit order (Most significant bit last), and data bytes
 * are in non-reverse bit order (or reverse relative to the crc).
 */

#include <stdint.h>

#define CRC32_POLY ((uint32_t)0xEDB88320) // reverse polynomial

/**
 * NOTE: An optimization would be to use a lookup-table implementation instead
 * (with the table in flash on the avr).
 */
uint32_t crc32_update(uint32_t crc, uint8_t data)
{
	crc ^= data; // xor in the new data

	for (uint_fast8_t i = 8; i--;)
	{
		if (crc & 1U) // is high bit 1? (reverse bit order)
		{
			crc = (crc >> 1) ^ CRC32_POLY; // note right rather than left shift
		}
		else
		{
			crc >>= 1;
		}
	}

	return crc;
}

uint32_t crc32_final_mask(uint32_t crc)
{
	return 0xFFFFFFFFU ^ crc; // aka ~crc
}
