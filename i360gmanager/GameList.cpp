#include "GameList.h"

GameList::GameList() : QAbstractTableModel()
{
	_header.push_back(QString("Bin"));          //0
	_header.push_back(QString("Hash"));         //1
	_header.push_back(QString("Hash"));         //2
	_header.push_back(QString("Title ID"));		//3
	_header.push_back(QString("Full Name"));	//4
	_header.push_back(QString("Iso"));          //5
}

//Operator overloads
GameMap GameList::getMissingGames(GameList *g)
{
	GameMap diffGames;
	GameVector *games = g->getGames();

	for(int i = 0; i < games->size(); i++)
		diffGames[games->at(i)->getUniqueId()] = games->at(i);

	for(int i = 0; i < _games.size(); i++)
		if(diffGames.count(_games.at(i)->getUniqueId()))
			diffGames.erase(_games.at(i)->getUniqueId());
	
	return diffGames;
}


GameMap GameList::getMap()
{
	return _gameMap;
}

void GameList::clearGames()
{
	_gameMap.clear();
	while(!_games.empty())
	{
		delete _games.back();
		_games.pop_back();
	}
	emit layoutChanged();
}

void GameList::addGame(Game *game)
{
	_gameMap[game->getUniqueId()] = game;
	_games.push_back(game);
	emit layoutChanged();
}

int GameList::rowCount(const QModelIndex& parent) const
{
	return _games.size();
}

int GameList::columnCount(const QModelIndex& parent) const
{
	return _header.size();
}

vector<Game*> *GameList::getGames()
{
	return &_games;
}

Game* GameList::getGame(int index)
{
	try
	{
		return _games.at(index);
	}
	catch(...)
	{
		return NULL;
	}
}

QVariant GameList::data(const QModelIndex& index, int role) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return _games.at(index.row())->getField(index.column());
		break;
		//case Qt::BackgroundRole:
		//	return _isos->at(index.row())->getBackground(index.column());
		//break;
	}
	return QVariant::Invalid;
}

QVariant GameList::headerData(int section, Qt::Orientation orientation, int role) const
{

	if(role == Qt::DisplayRole)
	{
		std::stringstream ss;
		if(orientation == Qt::Horizontal)
		{
			return _header.at(section);
		}
		else if(orientation == Qt::Vertical)
		{
			return NULL;
		}

	}
	return QVariant::Invalid;
}