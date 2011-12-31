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
	ui.treeWidget->header()->resizeSection(0, 400);
	ui.treeWidget->header()->resizeSection(1, 50);
	ui.treeWidget->header()->resizeSection(2, 70);
	refreshDir("D:/xbox/Games");

	//Create connections
	connect(ui.tableView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(slotOnClickList(const QModelIndex &, const QModelIndex &)) );
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(dumpDot()));
}

void Main::slotOnClickList(const QModelIndex &current, const QModelIndex &previous)
{
	
	Iso *iso = _model->getIso(current.row());

	//Fill the tree index
	FileNode *root = iso->getRootNode();
	this->ui.treeWidget->clear();

	QTreeWidgetItem *parent = new QTreeWidgetItem(this->ui.treeWidget);
	parent->setText(0, iso->getIso());
	addTreeToWidget(parent, root);

	this->ui.treeWidget->addTopLevelItem(parent);
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

void Main::dumpDot()
{
	Iso *iso = _model->getIso(ui.tableView->currentIndex().row());
	if(iso == NULL)
		return;

	FileNode *root = iso->getRootNode();

	QString sp;
	QString dotty;
	QString file = iso->getIso();
	file.chop(4);
	dotty = sp.sprintf("digraph %s {\n", file.toStdString().c_str());
	walkDot(dotty, root);
	dotty.append("}");

	file = sp.sprintf("C://dotty/%s.dot", file.toStdString().c_str());
	QFile out(file);
	out.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream write(&out);
	write << dotty;
	out.close();

}

void Main::addTreeToWidget(QTreeWidgetItem *&parent, FileNode *&node)
{
	QString str;
	QTreeWidgetItem *item = new QTreeWidgetItem(parent);
	item->setText(0, QString::fromAscii((const char*)node->file->name, node->file->length));
	item->setText(1, str.sprintf("%02X", node->file->type));
	item->setText(3, str.sprintf("%04X", node->file->sector));
	
	double kb = node->file->size/1000;
	if(kb < 1000)
		item->setText(2, str.sprintf("%.0f kB", kb)); //Yes we using kilobyte and not kibibyte so divided by 1000
	else if(kb < 1000*1000)
		item->setText(2, str.sprintf("%.2f MB", kb/1000));
	else
		item->setText(2, str.sprintf("%.2f GB", kb/(1000*1000)));

	if(node->isDir())
		addTreeToWidget(item, node->dir);
	if(node->hasLeft())
		addTreeToWidget(parent, node->left);
	if(node->hasRight())
		addTreeToWidget(parent, node->right);
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
