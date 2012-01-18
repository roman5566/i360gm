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

void Loader::readIsoDir(QString path, uint vptr)
{
	vector<Game*> *games = new vector<Game*>;
	readIsoDir(path, games);                    //Read recursive the directories and add all isos to the vector
	emit doProgressTotalIsos(games->size());    //Send total isos we are going to try to init

	for (vector<Game*>::iterator it = games->begin(); it!=games->end();)
	{
		Game *game = *it;
		bool init = game->Initialize();
		emit doProgressIso((init) ? NULL : game->getName());

		//Delete the iso if we where not able to init it and move the it to next element (by erase)
		if(!init)
		{
			delete game;
			it = games->erase(it);
		//Success go to next element
		}else
			++it;
		
	}
	emit doDoneLoading(games, vptr);
}

void Loader::readIsoDir(QString path, vector<Game*> *games)
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
			if(Raw::isRaw(fileInfo->absoluteFilePath()))
			{
				Raw *raw = new Raw(fileInfo->absoluteFilePath());
				games->push_back(raw);

			}else
				readIsoDir(fileInfo->absoluteFilePath(), games);
			continue;
		}
		Iso *iso = new Iso(fileInfo->absoluteFilePath());
		games->push_back(iso);
	}
}