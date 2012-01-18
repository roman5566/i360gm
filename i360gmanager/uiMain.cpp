#include "uiMain.h"
#include "GameList.h"

//Mutex
QMutex mTreeWidget;
bool stopTreeWidget;
QSettings *settings;

map<string, string> fileNameDb;
map<QString, QString> xbox1Name;

Main::Main(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	settings = new QSettings("./config.ini", QSettings::IniFormat, this);

	//Create the loader thread
	_loader = new Loader();
	
	_loader->connect(_loader, SIGNAL(doProgressTotalIsos(uint)), this, SLOT(progressTotalIsos(uint)));
	_loader->connect(_loader, SIGNAL(doProgressIso(QString)), this, SLOT(progressIso(QString)));
	_loader->connect(_loader, SIGNAL(doDoneLoading(vector<Game*> *, uint)), this, SLOT(loaderDone(vector<Game*> *, uint)));

	//Set model for iso list
	_source = new GameList();
	ui.sourceView->setModel(_source);
	ui.sourceView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	
	_external = new GameList();
	ui.externView->setModel(_external);
	ui.externView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

	//Fancy the file explorer
	ui.fileTree->header()->resizeSection(0, 250);
	ui.fileTree->header()->resizeSection(1, 30);
	ui.fileTree->header()->resizeSection(2, 55);
	ui.fileTree->header()->resizeSection(3, 45);

	//Directories
	_lastDotPath = settings->value("paths/dot", QDir::currentPath()).toString();
	_lastIsoPath = settings->value("paths/iso", QDir::currentPath()).toString();
	_gamePath = settings->value("paths/game", QDir::currentPath()).toString();
	_filePath = settings->value("paths/file", QDir::currentPath()).toString();
	sExternalPath = settings->value("paths/external", QDir::currentPath()).toString();

	//Gui from settings
	ui.externPath->setText(sExternalPath);
	ui.sourcePath->setText(_gamePath);

	//Set up extra forms
	about = new QDialog();
	uiAbout.setupUi(about);
	connect(ui.actionAbout, SIGNAL(triggered()), about, SLOT(show()));
	
	//Create connections
	connect(ui.sourceView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(slotOnClickList(const QModelIndex &, const QModelIndex &)) );
	
	//Menu bar connections
	connect(ui.actionSaveDot, SIGNAL(triggered()), this, SLOT(saveDot()));
	connect(ui.actionExtractIso, SIGNAL(triggered()), this, SLOT(extractIso()));
	connect(ui.actionSetGamePath, SIGNAL(triggered()), this, SLOT(setGamePath()));
	connect(ui.actionExtractFile, SIGNAL(triggered()), this, SLOT(extractFile()));
	connect(ui.actionCheckHashCollision, SIGNAL(triggered()), this, SLOT(checkHashCollision()));
	connect(ui.actionCalculateMemory, SIGNAL(triggered()), this, SLOT(calculateMemory()));
	connect(ui.actionReport, SIGNAL(triggered()), this, SLOT(reportIntline9()));

	//Show events for docks
	connect(ui.actionFileExplorer, SIGNAL(triggered()), getUi()->DockFileExplorer, SLOT(show()));
	connect(ui.actionLogWindow, SIGNAL(triggered()), getUi()->DockLog, SLOT(show()));

	//Set come custom menu's
	getUi()->sourceView->addAction(getUi()->actionExtractIso);
	getUi()->sourceView->addAction(getUi()->actionSaveDot);

	getUi()->fileTree->addAction(getUi()->actionExtractFile);

	//Some fancy style setting
	getUi()->progressBar->setStyle(new QPlastiqueStyle);
	getUi()->progressBar->reset();

	//Read the game name db
	readNameDb();

	//If we have a gamePath use it
	if(_gamePath.length() > 0)
		refreshDir(_gamePath, _source);

	if(sExternalPath.length() > 0)
		refreshDir(sExternalPath, _external);
}

Main::~Main()
{
	//Save settings
	if(QDir::currentPath() != _lastDotPath)
		settings->setValue("paths/dot", _lastDotPath);
	if(QDir::currentPath() != _lastIsoPath)
		settings->setValue("paths/iso", _lastIsoPath);
	if(QDir::currentPath() != _gamePath)
		settings->setValue("paths/game", _gamePath);
	if(QDir::currentPath() != _filePath)
		settings->setValue("paths/file", _filePath);
	if(QDir::currentPath() != sExternalPath)
		settings->setValue("paths/external", sExternalPath);
	
	delete settings;
	delete _loader;

	//Delete extra forms
	delete about;
}

