#ifndef ISOLIST_H
#define ISOLIST_H

#include <QAbstractTableModel>
#include <QtGui>
#include <QSet>

#include <iostream>
#include <sstream>
#include <algorithm>

#include "Xbox/Game.h"

using std::vector;

typedef vector<Game*> GameVector;
typedef map<QString, Game*> GameMap;
typedef GameMap::iterator GameMapIt;
#define forGameMap(v) for (GameMapIt it = v.begin(); it != v.end(); it++)

class GameList : public QAbstractTableModel
{
public:
	GameList();
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void addGame(Game *game);
	void clearGames();
	Game* getGame(int index);
	vector<Game*> *getGames();
	

	//Overloads
	GameMap getMap();
	GameMap getMissingGames(GameList *g);

	QVector<HeaderInfo> headers;

protected:
	vector<Game*> _games;
	GameMap _gameMap;

private:
	

};

#endif