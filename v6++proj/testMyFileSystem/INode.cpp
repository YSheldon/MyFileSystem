#include "INode.h"
#include "Utility.h"
#include "TestMyFileSystem.h"

extern BufferManager m_BufferManager;

/*==============================class Inode===================================*/

Inode::Inode()
{
	/* 清空Inode对象中的数据 */
	// this->Clean(); 
	/* 去除this->Clean();的理由：
	* Inode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	* 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	* 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	* 将其初始化为无效值。
	*/

	/* 将Inode对象的成员变量初始化为无效值 */
	this->i_flag = 0;
	this->i_mode = 0;
	this->i_count = 0;
	this->i_nlink = 0;
	this->i_dev = -1;
	this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = 0;
	}
}

Inode::~Inode()
{
	//nothing to do here
}

void Inode::ReadI(unsigned char* buffer, int &f_offset, int length)
{
	int lbn;	/* 文件逻辑块号 */
	int bn;		/* lbn对应的物理盘块号 */
	int offset;	/* 当前字符块内起始传送位置 */
	int nbytes;	/* 传送至用户目标区字节数量 */
	Buf* pBuf;

	if (0 == length)
	{
		/* 需要读字节数为零，则返回 */
		return;
	}

	this->i_flag |= Inode::IACC;

	while (length != 0)
	{
		lbn = bn = f_offset / Inode::BLOCK_SIZE;
		offset = f_offset % Inode::BLOCK_SIZE;
		/* 传送到用户区的字节数量，取读请求的剩余字节数与当前字符块内有效字节数较小值 */
		nbytes = Utility::Min(Inode::BLOCK_SIZE - offset /* 块内有效字节数 */, length);

		int remain = this->i_size - f_offset;
		/* 如果已读到超过文件结尾 */
		if (remain <= 0)
		{
			return;
		}
		/* 传送的字节数量还取决于剩余文件的长度 */
		nbytes = Utility::Min(nbytes, remain);

		/* 将逻辑块号lbn转换成物理盘块号bn */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}
		pBuf = m_BufferManager.Bread(bn);
		
		/* 记录最近读取字符块的逻辑块号 */
		this->i_lastr = lbn;

		/* 缓存中数据起始读位置 */
		unsigned char* start = pBuf->buffer + offset;

		/* 读操作: 从缓冲区拷贝到用户目标区 */
		Utility::IOMove(start, buffer, nbytes);

		/* 用传送字节数nbytes更新读写位置 */
		buffer += nbytes;
		f_offset += nbytes;
		length -= nbytes;

		m_BufferManager.Brelse(pBuf);	/* 使用完缓存，释放该资源 */
	}
}

void Inode::WriteI(unsigned char* buffer, int &f_offset, int length)
{
	int lbn;	/* 文件逻辑块号 */
	int bn;		/* lbn对应的物理盘块号 */
	int offset;	/* 当前字符块内起始传送位置 */
	int nbytes;	/* 传送字节数量 */
	Buf* pBuf;

	/* 设置Inode被访问标志位 */
	this->i_flag |= (Inode::IACC | Inode::IUPD);

	if (0 == length)
	{
		/* 需要读字节数为零，则返回 */
		return;
	}

	while (length != 0)
	{
		lbn = f_offset / Inode::BLOCK_SIZE;
		offset = f_offset % Inode::BLOCK_SIZE;
		nbytes = Utility::Min(Inode::BLOCK_SIZE - offset, length);

		/* 将逻辑块号lbn转换成物理盘块号bn */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}
		
		if (Inode::BLOCK_SIZE == nbytes)
		{
			/* 如果写入数据正好满一个字符块，则为其分配缓存 */
			pBuf = m_BufferManager.GetBlk(bn);
		}
		else
		{
			/* 写入数据不满一个字符块，读出该字符块以保护不需要重写的数据 */
			pBuf = m_BufferManager.Bread(bn);
		}

		/* 缓存中数据的起始写位置 */
		unsigned char* start = pBuf->buffer + offset;

		/* 写操作: 从用户目标区拷贝数据到缓冲区 */
		Utility::IOMove(buffer, start, nbytes);

		/* 用传送字节数nbytes更新读写位置 */
		buffer += nbytes;
		f_offset += nbytes;
		length -= nbytes;

		m_BufferManager.Bwrite(pBuf);

		/* 长度已变大*/
		if (this->i_size < f_offset)
		{
			this->i_size = f_offset;
		}

	}
}

