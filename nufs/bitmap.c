#include <stdint.h>
#include "bitmap.h"

int
bitmap_get(void* bm, int ii)
{
	int which_byte = ii / 8;
	int which_bit  = ii % 8;
	return (*((uint8_t*) bm + which_byte) >> which_bit) & 1;
}

void
bitmap_put(void* bm, int ii, int vv)
{
	int which_byte = ii / 8;
	int which_bit  = ii % 8;

	if (vv == 1) {
		*((uint8_t*) bm + which_byte) |= (vv << which_bit);
	} else if (vv == 0) {
		*((uint8_t*) bm + which_byte) &= ~(1 << which_bit);
	}
	
}

//unclear what the purpose of this is
void
bitmap_print(void* bm, int size)
{
	return;
}
