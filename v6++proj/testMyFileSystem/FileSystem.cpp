#include "FileSystem.h"
#include "OpenFileManager.h"
#include "INode.h"
#include "Utility.h"
#include "TestMyFileSystem.h"
#include <iostream>
using namespace std;
/*==============================class SuperBlock===================================*/
/* ϵͳȫ�ֳ�����SuperBlock���� */


SuperBlock::SuperBlock()
{
	//nothing to do here
}

SuperBlock::~SuperBlock()
{
	//nothing to do here
}


/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
	//nothing to do here
}

FileSystem::~FileSystem()
{
	//nothing to do here
}

void FileSystem::Initialize()
{
	this->updlock = 0;
}

void FileSystem::LoadSuperBlock()
{
	Buf* pBuf;

	for (int i = 0; i < 2; i++)
	{
		int* p = (int *)&m_SuperBlock + i * 128;
		pBuf = m_BufferManager.Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);

		Utility::DWordCopy((int *)pBuf->buffer, p, 128);
		m_BufferManager.Brelse(pBuf);
	}
	

	//this->m_Mount[0].m_dev = DeviceManager::ROOTDEV;
	//this->m_Mount[0].m_spb = &g_spb;

	m_SuperBlock.s_flock = 0;
	m_SuperBlock.s_ilock = 0;
	m_SuperBlock.s_ronly = 0;
	m_SuperBlock.s_time = 0;
}


void FileSystem::Update()
{
	Buf* pBuf;

	/* ͬ��SuperBlock������ */
	/* ��SuperBlock�޸ı�־ */
	m_SuperBlock.s_fmod = 0;
	/* д��SuperBlock�����ʱ�� */
	m_SuperBlock.s_time = 0;

	/*
	* Ϊ��Ҫд�ص�������ȥ��SuperBlock����һ�黺�棬���ڻ�����СΪ512�ֽڣ�
	* SuperBlock��СΪ1024�ֽڣ�ռ��2��������������������Ҫ2��д�������
	*/
	for (int j = 0; j < 2; j++)
	{
		/* ��һ��pָ��SuperBlock�ĵ�0�ֽڣ��ڶ���pָ���512�ֽ� */
		int* p = (int *)&m_SuperBlock + j * 128;

		/* ��Ҫд�뵽�豸dev�ϵ�SUPER_BLOCK_SECTOR_NUMBER + j������ȥ */
		pBuf = m_BufferManager.GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + j);

		/* ��SuperBlock�е�0 - 511�ֽ�д�뻺���� */
		Utility::DWordCopy(p, (int *)pBuf->buffer, 128);

		/* ���������е�����д�������� */
		m_BufferManager.Bwrite(pBuf);
	}

	/* ͬ���޸Ĺ����ڴ�Inode����Ӧ���Inode */
	m_InodeTable.UpdateInodeTable();
	
}


Inode* FileSystem::IAlloc()
{
	Buf* pBuf;
	Inode* pNode;
	int ino;	/* ���䵽�Ŀ������Inode��� */

	/*
	* SuperBlockֱ�ӹ���Ŀ���Inode�������ѿգ�
	* ���뵽��������������Inode���ȶ�inode�б�������
	* ��Ϊ�����³����л���ж��̲������ܻᵼ�½����л���
	* ���������п��ܷ��ʸ����������ᵼ�²�һ���ԡ�
	*/
	if (m_SuperBlock.s_ninode <= 0)
	{

		/* ���Inode��Ŵ�0��ʼ���ⲻͬ��Unix V6�����Inode��1��ʼ��� */
		ino = -1;

		/* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
		for (int i = 0; i < m_SuperBlock.s_isize; i++)
		{
			pBuf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

			/* ��ȡ��������ַ */
			int* p = (int *)pBuf->buffer;

			/* ���û�������ÿ�����Inode��i_mode != 0����ʾ�Ѿ���ռ�� */
			for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
			{
				ino++;

				int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

				/* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
				if (mode != 0)
				{
					continue;
				}

				/*
				* ������inode��i_mode==0����ʱ������ȷ��
				* ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
				* ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
				*/
				if (m_InodeTable.IsLoaded(ino) == -1)
				{
					/* �����Inodeû�ж�Ӧ���ڴ濽��������������Inode������ */
					m_SuperBlock.s_inode[m_SuperBlock.s_ninode++] = ino;

					/* ��������������Ѿ�װ�����򲻼������� */
					if (m_SuperBlock.s_ninode >= 100)
					{
						break;
					}
				}
			}

			/* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
			m_BufferManager.Brelse(pBuf);

			/* ��������������Ѿ�װ�����򲻼������� */
			if (m_SuperBlock.s_ninode >= 100)
			{
				break;
			}
		}
		
		/* ����ڴ�����û���������κο������Inode������NULL */
		if (m_SuperBlock.s_ninode <= 0)
		{
			return NULL;
		}
	}

	/*
	* ���沿���Ѿ���֤������ϵͳ��û�п������Inode��
	* �������Inode�������бض����¼�������Inode�ı�š�
	*/
	while (true)
	{
		/* ��������ջ������ȡ�������Inode��� */
		ino = m_SuperBlock.s_inode[--m_SuperBlock.s_ninode];

		/* ������Inode�����ڴ� */
		pNode = m_InodeTable.IGet(ino);
		/* δ�ܷ��䵽�ڴ�inode */
		if (NULL == pNode)
		{
			return NULL;
		}

		pNode->Clean();
		/* ����SuperBlock���޸ı�־ */
		m_SuperBlock.s_fmod = 1;
		return pNode;
		
	}
	return NULL;	/* GCC likes it! */
}

