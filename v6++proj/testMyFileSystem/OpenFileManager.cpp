#include "OpenFileManager.h"
#include "TestMyFileSystem.h"



OpenFileTable::OpenFileTable()
{
	//nothing to do here
}

OpenFileTable::~OpenFileTable()
{
	//nothing to do here
}


File* OpenFileTable::GetF(int fd)
{
	File* pFile;
	if (fd < 0 || fd >= NFILE)
	{
		return NULL;
	}
	pFile = &m_File[fd];
	if (pFile->f_count > 0)
		return pFile;
	return NULL;
}

int OpenFileTable::FAlloc()
{
	for (int i = 0; i < NFILE; i++)
	{
		if (m_File[i].f_count == 0)
		{
			m_File[i].f_count++;
			m_File[i].f_offset = 0;
			return i;
		}
	}
	return -1;
}

void OpenFileTable::CloseF(File* pFile)
{
	if (pFile->f_count <= 1)
	{
		/*
		* 如果当前进程是最后一个引用该文件的进程，
		* 对特殊块设备、字符设备文件调用相应的关闭函数
		*/
		//pFile->f_inode->CloseI(pFile->f_flag & File::FWRITE);
		m_InodeTable.IPut(pFile->f_inode);
	}

	/* 引用当前File的进程数减1 */
	pFile->f_count--;
}

InodeTable::InodeTable()
{

}

InodeTable::~InodeTable()
{

}
Inode* InodeTable::IGet(int inumber)
{
	Inode* pInode;

	while (true)
	{
		/* 检查指定设备dev中编号为inumber的外存Inode是否有内存拷贝 */
		int index = this->IsLoaded(inumber);
		if (index >= 0)	/* 找到内存拷贝 */
		{
			pInode = &(this->m_Inode[index]);
			pInode->i_count++;
			return pInode;
		}
		else	/* 没有Inode的内存拷贝，则分配一个空闲内存Inode */
		{
			pInode = this->GetFreeInode();
			/* 若内存Inode表已满，分配空闲Inode失败 */
			if (NULL == pInode)
			{
				return NULL;
			}
			else	/* 分配空闲Inode成功，将外存Inode读入新分配的内存Inode */
			{
				/* 设置新的设备号、外存Inode编号，增加引用计数，对索引节点上锁 */
				pInode->i_number = inumber;
				pInode->i_flag = Inode::ILOCK;
				pInode->i_count++;
				pInode->i_lastr = -1;

				/* 将该外存Inode读入缓冲区 */
				Buf* pBuf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);

				/* 将缓冲区中的外存Inode信息拷贝到新分配的内存Inode中 */
				pInode->ICopy(pBuf, inumber);
				/* 释放缓存 */
				m_BufferManager.Brelse(pBuf);
				return pInode;
			}
		}
	}
	return NULL;
}


void InodeTable::IPut(Inode* pNode)
{
	/* 当前进程为引用该内存Inode的唯一进程，且准备释放该内存Inode */
	if (pNode->i_count == 1)
	{
		/* 该文件已经没有目录路径指向它 */
		if (pNode->i_nlink <= 0)
		{
			/* 释放该文件占据的数据盘块 */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* 释放对应的外存Inode */
			m_FileSystem.IFree(pNode->i_number);
		}

		/* 更新外存Inode信息 */
		pNode->IUpdate(0);
		/* 清除内存Inode的所有标志位 */
		pNode->i_flag = 0;
		/* 这是内存inode空闲的标志之一，另一个是i_count == 0 */
		pNode->i_number = -1;
	}

	/* 减少内存Inode的引用计数，唤醒等待进程 */
	pNode->i_count--;
}


void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/*
		* 如果Inode对象没有被上锁，即当前未被其它进程使用，可以同步到外存Inode；
		* 并且count不等于0，count == 0意味着该内存Inode未被任何打开文件引用，无需同步。
		*/
		if (this->m_Inode[i].i_count != 0)
		{
			this->m_Inode[i].IUpdate(0);
		}
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* 寻找指定外存Inode的内存拷贝 */
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		if (this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0)
		{
			return i;
		}
	}
	return -1;
}

Inode* InodeTable::GetFreeInode()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/* 如果该内存Inode引用计数为零，则该Inode表示空闲 */
		if (this->m_Inode[i].i_count == 0)
		{
			return &(this->m_Inode[i]);
		}
	}
	return NULL;	/* 寻找失败 */
}

