#include "FileManager.h"
#include "TestMyFileSystem.h"
#include "Utility.h"
#include "TestMyFileSystem.h"


char * FileManager::pathname = NULL;

/*==========================class FileManager===============================*/
FileManager::FileManager()
{
	Buf *buf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR);
	rootDirInode = new Inode();
	rootDirInode->ICopy(buf, 0);
	m_BufferManager.Brelse(buf);
}

FileManager::~FileManager()
{
	delete rootDirInode;
	//nothing to do here
}


int FileManager::Open(char *pathname, int mode)
{
	Inode* pInode;
	this->pathname = pathname;
	pInode = this->NameI(NextChar, FileManager::OPEN);	/* 0 = Open, not create */
	/* 没有找到相应的Inode */
	if (NULL == pInode)
	{
		return -1;
	}
	return this->Open1(pInode, mode, 0);
}

int FileManager::Creat(char *pathname, int mode)
{
	Inode* pInode;
	this->pathname = pathname;
	/* 搜索目录的模式为1，表示创建 */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* 没有找到相应的Inode */
	if (NULL == pInode)
	{
		/* 创建Inode */
		pInode = this->MakNode(mode);
		/* 创建失败 */
		if (NULL == pInode)
		{
			return -1;
		}

		/*
		* 如果所希望的名字不存在，使用参数trf = 2来调用open1()。
		* 不需要进行权限检查，因为刚刚建立的文件的权限和传入参数mode
		* 所表示的权限内容是一样的。
		*/
		return this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* 如果NameI()搜索到已经存在要创建的文件，则清空该文件 */
		return this->Open1(pInode, File::FWRITE, 1);
	}
}

/*
* trf == 0由open调用
* trf == 1或者trf == 2由creat调用
* mode参数：打开文件模式，采用读、写还是读写的方式
*/
int FileManager::Open1(Inode* pInode, int mode, int trf)
{
	/*
	* 对所希望的文件已存在的情况下，即trf == 0或trf == 1进行权限检查
	* 如果所希望的名字不存在，即trf == 2，不需要进行权限检查，因为刚建立
	* 的文件的权限和传入的参数mode的所表示的权限内容是一样的。
	*/
	ERRNO = 0;
	if (trf != 2)
	{
		if (mode & File::FREAD)
		{
			/* 检查读权限 */
			this->Access(pInode, Inode::IREAD);
		}
		if (mode & File::FWRITE)
		{
			/* 检查写权限 */
			this->Access(pInode, Inode::IWRITE);
			/* 系统调用去写目录文件是不允许的 */
			if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
			{
				ERRNO = EISDIR;
			}
		}
	}

	if (ERRNO)
	{
		m_InodeTable.IPut(pInode);
		return -1;
	}

	/* 在creat文件的时候搜索到同文件名的文件，释放该文件所占据的所有盘块 */
	if (1 == trf)
	{
		pInode->ITrunc();
	}

	/* 分配打开文件控制块File结构 */
	int fd = m_OpenFileTable.FAlloc();
	File* pFile = m_OpenFileTable.GetF(fd);
	if (NULL == pFile)
	{
		m_InodeTable.IPut(pInode);
		return -1;
	}
	/* 设置打开文件方式，建立File结构和内存Inode的勾连关系 */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* 为打开或者创建文件的各种资源都已成功分配，函数返回 */
	if (ERRNO == 0)
	{
		return fd;
	}
	else	/* 如果出错则释放资源 */
	{
		/* 递减File结构和Inode的引用计数 */
		pFile->f_count--;
		m_InodeTable.IPut(pInode);
		return -1;
	}
}

void FileManager::Close(int fd)
{
	/* 获取打开文件控制块File结构 */
	File* pFile = m_OpenFileTable.GetF(fd);
	if (NULL == pFile)
	{
		return;
	}

	/* 释放打开文件描述符fd，递减File结构引用计数 */
	m_OpenFileTable.CloseF(pFile);
}

int FileManager::Seek(int fd, int position)
{
	File* pFile;

	pFile = m_OpenFileTable.GetF(fd);
	if (NULL == pFile || position < 0)
	{
		return -1;
	}
	return pFile->f_offset = position;
}

int FileManager::Read(int fd, char *buffer, int length)
{
	File* file = m_OpenFileTable.GetF(fd);
	if (file == NULL)
	{
		return -1;
	}
	if (!(file->f_flag & File::FREAD))
	{
		return -1;
	}
	Inode* pInode = file->f_inode;
		
	if (file->f_offset >= pInode->i_size)
		return 0;
	int old_offset = file->f_offset;
	pInode->ReadI((unsigned char*)buffer, file->f_offset, length);
	return (file->f_offset - old_offset);
}

