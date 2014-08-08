#include "FileManager.h"
#include "Shell.h"
#include <iostream>
#include <string>
#include "Utility.h"
#include <vector>
using namespace std;

Inode CURRENT_DIR;// 当前目录
string CURRENT_PATH;// 当前路径
InodeTable m_InodeTable;
OpenFileTable m_OpenFileTable;
BufferManager m_BufferManager;
SuperBlock	m_SuperBlock;
FileManager m_FileManager;
FileSystem m_FileSystem;
Shell m_Shell;
int ERRNO;

void ls()
{
	Inode pInode = CURRENT_DIR;
	DirectoryEntry directoryEntry;
	for (int i = 0; i < 10; i++)
	{
		Buf *pBuf = m_BufferManager.Bread(pInode.i_addr[i]);
		for (int offset = 0; offset < Inode::BLOCK_SIZE; offset += sizeof(DirectoryEntry))
		{
			int* src = (int *)(pBuf->buffer + (offset % Inode::BLOCK_SIZE));
			Utility::DWordCopy(src, (int *)&directoryEntry, sizeof(DirectoryEntry) / sizeof(int));
			if (directoryEntry.m_ino != 0)
				cout << directoryEntry.m_name << " ";
		}
		m_BufferManager.Brelse(pBuf);
	}
	cout << endl;
}

void cd(char *name)
{
	int i = m_FileManager.ChDir(name);
	if (i == -1)
		cout << "cd failed" << endl;
}

int fopen(char *name, int mode)
{
	int fmode;
	switch (mode)
	{
	case 1:
		fmode = File::FREAD;
		break;
	case 2:
		fmode = File::FWRITE;
		break;
	case 3:
		fmode = File::FREAD | File::FWRITE;
		break;
	default:
		cout << "error mode" << endl;
		return -1;
	}
	int fd = m_FileManager.Open(name, fmode);
	if (fd >= 0)
		cout << "the file number is :" << fd << endl;
	else
		cout << "open failed" << endl;
	return fd;

}

void fclose(int fd)
{
	m_FileManager.Close(fd);
}

int fread(int fd, char *buffer, int length)
{
	int i = m_FileManager.Read(fd, buffer, length);
	if (i < 0)
		cout << "read failed" << endl;
	else
	{
		cout << buffer << endl;
		return i;
	}
}

int fwrite(int fd, char *buffer, int length)
{
	int i = m_FileManager.Write(fd, buffer, length);
	if (i < 0)
		cout << "write failed" << endl;
	return i;
}

void flseek(int fd, int position)
{
	int i = m_FileManager.Seek(fd, position);
	if (i < 0)
		cout << "seek failed" << endl;
}

int fcreat(char *name, int mode)
{
	int fmode;
	switch (mode)
	{
	case 1:
		fmode = Inode::IREAD;
		break;
	case 2:
		fmode = Inode::IWRITE;
		break;
	case 3:
		fmode = Inode::IREAD | Inode::IWRITE;
		break;
	default:
		cout << "error mode" << endl;
		return -1;
	}
	int i = m_FileManager.Creat(name,fmode);
	if (i < 0)
		cout << "create failed" << endl;
	return i;
}

int fdelete(char *name)
{
	int i = m_FileManager.UnLink(name);
	if (i == -1)
		cout << "delete failed" << endl;
	return i;
}


int main()
{
	m_FileSystem.LoadSuperBlock();
	m_Shell.shell();
	m_InodeTable.UpdateInodeTable();
	return 0;
}