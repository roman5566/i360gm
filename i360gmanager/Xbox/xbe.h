#ifndef XBE_H
#define XBE_H

#include "common.h"

#pragma pack(1) //Byte alignment

typedef struct
{
	uchar magic[4]; //XBEH
	uchar signature[256];
	uint baseAddress;
	uint sizeHeader;
	uint sizeImage;
	uint sizeImageHeader;
	uint timeStamp;
	uint certifcateAddress;
	uint sectionNo;
	uint sectionHeaderAddress;
	uint flags;
	uint entryAddress;
	uint tlsAddress;
	uint peStack;
	uint peHeap;
	uint peBaseAddress;
	uint peSizeImage;
	uint peChecksum;
	uint peTime;
	uint debugPathAddress;
	uint debugFileAddress;
	uint debugUnicodeAddress;
	uint kernelImageThunkAddress;
	uint nonKernelImportDirAddress;
	uint libraryVersion;
	uint libraryVersionAddress;
	uint kernelLibraryVersionAddress;
	uint xapiLibraryVersionAddress;
	uint logoBitmapAddress;
	uint sizeLogoBitmap;

	uint getAddress(uint address)
	{
		return address-baseAddress;
	}
} XbeHeader;

typedef struct
{
	uint size;
	uint timeStamp;
	uint titleId;
	wchar_t titleName[0x25];		//50 bytes big
	uint alternativeTitle[40];
	uint allowedMedia;
	uint gameRegion;
	uint gameRating;
	uint diskNo;
	uint version;
	uchar lanKey[0x10];
	uchar sigKey[0x10];
	uchar altSig[0x100];
} XbeCertificate;
#pragma pack()

class Xbe
{
public:
	Xbe(XboxFileInfo *xbe, void *buffer, uint offset);
	~Xbe();

	string getTitleId();
	wstring getName();
private:
	//Classes
	XboxFileInfo *_xbe;
	void *_xbeBuffer;
	void *_buffer;

	//Information
	XbeHeader _header;
	XbeCertificate _certificate;
};
#endif