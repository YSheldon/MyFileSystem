#include "INode.h"
#include "Utility.h"
#include "TestMyFileSystem.h"

extern BufferManager m_BufferManager;

/*==============================class Inode===================================*/

Inode::Inode()
{
	/* ���Inode�����е����� */
	// this->Clean(); 
	/* ȥ��this->Clean();�����ɣ�
	* Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	* �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	* ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	* �����ʼ��Ϊ��Чֵ��
	*/

	/* ��Inode����ĳ�Ա������ʼ��Ϊ��Чֵ */
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
	int lbn;	/* �ļ��߼���� */
	int bn;		/* lbn��Ӧ�������̿�� */
	int offset;	/* ��ǰ�ַ�������ʼ����λ�� */
	int nbytes;	/* �������û�Ŀ�����ֽ����� */
	Buf* pBuf;

	if (0 == length)
	{
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}

	this->i_flag |= Inode::IACC;

	while (length != 0)
	{
		lbn = bn = f_offset / Inode::BLOCK_SIZE;
		offset = f_offset % Inode::BLOCK_SIZE;
		/* ���͵��û������ֽ�������ȡ�������ʣ���ֽ����뵱ǰ�ַ�������Ч�ֽ�����Сֵ */
		nbytes = Utility::Min(Inode::BLOCK_SIZE - offset /* ������Ч�ֽ��� */, length);

		int remain = this->i_size - f_offset;
		/* ����Ѷ��������ļ���β */
		if (remain <= 0)
		{
			return;
		}
		/* ���͵��ֽ�������ȡ����ʣ���ļ��ĳ��� */
		nbytes = Utility::Min(nbytes, remain);

		/* ���߼����lbnת���������̿��bn */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}
		pBuf = m_BufferManager.Bread(bn);
		
		/* ��¼�����ȡ�ַ�����߼���� */
		this->i_lastr = lbn;

		/* ������������ʼ��λ�� */
		unsigned char* start = pBuf->buffer + offset;

		/* ������: �ӻ������������û�Ŀ���� */
		Utility::IOMove(start, buffer, nbytes);

		/* �ô����ֽ���nbytes���¶�дλ�� */
		buffer += nbytes;
		f_offset += nbytes;
		length -= nbytes;

		m_BufferManager.Brelse(pBuf);	/* ʹ���껺�棬�ͷŸ���Դ */
	}
}

