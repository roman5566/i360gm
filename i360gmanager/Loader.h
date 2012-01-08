#ifndef LOADER_H
#define LOADER_H

#include "XGDF/Iso.h"
#include <QDir>
#include <QThread>

class Loader : public QObject
{
Q_OBJECT

public:
	Loader();
	~Loader();

public slots:
	void readIsoDir(QString path);

signals:
	void doProgressTotalIsos(uint isos);
	void doProgressIso(QString iso);
	void doDoneIsoDir(vector<Iso*> *isos);

private:
	void readIsoDirR(QString path, vector<Iso*> *isos);
	QThread *_thread;
};

#endif