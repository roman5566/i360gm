#ifndef COMMON_H
#define COMMON_H

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <map>
#include <time.h>
#include <vector>

#include <QString>
#include <QVariant>

#include "Helpers/MurmurHash3.h"

using std::vector;
using std::map;
using std::string;
using std::wstring;

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
#define XBE_MAGIC_BYTE 0x48454258 //XBEH
#define XBE_FILE "default.xbe"
#define XBE_FILE_SIZE 11
#define MURMUR_SEED 42

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

#if defined(_MSC_VER)
	#define endian8(l) l = _byteswap_ushort(l)
	#define endian32(l) l = _byteswap_ulong(l)
	#define endian64(l) l = _byteswap_uint64(l)
#else
	//Byte swapping TODO

#endif

enum HeaderCode
{
	TIME_BIN,
	TIME_HASH,
	HASH,
	TITLEID,
	NAME,
	PATH,
};
typedef struct HeaderInfoS
{
	bool visbility;
	QString name;
	QString description;
	HeaderCode code;

	HeaderInfoS()
	{
		visbility = false;
	}

	HeaderInfoS(bool v, QString n, QString d, HeaderCode c)
	{
		visbility = v;
		name = n;
		description = d;
		code = c;
	}
} HeaderInfo;
extern QSet<HeaderInfo*> allHeaders;
#define LOAD_ALL_HEADERS allHeaders.insert(new HeaderInfo(false, "Bin", "Binary search tree run time", TIME_BIN)); \
allHeaders.insert(new HeaderInfo(false, "Hash", "hashing algorithm run time", TIME_HASH)); \
allHeaders.insert(new HeaderInfo(true, "Hash", "Hash of the game", HASH)); \
allHeaders.insert(new HeaderInfo(true, "Title ID", "Title ID of the xex/xbe in hex", TITLEID)); \
allHeaders.insert(new HeaderInfo(true, "Name", "Name of this game*", NAME));\
allHeaders.insert(new HeaderInfo(false, "Path", "The path to this game", PATH))

//General functions
string toHex(char *bytes, uint size);
QString getHumenSize(uint64 size);

template <class T> class VPtr
{
public:
	static T* asPtr(QVariant v)
	{
		return  (T *) v.value<void *>();
	}

	static QVariant asQVariant(T* ptr)
	{
		return qVariantFromValue((void *) ptr);
	}

	static T* fromPtr(uint u)
	{
		return (T*)u;
	}

	static uint toPtr(T* t)
	{
		return (uint)t;
	}
};

enum DiscType
{
	NOTHING = -1,
	XSF = 0,
	GDFX = 0xFD90000,
	XGD3 = 0x2080000,
	LIVE,
	GOD
};

#pragma pack(1) //Byte alignment
typedef struct
{
	uchar magic[MEDIA_MAGIC_BYTE_SIZE];
	uint rootSector;
	uint rootSize;
	uint64 creationTime;
} MediaInfo;

typedef struct XboxFileInfoStruct
{
	ushort ltable;               //Left file offset from beginning of sector *4 to get address
	ushort rtable;				//Same but then right file
	uint sector;
	uint size;
	uchar type;
	uchar length;
	uchar name[MAX_PATH];		//This is nasty! But there is no other way i can think of...

	bool isDir()
	{
		return ((type & 16) != 0);
	}

	bool isEmpty()
	{
		return (size == 0);
	}

	uint getLOffset()
	{
		return (uint)ltable*TABLE_TO_ADDRESS;
	}

	uint getROffset()
	{
		return (uint)rtable*TABLE_TO_ADDRESS;
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

typedef struct SectorDataStruct
{
	void *data;
	uint size;
	SectorDataStruct()                 ///!< SectorData constructor (setting data pointer to NULL)
	{
		data = NULL;
	}
	~SectorDataStruct()                ///!< ~SectorData destructor freeing allocated memory
	{
		if(isInit())
		{
			delete [] data;
			this->data = NULL;
		}
	}
	bool init(uint size)               ///!< init initializes this sector allocation memory and stuff
	{
		this->size = size;
		this->data = new char[size];
		return isInit();
	}
	bool isInit()                      ///!< isInit returns if this sector data has allocated memory
	{
		return (data != NULL);
	}
} SectorData;

typedef struct FileNodeStruct  
{
	XboxFileInfo *file;
	FileNodeStruct *left;
	FileNodeStruct *right;
	FileNodeStruct *dir;

	FileNodeStruct(XboxFileInfo *file){this->file = file; dir = left = right = NULL;}
	bool isDir(){return (dir != NULL);}
	bool hasLeft(){return (left != NULL);}
	bool hasRight(){return (right != NULL);}
} FileNode;
#pragma pack()

#endif