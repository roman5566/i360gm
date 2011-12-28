#include "Iso.h"

Iso::Iso()
{
	_rootSector = NULL;
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

	if(_rootSector != NULL)    //If we allocated resources, free it
		free(_rootSector);

	_rootSector = NULL;        //Set pointer to NULL
	_isoHandle = -1;           //No iso handle on
	_isValidMedia = _isValidXex = NULL;
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
	_isoHandle = _open(_path.toStdString().c_str(), _O_RDONLY);
	return (_isoHandle >= 0);
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
		}
	}
	return (_isValidMedia == 1);
}

/**
 * getFiles returns a list of all files in the rootsector of the disc. (So path depth 1)
 * @param refresh forces to reread all files 
 * return vector of XboxFileInfo pointers
 */
vector<XboxFileInfo*> Iso::getFiles(bool refresh)
{
	if((refresh || _files.size() <= 0) && isValidMedia())
	{
		_files.clear();

		XboxFileInfo *fileInfo = NULL;
		_rootSector = (char*)malloc(_xboxMedia.rootSize*sizeof(char));
		if(_xboxMedia.readRootSector(_isoHandle, _rootSector, _offset))
		{
			for(int i = 0; i < _xboxMedia.rootSize; i += fileInfo->getStructSize())
			{
				fileInfo = fileInfo->load(_rootSector+i);
				_files.push_back(fileInfo);
			}
		}
	}
	return _files;
}

/**
 * getFile returns a pointer to a XboxFileInfo struct for the file with name and length. If there was no such file it returns NULL.
 * @param name contains the file name
 * @param length length of the name
 * @return XboxFileInfo struct containing information for that file
 */
XboxFileInfo* Iso::getFile(char* name, int length)
{
	if(_files.size() <= 0)
		getFiles();

	XboxFileInfo *file;
	for(int i = 0; i < _files.size(); i++)
	{
		file = _files.at(i);
		if(file->isEqual(name, length))
			return file;
	}
	return NULL;
}

/**
 * isDefaultXex check is this iso has a valid default xex (or if we can find the default.xex ;)).
 * @return True is is has a valid default.xex, else false
 */
bool Iso::isDefaultXex()
{
	if(isValidMedia())
	{
		if(_isValidXex == NULL)
		{
			XboxFileInfo *defaultxex = getFile(XEX_FILE, XEX_FILE_SIZE);
			if(defaultxex == NULL)
				return false;                                              //We could not find the file...
			if(defaultxex->isXex(_isoHandle, _offset))
				_isValidXex = 1;
		}
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
		return QString("a");
	case 1:
		return QString("b");
	case 2:
		return isDefaultXex();
	case 3:
		return getIso();
	default:
		return QVariant();
	}
}