#ifndef XBOX_H
#define XBOX_H

#define SECTOR_SIZE 2048
#define GAME_SECTOR 32
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

#pragma pack(1)
typedef struct
{
	uchar magic[MEDIA_MAGIC_BYTE_SIZE];
	uint rootSector;
	uint rootSize;
	
	void readMedia(int iso, uint offset = 0)
	{
		_lseeki64(iso, GAME_SECTOR*SECTOR_SIZE+offset, SEEK_SET);
		_read(iso, (char*)this, sizeof(XboxMedia));
	}

	bool isValidMedia()
	{
		if(magic == NULL) return false;
		return (memcmp(magic, MEDIA_MAGIC_BYTE, MEDIA_MAGIC_BYTE_SIZE) == 0);
	}

	unsigned __int64 getRootAddress(uint video)
	{
		return ((unsigned __int64)rootSector*SECTOR_SIZE)+video;
	}

} XboxMedia;

typedef struct
{
	short flag1;
	short flag2;
	uint sector;
	uint size;
	uchar type;
	uchar length;
	uchar name[MAX_PATH];										//This is nasty! But there is no other way.....
	unsigned __int64 getAddress(uint offset)
	{
		return ((unsigned __int64)sector*SECTOR_SIZE)+offset;
	}

	uint getStructSize()
	{
		return sizeof(XboxFileInfo)-MAX_PATH+length;
	}

	bool isEqual(char* _name, int _length)
	{
		if(length == _length)   //Speed up
			if(_strnicmp((char*)name, _name, _length) == 0)
				return true;
		return false;
	}

} XboxFileInfo;
#pragma pack()

#endif