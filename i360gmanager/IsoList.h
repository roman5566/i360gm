#ifndef ISOLIST_H
#define ISOLIST_H

#include <QAbstractTableModel>
#include <QtGui>

#include <iostream>
#include <sstream>

#include "Xbox/Game.h"

using std::vector;

class IsoList : public QAbstractTableModel
{
public:
	IsoList();
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void addGame(Game *game);
	void clearGames();
	Game* getGame(int index);
	vector<Game*> *getGames();

protected:
	vector<Game*> _games;

private:
	vector<QString> _header;

};

#endif