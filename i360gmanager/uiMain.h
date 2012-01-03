#ifndef UIMAIN_H
#define UIMAIN_H

#include <QtGui/QMainWindow>
#include <QDir>

#include "ui_uiMain.h"
#include "Iso.h"
#include "IsoList.h"

using std::vector;

class Main : public QMainWindow
{
	Q_OBJECT

public:
	Main(QWidget *parent = 0, Qt::WFlags flags = 0);
	~Main();


	Ui::MainClass *getUi(){return &ui;}
	void refreshDir(QString directory);

	//Data
	signals:

	public slots:
		void slotOnClickList(const QModelIndex &current, const QModelIndex &previous);
		void saveDot();
		void extractIso();
		void setGamePath();

		//Extraction
		void fileExtracted(QString name, uint size);
		void isoExtracted(Iso *iso);

private:
	void walkDot(QString &trace, FileNode *&node);
	void addLog(QString log);
	uint64 addTreeToWidget(QTreeWidgetItem *&parent, FileNode *node);

	//Helper functions
	QString getHumenSize(uint64 size);

	//Helper data
	IsoList *_model;
	Ui::MainClass ui;

	QString _lastDotPath;
	QString _lastIsoPath;
	QString _gamePath;
};
#endif // UIMAIN_H
