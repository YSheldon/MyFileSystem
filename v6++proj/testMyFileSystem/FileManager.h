#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "FileSystem.h"
#include "OpenFileManager.h"
#include "File.h"

/*
* 文件管理类(FileManager)
* 封装了文件系统的各种系统调用在核心态下处理过程，
* 如对文件的Open()、Close()、Read()、Write()等等
* 封装了对文件系统访问的具体细节。
*/

class DirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* 目录项中路径部分的最大字符串长度 */

	/* Functions */
public:
	/* Constructors */
	DirectoryEntry();
	/* Destructors */
	~DirectoryEntry();

	/* Members */
public:
	int m_ino;		/* 目录项中Inode编号部分 */
	char m_name[DIRSIZ];	/* 目录项中路径名部分 */
};


class FileManager
{
public:
	/* 目录搜索模式，用于NameI()函数 */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* 以打开文件方式搜索目录 */
		CREATE = 1,		/* 以新建文件方式搜索目录 */
		DELETE = 2		/* 以删除文件方式搜索目录 */
	};

	/* Functions */
public:
	/* Constructors */
	FileManager();
	/* Destructors */
	~FileManager();


	/*
	* @comment 初始化对全局对象的引用
	*/
	void Initialize();

	/*
	* @comment Open()系统调用处理过程
	*/
	int Open(char* , int );

	/*
	* @comment Creat()系统调用处理过程
	*/
	int Creat(char *pathname, int mode);

	/*
	* @comment Open()、Creat()系统调用的公共部分
	*/
	int Open1(Inode* pInode, int mode, int trf);

	/*
	* @comment Close()系统调用处理过程
	*/
	void Close(int fd);

	/*
	* @comment Seek()系统调用处理过程
	*/
	int Seek(int fd, int position);

	/*
	* @comment Read()系统调用处理过程
	*/
	int Read(int fd, char *buffer, int length);

	/*
	* @comment Write()系统调用处理过程
	*/
	int Write(int fd, char *buffer, int length);

	Inode* NameI(char(*func)(), enum DirectorySearchMode mode);

	/*
	* @comment 获取路径中的下一个字符
	*/
	static char NextChar();

	/*
	* @comment 被Creat()系统调用使用，用于为创建新文件分配内核资源
	*/
	Inode* MakNode(unsigned int mode);

	/*
	* @comment 向父目录的目录文件写入一个目录项
	*/
	void WriteDir(Inode* pInode);

	/*
	* @comment 设置当前工作路径
	*/
	void SetCurDir(char* pathname);

	/*
	* @comment 检查对文件或目录的搜索、访问权限，作为系统调用的辅助函数
	*/
	int Access(Inode* pInode, unsigned int mode);

	/* 改变当前工作目录 */
	int ChDir(char *pathname);

	int UnLink(char *pathname);
public:
	/* 根目录内存Inode */
	Inode* rootDirInode;
	Inode* parentDir;
	static char* pathname;
	char dirBuf[DirectoryEntry::DIRSIZ];
	int freeEntryOffset;
	DirectoryEntry dirEntry;
	IOParameter ioParam;
};



#endif
