#include "Iso.h"

Iso::Iso()
{
	_rootFile = NULL;
	_sectors[0] = NULL;
	cleanupIso();
}

Iso::~Iso()
{
	cleanupIso();
}

/**
 * cleanupIso cleans up all helper data for this class
 */
void Iso::cleanupIso()
{

	if(_isoHandle >= 0)        //If we had a valid iso, close it
		_close(_isoHandle);

	if(_sectors[0] != NULL)    //If we allocated resources, free it
		free(_sectors[0]);

	_sectors[0] = NULL;        //Set pointer to NULL
	_isoHandle = -1;           //No iso handle on
	_isValidMedia = _isValidXex = NULL;
	_defaultXex = NULL;
}

/**
 * getPath returns the current full path of this iso
 * @return QString containing full path
 */
QString Iso::getPath()
{
	return _path;
}

/**
 * setPath sets the path where to find the iso. The argument needs to be absolute or relative path to the iso. Example: C:\test.iso
 * @param path a QString containing the path
 * @return If the file exists returns true, else false
 */
bool Iso::setPath(QString path)
{
	cleanupIso();

	_path = path;
	
	//Got an iso
	_isoHandle = _open(_path.toStdString().c_str(), _O_BINARY | _O_RDONLY);
	return (_isoHandle >= 0 && isValidMedia());
}

/**
 * getIso returns the full name of the iso file (including .iso).
 * @return Full iso name. Example: blablabla.iso
 */
QString Iso::getIso()
{
	return _path.split("/").back();
}

/**
 * isValidMedia returns true when the iso was a valid XDG2 or XDG3 disc.
 * @return True on valid media, else false
 */
bool Iso::isValidMedia()
{
	if(_isoHandle < 0)                                       //No file handle to iso, so just wtf dont call me!
		return false;

	if(_isValidMedia == NULL)
	{
		if(_xboxMedia.isValidMedia(_isoHandle, _offset))
		{
			_isValidMedia = 1;
			if(_xboxMedia.rootSize > MAX_SECTOR)             //It has a freaking big root sector, so truncate it and hope for the best
				_xboxMedia.rootSize = MAX_SECTOR;

			getRootNode();                                   //Do a full index
		}
	}
	return (_isValidMedia == 1);
}


/**
 * getRootNode returns the root element of the binary tree of the file structure
 * @return a pointer to the root element
 */
FileNode* Iso::getRootNode()
{
	if(_rootFile == NULL) //Caching system
	{
		if(_xboxMedia.readRootSector(_isoHandle, _sectors[0], _offset))
		{
			_fileNo = 0;
			startTime();
			makeTree(_sectors[0], 0, _rootFile);
			_binTree = stopTime();
		}
	}

	return _rootFile;
}

/**
 * makeTree walks the complete iso in a binary tree and recreates the complete file structure.
 * @param sector address that points to the data of the sector we searching in
 * @param offset the offset to read the file info from
 * @param node the node to add the info to
 */
void Iso::makeTree(void *sector, uint offset, FileNode *&node)
{
	XboxFileInfo *fileInfo = NULL;
	fileInfo = XboxFileInfo::load((void*)((uint)sector+offset));
	
	if(fileInfo == NULL)                                           //Wtf load failed, this iso is corrupt or my code sucks!
		return;

	node = new FileNode(fileInfo);
	uint lOffset = fileInfo->getLOffset();
	uint rOffset = fileInfo->getROffset();
	_fileNo++;

	if(_defaultXex == NULL)                                        //While we at it, find default.xex
		if(fileInfo->isEqual(XEX_FILE, XEX_FILE_SIZE))
			_defaultXex = fileInfo;

	if((node->file->type & 16) != 0 && fileInfo->size > 0)         //This is a directory, read the sector and begin
	{
		if(_sectors[fileInfo] == NULL)                             //Read the directory sector and jump to that sector
			XboxMedia::readSector(_isoHandle, _sectors[fileInfo], fileInfo->getAddress(_offset), fileInfo->size);

		if(_sectors[fileInfo] != NULL)                             //Sanity check for allocation
			makeTree(_sectors[fileInfo], 0, node->dir);
	}

	if(lOffset > 0)                                                //Read the other files
		makeTree(sector, lOffset, node->left);
	if(rOffset > 0)
		makeTree(sector, rOffset, node->right);
}

/**
 * getFile returns a pointer to a XboxFileInfo struct for the file with name and length. If there was no such file it returns NULL.
 * @param name contains the file name
 * @param length length of the name
 * @return XboxFileInfo struct containing information for that file
 */
XboxFileInfo* Iso::getFile(char* name, int length)
{
	return FileNode::getXboxByName(getRootNode(), name, length);
}

/**
 * isDefaultXex check is this iso has a valid default xex (or if we can find the default.xex ;)).
 * @return True is is has a valid default.xex, else false
 */
bool Iso::isDefaultXex()
{
	if(_isValidXex == NULL)
	{
		if(_defaultXex == NULL)
			return false;                                              //We could not find the file...
		if(_defaultXex->isXex(_isoHandle, _offset))
			_isValidXex = 1;
	}

	return (_isValidXex == 1);
}

/**
 * getField returns data based on colum number, this is used for the ItemWidget of Qt
 * @param column referencing to what colum we want the data
 * @return QVariant containing data for that column
 */
QVariant Iso::getField(int column)
{
	switch(column)
	{
	case 0:
		return _binTree;
	case 1:
		return _fileNo;
	case 2:
		return QVariant();
	case 3:
		return getIso();
	case 4:
		return isDefaultXex();
	default:
		return QVariant();
	}
}