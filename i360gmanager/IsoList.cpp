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

void IsoList::clearIsos()
{
	while(!_isos.empty())
	{
		delete _isos.back();
		_isos.pop_back();
	}
	emit layoutChanged();
}

void IsoList::addIso(Iso *iso)
{
	_isos.push_back(iso);
	emit layoutChanged();
}

int IsoList::rowCount(const QModelIndex& parent) const
{
	return _isos.size();
}

int IsoList::columnCount(const QModelIndex& parent) const
{
	return _header.size();
}

vector<Iso*> *IsoList::getIsos()
{
	return &_isos;
}

Iso* IsoList::getIso(int index)
{
	return _isos.at(index);
}

QVariant IsoList::data(const QModelIndex& index, int role) const
{
	switch(role)
	{
		case Qt::DisplayRole:
			return _isos.at(index.row())->getField(index.column());
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