int FileManager::Write(int fd, char *buffer,int length)
{
	File* file = m_OpenFileTable.GetF(fd);
	if (file == NULL)
	{
		return -1;
	}
	if (!(file->f_flag & File::FWRITE))
	{
		return -1;
	}
	Inode* pInode = file->f_inode;
	int hole = file->f_offset - pInode->i_size;
	if (pInode->i_size > 0)
		hole++;
	if (hole > 0)
	{
		unsigned char* empty_buffer = new unsigned char[hole];
		memset(empty_buffer, 0, hole);
		pInode->WriteI(empty_buffer, file->f_offset, hole);
		delete[] empty_buffer;
	}

	int old_offset = file->f_offset;
	pInode->WriteI((unsigned char*)buffer, file->f_offset, length);
	return (file->f_offset - old_offset);
}

Inode* FileManager::NameI(char(*func)(), enum DirectorySearchMode mode)
{
	Inode* pInode;
	Buf* pBuf;
	char curchar;
	char* pChar;
	int freeEntryOffset;	/* 以创建文件模式搜索目录时，记录空闲目录项的偏移量 */

	/*
	* 如果该路径是'/'开头的，从根目录开始搜索，
	* 否则从进程当前工作目录开始搜索。
	*/
	pInode = &CURRENT_DIR;
	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;
	}

	/* 检查该Inode是否正在被使用，以及保证在整个目录搜索过程中该Inode不被释放 */
	m_InodeTable.IGet(pInode->i_number);

	/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();
	}
	/* 如果试图更改和删除当前目录文件则出错 */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		goto out;
	}

	/* 外层循环每次处理pathname中一段路径分量 */
	while (true)
	{
		

		/* 整个路径搜索完毕，返回相应Inode指针 */
		if ('\0' == curchar)
		{
			return pInode;
		}

		/* 如果要进行搜索的不是目录文件，释放相关Inode资源则退出 */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			ERRNO = ENOTDIR;
			break;	/* goto out; */
		}

		/* 进行目录搜索权限检查,IEXEC在目录文件中表示搜索权限 */
		if (this->Access(pInode, Inode::IEXEC))
		{
			break;	/* 不具备目录搜索权限，goto out; */
		}

		/*
		* 将Pathname中当前准备进行匹配的路径分量拷贝到dirBuf[]中，
		* 便于和目录项进行比较。
		*/
		pChar = dirBuf;
		while ('/' != curchar && '\0' != curchar)
		{
			if (pChar < &(dirBuf[DirectoryEntry::DIRSIZ]))
			{
				*pChar = curchar;
				pChar++;
			}
			curchar = (*func)();
		}
		/* 将u_dbuf剩余的部分填充为'\0' */
		while (pChar < &(dirBuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* 允许出现////a//b 这种路径 这种路径等价于/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();
		}

		/* 内层循环部分对于dirBuf[]中的路径名分量，逐个搜寻匹配的目录项 */
		ioParam.m_Offset = 0;
		/* 设置为目录项个数 */
		ioParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* 对目录项已经搜索完毕 */
			if (0 == ioParam.m_Count)
			{
				if (NULL != pBuf)
				{
					m_BufferManager.Brelse(pBuf);
				}
				/* 如果是创建新文件 */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* 判断该目录是否可写 */
					if (this->Access(pInode, Inode::IWRITE))
					{
						goto out;	/* Failed */
					}

					/* 将父目录Inode指针保存起来，以后写目录项WriteDir()函数会用到 */
					parentDir = pInode;

					if (freeEntryOffset)	/* 此变量存放了空闲目录项位于目录文件中的偏移量 */
					{
						/* 将空闲目录项偏移量存入u区中，写目录项WriteDir()会用到 */
						this->freeEntryOffset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* 找到可以写入的空闲目录项位置，NameI()函数返回 */
					return NULL;
				}

				/* 目录项搜索完毕而没有找到匹配项，释放相关Inode资源，并推出 */
				ERRNO = ENOENT;
				goto out;
			}

			/* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
			if (0 == ioParam.m_Offset % Inode::BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					m_BufferManager.Brelse(pBuf);
				}
				/* 计算要读的物理盘块号 */
				int phyBlkno = pInode->Bmap(ioParam.m_Offset / Inode::BLOCK_SIZE);
				pBuf = m_BufferManager.Bread(phyBlkno);
			}

			/* 没有读完当前目录项盘块，则读取下一目录项至dirEntry */
			int* src = (int *)(pBuf->buffer + (ioParam.m_Offset % Inode::BLOCK_SIZE));
			Utility::DWordCopy(src, (int *)&dirEntry, sizeof(DirectoryEntry) / sizeof(int));

			ioParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			ioParam.m_Count--;

			/* 如果是空闲目录项，记录该项位于目录文件中偏移量 */
			if (0 == dirEntry.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = ioParam.m_Offset;
				}
				/* 跳过空闲目录项，继续比较下一目录项 */
				continue;
			}

			int i;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (dirBuf[i] != dirEntry.m_name[i])
				{
					break;	/* 匹配至某一字符不符，跳出for循环 */
				}
			}

			if (i < DirectoryEntry::DIRSIZ)
			{
				/* 不是要搜索的目录项，继续匹配下一目录项 */
				continue;
			}
			else
			{
				/* 目录项匹配成功，回到外层While(true)循环 */
				break;
			}
		}

		/*
		* 从内层目录项匹配循环跳至此处，说明pathname中
		* 当前路径分量匹配成功了，还需匹配pathname中下一路径
		* 分量，直至遇到'\0'结束。
		*/
		if (NULL != pBuf)
		{
			m_BufferManager.Brelse(pBuf);
		}

		/* 如果是删除操作，则返回父目录Inode，而要删除文件的Inode号在dirEntry.m_ino中 */
		if (FileManager::DELETE == mode && '\0' == curchar)
		{
			/* 如果对父目录没有写的权限 */
			if (this->Access(pInode, Inode::IWRITE))
			{
				break;	/* goto out; */
			}
			return pInode;
		}

		/*
		* 匹配目录项成功，则释放当前目录Inode，根据匹配成功的
		* 目录项m_ino字段获取相应下一级目录或文件的Inode。
		*/
		short dev = pInode->i_dev;
		m_InodeTable.IPut(pInode);
		pInode = m_InodeTable.IGet(dirEntry.m_ino);
		/* 回到外层While(true)循环，继续匹配Pathname中下一路径分量 */

		if (NULL == pInode)	/* 获取失败 */
		{
			return NULL;
		}
	}
