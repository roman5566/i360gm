#include <Windows.h>
#include "xbe.h"

Xbe::Xbe(XboxFileInfo *Xbe, void *buffer, uint offset)
{
	_xbe = Xbe;
	_buffer = buffer;
	_xbeBuffer = (char*)buffer+offset;

	memcpy(&_header, (char*)_xbeBuffer, sizeof(XbeHeader));
	memcpy(&_certificate, (char*)_xbeBuffer+_header.getAddress(_header.certifcateAddress), sizeof(XbeHeader));

	//Fix stupid little endian
	endian32(_certificate.titleId);
}

Xbe::~Xbe()
{
	UnmapViewOfFile(_buffer);
}


string Xbe::getTitleId()
{
	return toHex((char*)&_certificate.titleId, 4);
}

wstring Xbe::getName()
{
	return wstring(_certificate.titleName);
}