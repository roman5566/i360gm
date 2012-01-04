#include "uiMain.h"
#include "IsoList.h"

//Mutex
QMutex mTreeWidget;
QSettings *settings;

Main::Main(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	settings = new QSettings("./config.ini", QSettings::IniFormat, this);

	//Set model for iso list
	_model = new IsoList();
	ui.tableView->setModel(_model);
	ui.tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

	//Fancy the file explorer
	ui.treeWidget->header()->resizeSection(0, 400);
	ui.treeWidget->header()->resizeSection(1, 50);
	ui.treeWidget->header()->resizeSection(2, 70);

	//Directories
	_lastDotPath = settings->value("paths/dotPath", QDir::currentPath()).toString();
	_lastIsoPath = settings->value("paths/isoPath", QDir::currentPath()).toString();
	_gamePath = settings->value("paths/gamePath", QDir::currentPath()).toString();

	//Create connections
	connect(ui.tableView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(slotOnClickList(const QModelIndex &, const QModelIndex &)) );
	
	//Menu bar connections
	connect(ui.actionSaveDot, SIGNAL(triggered()), this, SLOT(saveDot()));
	connect(ui.actionExtractIso, SIGNAL(triggered()), this, SLOT(extractIso()));
	connect(ui.actionSetGamePath, SIGNAL(triggered()), this, SLOT(setGamePath()));
	connect(ui.actionCheckHashCollision, SIGNAL(triggered()), this, SLOT(checkHashCollision()));

	//Show events for docks
	connect(ui.actionFileExplorer, SIGNAL(triggered()), getUi()->DockFileExplorer, SLOT(show()));
	connect(ui.actionLogWindow, SIGNAL(triggered()), getUi()->DockLog, SLOT(show()));

	//Set come custom menu's
	getUi()->tableView->addAction(getUi()->actionExtractIso);
	getUi()->tableView->addAction(getUi()->actionSaveDot);

	//Some fancy style setting
	getUi()->progressBar->setStyle(new QPlastiqueStyle);
	getUi()->progressBar->reset();

	//If we have a gamePath use it
	if(_gamePath.length() > 0)
		refreshDir(_gamePath);
}

Main::~Main()
{
	//Save settings
	if(QDir::currentPath() != _lastDotPath)
		settings->setValue("paths/dotPath", _lastDotPath);
	if(QDir::currentPath() != _lastIsoPath)
		settings->setValue("paths/isoPath", _lastIsoPath);
	if(QDir::currentPath() != _gamePath)
		settings->setValue("paths/gamePath", _gamePath);
	delete settings;
}

void Main::checkHashCollision()
{
	map<uint, uint> hashes;
	vector<Iso*> *isos = _model->getIsos();
	for(int i = 0; i < isos->size(); i++)
	{
		Iso *iso = isos->at(i);
		hashes[iso->getDisc()->getHash()]++;

		if(hashes[iso->getDisc()->getHash()] > 1)
		{
			addLog("Found a collision!!!!!!!!");
			return;
		}
	}
	addLog("No collision found!");
}

