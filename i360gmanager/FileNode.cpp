#include <Windows.h>
#include "FileNode.h"

FileNode::FileNode()
{
}

FileNode::FileNode(XboxFileInfo *file)
{
	this->file = file;
	this->left = NULL;
	this->right = NULL;
	this->dir = NULL;
}

uint FileNode::extractFile(HANDLE isoMap, const wchar_t *path, uint offset)
{
	//Get alignment
	SYSTEM_INFO info;
	GetSystemInfo(&info);

	//Get the closest correct aligned address
	DWORD bytes;
	uint64 address = file->getAddress(offset);
	uint offsetAddress = address % info.dwAllocationGranularity;
	address -= offsetAddress;
	DWORD high = HIDWORD(address);
	DWORD low = LODWORD(address);
	
	//Get data pointer
	void *isoData = MapViewOfFile(isoMap, FILE_MAP_READ, high, low, file->size+offsetAddress);
	HANDLE fileOut = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL);
	WriteFile(fileOut, (void*)((char*)isoData+offsetAddress), file->size, &bytes, NULL);

	//Cleanup
	CloseHandle(fileOut);
	UnmapViewOfFile(isoData);
	return bytes;
}

uint FileNode::getNodesNo(FileNode *node)
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

XboxFileInfo *FileNode::getXboxByName(FileNode *node, char *name, uint length)
{
	return getNodeByName(node, name, length)->file;
}

FileNode *FileNode::getNodeByName(FileNode *node, char *name, uint length)
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

bool FileNode::isDir()
{
	return (dir != NULL);
}

bool FileNode::hasLeft()
{
	return (left != NULL);
}

bool FileNode::hasRight()
{
	return (right != NULL);
}