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