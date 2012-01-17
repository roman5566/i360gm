#include "IsoList.h"

IsoList::IsoList() : QAbstractTableModel()
{
	_header.push_back(QString("Bin"));          //0
	_header.push_back(QString("Hash"));         //1
	_header.push_back(QString("Hash"));         //2
	_header.push_back(QString("Title ID"));		//3
	_header.push_back(QString("Full Name"));	//4
	_header.push_back(QString("Iso"));          //5
}

void IsoList::clearGames()
{
	while(!_games.empty())
	{
		delete _games.back();
		_games.pop_back();
	}
	emit layoutChanged();
}

void IsoList::addGame(Game *game)
{
	_games.push_back(game);
	emit layoutChanged();
}

int IsoList::rowCount(const QModelIndex& parent) const
{
	return _games.size();
}

int IsoList::columnCount(const QModelIndex& parent) const
{
	return _header.size();
}

vector<Game*> *IsoList::getGames()
{
	return &_games;
}

Game* IsoList::getGame(int index)
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

QVariant IsoList::data(const QModelIndex& index, int role) const
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

QVariant IsoList::headerData(int section, Qt::Orientation orientation, int role) const
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