#ifndef UIMAIN_H
#define UIMAIN_H

#include <QtGui/QMainWindow>
#include <QDir>

#include "ui_uiMain.h"
#include "IsoList.h"
#include "XGDF/Iso.h"

class Main : public QMainWindow
{
	Q_OBJECT

public:
	Main(QWidget *parent = 0, Qt::WFlags flags = 0);
	~Main();


	Ui::MainClass *getUi(){return &ui;}
	void refreshDir(QString directory, bool keep = false);

	//Data
	signals:

	public slots:
		void slotOnClickList(const QModelIndex &current, const QModelIndex &previous);
		void saveDot();
		void extractIso();
		void setGamePath();
		void checkHashCollision();
		void extractFile();
		void calculateMemory();
		void reportIntline9();

		//Extraction
		void fileExtractedSuccess(QString name);
		void bytesWritten(uint bytes);
		void setTree(QTreeWidgetItem *item);
		void fileExtracted(QString name, uint size);
		void isoExtracted(Iso *iso);

private:
	void readNameDb();
	void walkDot(QString &trace, FileNode *&node);
	void addLog(QString log);

	//Helper functions

	//Helper data
	IsoList *_model;
	Ui::MainClass ui;

	QString _lastDotPath;
	QString _lastIsoPath;
	QString _gamePath;
	QString _filePath;
};
#endif // UIMAIN_H
