#include <Windows.h>
#include "Iso.h"

Iso::Iso(QString path)
{
	//Make this a threaded object
	_thread = new QThread;
	moveToThread(_thread);
	_thread->start();

	//Default values
	_xex = NULL;
	_xbe = NULL;
	_defaultXex = _defaultXbe = NULL;
	_rootFile = NULL;
	_hash = 0;
	_realHandle = NULL;
	_type = NO;
	_path = path;

	//Some extra things to retrieve
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	_granularity = info.dwAllocationGranularity;

	//IO
	_handle = _open(path.toStdString().c_str(), _O_BINARY | _O_RDONLY);
	_mapHandle = CreateFileMapping(getHandle(), NULL, PAGE_READONLY, 0, 0, NULL);
}

/**
 * Please call this in a thread as it will take a few ms per iso and will cause slight GUI lag. We all know that users are spoiled!
 */
bool Iso::Initialize()
{
	if(!isValidMedia()) //Check if this disc is a good disc
		return false;
	try
	{
		getRootNode();      //Create the full disc file structure
		getHash();
	}
	catch(...)
	{
		return false; //Something was wrong with this iso, fuck you iso... fuck you!
	}
	return true;
}

Iso::~Iso()
{
	//Delete the xex
	if(_xex != NULL)
		delete _xex;

	//We should dealloc the bin tree
	deleteTree();

	//Dealloc all sectors we had to read
	map<uint,SectorData*>::iterator it;
	for(it = _sectors.begin(); it != _sectors.end(); it++)
		delete it->second;

	//Close handles
	CloseHandle(_mapHandle);
	_close(_handle);  //Close the handle to the iso, (this will also close the get_osfhandle)

	//Thread
	_thread->quit();
	_thread->wait();
	delete _thread;
}

/**
 * makeTree walks the complete iso in a binary tree and recreates the complete file structure.
 * @param sector address that points to the data of the sector we searching in
 * @param offset the offset to read the file info from
 * @param node the node to add the info to
 */
void Iso::makeTree(SectorData *sector, uint offset, FileNode *&node)
{
	XboxFileInfo *fileInfo = getFileInfo(sector, offset);
	if(fileInfo == NULL)                                           //Wtf load failed, this iso is corrupt or my code sucks!
		return;

	node = new FileNode(fileInfo);
	uint lOffset = fileInfo->getLOffset();
	uint rOffset = fileInfo->getROffset();
	_fileNo++;

	if(_defaultXex == NULL && _type != XSF)                                        //While we at it, find default.xex
		if(fileInfo->isEqual(XEX_FILE, XEX_FILE_SIZE))
		{
			_defaultXex = fileInfo;
			uint offset = 0;
			void *buffer = getMapOfFile(getAddress(_defaultXex->sector), _defaultXex->size, &offset);
			_xex = new Xex(_defaultXex, buffer, offset);
		}
	if(_defaultXbe == NULL && _type == XSF)
		if(fileInfo->isEqual(XBE_FILE, XBE_FILE_SIZE))
		{
			_defaultXbe = fileInfo;
			uint offset = 0;
			void *buffer = getMapOfFile(getAddress(_defaultXbe->sector), _defaultXbe->size, &offset);
			_xbe = new Xbe(_defaultXbe, buffer, offset);
		}

	if(fileInfo->isDir() && !fileInfo->isEmpty())                  //This is a directory, read the sector and begin
	{
		SectorData *dirSector = getSector(fileInfo->sector, fileInfo->size);
		if(dirSector->isInit())                                    //Sanity check for allocation
			makeTree(dirSector, 0, node->dir);
	}

	if(lOffset > 0)                                                //Read the other files
		makeTree(sector, lOffset, node->left);
	if(rOffset > 0)
		makeTree(sector, rOffset, node->right);
}

/**
 * isXboxMedia compares the magic bytes of the mediaInfo struct to see if this is a valid media
 * @return true if this is an valid Xbox media
 */
bool Iso::isXboxMedia()
{
	return (memcmp(_mediaInfo.magic, MEDIA_MAGIC_BYTE, MEDIA_MAGIC_BYTE_SIZE) == 0);
}

/**
 * isValidMedia tries all different types disc images known to this program and if it was able to find 
 * a pre defined magic byte it will return true
 * @return true if this is a valid xbox media, else false
 */
bool Iso::isValidMedia()
{
	if(_handle < 0)
		return false;                 //Valid to initalize the iso
	if(_type != NO)
		return true;                  //We already read the disc, and saw it was a good one!

	if(!readMagicMedia(XSF))          //Check for Xbox 1 disc
		if(!readMagicMedia(GDFX))     //Check for Xbox 360 disc
			if(!readMagicMedia(XGD3)) //Check for Xbox 360 XGD3 disc
				return false;         //There was no magic bytes at any of the specific offsets
	return true;
}