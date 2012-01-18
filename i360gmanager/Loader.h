#ifndef LOADER_H
#define LOADER_H

#include "GameList.h"
#include <QDir>
#include <QThread>

#include "XGDF/Iso.h"
#include "XGDF/Raw.h"

class Loader : public QObject
{
Q_OBJECT

public:
	Loader();
	~Loader();

public slots:
	void readIsoDir(QString path, uint vptr);

signals:
	void doProgressTotalIsos(uint isos);
	void doProgressIso(QString iso);
	void doDoneLoading(vector<Game*> *games, uint vptr);

private:
	void readIsoDir(QString path, vector<Game*> *isos);
	QThread *_thread;
};

#endif