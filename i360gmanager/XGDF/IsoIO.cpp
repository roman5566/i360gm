#include "iso.h"

/**
 * getHandle opens up a real handle to the iso, it does its own caching so you can call this many times
 * @return HANDLE to the iso
 */
HANDLE Iso::getHandle()
{
	if(_realHandle == NULL)
		_realHandle = (HANDLE)_get_osfhandle(_handle);
	return _realHandle;
}

/**
 * getMapOfFile creates a View of a file starting from address with a size
 * WARNING!!! Because of granularity the address can be off,
 * use outOffset to see how much offset you have and add that to the address.
 * @param address a uint64 address
 * @param size the size to read (this can get changed by this function)
 * @param outOffset a pointer to a uint where we can store the offset in
 * @return a pointer to the beginning of the file map
 */
void *Iso::getMapOfFile(uint64 address, uint size, void *outOffset)
{
	uint offset = address % _granularity;
	address -= offset;
	size += offset; 
	DWORD high = HIDWORD(address);
	DWORD low = LODWORD(address);

	if(outOffset != NULL)
		*(uint*)outOffset = offset;

	return MapViewOfFile(_mapHandle, FILE_MAP_READ, high, low, size);
}

/**
 * getHash will return a murmer3 hash for this disc iso
 * @return uint hash
 */
uint Iso::getHash()
{
	if(_hash != 0)    //Cache
		return _hash;

	//We read from the video offset -64 sectors to +64 sectors, why? Because currently (04-01-12) abgx uses 16 stealth sectors and we allow it to increase a lot more and
	//the game sector starts at video offset + 32 sectors so we read also 32 sectors of the game partition. So as far as i know i have covered my bases. And jeez my grammar sucks so much!
	uint size = 128*SECTOR_SIZE;
	uint address = (_type - (size/2));
	uint offset;
	//Get data pointer
	void *data = getMapOfFile(address, size, &offset);

	//Calculate hash
	_hash = 42;
	long clk = clock();
	MurmurHash3_x86_32(data, (size+offset), 42, &_hash);
	_hashTime = clock()-clk;

	//Cleanup
	UnmapViewOfFile(data);
	return _hash;
}

/**
 * readData reads data directly from the iso
 * TODO: Do we need this function?
 */
int Iso::readData(uint64 address, uint size, void *buffer)
{
	_lseeki64(_handle, address, SEEK_SET);
	return _read(_handle, (char*)buffer, size);
}

/**
 * getSector returns the data for that sector (or reads it if it not yet has been read)
 * @param sector The sector number
 * @param size A uint defining the length of this sector
 * @return SectorData containing a pointer to data and size of the sector
 */
SectorData *Iso::getSector(uint sector, uint size)
{
	SectorData *sectorData = _sectors[sector];
	if(sectorData != NULL)
	{
		if(sectorData->isInit() && sectorData->size == size)
			return sectorData;
	}

	sectorData = new SectorData();

	//Set this sector data
	sectorData->init(size);

	//Read the data
	_lseeki64(_handle, getAddress(sector), SEEK_SET);
	_read(_handle, sectorData->data, sectorData->size);

	//Push it in the cach
	_sectors[sector] = sectorData;
	return sectorData;
}

/**
 * readMagicMedia checks if we can find the MEDIA_MAGIC_BYTE string at a specefic offset defined by DiscType
 * if we found a good one it will set this DiscType to the correct type
 * @param type contains the type of disc (XSF, GDFX, XGD3)
 * @return true if valid disc, else false
 */
bool Iso::readMagicMedia(DiscType type)
{
	_lseeki64(_handle, (GAME_SECTOR*SECTOR_SIZE)+type, SEEK_SET);
	_read(_handle, (char*)&_mediaInfo, sizeof(MediaInfo));
	bool isValid = isXboxMedia();
	if(isValid)
	{
		_type = type;
		if(_mediaInfo.rootSize > MAX_SECTOR)   //It has a freaking big root sector, so truncate it and hope for the best
			_mediaInfo.rootSize = MAX_SECTOR;
	}
	return isValid;
}