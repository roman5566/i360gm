#include "Iso.h"

Iso::Iso()
{
	_valid = NOTHING;
}

Iso::~Iso()
{
	free(_rootSector);
}
QString Iso::getPath()
{
	return _path;
}

void Iso::setPath(QString path)
{
	_path = path;
	readXex();
}

QString Iso::getIso()
{
	return _path.split("/").back();
}


void Iso::readXex()
{
	XboxMedia xboxMedia;
	uint offset = 0;
	int iso = _open(_path.toStdString().c_str(), _O_RDONLY);

	if (iso < 0)
		return; //Could not open the file


	//Check if this is a xbox 360 media disk
	if(!xboxMedia.isValidMedia(iso, offset))
	{
			_valid = NO_MEDIA;                 //This is not a valid xbox disc
			return;
	}
	
	//We got valid media but its DAMN BIG!
	if(xboxMedia.rootSize > MAX_SECTOR)
		xboxMedia.rootSize = MAX_SECTOR;

	//Read buffer
	_rootSector = (char*)malloc(xboxMedia.rootSize*sizeof(char));
	_lseeki64(iso, xboxMedia.getRootAddress(offset), SEEK_SET);
	_read(iso, _rootSector, xboxMedia.rootSize);

	//Default.xex info
	unsigned __int64 xexAddress = NULL;
	uint xexSize = NULL;

	//Find default.xex
	XboxFileInfo *fileInfo = NULL;
	for(int i = 0; i < xboxMedia.rootSize; i += fileInfo->getStructSize())
	{
		fileInfo = fileInfo->load(_rootSector+i);
		if(fileInfo->isEqual(XEX_FILE, XEX_FILE_SIZE))
		{
			xexAddress = fileInfo->getAddress(offset);
			xexSize = fileInfo->size;
		}
		_files.push_back(fileInfo);
	}

	//Found default.xex entry, try to read it
	if(xexAddress != NULL && xexSize > 24)
	{
		//Read magic bytes
		uint magic;
		_lseeki64(iso, xexAddress, SEEK_SET);
		_read(iso, (char*)&magic, sizeof(uint));

		//Check magic bytes
		if(magic == XEX_MAGIC_BYTE)
		{
			//Seriusly good xex
			_valid = GOOD;
			return;
		}
		else
		{
			_valid = FALSE_XEX;
			return;
		}		
	}
	else
	{
		//Was not able to find the default.xex
		_valid = NO_XEX;
	}

	//Cleanup
	_close(iso);
}

vector<XboxFileInfo*> Iso::getFiles()
{
	return _files;
}

QBrush Iso::getBackground(int column)
{
	/*
	switch(column)
	{
	case 2:
		
	default:
		return QBrush();
	}*/
	QColor color = Qt::white;
	switch(_valid)
	{
		case GOOD:
			color = QColor::fromRgb(124, 197, 118);
		break;
		case NO_MEDIA:
			color = QColor::fromRgb(237, 28, 36);
		break;
		case NO_XEX:
			color = Qt::blue;
		break;
		case BIG_ROOT:
			color = Qt::yellow;
		break;
		case NOTHING:
			color = Qt::darkYellow;
		break;
		case FALSE_XEX:
			color = Qt::darkMagenta;
		break;
	}
	
	return QBrush(color);
}

QVariant Iso::getField(int column)
{
	switch(column)
	{
	case 0:
		return QString("a");
	case 1:
		return QString("b");
	case 2:
		return _valid;
	case 3:
		return getIso();
	default:
		return QVariant();
	}
}