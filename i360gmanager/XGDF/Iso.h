#ifndef PACKET_H
#define PACKET_H

#include <Windows.h>

#include <QVariant>
#include <QAbstractListModel>
#include <QPixmap>
#include <QString>
#include <QFile>
#include <QBool>
#include <QBrush>
#include <QThread>
#include <QTreeWidgetItem>
#include <QMutex>
#include <QSettings>

#include "common.h"
#include "Xbox/xex.h"

#define DATA_POINTER_TREE 4

//External shit
extern QMutex mTreeWidget;
extern bool stopTreeWidget;
extern map<string, string> fileNameDb;

template <class T> class VPtr
{
public:
	static T* asPtr(QVariant v)
	{
		return  (T *) v.value<void *>();
	}

	static QVariant asQVariant(T* ptr)
	{
		return qVariantFromValue((void *) ptr);
	}

	static T* fromPtr(uint u)
	{
		return (T*)u;
	}

	static uint toPtr(T* t)
	{
		return (uint)t;
	}
};

class Iso  : public QObject
{
	Q_OBJECT

public:
	Iso(QString path);
	~Iso();

	//Iso
	bool isXboxMedia();
	bool isValidMedia();

	//IsoFields
	map<uint, SectorData*> *getSectors();
	XboxFileInfo *getFileInfo(SectorData *sector, uint offset);
	SectorData *getRootData();
	SectorData *getSector(uint sector, uint size);
	uint getRootSize();
	uint getRootSector();
	uint64 getAddress(uint sector);
	uint64 getRootAddress();
	DiscType getDiscType();
	QString getPath();
	QString getIso();
	QString getShortIso();
	uint getFileNo();
	FileNode* getRootNode();
	QVariant getField(int column);

	//IsoIO
	uint getHash();
	HANDLE getHandle();
	void *getMapOfFile(uint64 address, uint size, void *outOffset = NULL);

	//IsoFiles
	void deleteTree();
	void deleteTree(FileNode *file);
	bool isDefaultXex();
	bool isXex(XboxFileInfo *file);
	int readData(uint64 address, uint size, void *buffer);
	XboxFileInfo* getFile(char* name, int length);
	int64 saveFile(QString &path, XboxFileInfo *file, bool blocks = false);
		//Static by FileNode
		static uint getNodesNo(FileNode *node);
		static FileNode *getNodeByName(FileNode *node, char *name, uint length);

	//GUI
	uint64 addTreeToWidget(QTreeWidgetItem *&parent, FileNode *node);

	long _binTree;
	long _hashTime;
	//Slots and signals
	public slots:
		void extractIso(QString output);
		void saveFile(QString path, uint ptr);
		void Initialize();

		//GUI
		void exploreIso();
		

	signals:
		void doFileExtractedSuccess(QString name);
		void doBytesWritten(uint bytes);
		void doSetTree(QTreeWidgetItem *item);
		void doIsoExtracted(Iso *iso);
		void doFileExtracted(QString name, uint size);

private:
	//Private functions
	void makeTree(SectorData *sector, uint offset, FileNode *&node);        //Iso
	bool readMagicMedia(DiscType type);                                     //IsoIO
	void walkFile(uint offset);                                             //IsoIO
	void extractFile(QString output, FileNode *node);                       //IsoIO

	//Disc info
	QString _path;
	long _fileNo;
	DiscType _type;
	MediaInfo _mediaInfo;

	//Disc files
	FileNode *_rootFile;
	Xex *_xex;
	XboxFileInfo *_defaultXex;

	//Disc data IO
	HANDLE _realHandle;
	HANDLE _mapHandle;
	int _handle;
	uint _granularity;
	uint _hash;
	map<uint, SectorData*> _sectors;

	//Timings and information
	long s;
	void startTime(){ s = clock();}
	long stopTime(){ return clock()-s;}

	//Helper data
	QThread *_thread;
};

extern QSettings *settings;
#endif