out:
	m_InodeTable.IPut(pInode);
	return NULL;
}

char FileManager::NextChar()
{
	return *pathname++;
}

Inode* FileManager::MakNode(unsigned int mode)
{
	Inode* pInode;

	/* 分配一个空闲DiskInode */
	pInode = m_FileSystem.IAlloc();
	if (NULL == pInode)
	{
		return NULL;
	}

	pInode->i_flag |= (Inode::IACC | Inode::IUPD);
	pInode->i_mode = mode | Inode::IALLOC;
	pInode->i_nlink = 1;
	pInode->i_uid = 0;
	pInode->i_gid = 0;
	/* 将目录项写入dirEntry，随后写入目录文件 */
	this->WriteDir(pInode);
	return pInode;
}

void FileManager::WriteDir(Inode* pInode)
{
	/* 设置目录项中Inode编号部分 */
	dirEntry.m_ino = pInode->i_number;

	/* 设置目录项中pathname分量部分 */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		dirEntry.m_name[i] = dirBuf[i];
	}

	int length = DirectoryEntry::DIRSIZ + 4;

	/* 将目录项写入父目录文件 TODO*/
	parentDir->WriteI((unsigned char *)&dirEntry, freeEntryOffset, length);
	m_InodeTable.IPut(parentDir);
}

void FileManager::SetCurDir(char* pathname)
{
	/* 路径不是从根目录'/'开始，则在现有u.u_curdir后面加上当前路径分量 */
	if (pathname[0] != '/')
	{
		int length = CURRENT_PATH.size();
		if (CURRENT_PATH[length - 1] != '/')
		{
			CURRENT_PATH += '/';
		}
		CURRENT_PATH += pathname;
	}
	else	/* 如果是从根目录'/'开始，则取代原有工作目录 */
	{
		CURRENT_PATH = pathname;
	}
}

int FileManager::Access(Inode* pInode, unsigned int mode)
{
	if ((pInode->i_mode & mode) != 0)
	{
		return 0;
	}
	return 1;
}

int FileManager::ChDir(char *pathname)
{
	this->pathname = pathname;
	Inode* pInode;

	pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
	if (NULL == pInode)
	{
		return -1;
	}
	/* 搜索到的文件不是目录文件 */
	if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
	{
		m_InodeTable.IPut(pInode);
		return -1;
	}
	if (this->Access(pInode, Inode::IEXEC))
	{
		m_InodeTable.IPut(pInode);
		return -1;
	}
	m_InodeTable.IPut(&CURRENT_DIR);
	CURRENT_DIR = *pInode;

	this->SetCurDir(pathname);
	return 0; // todo
}

int FileManager::UnLink(char *pathname)
{
	Inode* pInode;
	Inode* pDeleteInode;
	this->pathname = pathname;
	pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE);
	if (NULL == pDeleteInode)
	{
		return -1;
	}

	pInode = m_InodeTable.IGet(dirEntry.m_ino);
	if (NULL == pInode)
	{
		return -1;
	}
	
	/* 写入清零后的目录项 */
	ioParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	ioParam.m_Base = (unsigned char *)&dirEntry;
	ioParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	dirEntry.m_ino = 0;
	pDeleteInode->WriteI(ioParam.m_Base, ioParam.m_Offset, ioParam.m_Count);

	/* 修改inode项 */
	pInode->i_nlink--;
	pInode->i_flag |= Inode::IUPD;

	m_InodeTable.IPut(pDeleteInode);
	m_InodeTable.IPut(pInode);
	return 0;
}

/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
	this->m_ino = 0;
	this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry()
{
	//nothing to do here
}