void Inode::WriteI(unsigned char* buffer, int &f_offset, int length)
{
	int lbn;	/* �ļ��߼���� */
	int bn;		/* lbn��Ӧ�������̿�� */
	int offset;	/* ��ǰ�ַ�������ʼ����λ�� */
	int nbytes;	/* �����ֽ����� */
	Buf* pBuf;

	/* ����Inode�����ʱ�־λ */
	this->i_flag |= (Inode::IACC | Inode::IUPD);

	if (0 == length)
	{
		/* ��Ҫ���ֽ���Ϊ�㣬�򷵻� */
		return;
	}

	while (length != 0)
	{
		lbn = f_offset / Inode::BLOCK_SIZE;
		offset = f_offset % Inode::BLOCK_SIZE;
		nbytes = Utility::Min(Inode::BLOCK_SIZE - offset, length);

		/* ���߼����lbnת���������̿��bn */
		if ((bn = this->Bmap(lbn)) == 0)
		{
			return;
		}
		
		if (Inode::BLOCK_SIZE == nbytes)
		{
			/* ���д������������һ���ַ��飬��Ϊ����仺�� */
			pBuf = m_BufferManager.GetBlk(bn);
		}
		else
		{
			/* д�����ݲ���һ���ַ��飬�������ַ����Ա�������Ҫ��д������ */
			pBuf = m_BufferManager.Bread(bn);
		}

		/* ���������ݵ���ʼдλ�� */
		unsigned char* start = pBuf->buffer + offset;

		/* д����: ���û�Ŀ�����������ݵ������� */
		Utility::IOMove(buffer, start, nbytes);

		/* �ô����ֽ���nbytes���¶�дλ�� */
		buffer += nbytes;
		f_offset += nbytes;
		length -= nbytes;

		m_BufferManager.Bwrite(pBuf);

		/* �����ѱ��*/
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
	int phyBlkno;	/* ת����������̿�� */
	int* iTable;	/* ���ڷ��������̿���һ�μ�ӡ����μ�������� */
	int index;

	/*
	* Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	* (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	*
	* (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	* �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	*
	* (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	* �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/

	if (lbn >= Inode::HUGE_FILE_BLOCK)
	{
		return 0;
	}

	if (lbn < 6)		/* �����С���ļ����ӻ���������i_addr[0-5]�л�������̿�ż��� */
	{
		phyBlkno = this->i_addr[lbn];

		/*
		* ������߼���Ż�û����Ӧ�������̿����֮��Ӧ�������һ������顣
		* ��ͨ�������ڶ��ļ���д�룬��д��λ�ó����ļ���С�����Ե�ǰ
		* �ļ���������д�룬����Ҫ�������Ĵ��̿飬��Ϊ֮�����߼����
		* �������̿��֮���ӳ�䡣
		*/
		if (phyBlkno == 0 && (pFirstBuf = m_FileSystem.Alloc()) != NULL)
		{
			/*
			* ��Ϊ����ܿ������ϻ�Ҫ�õ��˴��·�������ݿ飬���Բ��������������
			* �����ϣ����ǽ�������Ϊ�ӳ�д��ʽ���������Լ���ϵͳ��I/O������
			*/
			m_BufferManager.Bwrite(pFirstBuf);
			phyBlkno = pFirstBuf->blkno;
			/* ���߼����lbnӳ�䵽�����̿��phyBlkno */
			this->i_addr[lbn] = phyBlkno;
			this->i_flag |= Inode::IUPD;
		}
		return phyBlkno;
	}
	
	else	/* lbn >= 6 ���͡������ļ� */
	{
		/* �����߼����lbn��Ӧi_addr[]�е����� */

		if (lbn < Inode::LARGE_FILE_BLOCK)	/* �����ļ�: ���Ƚ���7 - (128 * 2 + 6)���̿�֮�� */
		{
			index = (lbn - Inode::SMALL_FILE_BLOCK) / Inode::ADDRESS_PER_INDEX_BLOCK + 6;
		}
		else	/* �����ļ�: ���Ƚ���263 - (128 * 128 * 2 + 128 * 2 + 6)���̿�֮�� */
		{
			index = (lbn - Inode::LARGE_FILE_BLOCK) / (Inode::ADDRESS_PER_INDEX_BLOCK * Inode::ADDRESS_PER_INDEX_BLOCK) + 8;
		}

		phyBlkno = this->i_addr[index];
		/* ������Ϊ�㣬���ʾ��������Ӧ�ļ��������� */
		if (0 == phyBlkno)
		{
			this->i_flag |= Inode::IUPD;
			/* ����һ�����̿��ż�������� */
			if ((pFirstBuf = m_FileSystem.Alloc()) == NULL)
			{
				return 0;	/* ����ʧ�� */
			}
			/* i_addr[index]�м�¼���������������̿�� */
			this->i_addr[index] = pFirstBuf->blkno;
		}
		else
		{
			/* �����洢�����������ַ��� */
			pFirstBuf = m_BufferManager.Bread(phyBlkno);
		}
		/* ��ȡ��������ַ */
		iTable = (int *)pFirstBuf->buffer;

		/* �����߼����lbn����λ��һ�μ���������еı������index */

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
			/* �����䵽���ļ������̿�ŵǼ���һ�μ���������� */
			phyBlkno = pSecondBuf->blkno;
			iTable[index] = phyBlkno;
			/* �������̿顢���ĺ��һ�μ����������������� */
			m_BufferManager.Bwrite(pSecondBuf);
			m_BufferManager.Bwrite(pFirstBuf);
		}
		else
		{
			/* �ͷ�һ�μ��������ռ�û��� */
			m_BufferManager.Brelse(pFirstBuf);
		}
		return phyBlkno;
	}
}

void Inode::IUpdate(int time)
{
	Buf* pBuf;
	DiskInode dInode;

	/* ��IUPD��IACC��־֮һ�����ã�����Ҫ������ӦDiskInode */
	if ((this->i_flag & (Inode::IUPD | Inode::IACC)) != 0)
	{
		/* ���ô�Ÿ�DiskInode���ַ�����뻺���� */
		pBuf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR + this->i_number / FileSystem::INODE_NUMBER_PER_SECTOR);

		/* ���ڴ�Inode�����е���Ϣ���Ƶ�dInode�У�Ȼ��dInode���ǻ����оɵ����Inode */
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
			/* ����������ʱ�� */
			dInode.d_atime = time;
		}
		if (this->i_flag & Inode::IUPD)
		{
			/* ����������ʱ�� */
			dInode.d_mtime = time;
		}

		/* ��pָ�򻺴����о����Inode��ƫ��λ�� */
		unsigned char* p = pBuf->buffer + (this->i_number % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
		DiskInode* pNode = &dInode;

		/* ��dInode�е������ݸ��ǻ����еľ����Inode */
		Utility::DWordCopy((int *)pNode, (int *)p, sizeof(DiskInode) / sizeof(int));

		/* ������д�������̣��ﵽ���¾����Inode��Ŀ�� */
		m_BufferManager.Bwrite(pBuf);
	}
}

