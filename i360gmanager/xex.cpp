#include "xex.h"

Xex::Xex(XboxDisc *disc, XboxFileInfo *xex)
{
	_disc = disc;
	_xex = xex;

	parseHeaders();
}

Xex::~Xex()
{
	map<XexHeaderId, uchar*>::iterator it;
	for(it = _allocs.begin(); it != _allocs.end(); it++)
		delete [] it->second;
}

uchar *Xex::readArea(XexHeaderId type, uint size, uint64 address)
{
	uchar *area = new uchar[size];
	_allocs[type] = area;
	_disc->readData(address, size, area);
	return area;
}

string Xex::getTitleId()
{
	return toHex((char*)executionId->titleId, 4);
}

string Xex::getFullId()
{
	return toHex((char*)mediaId, 16);//.append(getTitleId());
}

void Xex::parseHeaders()
{
	_disc->readData(_disc->getAddress(_xex->sector), sizeof(XexHeader), &_header);
	_header.fix(); //Correct the endianiss
	XexOptionalHeader *optional = (XexOptionalHeader*)readArea(OPTIONAL_HEADER, _header.optionalHeaderNo*sizeof(XexOptionalHeader), _disc->getAddress(_xex->sector)+sizeof(XexHeader));
	_disc->readData(_disc->getAddress(_xex->sector)+_header.securityInfoOffset+OFFSET_MEDIA_ID, sizeof(mediaId), &mediaId);
	
	for(int i = 0; i < _header.optionalHeaderNo; i++)
	{
		XexOptionalHeader *header = &optional[i];
		header->fix();
		_optionalHeaders[header->headerId] = header;
	}

	//Read execution section
	XexOptionalHeader *header = _optionalHeaders[EXECUTION_ID];
	executionId = (ExecutionId*)readArea(EXECUTION_ID, header->getRealSize(), _disc->getAddress(_xex->sector)+header->headerData);
	executionId->fix();
	
}