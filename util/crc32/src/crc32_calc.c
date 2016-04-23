/**
 * Usage: Write data to STDIN, and it will print the CRC32 in decimal and hex
 * of all the data once it gets EOF
 */

#include <stdio.h>

#include "crc32.h"


int main(int argc, char *argv[])
{
	(void)argc, (void)argv;
	uint32_t crc = CRC32_INIT_SEED;
	for (int c; (c = getchar()) != EOF;)
	{
		crc = crc32_update(crc, c);
	}
	printf("\n");
	printf("Without final xor mask: Hex: %08X\n", crc);

	crc = crc32_final_mask(crc);

	printf("Decimal:\t%u\n", crc);
	printf("Hex:\t\t%08X\n", crc);
	return 0;
}
