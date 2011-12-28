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

using std::vector;

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

	//Qt data passing
	QVariant getField(int column);
	
	//Set and get functions
	QString getPath();
	QString getIso();
	bool setPath(QString path);

	//Iso functions
	bool isDefaultXex();
	bool isValidMedia();
	vector<XboxFileInfo*> getFiles(bool refresh = false);
	XboxFileInfo* getFile(char* name, int length);

private:
	//Helper functions
	void cleanupIso();

	//Helper data
	QString _path;
	char* _rootSector;
	vector<XboxFileInfo*> _files;

	//Iso data
	int _isoHandle;
	char _isValidMedia;
	char _isValidXex;
	XboxMedia _xboxMedia;
	uint _offset;
};

#endif