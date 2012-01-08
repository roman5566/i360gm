#include <Windows.h>
#include "xex.h"

Xex::Xex(XboxFileInfo *xex, void *buffer, uint offset)
{
	_xex = xex;
	_buffer = buffer;
	_xexBuffer = (char*)buffer+offset;

	parseHeaders();
}

Xex::~Xex()
{
	map<XexHeaderId, uchar*>::iterator it;
	for(it = _allocs.begin(); it != _allocs.end(); it++)
		delete [] it->second;

	UnmapViewOfFile(_buffer);
}

uchar *Xex::readArea(XexHeaderId type, uint size, uint address)
{
	uchar *area = new uchar[size];
	_allocs[type] = area;
	memcpy(area, (char*)_xexBuffer+address, size);
	return area;
}

string Xex::getTitleId()
{
	return toHex((char*)executionId->titleId, 4);
}

string Xex::getFullId()
{
	return toHex((char*)mediaId, 16);
}

void Xex::parseHeaders()
{
	_header = (XexHeader*)readArea(HEADER, sizeof(XexHeader), 0);
	_header->fix(); //Correct the endianiss
	XexOptionalHeader *optional = (XexOptionalHeader*)readArea(OPTIONAL_HEADER, _header->optionalHeaderNo*sizeof(XexOptionalHeader), sizeof(XexHeader));
	memcpy(mediaId, (char*)_xexBuffer+_header->securityInfoOffset+OFFSET_MEDIA_ID, sizeof(mediaId));
	
	for(int i = 0; i < _header->optionalHeaderNo; i++)
	{
		XexOptionalHeader *header = &optional[i];
		header->fix();
		_optionalHeaders[header->headerId] = header;
	}

	//Read execution section
	XexOptionalHeader *optionalHeader = _optionalHeaders[EXECUTION_ID];
	executionId = (ExecutionId*)readArea(EXECUTION_ID, optionalHeader->getRealSize(), optionalHeader->headerData);
	executionId->fix();
	
}