void Main::progressTotalIsos(uint isos)
{
	getUi()->progressBar->setMaximum(isos);
	getUi()->progressBar->setValue(0);
}

void Main::progressIso(QString iso)
{
	if(iso != NULL)
		addLog(tr("Was not able to load: ")+iso);
	getUi()->progressBar->setValue(getUi()->progressBar->value()+1);
}

void Main::loaderDone(vector<Game*> *games, uint vptr)
{
	GameList *list = VPtr<GameList>::fromPtr(vptr);
	list->clearGames();                      //Clear the list

	for (int i = 0; i < games->size(); ++i)
	{
		Game *game = games->at(i);
		//Set up the connections for feedback and threading
		connect(game, SIGNAL(doFileExtracted(QString, uint)), this, SLOT(fileExtracted(QString, uint)));
		connect(game, SIGNAL(doBytesWritten(uint)), this, SLOT(bytesWritten(uint)));
		connect(game, SIGNAL(doIsoExtracted(Iso *)), this, SLOT(isoExtracted(Iso *)));
		connect(game, SIGNAL(doSetTree(QTreeWidgetItem *)), this, SLOT(setTree(QTreeWidgetItem *)));
		connect(game, SIGNAL(doFileExtractedSuccess(QString)), this, SLOT(fileExtractedSuccess(QString)));
		list->addGame(game);
	}
	delete games; //Delete that nasty vector!!!
	getUi()->progressBar->setMaximum(100);
	getUi()->progressBar->reset();
}

