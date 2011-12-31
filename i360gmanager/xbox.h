#ifndef XBOX_H
#define XBOX_H

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SECTOR_SIZE 2048
#define MAX_SECTOR 10240000
#define GAME_SECTOR 32
#define TABLE_TO_ADDRESS 4
#define GLOBAL_LSEEK_OFFSET 0xFD90000ul
#define XGD3_LSEEK_OFFSET 0x2080000ul
#define MEDIA_MAGIC_BYTE "MICROSOFT*XBOX*MEDIA"
#define MEDIA_MAGIC_BYTE_SIZE 20
#define XEX_MAGIC_BYTE 0x32584558 //XEX2
#define XEX_FILE "default.xex"
#define XEX_FILE_SIZE 11
#ifndef MAX_PATH
	#define MAX_PATH 255
#endif

#ifndef uchar
	typedef unsigned char uchar;
	typedef unsigned short ushort;
	typedef unsigned int uint;
	typedef unsigned long ulong;
#endif

#ifndef int64
	typedef __int64 int64;
	typedef unsigned __int64 uint64;
#endif

#ifndef HANDLE
	typedef void *HANDLE;
#endif

#ifndef LODWORD
	#define LODWORD(l)           ((DWORD)(l & 0xffffffff))
	#define HIDWORD(l)           ((DWORD)((l >> 32) & 0xffffffff))
	#define MAKEQWORD(a, b)      (((uint64)(a & 0xffffffff)) | (((uint64)(b & 0xffffffff)) << 32))
#endif

#pragma pack(1)
typedef struct
{
	uchar magic[MEDIA_MAGIC_BYTE_SIZE];
	uint rootSector;
	uint rootSize;
	
	bool isValidMedia(int iso, uint &offset)
	{
		offset = 0;
		readMedia(iso);
		if(!isXboxMedia())
		{
			//Check for XDG2 disc
			offset = GLOBAL_LSEEK_OFFSET;
			readMedia(iso, offset);
			if(!isXboxMedia())
			{
				//Check for XDG3 disc
				offset = XGD3_LSEEK_OFFSET;
				readMedia(iso, offset);
				if(!isXboxMedia())
				{
					return false;
				}
			}
		}
		return true;
	}

	int readMedia(int iso, uint offset = 0)
	{
		_lseeki64(iso, GAME_SECTOR*SECTOR_SIZE+offset, SEEK_SET);
		return _read(iso, (char*)this, sizeof(XboxMedia));
	}

	int readRootSector(int iso, void *&buffer, uint offset = 0)
	{
		return readSector(iso, buffer, getRootAddress(offset), rootSize);
	}

	static int readSector(int iso, void *&buffer, uint64 address, uint size)
	{
		buffer = malloc(size);
		_lseeki64(iso, address, SEEK_SET);
		return _read(iso, buffer, size);
	}

	bool isXboxMedia()
	{
		if(magic == NULL) return false;
		return (memcmp(magic, MEDIA_MAGIC_BYTE, MEDIA_MAGIC_BYTE_SIZE) == 0);
	}

	uint64 getRootAddress(uint video)
	{
		return ((uint64)rootSector*SECTOR_SIZE)+video;
	}

} XboxMedia;

typedef struct XboxFileInfoStruct
{
	ushort ltable;               //Left file offset from beginning of sector *4 to get address
	ushort rtable;				//Same but then right file
	uint sector;
	uint size;
	uchar type;
	uchar length;
	uchar name[MAX_PATH];		//This is nasty! But there is no other way i can think of...


	XboxFileInfoStruct(char* _name, int _length)
	{
		memcpy((char*)name, _name, _length);
		length = _length;
	}

	static XboxFileInfoStruct* load(void *pointer)
	{
		return (XboxFileInfoStruct*)pointer;
	}

	uint getLOffset()
	{
		return (uint)ltable*TABLE_TO_ADDRESS;
	}

	uint getROffset()
	{
		return (uint)rtable*TABLE_TO_ADDRESS;
	}

	uint64 getAddress(uint offset)
	{
		return ((uint64)sector*SECTOR_SIZE)+offset;
	}

	bool isXex(int iso, uint offset)
	{
		uint magic;
		_lseeki64(iso, getAddress(offset), SEEK_SET);
		_read(iso, (void*)&magic, sizeof(uint));

		return (magic == XEX_MAGIC_BYTE);
	}

	uint getStructSize()
	{
		return sizeof(XboxFileInfo)-MAX_PATH+length;
	}

	friend bool operator==(const XboxFileInfoStruct &left, const XboxFileInfoStruct &right)
	{
		if(left.length == right.length)
			return (_strnicmp((char*)left.name, (char*)right.name, left.length) == 0);
		return false;
	}

	bool isEqual(char* _name, int _length)
	{
		if(length == _length)   //Speed up
			return (_strnicmp((char*)name, _name, _length) == 0);
		return false;
	}


} XboxFileInfo;
#pragma pack()

#endif