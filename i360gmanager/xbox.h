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

#pragma pack(1)
typedef struct
{
	uchar magic[MEDIA_MAGIC_BYTE_SIZE];
	uint rootSector;
	uint rootSize;
	
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
	uint sector;
	uint size;
	uchar type;
	uchar lenght;
	//uchar *name;
	unsigned __int64 getAddress(uint video)
	{
		return ((unsigned __int64)sector*SECTOR_SIZE)+video;
	}
} XboxFileInfo;
#pragma pack()

#endif