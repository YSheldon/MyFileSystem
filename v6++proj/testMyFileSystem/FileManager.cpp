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
	/* û���ҵ���Ӧ��Inode */
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
	/* ����Ŀ¼��ģʽΪ1����ʾ���� */
	pInode = this->NameI(NextChar, FileManager::CREATE);
	/* û���ҵ���Ӧ��Inode */
	if (NULL == pInode)
	{
		/* ����Inode */
		pInode = this->MakNode(mode);
		/* ����ʧ�� */
		if (NULL == pInode)
		{
			return -1;
		}

		/*
		* �����ϣ�������ֲ����ڣ�ʹ�ò���trf = 2������open1()��
		* ����Ҫ����Ȩ�޼�飬��Ϊ�ոս������ļ���Ȩ�޺ʹ������mode
		* ����ʾ��Ȩ��������һ���ġ�
		*/
		return this->Open1(pInode, File::FWRITE, 2);
	}
	else
	{
		/* ���NameI()�������Ѿ�����Ҫ�������ļ�������ո��ļ� */
		return this->Open1(pInode, File::FWRITE, 1);
	}
}

/*
* trf == 0��open����
* trf == 1����trf == 2��creat����
* mode���������ļ�ģʽ�����ö���д���Ƕ�д�ķ�ʽ
*/
int FileManager::Open1(Inode* pInode, int mode, int trf)
{
	/*
	* ����ϣ�����ļ��Ѵ��ڵ�����£���trf == 0��trf == 1����Ȩ�޼��
	* �����ϣ�������ֲ����ڣ���trf == 2������Ҫ����Ȩ�޼�飬��Ϊ�ս���
	* ���ļ���Ȩ�޺ʹ���Ĳ���mode������ʾ��Ȩ��������һ���ġ�
	*/
	ERRNO = 0;
	if (trf != 2)
	{
		if (mode & File::FREAD)
		{
			/* ����Ȩ�� */
			this->Access(pInode, Inode::IREAD);
		}
		if (mode & File::FWRITE)
		{
			/* ���дȨ�� */
			this->Access(pInode, Inode::IWRITE);
			/* ϵͳ����ȥдĿ¼�ļ��ǲ������ */
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

	/* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
	if (1 == trf)
	{
		pInode->ITrunc();
	}

	/* ������ļ����ƿ�File�ṹ */
	int fd = m_OpenFileTable.FAlloc();
	File* pFile = m_OpenFileTable.GetF(fd);
	if (NULL == pFile)
	{
		m_InodeTable.IPut(pInode);
		return -1;
	}
	/* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
	pFile->f_flag = mode & (File::FREAD | File::FWRITE);
	pFile->f_inode = pInode;

	/* Ϊ�򿪻��ߴ����ļ��ĸ�����Դ���ѳɹ����䣬�������� */
	if (ERRNO == 0)
	{
		return fd;
	}
	else	/* ����������ͷ���Դ */
	{
		/* �ݼ�File�ṹ��Inode�����ü��� */
		pFile->f_count--;
		m_InodeTable.IPut(pInode);
		return -1;
	}
}

void FileManager::Close(int fd)
{
	/* ��ȡ���ļ����ƿ�File�ṹ */
	File* pFile = m_OpenFileTable.GetF(fd);
	if (NULL == pFile)
	{
		return;
	}

	/* �ͷŴ��ļ�������fd���ݼ�File�ṹ���ü��� */
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
	int freeEntryOffset;	/* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */

	/*
	* �����·����'/'��ͷ�ģ��Ӹ�Ŀ¼��ʼ������
	* ����ӽ��̵�ǰ����Ŀ¼��ʼ������
	*/
	pInode = &CURRENT_DIR;
	if ('/' == (curchar = (*func)()))
	{
		pInode = this->rootDirInode;
	}

	/* ����Inode�Ƿ����ڱ�ʹ�ã��Լ���֤������Ŀ¼���������и�Inode�����ͷ� */
	m_InodeTable.IGet(pInode->i_number);

	/* �������////a//b ����·�� ����·���ȼ���/a/b */
	while ('/' == curchar)
	{
		curchar = (*func)();
	}
	/* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
	if ('\0' == curchar && mode != FileManager::OPEN)
	{
		goto out;
	}

	/* ���ѭ��ÿ�δ���pathname��һ��·������ */
	while (true)
	{
		

		/* ����·��������ϣ�������ӦInodeָ�� */
		if ('\0' == curchar)
		{
			return pInode;
		}

		/* ���Ҫ���������Ĳ���Ŀ¼�ļ����ͷ����Inode��Դ���˳� */
		if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
		{
			ERRNO = ENOTDIR;
			break;	/* goto out; */
		}

		/* ����Ŀ¼����Ȩ�޼��,IEXEC��Ŀ¼�ļ��б�ʾ����Ȩ�� */
		if (this->Access(pInode, Inode::IEXEC))
		{
			break;	/* ���߱�Ŀ¼����Ȩ�ޣ�goto out; */
		}

		/*
		* ��Pathname�е�ǰ׼������ƥ���·������������dirBuf[]�У�
		* ���ں�Ŀ¼����бȽϡ�
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
		/* ��u_dbufʣ��Ĳ������Ϊ'\0' */
		while (pChar < &(dirBuf[DirectoryEntry::DIRSIZ]))
		{
			*pChar = '\0';
			pChar++;
		}

		/* �������////a//b ����·�� ����·���ȼ���/a/b */
		while ('/' == curchar)
		{
			curchar = (*func)();
		}

		/* �ڲ�ѭ�����ֶ���dirBuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
		ioParam.m_Offset = 0;
		/* ����ΪĿ¼����� */
		ioParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
		freeEntryOffset = 0;
		pBuf = NULL;

		while (true)
		{
			/* ��Ŀ¼���Ѿ�������� */
			if (0 == ioParam.m_Count)
			{
				if (NULL != pBuf)
				{
					m_BufferManager.Brelse(pBuf);
				}
				/* ����Ǵ������ļ� */
				if (FileManager::CREATE == mode && curchar == '\0')
				{
					/* �жϸ�Ŀ¼�Ƿ��д */
					if (this->Access(pInode, Inode::IWRITE))
					{
						goto out;	/* Failed */
					}

					/* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
					parentDir = pInode;

					if (freeEntryOffset)	/* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */
					{
						/* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
						this->freeEntryOffset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);
					}
					else
					{
						pInode->i_flag |= Inode::IUPD;
					}
					/* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
					return NULL;
				}

				/* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ�����Ƴ� */
				ERRNO = ENOENT;
				goto out;
			}

			/* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
			if (0 == ioParam.m_Offset % Inode::BLOCK_SIZE)
			{
				if (NULL != pBuf)
				{
					m_BufferManager.Brelse(pBuf);
				}
				/* ����Ҫ���������̿�� */
				int phyBlkno = pInode->Bmap(ioParam.m_Offset / Inode::BLOCK_SIZE);
				pBuf = m_BufferManager.Bread(phyBlkno);
			}

			/* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����dirEntry */
			int* src = (int *)(pBuf->buffer + (ioParam.m_Offset % Inode::BLOCK_SIZE));
			Utility::DWordCopy(src, (int *)&dirEntry, sizeof(DirectoryEntry) / sizeof(int));

			ioParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
			ioParam.m_Count--;

			/* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
			if (0 == dirEntry.m_ino)
			{
				if (0 == freeEntryOffset)
				{
					freeEntryOffset = ioParam.m_Offset;
				}
				/* ��������Ŀ¼������Ƚ���һĿ¼�� */
				continue;
			}

			int i;
			for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
			{
				if (dirBuf[i] != dirEntry.m_name[i])
				{
					break;	/* ƥ����ĳһ�ַ�����������forѭ�� */
				}
			}

			if (i < DirectoryEntry::DIRSIZ)
			{
				/* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
				continue;
			}
			else
			{
				/* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
				break;
			}
		}

		/*
		* ���ڲ�Ŀ¼��ƥ��ѭ�������˴���˵��pathname��
		* ��ǰ·������ƥ��ɹ��ˣ�����ƥ��pathname����һ·��
		* ������ֱ������'\0'������
		*/
		if (NULL != pBuf)
		{
			m_BufferManager.Brelse(pBuf);
		}

		/* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����dirEntry.m_ino�� */
		if (FileManager::DELETE == mode && '\0' == curchar)
		{
			/* ����Ը�Ŀ¼û��д��Ȩ�� */
			if (this->Access(pInode, Inode::IWRITE))
			{
				break;	/* goto out; */
			}
			return pInode;
		}

		/*
		* ƥ��Ŀ¼��ɹ������ͷŵ�ǰĿ¼Inode������ƥ��ɹ���
		* Ŀ¼��m_ino�ֶλ�ȡ��Ӧ��һ��Ŀ¼���ļ���Inode��
		*/
		short dev = pInode->i_dev;
		m_InodeTable.IPut(pInode);
		pInode = m_InodeTable.IGet(dirEntry.m_ino);
		/* �ص����While(true)ѭ��������ƥ��Pathname����һ·������ */

		if (NULL == pInode)	/* ��ȡʧ�� */
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

	/* ����һ������DiskInode */
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
	/* ��Ŀ¼��д��dirEntry�����д��Ŀ¼�ļ� */
	this->WriteDir(pInode);
	return pInode;
}

void FileManager::WriteDir(Inode* pInode)
{
	/* ����Ŀ¼����Inode��Ų��� */
	dirEntry.m_ino = pInode->i_number;

	/* ����Ŀ¼����pathname�������� */
	for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
	{
		dirEntry.m_name[i] = dirBuf[i];
	}

	int length = DirectoryEntry::DIRSIZ + 4;

	/* ��Ŀ¼��д�븸Ŀ¼�ļ� TODO*/
	parentDir->WriteI((unsigned char *)&dirEntry, freeEntryOffset, length);
	m_InodeTable.IPut(parentDir);
}

void FileManager::SetCurDir(char* pathname)
{
	/* ·�����ǴӸ�Ŀ¼'/'��ʼ����������u.u_curdir������ϵ�ǰ·������ */
	if (pathname[0] != '/')
	{
		int length = CURRENT_PATH.size();
		if (CURRENT_PATH[length - 1] != '/')
		{
			CURRENT_PATH += '/';
		}
		CURRENT_PATH += pathname;
	}
	else	/* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
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
	/* ���������ļ�����Ŀ¼�ļ� */
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
	
	/* д��������Ŀ¼�� */
	ioParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
	ioParam.m_Base = (unsigned char *)&dirEntry;
	ioParam.m_Count = DirectoryEntry::DIRSIZ + 4;
	dirEntry.m_ino = 0;
	pDeleteInode->WriteI(ioParam.m_Base, ioParam.m_Offset, ioParam.m_Count);

	/* �޸�inode�� */
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

