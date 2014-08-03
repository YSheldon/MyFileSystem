#include "FileManager.h"
#include <iostream>
#include <string>
using namespace std;

string CURRENT_PATH;// µ±Ç°Â·¾¶
InodeTable m_InodeTable;
OpenFileTable m_OpenFileTable;
BufferManager m_BufferManager;
SuperBlock	m_SuperBlock;
Inode CURRENT_DIR;
FileManager m_FileManager;
FileSystem m_FileSystem;
int ERRNO;

void ls()
{

}
// int mode 1:read 2:write 3:read & write
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
// int mode 1:read 2:write 3:read & write
int fread(int fd, char *buffer, int length)
{
	int i = m_FileManager.Read(fd, buffer, length);
	if (i < 0)
		cout << "read failed" << endl;
	return i;
}
// int mode 1:read 2:write 3:read & write
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
// int mode 1:read 2:write 3:read & write
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

int test_m_BufferManager()
{
	for (int i = 1; i <= 15; i++)
		m_BufferManager.Brelse(m_BufferManager.GetBlk(i));

	return 0;
}

int main()
{
	m_FileSystem.LoadSuperBlock();
	fcreat("/home/test.txt", 1);
	if (fopen("/home/test.txt", 3) + 1)
	{
		//fclose(0);
//		char *content = "djskfjsdklfjsdklfjksdlfjkl;sdajfka;lsdjfkl;asdjfkl;sadjfkl;sdjfkla;sdjfk;lsadjf";
//		fwrite(0, content, strlen(content));
		flseek(0, 0);
		char buffer[512];
		int read = fread(0, buffer, 748);
		if (read >= 0)
		{
			buffer[read] = 0;
			cout << "Read " << read << endl;
			cout << buffer << endl;
		}
	}
	//cout << fdelete("/home/test.txt") << endl;
	char c;
	cin >> c;
	m_InodeTable.UpdateInodeTable();
	return 0;
}