void FileSystem::IFree(int number)
{
	/*
	* ���������ֱ�ӹ���Ŀ������Inode����100����
	* ͬ�����ͷŵ����Inodeɢ���ڴ���Inode���С�
	*/
	if (m_SuperBlock.s_ninode >= 100)
	{
		return;
	}

	m_SuperBlock.s_inode[m_SuperBlock.s_ninode++] = number;

	/* ����SuperBlock���޸ı�־ */
	m_SuperBlock.s_fmod = 1;
}

Buf* FileSystem::Alloc()
{
	int blkno;	/* ���䵽�Ŀ��д��̿��� */
	Buf* pBuf;


	/* ��������ջ������ȡ���д��̿��� */
	blkno = m_SuperBlock.s_free[--m_SuperBlock.s_nfree];

	/*
	* ����ȡ���̿���Ϊ�㣬���ʾ�ѷ��価���еĿ��д��̿顣
	* ���߷��䵽�Ŀ��д��̿��Ų����������̿�������(��BadBlock()���)��
	* ����ζ�ŷ�����д��̿����ʧ�ܡ�
	*/
	if (0 == blkno)
	{
		m_SuperBlock.s_nfree = 0;
		return NULL;
	}

	/*
	* ջ�ѿգ��·��䵽���д��̿��м�¼����һ����д��̿�ı��,
	* ����һ����д��̿�ı�Ŷ���SuperBlock�Ŀ��д��̿�������s_free[100]�С�
	*/
	if (m_SuperBlock.s_nfree <= 0)
	{
		/* ����ÿ��д��̿� */
		pBuf = m_BufferManager.Bread(blkno);

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->buffer;

		/* ���ȶ��������̿���s_nfree */
		m_SuperBlock.s_nfree = *p++;

		/* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
		Utility::DWordCopy(p, m_SuperBlock.s_free, 100);

		/* ����ʹ����ϣ��ͷ��Ա㱻��������ʹ�� */
		m_BufferManager.Brelse(pBuf);
	}

	/* ��ͨ����³ɹ����䵽һ���д��̿� */
	pBuf = m_BufferManager.GetBlk(blkno);	/* Ϊ�ô��̿����뻺�� */
	pBuf->blkno = blkno;
	m_BufferManager.ClrBuf(pBuf);	/* ��ջ����е����� */
	m_SuperBlock.s_fmod = 1;	/* ����SuperBlock���޸ı�־ */

	return pBuf;
}

void FileSystem::Free(int blkno)
{
	Buf* pBuf;

	/*
	* ��������SuperBlock���޸ı�־���Է�ֹ���ͷ�
	* ���̿�Free()ִ�й����У���SuperBlock�ڴ渱��
	* ���޸Ľ�������һ�룬�͸��µ�����SuperBlockȥ
	*/
	m_SuperBlock.s_fmod = 1;

	/*
	* �����ǰϵͳ���Ѿ�û�п����̿飬
	* �����ͷŵ���ϵͳ�е�1������̿�
	*/
	if (m_SuperBlock.s_nfree <= 0)
	{
		m_SuperBlock.s_nfree = 1;
		m_SuperBlock.s_free[0] = 0;	/* ʹ��0��ǿ����̿���������־ */
	}

	/* SuperBlock��ֱ�ӹ�����д��̿�ŵ�ջ���� */
	if (m_SuperBlock.s_nfree >= 100)
	{
		m_SuperBlock.s_flock++;

		/*
		* ʹ�õ�ǰFree()������Ҫ�ͷŵĴ��̿飬���ǰһ��100������
		* ���̿��������
		*/
		pBuf = m_BufferManager.GetBlk(blkno);	/* Ϊ��ǰ��Ҫ�ͷŵĴ��̿���仺�� */

		/* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
		int* p = (int *)pBuf->buffer;

		/* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
		*p++ = m_SuperBlock.s_nfree;
		/* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
		Utility::DWordCopy(m_SuperBlock.s_free, p, 100);

		m_SuperBlock.s_nfree = 0;
		/* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
		m_BufferManager.Bwrite(pBuf);
	}
	m_SuperBlock.s_free[m_SuperBlock.s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
	m_SuperBlock.s_fmod = 1;
}

