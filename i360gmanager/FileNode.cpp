#include "FileNode.h"

FileNode::FileNode()
{
	FileNode(NULL);
}

FileNode::FileNode(XboxFileInfo *file)
{
	FileNode(file, NULL, NULL);
}

FileNode::FileNode(XboxFileInfo *file, FileNode *left, FileNode *right)
{
	this->file = file;
	this->left = left;
	this->right = right;
	this->dir = NULL;
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