int Inode::Bmap(int lbn)
{
	Buf* pFirstBuf;
	Buf* pSecondBuf;
	int phyBlkno;	/* 转换后的物理盘块号 */
	int* iTable;	/* 用于访问索引盘块中一次间接、两次间接索引表 */
	int index;

	/*
	* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	* (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	*
	* (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	* 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	*
	* (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	* 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/

	if (lbn >= Inode::HUGE_FILE_BLOCK)
	{
		return 0;
	}

	if (lbn < 6)		/* 如果是小型文件，从基本索引表i_addr[0-5]中获得物理盘块号即可 */
	{
		phyBlkno = this->i_addr[lbn];

		/*
		* 如果该逻辑块号还没有相应的物理盘块号与之对应，则分配一个物理块。
		* 这通常发生在对文件的写入，当写入位置超出文件大小，即对当前
		* 文件进行扩充写入，就需要分配额外的磁盘块，并为之建立逻辑块号
		* 与物理盘块号之间的映射。
		*/
		if (phyBlkno == 0 && (pFirstBuf = m_FileSystem.Alloc()) != NULL)
		{
			/*
			* 因为后面很可能马上还要用到此处新分配的数据块，所以不急于立刻输出到
			* 磁盘上；而是将缓存标记为延迟写方式，这样可以减少系统的I/O操作。
			*/
			m_BufferManager.Bwrite(pFirstBuf);
			phyBlkno = pFirstBuf->blkno;
			/* 将逻辑块号lbn映射到物理盘块号phyBlkno */
			this->i_addr[lbn] = phyBlkno;
			this->i_flag |= Inode::IUPD;
		}
		return phyBlkno;
	}
	
	else	/* lbn >= 6 大型、巨型文件 */
	{
		/* 计算逻辑块号lbn对应i_addr[]中的索引 */

		if (lbn < Inode::LARGE_FILE_BLOCK)	/* 大型文件: 长度介于7 - (128 * 2 + 6)个盘块之间 */
		{
			index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
		}
		else	/* 巨型文件: 长度介于263 - (128 * 128 * 2 + 128 * 2 + 6)个盘块之间 */
		{
			index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
		}

		phyBlkno = this->i_addr[index];
		/* 若该项为零，则表示不存在相应的间接索引表块 */
		if (0 == phyBlkno)
		{
			this->i_flag |= Inode::IUPD;
			/* 分配一空闲盘块存放间接索引表 */
			if ((pFirstBuf = m_FileSystem.Alloc()) == NULL)
			{
				return 0;	/* 分配失败 */
			}
			/* i_addr[index]中记录间接索引表的物理盘块号 */
			this->i_addr[index] = pFirstBuf->blkno;
		}
		else
		{
			/* 读出存储间接索引表的字符块 */
			pFirstBuf = m_BufferManager.Bread(phyBlkno);
		}
		/* 获取缓冲区首址 */
		iTable = (int *)pFirstBuf->buffer;

		/* 计算逻辑块号lbn最终位于一次间接索引表中的表项序号index */

		if (lbn < Inode::LARGE_FILE_BLOCK)
		{
			index = (lbn - Inode::SMALL_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
		}
		else
		{
			index = (lbn - Inode::LARGE_FILE_BLOCK) % Inode::ADDRESS_PER_INDEX_BLOCK;
		}

		if ((phyBlkno = iTable[index]) == 0 && (pSecondBuf = m_FileSystem.Alloc()) != NULL)
		{
			/* 将分配到的文件数据盘块号登记在一次间接索引表中 */
			phyBlkno = pSecondBuf->blkno;
			iTable[index] = phyBlkno;
			/* 将数据盘块、更改后的一次间接索引表输出到磁盘 */
			m_BufferManager.Bwrite(pSecondBuf);
			m_BufferManager.Bwrite(pFirstBuf);
		}
		else
		{
			/* 释放一次间接索引表占用缓存 */
			m_BufferManager.Brelse(pFirstBuf);
		}
		return phyBlkno;
	}
}

void Inode::IUpdate(int time)
{
	Buf* pBuf;
	DiskInode dInode;

	/* 当IUPD和IACC标志之一被设置，才需要更新相应DiskInode */
	if ((this->i_flag & (Inode::IUPD | Inode::IACC)) != 0)
	{
		/* 将该存放该DiskInode的字符块读入缓冲区 */
		pBuf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR + this->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

		/* 将内存Inode副本中的信息复制到dInode中，然后将dInode覆盖缓存中旧的外存Inode */
		dInode.d_mode = this->i_mode;
		dInode.d_nlink = this->i_nlink;
		dInode.d_uid = this->i_uid;
		dInode.d_gid = this->i_gid;
		dInode.d_size = this->i_size;
		for (int i = 0; i < 10; i++)
		{
			dInode.d_addr[i] = this->i_addr[i];
		}
		if (this->i_flag & Inode::IACC)
		{
			/* 更新最后访问时间 */
			dInode.d_atime = time;
		}
		if (this->i_flag & Inode::IUPD)
		{
			/* 更新最后访问时间 */
			dInode.d_mtime = time;
		}

		/* 将p指向缓存区中旧外存Inode的偏移位置 */
		unsigned char* p = pBuf->buffer + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;

		/* 用dInode中的新数据覆盖缓存中的旧外存Inode */
		Utility::DWordCopy((int *)pNode, (int *)p, sizeof(DiskInode) / sizeof(int));

		/* 将缓存写回至磁盘，达到更新旧外存Inode的目的 */
		m_BufferManager.Bwrite(pBuf);
	}
}

void Inode::ITrunc()
{

	/* 采用FILO方式释放，以尽量使得SuperBlock中记录的空闲盘块号连续。
	*
	* Unix V6++的文件索引结构：(小型、大型和巨型文件)
	* (1) i_addr[0] - i_addr[5]为直接索引表，文件长度范围是0 - 6个盘块；
	*
	* (2) i_addr[6] - i_addr[7]存放一次间接索引表所在磁盘块号，每磁盘块
	* 上存放128个文件数据盘块号，此类文件长度范围是7 - (128 * 2 + 6)个盘块；
	*
	* (3) i_addr[8] - i_addr[9]存放二次间接索引表所在磁盘块号，每个二次间接
	* 索引表记录128个一次间接索引表所在磁盘块号，此类文件长度范围是
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/
	for (int i = 9; i >= 0; i--)		/* 从i_addr[9]到i_addr[0] */
	{
		/* 如果i_addr[]中第i项存在索引 */
		if (this->i_addr[i] != 0)
		{
			/* 如果是i_addr[]中的一次间接、两次间接索引项 */
			if (i >= 6 && i <= 9)
			{
				/* 将间接索引表读入缓存 */
				Buf* pFirstBuf = m_BufferManager.Bread(this->i_addr[i]);
				/* 获取缓冲区首址 */
				int* pFirst = (int *)pFirstBuf->buffer;

				/* 每张间接索引表记录 512/sizeof(int) = 128个磁盘块号，遍历这全部128个磁盘块 */
				for (int j = 128 - 1; j >= 0; j--)
				{
					if (pFirst[j] != 0)	/* 如果该项存在索引 */
					{
						m_FileSystem.Free(pFirst[j]);
					}
				}
				m_BufferManager.Brelse(pFirstBuf);
			}
			/* 释放索引表本身占用的磁盘块 */
			m_FileSystem.Free(this->i_addr[i]);
			/* 0表示该项不包含索引 */
			this->i_addr[i] = 0;
		}
	}

	/* 盘块释放完毕，文件大小清零 */
	this->i_size = 0;
	/* 增设IUPD标志位，表示此内存Inode需要同步到相应外存Inode */
	this->i_flag |= Inode::IUPD;
}


void Inode::Clean()
{
	/*
	* Inode::Clean()特定用于IAlloc()中清空新分配DiskInode的原有数据，
	* 即旧文件信息。Clean()函数中不应当清除i_dev, i_number, i_flag, i_count,
	* 这是属于内存Inode而非DiskInode包含的旧文件信息，而Inode类构造函数需要
	* 将其初始化为无效值。
	*/

	// this->i_flag = 0;
	this->i_mode = 0;
	//this->i_count = 0;
	this->i_nlink = 0;
	//this->i_dev = -1;
	//this->i_number = -1;
	this->i_uid = -1;
	this->i_gid = -1;
	this->i_size = 0;
	this->i_lastr = -1;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = 0;
	}
}

