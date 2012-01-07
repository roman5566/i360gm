#include "common.h"

QString getHumenSize(uint64 size)
{
	QString str;
	double kb = size/1000;
	if(kb < 1000)
		str = str.sprintf("%.0f kB", kb); //Yes we using kilobyte and not kibibyte so divided by 1000
	else if(kb < 1000*1000)
		str = str.sprintf("%.2f MB", kb/1000);
	else
		str = str.sprintf("%.2f GB", kb/(1000*1000));

	return str;
}

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
