#ifndef XEX_H
#define XEX_H

#include "common.h"

#pragma pack(1) //Byte alignment
typedef struct 
{
	/*
	bit 0 - Title Module
	bit 1 - Exports To Title
	bit 2 - System Debugger
	bit 3 - DLL Module
	bit 4 - Module Patch
	bit 5 - Patch Full
	bit 6 - Patch Delta
	bit 7 - User Mode
	*/
	uchar magic[4];
	uint flags; //Bitfield
	uint peDataOffset;
	uint reserved;
	uint securityInfoOffset;
	uint optionalHeaderNo;

	void fix()
	{
		endian32(flags);
		endian32(peDataOffset);
		endian32(reserved);
		endian32(securityInfoOffset);
		endian32(optionalHeaderNo);
	}
} XexHeader;

#define OFFSET_MEDIA_ID 0x140

enum XexHeaderId
{
	HEADER = 0xFFFE,
	OPTIONAL_HEADER = 0xFFFF,
	RESOURCE_INFO = 0x2FF,
	BASE_FILE_FORMAT = 0x3FF,
	BASE_REFERENCE = 0x405,
	DELTA_PATCH_DESCRIPTOR = 0x5FF,
	BOUNDING_PATH = 0x80FF,
	DEVICE_ID = 0x8105,
	ORIGINAL_BASE_ADDRESS = 0x10001,
	ENTRY_POINT = 0x10100,
	IMAGE_BASE_ADDRESS = 0x10201,
	IMPORT_LIBRARIES = 0x103FF,
	CHECKSUM_TIMESTAMP = 0x18002,
	ENABLED_FOR_CALLCAP = 0x18102,
	ENABLED_FOR_FASTCAP = 0x18200,
	ORIGINAL_PE_NAME = 0x183FF,
	STATIC_LIBRARIES = 0x200FF,
	TLS_INFO = 0x20104,
	DEFAULT_STACK_SIZE = 0x20200,
	DEFAULT_FILESYSTEM_CACHE_SIZE = 0x20301,
	DEFAULT_HEAP_SIZE = 0x20401,
	PAGE_HEAP_SIZE_AND_FLAGS = 0x28002,
	SYSTEM_FLAGS = 0x30000,
	EXECUTION_ID = 0x40006,
	SERVICE_ID_LIST = 0x401FF,
	TITLE_WORKSPACE_SIZE = 0x40201,
	GAME_RATINGS = 0x40310,
	LAN_KEY = 0x40404,
	XBOX_360_LOGO = 0x405FF,
	MULTIDISC_MEDIA_IDS = 0x406FF,
	ALTERNATE_TITLE_IDS = 0x407FF,
	ADDITIONAL_TITLE_MEMORY = 0x40801,
	EXPORTS_BY_NAME_ = 0xE10402
};

typedef struct
{
	/*
	If ID & 0xFF == 0x01 then the Header Data field is used to store the headers data, otherwise it's used to store the data's offset.
	if ID & 0xFF == 0xFF then the Header's data will contain its size
	if ID & 0xFF == (Anything else) the value of this is the size of the entry in number of DWORDS (times by 4 to get real size)
	*/
	XexHeaderId headerId;
	uint headerData;

	void fix()
	{
		uint tmp = (uint)headerId; endian32(tmp); headerId = (XexHeaderId)tmp;
		endian32(headerData);
	}

	uint getRealSize()
	{
		return ((headerId & 0xFF)*4);
	}
} XexOptionalHeader;

// Here comes the OptionalHeader structs to just overlay
typedef struct  
{
	uchar mediaId[4];
	uint version;
	uint baseVersion;
	uchar titleId[4];
	uchar platform;
	uchar executionType;
	uchar discNumber;
	uchar discCount;

	void fix()
	{
		endian32(version);
		endian32(baseVersion);
	}
} ExecutionId;
#pragma pack()

class Xex
{
public:
	Xex(XboxFileInfo *xex, void *buffer, uint offset);
	~Xex();

	ExecutionId *executionId;
	uchar mediaId[16];

	string getTitleId();
	string getFullId();
private:
	//Classes
	XboxFileInfo *_xex;
	void *_xexBuffer;
	void *_buffer;

	//Data
	XexHeader *_header;
	map<XexHeaderId, XexOptionalHeader*> _optionalHeaders;
	map<XexHeaderId, uchar*> _allocs;

	//Functions
	void parseHeaders();
	uchar *readArea(XexHeaderId type, uint size, uint address);
};
#endif