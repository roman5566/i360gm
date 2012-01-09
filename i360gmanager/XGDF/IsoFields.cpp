#include "Iso.h"
#include "uiMain.h"
extern Main *mainGui;

map<uint, SectorData*> *Iso::getSectors()
{
	return &_sectors;
}
/**
 * getField returns data based on colum number, this is used for the ItemWidget of Qt
 * @param column referencing to what colum we want the data
 * @return QVariant containing data for that column
 */
QVariant Iso::getField(int column)
{
	QString str;
	switch(column)
	{
	case 0:
		return _binTree;
	case 1:
		return _hashTime;
	case 2:
		str = str.sprintf("0x%08X", getHash());
		return str;
	case 3:
		return getTitleId();
	case 4:
		return getName();
	case 5:
		return getIso();
	default:
		return QVariant();
	}
}

QString Iso::getTitleId()
{
	if(_titleId.isEmpty())
	{
		if(_xex != NULL)
			_titleId = QString::fromStdString(_xex->getTitleId());
		else if(_xbe != NULL)
			_titleId = QString::fromStdString(_xbe->getTitleId());
	}
	return _titleId;
}

QString Iso::getName()
{
	if(_name.isEmpty())
	{
		if(_xex != NULL)
			_name = QString::fromStdString(fileNameDb[_xex->getFullId()]);
		else if(_xbe != NULL)
			_name = QString::fromStdWString(_xbe->getName());
		if(_name.isEmpty())
		{
			mainGui->addLog(tr("Was not able to resolve name of: ")+getShortIso());
			_name = getShortIso(); //Default it to file name
		}
	}
	return _name;
}
/**
 * Get total number of files inside this iso (including dirs and empty files)
 */
uint Iso::getFileNo()
{
	return _fileNo;
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
 * getIso returns the full name of the iso file (including .iso)
 * @return Full iso name. Example: blablabla.iso
 */
QString Iso::getIso()
{
	return _path.split("/").back();
}

/**
 * getShortIso returns the name of the iso file (exluding .iso)
 * @return Iso name, example: cool_game
 */
QString Iso::getShortIso()
{
	QString shortIso = getIso(); shortIso.chop(4);
	return shortIso;
}

/**
 * getRootNode returns the root element of the binary tree of the file structure
 * @return a pointer to the root element
 */
FileNode* Iso::getRootNode()
{
	if(_rootFile == NULL) //Caching system
	{
		SectorData *data = getRootData();
		if(data->isInit())
		{
			_fileNo = 0;
			startTime();
			makeTree(data, 0, _rootFile);
			_binTree = stopTime();
		}
	}
	return _rootFile;
}

/**
 * getDiscType returns the type of disc, see DiscType enum
 * @return DiscType
 */
DiscType Iso::getDiscType()
{
	return _type;
}

/**
 * getRootAddress returns the address to the root of this disc
 * @return uint64 containing the root address
 */
uint64 Iso::getRootAddress()
{
	return getAddress(getRootSector());
}

/**
 * getRootData gets/reads the data of the root sector
 * @return SectorData containing the rootsector
 */
SectorData *Iso::getRootData()
{
	return getSector(getRootSector(), getRootSize());
}

/**
 * getRootSector returns the sector of this discs root
 * @return uint sector number of the root
 */
uint Iso::getRootSector()
{
	return _mediaInfo.rootSector;
}

/**
 * getRootSize returns the size of this root sector
 * @return uint size of the root sector
 */
uint Iso::getRootSize()
{
	return _mediaInfo.rootSize;
}

/**
 * getAddress calculates the pure address from a sector
 * @return uint64 address of the sector
 */
uint64 Iso::getAddress(uint sector)
{
	return ((uint64)sector*SECTOR_SIZE)+_type;
}