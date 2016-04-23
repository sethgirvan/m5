#ifndef CRC32_H
#define CRC32_H

/**
 * Implements the ANSI X3.66 32 bit cyclic reduncy check.
 */

#include <stdint.h>


/**
 * Pass to crc32_update() as arg crc with the first byte of data.
 */
#define CRC32_INIT_SEED ((uint32_t)0xFFFFFFFF)

/**
 * The CRC32 of any value with its little endian CRC32 appended
 */
#define CRC32_LE_RESIDUE ((uint32_t)0x2144DF1C)


/**
 * returns arg crc updated with data.
 */
uint32_t crc32_update(uint32_t crc, uint8_t data);

/**
 * Once crc32_update() has been called for all data, call to apply the final
 * xor mask (of 0xFFFFFFFF).
 */
uint32_t crc32_final_mask(uint32_t crc);


#endif