void Main::readNameDb()
{
	try
	{
		QFile db("GameNameLookup.csv");
		if(!db.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		QString line;
		QTextStream in(&db);
		while (!in.atEnd()) {
			line = in.readLine();
			QStringList splits = line.split(',');
			for(int i = 1; i < splits.size(); i++)
				fileNameDb[splits.at(i).toStdString()] = splits[0].toStdString();
		}
		db.close();

		//Read xbox1 stuff
		QFile dbXbox1("Xbox1NameLookup.csv");
		if(!dbXbox1.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		QTextStream in1(&dbXbox1);
		while (!in1.atEnd()) {
			line = in1.readLine();
			if(line.isEmpty()) continue;
			QStringList splits = line.split(',');
			
			xbox1Name[splits[0]] = splits[1];
		}
		dbXbox1.close();
	}
	catch(...)
	{
		//To bad
	}
}

void Main::reportIntline9()
{
	getUi()->textLog->clear();
	checkHashCollision();
	calculateMemory();

	double avgHash = 0, avgBin = 0;
	vector<Game*> *isos = _source->getGames();
	for(int i = 0; i < isos->size(); i++)
	{
		Iso *iso = dynamic_cast<Iso*>(isos->at(i));

		avgHash += iso->_hashTime;
		avgBin += iso->_binTree;
	}

	addLog(QString("Hash, average: ")+QString::number(avgHash/isos->size())+QString(", total: ")+QString::number(avgHash));
	addLog(QString("BinTree, average: ")+QString::number(avgBin/isos->size())+QString(", total: ")+QString::number(avgBin));
	addLog(QString("Total isos:")+QString::number(isos->size()));

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(tr("[code]")+getUi()->textLog->toPlainText()+tr("[/code]"));

	QMessageBox::information(this, tr("Thanks for the report!"), tr("I've copied the report to your clipboard, so please make a post in the forum and paste this report. Thanks a lot in advance"));
}

void Main::checkHashCollision()
{
	map<uint, uint> hashes;
	vector<Game*> *isos = _source->getGames();
	for(int i = 0; i < isos->size(); i++)
	{
		Iso *iso = dynamic_cast<Iso*>(isos->at(i));
		hashes[iso->getHash()]++;

		if(hashes[iso->getHash()] > 1)
		{
			addLog("Found a collision!!!!!!!!");
			return;
		}
	}
	addLog("No collision found!");
}

void Main::calculateMemory()
{
	map<uint,SectorData*>::iterator it;
	uint64 total = 0, fileNodeKb = 0, sectorsKb = 0;
	vector<Game*> *isos = _source->getGames();
	for(int i = 0; i < isos->size(); i++)
	{
		Iso *iso = dynamic_cast<Iso*>(isos->at(i));

		fileNodeKb += (iso->getFileNo() * sizeof(FileNode))+4;

		//Get sector data
		map<uint, SectorData*> *sectors = iso->getSectors();
		for(it = sectors->begin(); it != sectors->end(); it++)
		{
			SectorData *data = it->second;
			sectorsKb += sizeof(SectorData) + data->size + ((4+4+4)*sectors->size()); //sizeof map i think
		}
	}
	total += fileNodeKb + sectorsKb;
	addLog(QString("    Sectors:               ")+getHumenSize(sectorsKb));
	addLog(QString("    FileNode structs:  ")+getHumenSize(fileNodeKb));
	addLog(QString("Total memory footprint: ")+getHumenSize(total));
}

void Main::setGamePath()
{
	//Get directory from user
	QString dir = QFileDialog::getExistingDirectory(this, NULL, _gamePath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(dir.length() <= 0)
		return;
	_gamePath = dir;        

	if(_gamePath.length() > 0)
		refreshDir(_gamePath, _source);

	getUi()->sourceView->showRow(1);
}

void Main::slotOnClickList(const QModelIndex &current, const QModelIndex &previous)
{	
	//Only update the list if the dock is visible, if it is not visible then dont update!
	if(getUi()->DockFileExplorer->isVisible())
	{
		Iso *iso = dynamic_cast<Iso*>(_source->getGame(current.row()));
		if(iso == NULL)
			return;

		QMetaObject::invokeMethod(iso, "exploreIso");
	}
}

void Main::setTree(QTreeWidgetItem *item)
{
	getUi()->fileTree->clear();
	getUi()->fileTree->addTopLevelItem(item);
}

void Main::isoExtracted(Iso *iso)
{
	addLog(QString("Finished extracting iso "+iso->getIso()));
	getUi()->actionExtractIso->setDisabled(false);
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
void Main::fileExtractedSuccess(QString name)
{
	addLog(QString("Finished extracting file ")+name);
	getUi()->actionExtractFile->setDisabled(false);
}

void Main::bytesWritten(uint bytes)
{
	getUi()->progressBar->setValue(getUi()->progressBar->value()+1);
}

void Main::extractFile()
{
	Iso *iso = dynamic_cast<Iso*>(_source->getGame(getUi()->sourceView->currentIndex().row()));
	if(iso == NULL || getUi()->fileTree->currentIndex().row() < 0)
		return addLog("No file selected!");

	XboxFileInfo *file = VPtr<XboxFileInfo>::asPtr(getUi()->fileTree->currentItem()->data(DATA_POINTER_TREE, Qt::DisplayRole));
	if(file->isDir())
		return addLog("Can not save recursive dir yet!");
		


	QString dir = QFileDialog::getExistingDirectory(this, NULL, _filePath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if(dir.length() <= 0)
		return;
	_filePath = dir;

	//Save a single file, start thread and do some GUI stuff
	getUi()->actionExtractFile->setDisabled(true);
	getUi()->progressBar->setValue(0);
	getUi()->progressBar->setMaximum(100);
	QMetaObject::invokeMethod(iso, "saveFile", Q_ARG(QString, dir), Q_ARG(uint, VPtr<XboxFileInfo>::toPtr(file)));
}

void Main::extractIso()
{
	//Get the iso
	Iso *iso = dynamic_cast<Iso*>(_source->getGame(getUi()->sourceView->currentIndex().row()));
	if(iso == NULL)
		return addLog("No iso selected!");

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
	Iso *iso = dynamic_cast<Iso*>(_source->getGame(getUi()->sourceView->currentIndex().row()));
	if(iso == NULL)
		return addLog("No iso selected!");
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

void Main::refreshDir(QString directory, GameList *list, bool keep)
{
	getUi()->fileTree->clear();
	if(list == _source) getUi()->sourceView->reset();
	if(list == _external) getUi()->externView->reset();

	//Call the loader
	QMetaObject::invokeMethod(_loader, "readIsoDir", Q_ARG(QString, directory), Q_ARG(uint, VPtr<GameList>::toPtr(list)));
}