#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "FileSystem.h"
#include "OpenFileManager.h"
#include "File.h"

/*
* �ļ�������(FileManager)
* ��װ���ļ�ϵͳ�ĸ���ϵͳ�����ں���̬�´�����̣�
* ����ļ���Open()��Close()��Read()��Write()�ȵ�
* ��װ�˶��ļ�ϵͳ���ʵľ���ϸ�ڡ�
*/

class DirectoryEntry
{
	/* static members */
public:
	static const int DIRSIZ = 28;	/* Ŀ¼����·�����ֵ�����ַ������� */

	/* Functions */
public:
	/* Constructors */
	DirectoryEntry();
	/* Destructors */
	~DirectoryEntry();

	/* Members */
public:
	int m_ino;		/* Ŀ¼����Inode��Ų��� */
	char m_name[DIRSIZ];	/* Ŀ¼����·�������� */
};


class FileManager
{
public:
	/* Ŀ¼����ģʽ������NameI()���� */
	enum DirectorySearchMode
	{
		OPEN = 0,		/* �Դ��ļ���ʽ����Ŀ¼ */
		CREATE = 1,		/* ���½��ļ���ʽ����Ŀ¼ */
		DELETE = 2		/* ��ɾ���ļ���ʽ����Ŀ¼ */
	};

	/* Functions */
public:
	/* Constructors */
	FileManager();
	/* Destructors */
	~FileManager();


	/*
	* @comment ��ʼ����ȫ�ֶ��������
	*/
	void Initialize();

	/*
	* @comment Open()ϵͳ���ô������
	*/
	int Open(char* , int );

	/*
	* @comment Creat()ϵͳ���ô������
	*/
	int Creat(char *pathname, int mode);

	/*
	* @comment Open()��Creat()ϵͳ���õĹ�������
	*/
	int Open1(Inode* pInode, int mode, int trf);

	/*
	* @comment Close()ϵͳ���ô������
	*/
	void Close(int fd);

	/*
	* @comment Seek()ϵͳ���ô������
	*/
	int Seek(int fd, int position);

	/*
	* @comment Read()ϵͳ���ô������
	*/
	int Read(int fd, char *buffer, int length);

	/*
	* @comment Write()ϵͳ���ô������
	*/
	int Write(int fd, char *buffer, int length);

	Inode* NameI(char(*func)(), enum DirectorySearchMode mode);

	/*
	* @comment ��ȡ·���е���һ���ַ�
	*/
	static char NextChar();

	/*
	* @comment ��Creat()ϵͳ����ʹ�ã�����Ϊ�������ļ������ں���Դ
	*/
	Inode* MakNode(unsigned int mode);

	/*
	* @comment ��Ŀ¼��Ŀ¼�ļ�д��һ��Ŀ¼��
	*/
	void WriteDir(Inode* pInode);

	/*
	* @comment ���õ�ǰ����·��
	*/
	void SetCurDir(char* pathname);

	/*
	* @comment �����ļ���Ŀ¼������������Ȩ�ޣ���Ϊϵͳ���õĸ�������
	*/
	int Access(Inode* pInode, unsigned int mode);

	/* �ı䵱ǰ����Ŀ¼ */
	int ChDir(char *pathname);

	int UnLink(char *pathname);
public:
	/* ��Ŀ¼�ڴ�Inode */
	Inode* rootDirInode;
	Inode* parentDir;
	static char* pathname;
	char dirBuf[DirectoryEntry::DIRSIZ];
	int freeEntryOffset;
	DirectoryEntry dirEntry;
	IOParameter ioParam;
};



#endif
