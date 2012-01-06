#ifndef XBOX_H
#define XBOX_H

#include "common.h"

//Callback system
class XboxDisc
{
public:
	XboxDisc(string path);
	~XboxDisc();

	//Public functions
	//Booleans
	bool isXboxMedia();
	bool isValidMedia();

	//Gets and Sets
	XboxFileInfo *getFileInfo(SectorData *sector, uint offset);
	SectorData *getRootData();
	SectorData *getSector(uint sector, uint size);
	uint getRootSize();
	uint getRootSector();
	uint64 getAddress(uint sector);
	uint64 getRootAddress();
	DiscType getDiscType();
	uint getHash();
	HANDLE getHandle();

	//XboxFileInfo functions
	bool isXex(XboxFileInfo *file);
	void *getMapOfFile(uint64 address, uint size, void *outOffset = NULL);
	int64 saveFile(string &path, XboxFileInfo *file);

	int readData(uint64 address, uint size, void *buffer);
	
	uint hashTime;
private:
	//Disc data
	HANDLE _realHandle;
	HANDLE _mapHandle;
	int _handle;
	uint _granularity;
	uint _hash;

	DiscType _type;
	MediaInfo _mediaInfo;
	map<uint, SectorData*> _sectors;

	

	//Private helper functions
	bool readMagicMedia(DiscType type);
};
#endif