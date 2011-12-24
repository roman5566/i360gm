#include "uiMain.h"
#include "IsoList.h"

Main::Main(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);


	_model = new IsoList();
	_model->setIsos(&_isos);
	ui.tableView->setModel(_model);
	ui.tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	refreshDir("D:/xbox/Games");

	QObject::connect(ui.tableView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(slotOnClickList(const QModelIndex &, const QModelIndex &)) );
}

void Main::slotOnClickList(const QModelIndex &current, const QModelIndex &previous)
{
	Iso *iso = _model->getIso(current.row());

	this->ui.listWidget->clear();
	vector<XboxFileInfo*> files = iso->getFiles();

	for(int i = 0; i < files.size(); i++)
	{
		XboxFileInfo *file = files.at(i);

		QString str;
		str.resize(file->length);

		for(int x = 0; x < file->length; x++)
			str[x] = QChar(file->name[x]);

		this->ui.listWidget->addItem(str);
	}
	this->ui.listWidget->sortItems();
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
