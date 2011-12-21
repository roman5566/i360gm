#ifndef PACKET_H
#define PACKET_H

#include <QVariant>
#include <QAbstractListModel>
#include <QPixmap>
#include <QString>
#include <QFile>
#include <QBool>
#include <QBrush>

#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include "xbox.h"

enum IsoCode
{
	BIG_ROOT,
	NOTHING,
	NO_XEX,
	NO_MEDIA,
	GOOD,
	FALSE_XEX
};
class Iso
{
public:
	Iso();
	~Iso();

	//Custom stuff
	QVariant getField(int column);
	QBrush getBackground(int column);
	
	//Set/Get
	QString getPath();
	void setPath(QString path);

	//Fields
	QString getIso();

	//Iso manip
	void readXex();

private:
	//Data
	QString _path;

	IsoCode _valid;

};

#endif