void Inode::ICopy(Buf *bp, int inumber)
{
	DiskInode dInode;
	DiskInode* pNode = &dInode;

	/* 将p指向缓存区中编号为inumber外存Inode的偏移位置 */
	unsigned char* p = bp->buffer+ (inumber % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	/* 将缓存中外存Inode数据拷贝到临时变量dInode中，按4字节拷贝 */
	Utility::DWordCopy((int *)p, (int *)pNode, sizeof(DiskInode) / sizeof(int));

	/* 将外存Inode变量dInode中信息复制到内存Inode中 */
	this->i_mode = dInode.d_mode;
	this->i_nlink = dInode.d_nlink;
	this->i_uid = dInode.d_uid;
	this->i_gid = dInode.d_gid;
	this->i_size = dInode.d_size;
	for (int i = 0; i < 10; i++)
	{
		this->i_addr[i] = dInode.d_addr[i];
	}
}


/*============================class DiskInode=================================*/

DiskInode::DiskInode()
{
	/*
	* 如果DiskInode没有构造函数，会发生如下较难察觉的错误：
	* DiskInode作为局部变量占据函数Stack Frame中的内存空间，但是
	* 这段空间没有被正确初始化，仍旧保留着先前栈内容，由于并不是
	* DiskInode所有字段都会被更新，将DiskInode写回到磁盘上时，可能
	* 将先前栈内容一同写回，导致写回结果出现莫名其妙的数据。
	*/
	this->d_mode = 0;
	this->d_nlink = 0;
	this->d_uid = -1;
	this->d_gid = -1;
	this->d_size = 0;
	for (int i = 0; i < 10; i++)
	{
		this->d_addr[i] = 0;
	}
	this->d_atime = 0;
	this->d_mtime = 0;
}

DiskInode::~DiskInode()
{
	//nothing to do here
}
