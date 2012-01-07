#include "Iso.h"

void Iso::exploreIso()
{
	stopTreeWidget = true;
	mTreeWidget.lock();    //Wait till other thread is done exiting there request
	stopTreeWidget = false;

	QTreeWidgetItem *parent = new QTreeWidgetItem(NULL);
	parent->setText(0, getShortIso());
	uint64 totalSize = addTreeToWidget(parent, getRootNode());
	if(!stopTreeWidget)
	{
		parent->setText(2, getHumenSize(totalSize));
		emit doSetTree(parent); //Send the data to Main outside this thread
	}
	else
	{
		//Delete everything we created so far when we had to quit! as else we have major a major memory leak!
		 qDeleteAll(parent->takeChildren());
		 delete parent;
	}
	mTreeWidget.unlock();	
}

uint64 Iso::addTreeToWidget(QTreeWidgetItem *&parent, FileNode *node)
{
	if(stopTreeWidget)
		return 0;
	QString str;
	QTreeWidgetItem *item = new QTreeWidgetItem(parent);
	item->setText(0, QString::fromAscii((const char*)node->file->name, node->file->length));
	item->setText(1, str.sprintf("%02X", node->file->type));
	item->setText(3, str.sprintf("%04X", node->file->sector));
	item->setData(DATA_POINTER_TREE, Qt::DisplayRole, VPtr<XboxFileInfo>::asQVariant(node->file));             //Give this item a pointer to its own node

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
