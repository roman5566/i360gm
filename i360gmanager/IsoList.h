#ifndef ISOLIST_H
#define ISOLIST_H

#include <QAbstractTableModel>
#include <QtGui>

#include <iostream>
#include <sstream>

#include "Iso.h"

using std::vector;

class IsoList : public QAbstractTableModel
{
public:
	IsoList();
	int rowCount(const QModelIndex& parent) const;
	int columnCount(const QModelIndex& parent) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void addIso(Iso *iso);
	void clearIsos();
	Iso* getIso(int index);
	vector<Iso*> *getIsos();

protected:
	vector<Iso*> _isos;

private:
	vector<QString> _header;

};

#endif