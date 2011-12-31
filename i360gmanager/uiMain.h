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


	void addTreeToWidget(QTreeWidgetItem *&parent, FileNode *&node);
	void refreshDir(QString directory);

	public slots:
		void slotOnClickList(const QModelIndex &current, const QModelIndex &previous);
		void dumpDot();

private:
	void walkDot(QString &trace, FileNode *&node);

	IsoList *_model;
	Ui::MainClass ui;
	vector<Iso*> _isos;
};

#endif // UIMAIN_H
