#ifndef FILENODE_H
#define FILENODE_H

#include "xbox.h"

class FileNode
{
	public:
		XboxFileInfo *file;
		FileNode *left;
		FileNode *right;
		FileNode *dir;

		FileNode();
		FileNode(XboxFileInfo *file);
		FileNode(XboxFileInfo *file, FileNode *left, FileNode *right);
		uint getNodesNo(FileNode *node);

};

#endif