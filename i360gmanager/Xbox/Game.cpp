#include "Game.h"

Game::Game(QString path)
{
	//Defaults
	_hash = 0;
	_type = NOTHING;
	_path = path;
	_xex = NULL;
	_xbe = NULL;
	_realHandle = NULL;

	//Give every instance an own thread
	_thread = new QThread;
	moveToThread(_thread);
	_thread->start();

	//Get granularity
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	granularity = info.dwAllocationGranularity;
}

Game::~Game()
{
	if(_xex != NULL) delete _xex;      //Delete the xex (if we have one)
	if(_xbe != NULL) delete _xbe;      //Delete the xbe (if we have one)

	//Close handles
	CloseHandle(_mapHandle);
	_close(_handle);  //Close the handle to the iso, (this will also close the get_osfhandle)

	//Stop this thread and delete it
	_thread->quit();
	_thread->wait();
	delete _thread;
}

QString Game::getUniqueId()
{
	if(_unqiueId.isEmpty())
	{
		if(_xex != NULL)
			_unqiueId = QString::fromStdString(_xex->getFullId());
		else if(_xbe != NULL)
			_unqiueId = QString::fromStdString(_xbe->getTitleId());
	}
	return _unqiueId;
}

QString Game::getTitleId()
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

QString Game::getName()
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

/**
 * getHandle opens up a real handle to the iso, it does its own caching so you can call this many times
 * @return HANDLE to the iso
 */
HANDLE Game::getHandle()
{
	if(_realHandle == NULL)
		_realHandle = (HANDLE)_get_osfhandle(_handle);
	return _realHandle;
}

DiscType Game::getDiscType()
{
	return _type;
}

QString Game::getPath()
{
	return _path;
}