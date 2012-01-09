#include "Iso.h"
#include <QDir>

void Iso::extractIso(QString output)
{
	//Start extracting every file
	CreateDirectory(output.toStdWString().c_str(), NULL);
	extractFile(output, _rootFile);
	emit doIsoExtracted(this);
}

void Iso::extractFile(QString output, FileNode *node)
{
	QString dir = output;
	int64 writen = saveFile(dir, node->file);

	if(writen == 0) //Was a dir so dive into the dir
		extractFile(dir, node->dir);
	emit doFileExtracted(QString::fromAscii((char*)node->file->name, node->file->length), writen);

	if(node->hasLeft())
		extractFile(output, node->left);
	if(node->hasRight())
		extractFile(output, node->right);
}

void Iso::saveFile(QString path, uint ptr)
{
	saveFile(path, VPtr<XboxFileInfo>::fromPtr(ptr), true);	
}

/**
 * saveFile will save a file, to identify what file pass a XboxFileInfo struct to this function
 * @param out string path where to save (note this will note make directories)
 * @param file a pointer to a XboxFileInfo struct
 * @returns bytes written, or -1 if it was a dir
 */
int64 Iso::saveFile(QString &path, XboxFileInfo *file, bool blocks)
{
	QString fileName = QString::fromAscii((char*)file->name, file->length);
	path.append(QDir::separator()).append(fileName);

	if(file->isDir())
	{
		if(CreateDirectoryA(path.toStdString().c_str(), NULL))
			return 0;
		return -1;
	}
	else
	{
		uint offset;
		DWORD written = 0;
		uint64 totalWritten = 0;
		
		void *data = getMapOfFile(getAddress(file->sector), file->size, &offset);
		HANDLE fileOut = CreateFileA(path.toStdString().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
		if(blocks)
		{
			//Calculate amount of blocks, this meens also that blocks can be max 2GB big, and a file can be 200GB big (TROLOLOLOLOL)
			DWORD subWritten;
			div_t nblock = div(file->size, 100);
			int blockSize = nblock.quot;

			for(int i = 0; i < 100; i++)
			{
				if(i == 99) //Last block is a little bit bigger because of quotient
					blockSize += nblock.rem;

				WriteFile(fileOut, (void*)((char*)data+offset+(i*blockSize)), blockSize, &subWritten, NULL);
				emit doBytesWritten(subWritten);
				totalWritten += subWritten;
			}
			emit doFileExtractedSuccess(fileName);
		}
		else
			if(!WriteFile(fileOut, (void*)((char*)data+offset), file->size, &written, NULL))
				written = -1;

		//Cleanup
		CloseHandle(fileOut);
		UnmapViewOfFile(data);
		return written;
	}
}

void Iso::deleteTree()
{
	deleteTree(_rootFile);
}

void Iso::deleteTree(FileNode *file)
{
	if(file == NULL)
		return;
	if(file->hasLeft())
		deleteTree(file->left);
	if(file->hasRight())
		deleteTree(file->right);
	if(file->isDir())
		deleteTree(file->dir);

	delete file;
}

uint Iso::getNodesNo(FileNode *node)
{
	if(node == NULL)
		return 0;
	else
	{
		int nodes = 1;                     //This is a file so +1
		nodes += getNodesNo(node->left);   //Count left node
		nodes += getNodesNo(node->right);  //Count right node
		return nodes;
	}
}

FileNode *Iso::getNodeByName(FileNode *node, char *name, uint length)
{
	if(node == NULL)
		return NULL;

	XboxFileInfo *file = node->file;
	uint cLength = length;

	if(cLength > file->length) cLength = file->length; //Posible bufferoverflow fix

	int cmp = _strnicmp((char*)file->name, name, cLength);
	if(cmp == 0)
		return node;
	if(cmp > 0 )
		return getNodeByName(node->right, name, length);
	else
		return getNodeByName(node->left, name, length);
}

/**
 * getFile returns a pointer to a XboxFileInfo struct for the file with name and length. If there was no such file it returns NULL.
 * @param name contains the file name
 * @param length length of the name
 * @return XboxFileInfo struct containing information for that file
 */
XboxFileInfo* Iso::getFile(char* name, int length)
{
	return getNodeByName(getRootNode(), name, length)->file;
}

/**
 * isDefaultXex check is this iso has a valid default xex (or if we can find the default.xex ;)).
 * @return True is is has a valid default.xex, else false
 */
bool Iso::isDefaultXex()
{
	if(_defaultXex == NULL)
		return false;                                              //We could not find the file...

	return isXex(_defaultXex);
}

/**
 * isXex checks if a XboxFile is indeed a xex file
 * @param file a pointer to a XboxFileInfo struct
 * @return True if that is a XeX file, else flase
 */
bool Iso::isXex(XboxFileInfo *file)
{
	uint magic;
	_lseeki64(_handle, getAddress(file->sector), SEEK_SET);
	_read(_handle, (void*)&magic, sizeof(uint));

	return (magic == XEX_MAGIC_BYTE);
}

/**
 * isXbe checks if a XboxFile is indeed a xbe file
 * @param file a pointer to a XboxFileInfo struct
 * @return True if that is a Xbe file, else flase
 */
bool Iso::isXbe(XboxFileInfo *file)
{
	uint magic;
	_lseeki64(_handle, getAddress(file->sector), SEEK_SET);
	_read(_handle, (void*)&magic, sizeof(uint));

	return (magic == XBE_MAGIC_BYTE);
}
/**
 * getFileInfo gets a XboxFileInfo struct from a file in a sector
 * @param sector a pointer to SectorData from where to read
 * @param offset The offset from where to read the data from that sector
 * @return XboxFileInfo* struct or NULL when tried to read outside area
 */
XboxFileInfo *Iso::getFileInfo(SectorData *sector, uint offset)
{
	if(sector->size <= offset)
		return NULL;           //Tried to read outside this sector that wrong!
	return (XboxFileInfo*)((char*)sector->data+offset);
}