void Main::setGamePath()
{
	//Get directory from user
	QString dir = QFileDialog::getExistingDirectory(this, NULL, _gamePath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(dir.length() <= 0)
		return;
	_gamePath = dir;        

	if(_gamePath.length() > 0)
		refreshDir(_gamePath);

	getUi()->tableView->showRow(1);
}

QString Main::getHumenSize(uint64 size)
{
	QString str;
	double kb = size/1000;
	if(kb < 1000)
		str = str.sprintf("%.0f kB", kb); //Yes we using kilobyte and not kibibyte so divided by 1000
	else if(kb < 1000*1000)
		str = str.sprintf("%.2f MB", kb/1000);
	else
		str = str.sprintf("%.2f GB", kb/(1000*1000));

	return str;
}

void Main::slotOnClickList(const QModelIndex &current, const QModelIndex &previous)
{	
	//Only update the list if the dock is visible, if it is not visible then dont update!
	if(getUi()->DockFileExplorer->isVisible())
	{
		Iso *iso = _model->getIso(current.row());
		if(iso == NULL)
			return;

		getUi()->treeWidget->clear();

		QTreeWidgetItem *parent = new QTreeWidgetItem(getUi()->treeWidget);
		parent->setText(0, iso->getShortIso());
		uint64 totalSize = addTreeToWidget(parent, iso->getRootNode());
		parent->setText(2, getHumenSize(totalSize));

		getUi()->treeWidget->addTopLevelItem(parent);
	}
}

uint64 Main::addTreeToWidget(QTreeWidgetItem *&parent, FileNode *node)
{
	QString str;
	QTreeWidgetItem *item = new QTreeWidgetItem(parent);
	item->setText(0, QString::fromAscii((const char*)node->file->name, node->file->length));
	item->setText(1, str.sprintf("%02X", node->file->type));
	item->setText(3, str.sprintf("%04X", node->file->sector));

	uint64 totalSize = node->file->size;
	item->setText(2, getHumenSize(node->file->size));

	if(node->isDir())
		totalSize += addTreeToWidget(item, node->dir);
	if(node->hasLeft())
		totalSize += addTreeToWidget(parent, node->left);
	if(node->hasRight())
		totalSize += addTreeToWidget(parent, node->right);

	return totalSize;
}

void Main::isoExtracted(Iso *iso)
{
	addLog(QString("Finished extracting iso "+iso->getIso()));
	getUi()->actionExtractIso->setDisabled(false);

	//Reset the QProgressBar in 5 secs
	QTimer::singleShot(5000, getUi()->progressBar, SLOT(reset()));
}

void Main::fileExtracted(QString name, uint size)
{
	addLog("["+QString::number(getUi()->progressBar->value())+"] Extracted: "+name);
	getUi()->progressBar->setValue(getUi()->progressBar->value()+1);
}

void Main::addLog(QString log)
{
	getUi()->textLog->setPlainText(log.append("\n"+getUi()->textLog->toPlainText()));

}
void Main::extractIso()
{
	//Get the iso
	Iso *iso = _model->getIso(ui.tableView->currentIndex().row());
	if(iso == NULL)
		return;

	//Get directory from user
	QString dir = QFileDialog::getExistingDirectory(this, NULL, _lastIsoPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(dir.length() <= 0)
		return;
	_lastIsoPath = dir;                                                                                          //Save this path so users can save a bit easier

	//UI setup
	getUi()->actionExtractIso->setDisabled(true);
	getUi()->progressBar->setMaximum(iso->getFileNo());
	getUi()->progressBar->setValue(0);

	//Set log and start the thread*
	addLog(QString("Extracting iso with %1 files").arg(iso->getFileNo()));
	QMetaObject::invokeMethod(iso, "extractIso", Q_ARG(QString, dir.append("/").append(iso->getShortIso())));    //Start the extraction threaded
}

void Main::saveDot()
{
	//Get info
	Iso *iso = _model->getIso(ui.tableView->currentIndex().row());
	if(iso == NULL)
		return;
	FileNode *root = iso->getRootNode();

	//Ask user for save
	QString file = iso->getShortIso();
	QString fileName = QFileDialog::getSaveFileName(this, NULL, _lastDotPath + QDir::separator() + file + QString(".gv"), "DOT Graph (*.gv *.dot)");      //Default file extension is .gv as .dot is in use by office
	_lastDotPath = QFileInfo(fileName).absoluteDir().absolutePath();
	
	//Make the dot file
	QString dotty = "digraph " + file.remove(QRegExp("\\W")) + " {\n";                                                                                 //Remove all non [a-zA-Z0-9] (except _) chars from the string because it breaks Graphviz
	walkDot(dotty, root);
	dotty.append("}");

	//Write the file
	QFile out(fileName);
	out.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream write(&out);
	write << dotty;
	out.close();

	//Log
	addLog(tr("Saved dot graph of ") + file);
}

void Main::walkDot(QString &trace, FileNode *&node)
{
	QString sp;
	if(node->hasLeft())
		trace.append(sp.sprintf("\t %i -> %i;\n", (uint)node, (uint)node->left));
	if(node->hasRight())
		trace.append(sp.sprintf("\t %i -> %i;\n", (uint)node, (uint)node->right));
	if(node->isDir())
		trace.append(sp.sprintf("\t %i -> %i [style=dashed];\n", (uint)node, (uint)node->dir));

	if(node->hasLeft())
		walkDot(trace, node->left);
	if(node->hasRight())
		walkDot(trace, node->right);
	if(node->isDir())
		walkDot(trace, node->dir);

	//Add myself to label list
	trace.append(sp.sprintf("\t %i [label=\"%s (%i)\"];\n", (uint)node, QString::fromAscii((const char*)node->file->name, node->file->length).toStdString().c_str(), node->file->length));
}

void Main::refreshDir(QString directory)
{
	_model->clearIsos();                          //Clear the list

	QDir dir(directory);
	QStringList filter; filter << "*.iso";        //Filter the dir list
	QStringList list = dir.entryList(filter);

	for (int i = 0; i < list.size(); ++i)
	{
		Iso *iso = new Iso();
		iso->setPath(dir.filePath(list.at(i)));

		//Set up the connections
		connect(iso, SIGNAL(doFileExtracted(QString, uint)), this, SLOT(fileExtracted(QString, uint)));
		connect(iso, SIGNAL(doIsoExtracted(Iso *)), this, SLOT(isoExtracted(Iso *)));

		_model->addIso(iso);
	}
}