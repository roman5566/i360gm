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

		//General functions
		static uint getNodesNo(FileNode *node);
		static FileNode *getNodeByName(FileNode *node, char *name, uint length);
		static XboxFileInfo *getXboxByName(FileNode *node, char *name, uint length);

		//Special functions
		uint extractFile(HANDLE isoMap, const wchar_t *path, uint offset);

		void deleteTree();

		//Traversing
		bool isDir();
		bool hasLeft();
		bool hasRight();

};

#endif