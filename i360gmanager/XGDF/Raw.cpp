#include "Raw.h"

#include <QDir>

Raw::Raw(QString path) : Game(path)
{

}


/**
 * getField returns data based on colum number, this is used for the ItemWidget of Qt
 * @param column referencing to what colum we want the data
 * @return QVariant containing data for that column
 */
QVariant Raw::getField(int column)
{
	QString str;
	switch(column)
	{
	case 0:
		return 0;
	case 1:
		return 0;
	case 2:
		str = str.sprintf("0x%08X", getHash());
		return str;
	case 3:
		return getTitleId();
	case 4:
		return getName();
	case 5:
		return QVariant();
	default:
		return QVariant();
	}
}

QString Raw::getTitleId()
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

QString Raw::getName()
{
	if(_name.isEmpty())
	{
		if(_xex != NULL)
			_name = QString::fromStdString(fileNameDb[_xex->getFullId()]);
		else if(_xbe != NULL)
		{
			_name = xbox1Name[getTitleId()];
			if(_name.isEmpty())
			{
				//mainGui->addLog(tr("This xbox 1 game entry to net yet in DB! Update!")+getShortIso());
				_name = QString::fromStdWString(_xbe->getName());
			}
		}
		if(_name.isEmpty())
		{
			//mainGui->addLog(tr("Was not able to resolve name of: ")+getShortIso());
			_name = "Fudge"; //Default it to file name
		}
	}
	return _name;
}

bool Raw::Initialize()
{
	QString filePath = getExecutable();
	if(!filePath.isEmpty())
	{
		_handle = _open(filePath.toStdString().c_str(), _O_BINARY | _O_RDONLY);
		_mapHandle = CreateFileMapping(getHandle(), NULL, PAGE_READONLY, 0, 0, NULL);
		void *buffer = MapViewOfFile(_mapHandle, FILE_MAP_READ, 0, 0, 0);

		//We got the file
		if(filePath.endsWith("xex"))
		{
			_type = GDFX;
			_xex = new Xex(NULL, buffer, 0);
		}
		else if(filePath.endsWith("xbe"))
		{
			_type = XSF;
			_xbe = new Xbe(NULL, buffer, 0);
		}
		return false;

	}
	else
		return false;
	return true;
}

QString Raw::getExecutable()
{
	QDir dir(_path, "default.xex; default.xbe", QDir::IgnoreCase, QDir::Files | QDir::NoDotAndDotDot);
	QStringList list = dir.entryList();

	if(list.size() != 1)
		return NULL;
	return _path+QDir::separator()+list.at(0);
}

bool Raw::isRaw(QString path)
{
	QDir dir(path, "default.xex; default.xbe", QDir::IgnoreCase, QDir::Files | QDir::NoDotAndDotDot);
	QStringList list = dir.entryList();

	return (list.size() == 1); //If there is only 1 entry, that means that it has to be default.xex or default.xbe! So hell yess!
}

uint Raw::getHash()
{
	return 0;
}