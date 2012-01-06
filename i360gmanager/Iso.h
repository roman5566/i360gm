#ifndef PACKET_H
#define PACKET_H

#include <QVariant>
#include <QAbstractListModel>
#include <QPixmap>
#include <QString>
#include <QFile>
#include <QBool>
#include <QBrush>
#include <QThread>
#include <QTreeWidgetItem>
#include <QMutex>
#include <QSettings>

#include "xbox.h"
#include "xex.h"
#include "FileNode.h"

#include <time.h>

#include <vector>
#include <map>

using std::map;
using std::vector;

//Mutex
extern QMutex mTreeWidget;
extern map<string, string> fileNameDb;

class Iso  : public QObject
{
	Q_OBJECT

public:
	Iso();
	~Iso();

	//Qt data passing
	QVariant getField(int column);
	
	//Set and get functions
	QString getPath();
	QString getIso();
	QString getShortIso();
	uint getFileNo();
	FileNode* getRootNode();

	//Iso functions
	bool isDefaultXex();
	bool isValidMedia();
	XboxFileInfo* getFile(char* name, int length);
	XboxDisc *getDisc();
	
public slots:
	bool setPath(QString path);
	void extractIso(QString output);

signals:
	void doIsoExtracted(Iso *iso);
	void doFileExtracted(QString name, uint size);


private:
	//Helper functions
	void cleanupIso();
	void walkFile(uint offset);
	void makeTree(SectorData *sector, uint offset, FileNode *&node);
	void extractFile(QString output, FileNode *node);

	//Helper data
	QThread *_thread;
	QString _path;
	FileNode *_rootFile;

	//Iso data
	XboxDisc *_disc;
	Xex *defaultXex;
	XboxFileInfo *_defaultXex;
	char _isValidXex;

	//Timing system
	long s;
	void startTime(){ s = clock();}
	long stopTime(){ return clock()-s;}

	long _binTree;
	long _fileNo;
};

extern QSettings *settings;
#endif