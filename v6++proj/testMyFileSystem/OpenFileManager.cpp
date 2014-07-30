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
		* �����ǰ���������һ�����ø��ļ��Ľ��̣�
		* ��������豸���ַ��豸�ļ�������Ӧ�Ĺرպ���
		*/
		//pFile->f_inode->CloseI(pFile->f_flag & File::FWRITE);
		m_InodeTable.IPut(pFile->f_inode);
	}

	/* ���õ�ǰFile�Ľ�������1 */
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
		/* ���ָ���豸dev�б��Ϊinumber�����Inode�Ƿ����ڴ濽�� */
		int index = this->IsLoaded(inumber);
		if (index >= 0)	/* �ҵ��ڴ濽�� */
		{
			pInode = &(this->m_Inode[index]);
			pInode->i_count++;
			return pInode;
		}
		else	/* û��Inode���ڴ濽���������һ�������ڴ�Inode */
		{
			pInode = this->GetFreeInode();
			/* ���ڴ�Inode���������������Inodeʧ�� */
			if (NULL == pInode)
			{
				return NULL;
			}
			else	/* �������Inode�ɹ��������Inode�����·�����ڴ�Inode */
			{
				/* �����µ��豸�š����Inode��ţ��������ü������������ڵ����� */
				pInode->i_number = inumber;
				pInode->i_flag = Inode::ILOCK;
				pInode->i_count++;
				pInode->i_lastr = -1;

				/* �������Inode���뻺���� */
				Buf* pBuf = m_BufferManager.Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);

				/* ���������е����Inode��Ϣ�������·�����ڴ�Inode�� */
				pInode->ICopy(pBuf, inumber);
				/* �ͷŻ��� */
				m_BufferManager.Brelse(pBuf);
				return pInode;
			}
		}
	}
	return NULL;
}


void InodeTable::IPut(Inode* pNode)
{
	/* ��ǰ����Ϊ���ø��ڴ�Inode��Ψһ���̣���׼���ͷŸ��ڴ�Inode */
	if (pNode->i_count == 1)
	{
		/* ���ļ��Ѿ�û��Ŀ¼·��ָ���� */
		if (pNode->i_nlink <= 0)
		{
			/* �ͷŸ��ļ�ռ�ݵ������̿� */
			pNode->ITrunc();
			pNode->i_mode = 0;
			/* �ͷŶ�Ӧ�����Inode */
			m_FileSystem.IFree(pNode->i_number);
		}

		/* �������Inode��Ϣ */
		pNode->IUpdate(0);
		/* ����ڴ�Inode�����б�־λ */
		pNode->i_flag = 0;
		/* �����ڴ�inode���еı�־֮һ����һ����i_count == 0 */
		pNode->i_number = -1;
	}

	/* �����ڴ�Inode�����ü��������ѵȴ����� */
	pNode->i_count--;
}


void InodeTable::UpdateInodeTable()
{
	for (int i = 0; i < InodeTable::NINODE; i++)
	{
		/*
		* ���Inode����û�б�����������ǰδ����������ʹ�ã�����ͬ�������Inode��
		* ����count������0��count == 0��ζ�Ÿ��ڴ�Inodeδ���κδ��ļ����ã�����ͬ����
		*/
		if (this->m_Inode[i].i_count != 0)
		{
			this->m_Inode[i].IUpdate(0);
		}
	}
}

int InodeTable::IsLoaded(int inumber)
{
	/* Ѱ��ָ�����Inode���ڴ濽�� */
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
		/* ������ڴ�Inode���ü���Ϊ�㣬���Inode��ʾ���� */
		if (this->m_Inode[i].i_count == 0)
		{
			return &(this->m_Inode[i]);
		}
	}
	return NULL;	/* Ѱ��ʧ�� */
}

