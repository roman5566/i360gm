#include "uiMain.h"
#include "IsoList.h"

Main::Main(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);


	IsoList *model = new IsoList();
	model->setIsos(&_isos);
	ui.tableView->setModel(model);
	ui.tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	refreshDir("D:/xbox/Games");
}

void Main::refreshDir(QString directory)
{
	_isos.clear();                                //Clear the list

	QDir dir(directory);
	QStringList filter; filter << "*.iso";        //Filter the dir list
	QStringList list = dir.entryList(filter);

	for (int i = 0; i < list.size(); ++i)
	{
		QString path = dir.filePath(list.at(i));
		Iso *iso = new Iso();
		iso->setPath(path);

		_isos.push_back(iso);
	}
}

Main::~Main()
{

}
