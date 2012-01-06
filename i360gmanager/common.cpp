#include "common.h"

string toHex(char *bytes, uint size)
{
	string hexed = string(size*2, '0');
	string hex = "0123456789ABCDEF";
	for(int l = 0, r = 0; r < size; l += 2, r++)
	{
		hexed[l] = hex[(bytes[r] >> 4) & 0x0F];
		hexed[l + 1] = hex[bytes[r] & 0x0F];
	} 
	return hexed;
}
