#include "Loader.h"

Loader::Loader()
{
	_thread = new QThread;
	moveToThread(_thread);
	_thread->start();
}

Loader::~Loader()
{
	delete _thread;
}

void Loader::readIsoDir(QString path)
{
	vector<Iso*> *isos = new vector<Iso*>;
	readIsoDirR(path, isos);                   //Read recursive the directories and add all isos to the vector
	emit doProgressTotalIsos(isos->size());    //Send total isos we are going to try to init

	for (vector<Iso*>::iterator it = isos->begin(); it!=isos->end();)
	{
		Iso *iso = *it;
		bool init = iso->Initialize();
		emit doProgressIso((init) ? NULL : iso->getIso());

		//Delete the iso if we wherent able to init it and move the it to next element (by erase)
		if(!init)
		{
			delete iso;
			it = isos->erase(it);
			iso = NULL;
		//Success go to next element
		}else
			++it;
		
	}
	emit doDoneIsoDir(isos);
}

void Loader::readIsoDirR(QString path, vector<Iso*> *isos)
{
	QDir dir(path);
	QStringList filter; filter << "*.iso" << "*.360" << "*.000";        //Filter the dir list
	dir.setNameFilters(filter);
	dir.setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
	QFileInfoList list = dir.entryInfoList();

	for (int i = 0; i < list.size(); ++i)
	{
		const QFileInfo *fileInfo = &list.at(i);
		if(fileInfo->isDir())
		{
			readIsoDirR(fileInfo->absoluteFilePath(), isos);
			continue;
		}
		Iso *iso = new Iso(fileInfo->absoluteFilePath());
		isos->push_back(iso);
	}
}