void Inode::ITrunc()
{

	/* ����FILO��ʽ�ͷţ��Ծ���ʹ��SuperBlock�м�¼�Ŀ����̿��������
	*
	* Unix V6++���ļ������ṹ��(С�͡����ͺ;����ļ�)
	* (1) i_addr[0] - i_addr[5]Ϊֱ���������ļ����ȷ�Χ��0 - 6���̿飻
	*
	* (2) i_addr[6] - i_addr[7]���һ�μ�����������ڴ��̿�ţ�ÿ���̿�
	* �ϴ��128���ļ������̿�ţ������ļ����ȷ�Χ��7 - (128 * 2 + 6)���̿飻
	*
	* (3) i_addr[8] - i_addr[9]��Ŷ��μ�����������ڴ��̿�ţ�ÿ�����μ��
	* �������¼128��һ�μ�����������ڴ��̿�ţ������ļ����ȷ�Χ��
	* (128 * 2 + 6 ) < size <= (128 * 128 * 2 + 128 * 2 + 6)
	*/
	for (int i = 9; i >= 0; i--)		/* ��i_addr[9]��i_addr[0] */
	{
		/* ���i_addr[]�е�i��������� */
		if (this->i_addr[i] != 0)
		{
			/* �����i_addr[]�е�һ�μ�ӡ����μ�������� */
			if (i >= 6 && i <= 9)
			{
				/* �������������뻺�� */
				Buf* pFirstBuf = m_BufferManager.Bread(this->i_addr[i]);
				/* ��ȡ��������ַ */
				int* pFirst = (int *)pFirstBuf->buffer;

				/* ÿ�ż���������¼ 512/sizeof(int) = 128�����̿�ţ�������ȫ��128�����̿� */
				for (int j = 128 - 1; j >= 0; j--)
				{
					if (pFirst[j] != 0)	/* �������������� */
					{
						m_FileSystem.Free(pFirst[j]);
					}
				}
				m_BufferManager.Brelse(pFirstBuf);
			}
			/* �ͷ���������ռ�õĴ��̿� */
			m_FileSystem.Free(this->i_addr[i]);
			/* 0��ʾ����������� */
			this->i_addr[i] = 0;
		}
	}

	/* �̿��ͷ���ϣ��ļ���С���� */
	this->i_size = 0;
	/* ����IUPD��־λ����ʾ���ڴ�Inode��Ҫͬ������Ӧ���Inode */
	this->i_flag |= Inode::IUPD;
}


void Inode::Clean()
{
	/*
	* Inode::Clean()�ض�����IAlloc()������·���DiskInode��ԭ�����ݣ�
	* �����ļ���Ϣ��Clean()�����в�Ӧ�����i_dev, i_number, i_flag, i_count,
	* ���������ڴ�Inode����DiskInode�����ľ��ļ���Ϣ����Inode�๹�캯����Ҫ
	* �����ʼ��Ϊ��Чֵ��
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

	/* ��pָ�򻺴����б��Ϊinumber���Inode��ƫ��λ�� */
	unsigned char* p = bp->buffer+ (inumber % FileSystem::INODE_NUMBER_PER_SECTOR) * sizeof(DiskInode);
	/* �����������Inode���ݿ�������ʱ����dInode�У���4�ֽڿ��� */
	Utility::DWordCopy((int *)p, (int *)pNode, sizeof(DiskInode) / sizeof(int));

	/* �����Inode����dInode����Ϣ���Ƶ��ڴ�Inode�� */
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
	* ���DiskInodeû�й��캯�����ᷢ�����½��Ѳ���Ĵ���
	* DiskInode��Ϊ�ֲ�����ռ�ݺ���Stack Frame�е��ڴ�ռ䣬����
	* ��οռ�û�б���ȷ��ʼ�����Ծɱ�������ǰջ���ݣ����ڲ�����
	* DiskInode�����ֶζ��ᱻ���£���DiskInodeд�ص�������ʱ������
	* ����ǰջ����һͬд�أ�����д�ؽ������Ī����������ݡ�
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
