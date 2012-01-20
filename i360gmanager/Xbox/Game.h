#ifndef GAME_H
#define GAME_H

#include <Windows.h>

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

#include "common.h"
#include "Xbox/xex.h"
#include "Xbox/xbe.h"

extern map<string, string> fileNameDb;
extern map<QString, QString> xbox1Name;

class Game : public QObject
{
	Q_OBJECT

public:
	Game(QString path);
	~Game();

	//All functions that need implementations
	virtual uint getHash() = 0;

	//Default implementations
	QString getPath();
	HANDLE getHandle();
	DiscType getDiscType();

	QString getName();
	QString getTitleId();
	QString getUniqueId();

	//Public handling
	void setHeaderList(QVector<HeaderInfo> *headerList);
	QVariant getField(int column);

public slots:
	virtual bool Initialize() = 0;

protected:
	uint granularity;
	QVector<HeaderInfo> *_headerList;

	//Game disc info
	QString _path;
	DiscType _type;      //What kind of type game this is
	Xex *_xex;           //The executable for Xbox 360
	Xbe *_xbe;           //The executable for Xbox
	uint _hash;          //murmur hash
	QString _titleId, _name,_unqiueId;

	//Handles
	HANDLE _realHandle;
	HANDLE _mapHandle;
	int _handle;

	//Timings and information
	long s;
	void startTime(){ s = clock();}
	long stopTime(){ return clock()-s;}

private:
	QThread *_thread;

signals:
	void doFileExtractedSuccess(QString name);
	void doBytesWritten(uint bytes);
	void doSetTree(QTreeWidgetItem *item);
	//void doIsoExtracted(Iso *iso);
	void doFileExtracted(QString name, uint size);
};
#endif